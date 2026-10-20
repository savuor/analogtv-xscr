#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv_input.hpp"
#include "source.hpp"

namespace atv
{


template<typename Message>
class MessageQueue
{
public:
  void push(Message message)
  {
    {
      std::lock_guard<std::mutex> lock(this->mtx);
      this->messages.push_back(std::move(message));
    }
    this->cond.notify_one();
  }

  Message waitPop()
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    this->cond.wait(lock, [this] { return !this->messages.empty(); });
    Message message = std::move(this->messages.front());
    this->messages.pop_front();
    return message;
  }

  // an empty result means that the timeout has expired
  std::optional<Message> waitPopFor(std::chrono::milliseconds timeout)
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    if (!this->cond.wait_for(lock, timeout, [this] { return !this->messages.empty(); }))
      return std::nullopt;
    Message message = std::move(this->messages.front());
    this->messages.pop_front();
    return message;
  }

  std::optional<Message> tryPop()
  {
    std::lock_guard<std::mutex> lock(this->mtx);
    std::optional<Message> result;
    if (!this->messages.empty())
    {
      result = std::move(this->messages.front());
      this->messages.pop_front();
    }
    return result;
  }

private:
  std::mutex mtx;
  std::condition_variable cond;
  std::deque<Message> messages;
};


struct StopWorker { };
// epoch identifies the frames requested by this seek, see FrameQueue::reset()
struct SeekVideoFrame { uint64_t epoch; int64_t index; };
using WorkerCommand = std::variant<StopWorker, SeekVideoFrame>;


// What a source has to show right now.
enum class FrameStatus
{
  Waiting, // the source is not opened yet, nothing to show
  Frame,   // the frame is valid
  Error,   // the capture has failed after it was opened
  End      // the video is over
};

struct AcquiredFrame
{
  FrameStatus status = FrameStatus::Waiting;
  // valid until the next acquireFrame() call, do not keep it
  cv::Mat frame;
};


struct IndexedFrame
{
  int64_t index = 0;
  cv::Mat frame;
};


// Bounded queue of consecutive video frames: the worker fills it, the main thread takes the frame to show.
// Frames are identified by their numbers; the numbers requested by the main thread never decrease.
class FrameQueue
{
public:
  enum class Status
  {
    Frame,    // the frame to show is returned
    NeedSeek, // the queue holds no suitable frame, the worker should be asked to seek
    End,      // the video is over
    Error     // the video can't be read
  };

  explicit FrameQueue(size_t maxFrames) : capacity(maxFrames) { }

  // worker: the stream is opened; if the requested frame is more than `threshold` frames ahead
  // of the shown one, a seek is considered cheaper than reading the frames one by one
  void start(int64_t threshold);

  // the frames retrieved before a reset (seek/error) belong to the old epoch and are considered stale
  // epoch ID is used to distinguish between frames from different playback sessions
  // returns true if the frame was accepted, false if the queue was reset meanwhile
  bool push(uint64_t frameEpoch, int64_t index, cv::Mat frame);

  // worker: no more frames are coming in this epoch, `terminal` is Status::End or Status::Error
  void finish(uint64_t finishedEpoch, Status terminal);

  // worker: the worker is dead, every pull() fails from now on
  void abort();

  // main: finds the latest frame not later than `index`, dropping the earlier ones; waits for the worker if needed
  // `index` should not decrease; if it does, the current frame is returned
  Status pull(int64_t index, cv::Mat& frame);

  // main: drops all the frames, returns a new epoch; the frames of the older epochs are ignored from now on
  uint64_t reset();

private:
  const size_t capacity;
  std::mutex mtx;
  std::condition_variable cond;
  std::deque<IndexedFrame> frames;
  std::optional<IndexedFrame> current;
  int64_t seekThreshold = 0;
  uint64_t epoch = 0;
  bool finished = false;
  Status terminalStatus = Status::End;
  bool aborted = false;
};


// Common part of video file and camera sources.
struct VideoSource : Source
{
  void update(AnalogInput& input, double time) override;

  ~VideoSource() override
  {
    stopWorker();
  }

  // Blocks until the worker has tried to open the source; throws std::runtime_error if it has failed.
  // update() throws it as well, so calling this is optional: it just reports the failure earlier.
  void waitOpened();

  // size of the latest shown frame, changed by update() only
  cv::Size frameSize;
  // frame rate reported by the capture, 0 if unknown
  std::atomic<double> fps;

protected:
  static constexpr int defaultFrameWidth = 640;
  static constexpr int defaultFrameHeight = 480;

  VideoSource() : Source(), frameSize(defaultFrameWidth, defaultFrameHeight), fps(0.0) { }

  // Derived constructors start the worker after initializing their capture parameters.
  void startWorker();
  void stopWorker();

  // main thread: what to show at `time`, throws std::runtime_error if the source has failed to open
  virtual AcquiredFrame acquireFrame(double time) = 0;

  // called before the worker is stopped, unblocks it if it waits for something else than commands
  virtual void wakeWorker() { }

  // called in the worker thread if workerLoop() has thrown, makes the main thread stop waiting for the worker
  virtual void onWorkerCrash() { }

  // runs in the worker thread
  virtual bool open(cv::VideoCapture& capture) = 0;

  // runs in the worker thread
  bool getFrame(cv::VideoCapture& capture, cv::Mat& frame);
  bool initializeCapture(cv::VideoCapture& capture);
  virtual void workerLoop() = 0;

  // worker: the first attempt to open the source is over, call exactly one of them
  void reportOpened();
  void reportOpenFailure();

  // main thread, non-blocking: throws if the first attempt to open the source has failed
  void checkOpened();

  MessageQueue<WorkerCommand> commands;

private:
  void handleWorkerCrash(const std::string& what);
  cv::Mat getPlaceholder(FrameStatus status, cv::Size size);

  std::thread worker;

  std::promise<void> openPromise;
  std::shared_future<void> openFuture = openPromise.get_future().share();
  // main thread only: the open result is known and it is a success
  bool openConfirmed = false;

  // one per FrameStatus value, the one for FrameStatus::Frame stays empty
  std::array<cv::Mat, 4> placeholders;
};


struct VideoFileSource : VideoSource
{
  VideoFileSource(const std::string& fileName);

  ~VideoFileSource() override
  {
    stopWorker();
  }

  std::string getName() const override
  {
    return "Video File " + videoFileName;
  }

  std::string videoFileName;

protected:
  AcquiredFrame acquireFrame(double time) override;
  void wakeWorker() override;
  void onWorkerCrash() override;
  void workerLoop() override;
  bool open(cv::VideoCapture& capture) override;

private:
  static constexpr size_t frameQueueCapacity = 8;
  // if the requested frame is further than this, the worker seeks instead of reading the frames one by one
  static constexpr double seekThresholdSeconds = 1.0;
  // protects against 29.99999 instead of 30 when a time is converted to a frame number
  static constexpr double indexEpsilon = 1e-6;
  static constexpr double fallbackFps = 30.0;

  FrameQueue frames{frameQueueCapacity};
  // the worker writes it before reportOpened(), the main thread reads it after waitOpened()
  double playbackFps = 0.0;
  std::optional<int64_t> lastIndex;
  AcquiredFrame lastResult;
};


// Hands the latest camera frame from the capture thread over to the main thread without copying pixels:
// three buffers (the worker's, the mailbox's and the consumer's) are swapped around.
class CameraFrameMailbox
{
public:
  // worker: puts the frame into the mailbox, `frame` gets a spare buffer back to retrieve the next frame into
  void publishFrame(cv::Mat& frame);

  // worker: there is nothing to show (FrameStatus::Waiting or FrameStatus::Error)
  void publishStatus(FrameStatus status);

  // worker: true if the consumer has taken the previous frame and wants a new one
  bool frameRequested() const
  {
    return this->requested.load();
  }

  // main: never blocks, the returned frame is valid until the next pull() call
  AcquiredFrame pull();

private:
  std::mutex mtx;
  FrameStatus status = FrameStatus::Waiting;
  // `latest` holds a frame that was not taken yet
  bool fresh = false;
  cv::Mat latest;
  cv::Mat acquired;
  std::atomic<bool> requested{true};
};


struct CameraSource : VideoSource
{
  CameraSource(int nCam);

  ~CameraSource() override
  {
    stopWorker();
  }

  std::string getName() const override
  {
    return "Camera #" + std::to_string(nCamera);
  }

  int nCamera;

protected:
  AcquiredFrame acquireFrame(double time) override;
  void onWorkerCrash() override;
  void workerLoop() override;
  bool open(cv::VideoCapture& capture) override;

private:
  // grab() always runs to drain the device buffer, but retrieve() (decoding + color conversion)
  // runs only when the consumer has taken the previous frame, which saves CPU when the consumer is slower
  // than the camera. The price: the frame is up to one consumer period older than it could be.
  // Set it to false to decode every frame.
  static constexpr bool retrieveOnlyWhenRequested = true;
  // a single bad frame is skipped, a series of them is considered a lost camera
  static constexpr int maxBadFrames = 10;
  static constexpr std::chrono::milliseconds reconnectDelayMin{500};
  static constexpr std::chrono::milliseconds reconnectDelayMax{5000};

  CameraFrameMailbox messages;
};


} // ::atv