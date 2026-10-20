#include "precomp.hpp"

#include "video_source.hpp"

namespace atv
{

void VideoSource::startWorker()
{
  this->worker = std::thread([this]
  {
    // an exception must not leave the thread: it would terminate the whole host application
    try
    {
      this->workerLoop();
    }
    catch (const std::exception& e)
    {
      this->handleWorkerCrash(e.what());
    }
    catch (...)
    {
      this->handleWorkerCrash("unknown exception");
    }
  });
}

void VideoSource::stopWorker()
{
  if (this->worker.joinable())
  {
    this->commands.push(StopWorker{});
    this->wakeWorker();
    this->worker.join();
  }
}

void VideoSource::handleWorkerCrash(const std::string& what)
{
  Log::write(0, "VideoSource worker has failed: " + what);
  try
  {
    // it reaches the main thread if the source was not opened yet
    this->openPromise.set_exception(
      std::make_exception_ptr(std::runtime_error("Failed to run source " + this->getName() + ": " + what)));
  }
  catch (const std::future_error&)
  {
    // the source was opened before the failure, the main thread will show the error placeholder
  }
  this->onWorkerCrash();
}

void VideoSource::reportOpened()
{
  this->openPromise.set_value();
}

void VideoSource::reportOpenFailure()
{
  this->openPromise.set_exception(
    std::make_exception_ptr(std::runtime_error("Failed to open source: " + this->getName())));
}

void VideoSource::waitOpened()
{
  // get() rethrows the stored exception
  this->openFuture.get();
  this->openConfirmed = true;
}

void VideoSource::checkOpened()
{
  if (!this->openConfirmed && this->openFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
  {
    this->openFuture.get();
    this->openConfirmed = true;
  }
}

bool VideoSource::getFrame(cv::VideoCapture& capture, cv::Mat& frame)
{
  return capture.grab() && capture.retrieve(frame);
}

bool VideoSource::initializeCapture(cv::VideoCapture& capture)
{
  bool ok = this->open(capture);
  if (ok)
  {
    const cv::Size captureFrameSize = {
      static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
      static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT))
    };
    const double captureFps = capture.get(cv::CAP_PROP_FPS);
    this->fps = captureFps;

    Log::write(2, "Source opened: " + this->getName() + " frame size " +
                  std::to_string(captureFrameSize.width) + "x" + std::to_string(captureFrameSize.height) +
                  " FPS " + std::to_string(captureFps));
  }
  else
  {
    Log::write(0, "Failed to open source: " + this->getName());
  }
  return ok;
}


VideoFileSource::VideoFileSource(const std::string& fileName)
  : VideoSource(),
    videoFileName(fileName)
{
  startWorker();
}

bool VideoFileSource::open(cv::VideoCapture& capture)
{
  return capture.open(videoFileName);
}

void VideoFileSource::wakeWorker()
{
  // a worker waiting for a free slot in the queue gets unblocked
  this->frames.reset();
}

void VideoFileSource::onWorkerCrash()
{
  this->frames.abort();
}

AcquiredFrame VideoFileSource::acquireFrame(double time)
{
  // the frame rate is known only after that; throws if the file could not be opened
  this->waitOpened();

  const int64_t index = std::max<int64_t>(
    0, static_cast<int64_t>(std::floor(time * this->playbackFps + indexEpsilon)));

  if (this->lastIndex && *this->lastIndex == index)
    return this->lastResult;

  cv::Mat frame;
  FrameQueue::Status status = this->frames.pull(index, frame);
  if (status == FrameQueue::Status::NeedSeek)
  {
    const uint64_t epoch = this->frames.reset();
    this->commands.push(SeekVideoFrame{epoch, index});
    // The queue is empty after the reset and the worker starts from the requested frame,
    // so the next pull waits for that frame or reports a failure.
    status = this->frames.pull(index, frame);
  }

  AcquiredFrame result;
  switch (status)
  {
  case FrameQueue::Status::Frame:
    result.status = FrameStatus::Frame;
    result.frame = frame;
    break;
  case FrameQueue::Status::End:
    result.status = FrameStatus::End;
    break;
  case FrameQueue::Status::Error:
    result.status = FrameStatus::Error;
    break;
  case FrameQueue::Status::NeedSeek:
    // should not happen, but a broken invariant must not turn into an endless seeking
    Log::write(0, "Video file: seek did not provide frame " + std::to_string(index));
    result.status = FrameStatus::Error;
    break;
  }

  this->lastIndex = index;
  this->lastResult = result;
  return result;
}

void VideoFileSource::workerLoop()
{
  cv::VideoCapture capture;
  if (!this->initializeCapture(capture))
  {
    this->reportOpenFailure();
    return;
  }

  const double reportedFps = this->fps;
  this->playbackFps = reportedFps > 0.0 ? reportedFps : fallbackFps;
  this->frames.start(static_cast<int64_t>(std::ceil(seekThresholdSeconds * this->playbackFps)));
  this->reportOpened();

  uint64_t epoch = 0;
  // index of the frame that is read next
  int64_t index = 0;
  // the video is over or the queue was reset: wait for the next command
  bool idle = false;
  bool running = true;

  while (running)
  {
    std::optional<WorkerCommand> command;
    if (idle)
      command = this->commands.waitPop();
    else
      command = this->commands.tryPop();

    if (command)
    {
      if (std::holds_alternative<StopWorker>(*command))
      {
        running = false;
      }
      else if (auto* seek = std::get_if<SeekVideoFrame>(&*command))
      {
        epoch = seek->epoch;
        const int64_t target = seek->index;
        Log::write(2, "Video file seek to frame " + std::to_string(target));
        idle = false;
        if (target != index && !capture.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(target)))
        {
          Log::write(0, "Video file seek failed");
          this->frames.finish(epoch, FrameQueue::Status::Error);
          idle = true;
        }
        index = target;
      }
    }
    else
    {
      cv::Mat frame;
      if (this->getFrame(capture, frame))
      {
        // a stale epoch means that a new command is on the way
        idle = !this->frames.push(epoch, index, std::move(frame));
        ++index;
      }
      else
      {
        Log::write(2, "Video file is over or failed to read a frame at " + std::to_string(index));
        this->frames.finish(epoch, FrameQueue::Status::End);
        idle = true;
      }
    }
  }
}


void FrameQueue::start(int64_t threshold)
{
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->seekThreshold = threshold;
  }
  this->cond.notify_all();
}

bool FrameQueue::push(uint64_t frameEpoch, int64_t index, cv::Mat frame)
{
  std::unique_lock<std::mutex> lock(this->mtx);
  this->cond.wait(lock, [this, frameEpoch] { return this->frames.size() < this->capacity || frameEpoch != this->epoch; });

  const bool accepted = (frameEpoch == this->epoch);
  if (accepted)
  {
    this->frames.push_back({index, std::move(frame)});
  }
  lock.unlock();

  this->cond.notify_all();
  return accepted;
}

void FrameQueue::finish(uint64_t finishedEpoch, Status terminal)
{
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    if (finishedEpoch == this->epoch)
    {
      this->finished = true;
      this->terminalStatus = terminal;
    }
  }
  this->cond.notify_all();
}

void FrameQueue::abort()
{
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->aborted = true;
  }
  this->cond.notify_all();
}

uint64_t FrameQueue::reset()
{
  uint64_t newEpoch;
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->frames.clear();
    this->current.reset();
    this->finished = false;
    newEpoch = ++this->epoch;
  }
  this->cond.notify_all();
  return newEpoch;
}

FrameQueue::Status FrameQueue::pull(int64_t index, cv::Mat& frame)
{
  std::unique_lock<std::mutex> lock(this->mtx);

  while (true)
  {
    if (this->aborted)
      return Status::Error;

    // the latest frame that is not later than the requested one wins, the earlier ones are dropped
    bool dropped = false;
    while (!this->frames.empty() && this->frames.front().index <= index)
    {
      this->current = std::move(this->frames.front());
      this->frames.pop_front();
      dropped = true;
    }
    if (dropped)
      this->cond.notify_all();

    // The queued frames are consecutive, so after the dropping the current frame is the requested one.
    // If the index went backwards, the current frame is returned as well.
    if (this->current && index <= this->current->index)
    {
      frame = this->current->frame;
      return Status::Frame;
    }

    // the queue starts later than the requested frame: there is a gap that can be skipped by a seek only
    if (!this->frames.empty())
      return Status::NeedSeek;

    if (this->finished)
      return this->terminalStatus;

    // reading frame by frame up to the requested one takes longer than a seek
    if (this->current && index - this->current->index > this->seekThreshold)
      return Status::NeedSeek;

    this->cond.wait(lock);
  }
}


void CameraFrameMailbox::publishFrame(cv::Mat& frame)
{
  std::lock_guard<std::mutex> lock(this->mtx);
  std::swap(frame, this->latest);
  this->fresh = true;
  this->status = FrameStatus::Frame;
  this->requested = false;
}

void CameraFrameMailbox::publishStatus(FrameStatus newStatus)
{
  std::lock_guard<std::mutex> lock(this->mtx);
  this->fresh = false;
  this->status = newStatus;
  // the first frame after the failure should not wait for the consumer
  this->requested = true;
}

AcquiredFrame CameraFrameMailbox::pull()
{
  std::lock_guard<std::mutex> lock(this->mtx);

  if (this->fresh)
  {
    std::swap(this->latest, this->acquired);
    this->fresh = false;
    this->requested = true;
  }

  AcquiredFrame result;
  result.status = this->status;
  if (this->status == FrameStatus::Frame)
    result.frame = this->acquired;
  return result;
}


CameraSource::CameraSource(int nCam)
  : VideoSource(),
    nCamera(nCam)
{
  startWorker();
}

bool CameraSource::open(cv::VideoCapture& capture)
{
  bool ok = capture.open(nCamera);
  if (ok)
  {
    // Best effort; some backends ignore CAP_PROP_BUFFERSIZE.
    capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
  }
  return ok;
}

AcquiredFrame CameraSource::acquireFrame(double /* time */)
{
  // throws if the camera could not be opened; does not wait for it otherwise
  this->checkOpened();
  return this->messages.pull();
}

void CameraSource::onWorkerCrash()
{
  this->messages.publishStatus(FrameStatus::Error);
}

void CameraSource::workerLoop()
{
  cv::VideoCapture capture;
  // a spare buffer: it is swapped with the mailbox on every publish, so no pixels are copied
  cv::Mat frame;
  bool everOpened = false;
  bool running = true;
  std::chrono::milliseconds reconnectDelay = reconnectDelayMin;

  // The camera is lost: shows the error, waits before the next attempt and makes the wait longer each time.
  // The wait is interrupted if the worker is asked to stop, then it returns false.
  auto waitBeforeReconnect = [&]() -> bool
  {
    this->messages.publishStatus(FrameStatus::Error);
    auto command = this->commands.waitPopFor(reconnectDelay);
    reconnectDelay = std::min(reconnectDelay * 2, reconnectDelayMax);
    return !(command && std::holds_alternative<StopWorker>(*command));
  };

  while (running)
  {
    if (!this->initializeCapture(capture))
    {
      // a camera that was never opened is a configuration problem: the main thread throws
      if (!everOpened)
      {
        this->reportOpenFailure();
        return;
      }

      running = waitBeforeReconnect();
      continue;
    }

    if (!everOpened)
    {
      everOpened = true;
      this->reportOpened();
    }

    int badFrames = 0;
    bool gotFrames = false;
    while (running)
    {
      auto command = this->commands.tryPop();
      if (command && std::holds_alternative<StopWorker>(*command))
      {
        running = false;
        break;
      }

      if (!capture.grab())
      {
        Log::write(0, "VideoSource worker lost the camera: grab() failed");
        break;
      }

      if (retrieveOnlyWhenRequested && !this->messages.frameRequested())
        continue;

      if (!capture.retrieve(frame))
      {
        if (++badFrames >= maxBadFrames)
        {
          Log::write(0, "VideoSource worker lost the camera: retrieve() keeps failing");
          break;
        }
        continue;
      }
      badFrames = 0;

      this->messages.publishFrame(frame);
      gotFrames = true;
    }

    capture.release();
    if (running)
    {
      // the loop was left because of a failure; the camera is reopened in the next iteration,
      // and the delay starts over only if the camera has worked for a while
      if (gotFrames)
        reconnectDelay = reconnectDelayMin;
      running = waitBeforeReconnect();
    }
  }
}


struct PlaceholderStyle
{
  const char* text;
  cv::Scalar color;
};

PlaceholderStyle getPlaceholderStyle(FrameStatus status)
{
  switch (status)
  {
  case FrameStatus::Waiting: return {"waiting for source...", cv::Scalar(128, 64, 0)};
  case FrameStatus::End:     return {"end of video",          cv::Scalar(64, 64, 64)};
  case FrameStatus::Error:
  case FrameStatus::Frame:
  default:                   return {"capture error :(",      cv::Scalar(0, 0, 128)};
  }
}


cv::Mat VideoSource::getPlaceholder(FrameStatus status, cv::Size size)
{
  cv::Mat& placeholder = this->placeholders[static_cast<size_t>(status)];

  // generate it once if necessary
  if (placeholder.empty() || placeholder.size() != size)
  {
    const PlaceholderStyle style = getPlaceholderStyle(status);
    placeholder = cv::Mat(size, CV_8UC4, style.color);

    int baseline = 0;
    double scale = 2.0;
    std::string text = style.text;
    cv::Size szOsd = cv::getTextSize(text, cv::FONT_HERSHEY_PLAIN, scale, 1, &baseline);
    cv::Mat osd(szOsd.height + baseline, szOsd.width, CV_8UC4, style.color);
    cv::putText(osd, text, {0, osd.rows - baseline}, cv::FONT_HERSHEY_PLAIN, scale, cv::Scalar::all(255), 1);
    cv::Mat resized;
    cv::resize(osd, resized, {}, signalStretch, 1.0);
    const cv::Point textOrg((size.width  - resized.cols) / 2,
                            (size.height - resized.rows) / 2);
    // the text can be larger than a small frame: only the visible part is copied
    const cv::Rect visible = cv::Rect(textOrg, resized.size()) & cv::Rect(cv::Point(), size);
    if (!visible.empty())
    {
      resized(visible - textOrg).copyTo(placeholder(visible));
    }
  }

  return placeholder;
}


void VideoSource::update(AnalogInput& input, double time)
{
  cv::Mat prepared;
  // throws std::runtime_error if the source could not be opened
  const AcquiredFrame acquired = this->acquireFrame(time);
  const bool hasFrame = acquired.status == FrameStatus::Frame && !acquired.frame.empty();

  // the size follows the frames, it can change in realtime (e.g. after a camera reconnection);
  // while there are no frames, the previous or the default size is used
  if (hasFrame)
    this->frameSize = acquired.frame.size();

  // these properties can be changed in realtime
  cv::Size fittedSize = fitSize(this->frameSize, this->vertUnderscan, this->horizUnderscan, this->lineOverscan);

  cv::Point offset;
  offset.x = (ANALOGTV_VIS_LEN  - fittedSize.width) / 2;
  offset.y = (ANALOGTV_VISLINES - fittedSize.height) / 2;

  if (!hasFrame)
  {
    // a frame that is marked valid but is empty is an error as well
    const FrameStatus shown = acquired.status == FrameStatus::Frame ? FrameStatus::Error : acquired.status;
    prepared = this->getPlaceholder(shown, fittedSize);
  }
  else
  {
    cv::Mat resized;
    cv::resize(acquired.frame, resized, fittedSize);
    std::vector<cv::Mat> ch;
    cv::split(resized, ch);
    cv::Mat z = cv::Mat(fittedSize, CV_8UC1, cv::Scalar(0));
    cv::merge(std::vector<cv::Mat> {ch[0], ch[1], ch[2], z}, prepared);
  }

  this->setupSync(input);

  this->loadXImage(input, prepared, cv::Mat4b(), offset.x, offset.y);

  this->finishFrame(input, time);
}


} // ::atv