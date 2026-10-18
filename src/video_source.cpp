#include "precomp.hpp"

#include "video_source.hpp"

namespace atv
{

static constexpr double timeEpsilon = 1e-6;

void VideoSource::startWorker()
{
  this->worker = std::thread([this] { this->workerLoop(); });
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

    {
      std::lock_guard<std::mutex> lock(this->frameMtx);
      this->frameSize = captureFrameSize;
      this->fps = captureFps;
      this->latestFrame.create(this->frameSize, CV_8UC3);
      this->acquiredFrame.create(this->frameSize, CV_8UC3);
    }

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

cv::Mat VideoFileSource::acquireFrame(double time)
{
  const bool sameTime = this->lastTime && std::abs(time - *this->lastTime) < timeEpsilon;
  if (!sameTime)
  {
    cv::Mat frame;
    TimedFrameQueue::Status status = this->frames.pull(time, frame);
    if (status == TimedFrameQueue::Status::NeedSeek)
    {
      uint64_t epoch = this->frames.reset();
      this->commands.push(SeekVideoFrame{epoch, time});
      // after a reset `current` is empty, so a second NeedSeek is impossible
      status = this->frames.pull(time, frame);
      assert(status != TimedFrameQueue::Status::NeedSeek);
    }

    // the frame is empty if the video is over
    this->lastFrame = frame;
    this->lastTime = time;
  }

  return this->lastFrame;
}

void VideoFileSource::workerLoop()
{
  cv::VideoCapture capture;
  bool running = this->initializeCapture(capture);

  const double frameDuration = 1.0 / (this->fps > 0.0 ? this->fps : 30.0);
  uint64_t epoch = 0;
  // index of the frame that is read next
  int64_t index = 0;
  // the video is over or the queue was reset: wait for the next command
  bool idle = false;

  if (running)
  {
    this->frames.start(frameDuration);
  }
  else
  {
    this->frames.finish(epoch);
  }

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
        const int64_t target = static_cast<int64_t>(std::floor(seek->time / frameDuration + timeEpsilon));
        Log::write(2, "Video file seek to frame " + std::to_string(target));
        idle = false;
        if (target != index && !capture.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(target)))
        {
          Log::write(0, "Video file seek failed");
          this->frames.finish(epoch);
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
        idle = !this->frames.push(epoch, static_cast<double>(index) * frameDuration, std::move(frame));
        ++index;
      }
      else
      {
        Log::write(2, "Video file is over or failed to read a frame at " + std::to_string(index));
        this->frames.finish(epoch);
        idle = true;
      }
    }
  }
}


void TimedFrameQueue::start(double duration)
{
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->frameDuration = duration;
  }
  this->cond.notify_all();
}

bool TimedFrameQueue::push(uint64_t frameEpoch, double time, cv::Mat frame)
{
  std::unique_lock<std::mutex> lock(this->mtx);
  this->cond.wait(lock, [this, frameEpoch] { return this->frames.size() < this->capacity || frameEpoch != this->epoch; });

  const bool accepted = (frameEpoch == this->epoch);
  if (accepted)
  {
    this->frames.push_back({time, std::move(frame)});
  }
  lock.unlock();

  this->cond.notify_all();
  return accepted;
}

void TimedFrameQueue::finish(uint64_t finishedEpoch)
{
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    if (finishedEpoch == this->epoch)
      this->finished = true;
  }
  this->cond.notify_all();
}


uint64_t TimedFrameQueue::reset()
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

TimedFrameQueue::Status TimedFrameQueue::pull(double time, cv::Mat& frame)
{
  std::unique_lock<std::mutex> lock(this->mtx);
  // the frame duration is unknown until the worker opens the video
  this->cond.wait(lock, [this] { return this->frameDuration > 0.0 || this->finished; });

  std::optional<Status> status;
  while (!status)
  {
    // the latest frame that is not later than the requested time wins, the earlier ones are dropped
    bool dropped = false;
    while (!this->frames.empty() && this->frames.front().time <= time + timeEpsilon)
    {
      this->current = std::move(this->frames.front());
      this->frames.pop_front();
      dropped = true;
    }
    if (dropped)
      this->cond.notify_all();

    // time does not go backwards: an earlier time just shows the current frame
    if (this->current && time < this->current->time + this->frameDuration - timeEpsilon)
    {
      frame = this->current->frame;
      status = Status::Frame;
    }
    else if (!this->frames.empty())
    {
      if (this->current)
      {
        // the frames after the current one are skipped
        status = Status::NeedSeek;
      }
      else
      {
        // nothing was shown yet, the earliest frame is the closest one
        frame = this->frames.front().frame;
        status = Status::Frame;
      }
    }
    else if (this->finished)
    {
      status = Status::End;
    }
    else if (this->current && time - this->current->time > this->frameDuration * this->capacity)
    {
      // reading frame by frame up to the requested time takes longer than a seek
      status = Status::NeedSeek;
    }
    else
    {
      this->cond.wait(lock);
    }
  }

  return *status;
}


void CameraFrameMailbox::publishFrame(const cv::Mat& frame, cv::Mat& latestFrame)
{
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    frame.copyTo(latestFrame);
    ++this->sequence;
    this->latestMessage = CameraFrameAvailable{};
  }
  this->cond.notify_all();
}

void CameraFrameMailbox::publishFailure()
{
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->latestMessage = CameraCaptureFailed{};
    ++this->sequence;
  }
  this->cond.notify_all();
}

cv::Mat CameraFrameMailbox::pull(uint64_t& consumedSequence, cv::Mat& latestFrame, cv::Mat& acquiredFrame)
{
  std::unique_lock<std::mutex> lock(this->mtx);
  this->cond.wait(lock, [this]
  {
    return this->sequence > 0;
  });

  if (std::holds_alternative<CameraCaptureFailed>(this->latestMessage))
  {
    consumedSequence = this->sequence;
    return {};
  }

  if (this->sequence > consumedSequence)
  {
    consumedSequence = this->sequence;
    std::swap(latestFrame, acquiredFrame);
  }

  return acquiredFrame;
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

cv::Mat CameraSource::acquireFrame(double /* time */)
{
  return this->messages.pull(this->consumedMessageSequence, this->latestFrame, this->acquiredFrame);
}

void CameraSource::workerLoop()
{
  cv::VideoCapture capture;
  bool ok = this->initializeCapture(capture);
  if (!ok)
  {
    this->messages.publishFailure();
  }

  cv::Mat frame;
  while (ok)
  {
    auto command = this->commands.tryPop();
    if (command && std::holds_alternative<StopWorker>(*command))
      break;

    ok = this->getFrame(capture, frame);

    if (ok)
    {
      this->messages.publishFrame(frame, this->latestFrame);
    }
    else
    {
      this->messages.publishFailure();
      Log::write(0, "VideoSource worker failed during camera capture");
    }
  }
}


void VideoSource::update(AnalogInput& input, double time)
{
  cv::Mat prepared;
  cv::Mat frame = this->acquireFrame(time);

  // these properties can be changed in realtime
  cv::Size fittedSize = fitSize(this->frameSize, this->vertUnderscan, this->horizUnderscan, this->lineOverscan);

  cv::Point offset;
  offset.x = (ANALOGTV_VIS_LEN  - fittedSize.width) / 2;
  offset.y = (ANALOGTV_VISLINES - fittedSize.height) / 2;

  if (frame.empty())
  {
    // generate it once if necessary
    if (this->emptyFramePlaceholder.empty() || this->emptyFramePlaceholder.size() != fittedSize)
    {
      this->emptyFramePlaceholder = cv::Mat(fittedSize, CV_8UC4, cv::Scalar(128, 64, 0));
      int baseline = 0;
      double scale = 2.0;
      std::string text = "no frame :(";
      cv::Size szOsd = cv::getTextSize(text, cv::FONT_HERSHEY_PLAIN, scale, 1, &baseline);
      cv::Mat osd(szOsd.height + baseline, szOsd.width, CV_8UC4, cv::Scalar(128, 64, 0));
      cv::putText(osd, text, {0, osd.rows - baseline}, cv::FONT_HERSHEY_PLAIN, scale, cv::Scalar::all(255), 1);
      cv::Mat resized;
      cv::resize(osd, resized, {}, signalStretch, 1.0);
      cv::Point textOrg((fittedSize.width  - resized.cols) / 2,
                        (fittedSize.height - resized.rows) / 2);
      resized.copyTo(this->emptyFramePlaceholder(cv::Rect(textOrg.x, textOrg.y, resized.cols, resized.rows)));
    }

    prepared = this->emptyFramePlaceholder;
  }
  else
  {
    cv::Mat resized;
    cv::resize(frame, resized, fittedSize);
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