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
// epoch identifies the frames requested by this seek, see TimedFrameQueue::reset()
struct SeekVideoFrame { uint64_t epoch; double time; };
using WorkerCommand = std::variant<StopWorker, SeekVideoFrame>;


struct TimedFrame
{
  double time = 0.0;
  cv::Mat frame;
};


// Bounded queue of consecutive video frames: the worker fills it, the main thread takes the frame to show.
class TimedFrameQueue
{
public:
  enum class Status
  {
    Frame,    // the frame to show is returned
    NeedSeek, // the queue holds no suitable frame, the worker should be asked to seek
    End       // the video is over or failed
  };

  explicit TimedFrameQueue(size_t maxFrames) : capacity(maxFrames) { }

  // worker: the stream is opened
  void start(double duration);

  // the frames retrieved before a reset (seek/error) belong to the old epoch and are considered stale
  // epoch ID is used to distinguish between frames from different playback sessions
  // returns true if the frame was accepted, false if the queue was reset meanwhile
  bool push(uint64_t frameEpoch, double time, cv::Mat frame);

  // worker: no more frames are coming in this epoch (end of video or an error)
  void finish(uint64_t finishedEpoch);

  // main: finds the latest frame not later than `time`, dropping the earlier ones; waits for the worker if needed
  // `time` should not decrease
  Status pull(double time, cv::Mat& frame);

  // main: drops all the frames, returns a new epoch; the frames of the older epochs are ignored from now on
  uint64_t reset();

private:
  const size_t capacity;
  std::mutex mtx;
  std::condition_variable cond;
  std::deque<TimedFrame> frames;
  std::optional<TimedFrame> current;
  double frameDuration = 0.0;
  uint64_t epoch = 0;
  bool finished = false;
};


// Common part of video file and camera sources.
struct VideoSource : Source
{
  void update(AnalogInput& input, double time) override;

  ~VideoSource() override
  {
    stopWorker();
  }

  cv::Size frameSize;
  double fps = 0;
  cv::Mat emptyFramePlaceholder;

protected:
  VideoSource() : Source(), frameSize(), fps(0), emptyFramePlaceholder() { }

  // Derived constructors start the worker after initializing their capture parameters.
  void startWorker();
  void stopWorker();

  virtual cv::Mat acquireFrame(double time) = 0;

  // called before the worker is stopped, unblocks it if it waits for something else than commands
  virtual void wakeWorker() { }

  // runs in the worker thread
  virtual bool open(cv::VideoCapture& capture) = 0;

  // runs in the worker thread, mtx is not held
  bool getFrame(cv::VideoCapture& capture, cv::Mat& frame);
  bool initializeCapture(cv::VideoCapture& capture);
  virtual void workerLoop() = 0;

  std::mutex frameMtx;
  cv::Mat latestFrame;
  cv::Mat acquiredFrame;
  MessageQueue<WorkerCommand> commands;

private:
  std::thread worker;
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
  cv::Mat acquireFrame(double time) override;
  void wakeWorker() override;
  void workerLoop() override;
  bool open(cv::VideoCapture& capture) override;

private:
  static constexpr size_t frameQueueCapacity = 8;
  TimedFrameQueue frames{frameQueueCapacity};
  std::optional<double> lastTime;
  cv::Mat lastFrame;
};



struct CameraFrameAvailable { };
struct CameraCaptureFailed { };
using CameraMessage = std::variant<CameraFrameAvailable, CameraCaptureFailed>;

class CameraFrameMailbox
{
public:
  void publishFrame(const cv::Mat& frame, cv::Mat& latestFrame);

  void publishFailure();

  cv::Mat pull(uint64_t& consumedSequence, cv::Mat& latestFrame, cv::Mat& acquiredFrame);

private:
  std::mutex mtx;
  std::condition_variable cond;
  uint64_t sequence = 0;
  CameraMessage latestMessage = CameraFrameAvailable{};
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
  CameraFrameMailbox messages;
  uint64_t consumedMessageSequence = 0;

protected:
  cv::Mat acquireFrame(double time) override;
  void workerLoop() override;
  bool open(cv::VideoCapture& capture) override;
};


} // ::atv