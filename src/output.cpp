#include "precomp.hpp"

#include "output.hpp"
#include "utils.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

namespace atv
{

// Custom output factory registry
static std::map<std::string, std::pair<Output::JsonFactory, Output::ParametricFactory>>& getCustomFactories()
{
  static std::map<std::string, std::pair<Output::JsonFactory, Output::ParametricFactory>> factories;
  return factories;
}

void Output::registerFactory(const std::string& type, JsonFactory jsonFactory, ParametricFactory paramFactory)
{
  getCustomFactories()[type] = {jsonFactory, paramFactory};
}

// Output classes

struct HighguiOutput : Output
{
  HighguiOutput();

  void send(const cv::Mat& m) override;

  ~HighguiOutput();
};

struct VideoOutput : Output
{
  VideoOutput(const std::string& s, cv::Size imgSize);

  virtual void send(const cv::Mat& m) override;

  ~VideoOutput() { }

  cv::VideoWriter writer;
};


HighguiOutput::HighguiOutput()
{
    cv::namedWindow("tv");
}

void HighguiOutput::send(const cv::Mat &m)
{
    cv::imshow("tv", m);
    cv::waitKey(1);
}

HighguiOutput::~HighguiOutput()
{
    cv::destroyAllWindows();
}

// used with ffmpeg:
// const enum AVCodecID video_codec = AV_CODEC_ID_H264;
// const enum AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;

VideoOutput::VideoOutput(const std::string &s, cv::Size imgSize)
{
    // cv::VideoWriter::fourcc('M', 'J', 'P', 'G')
    if (!writer.open(s, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), /* fps */ 30, imgSize))
    {
        throw std::runtime_error("Failed to open VideoWriter");
    }
    Log::write(2, "writing to " + s + " " + std::to_string(imgSize.width) + "x" + std::to_string(imgSize.height));
}

void VideoOutput::send(const cv::Mat &m)
{
    cv::Mat out = m;
    if (m.channels() == 4)
    {
        cvtColor(m, out, cv::COLOR_BGRA2BGR);
    }
    writer.write(out);
}

std::shared_ptr<Output> Output::create(const nlohmann::json& j, cv::Size imgSize)
{
    if (j.is_string())
    {
        return create(ParametricString::parse(j.get<std::string>()), imgSize);
    }

    if (!j.is_object())
    {
        throw std::runtime_error("Invalid output JSON: expected object or string");
    }

    std::string type = j.value("type", "");

    if (type == "highgui")
    {
        return std::make_shared<HighguiOutput>();
    }
    else if (type == "video")
    {
        std::string path = j.value("path", j.value("file", ""));
        if (path.empty())
        {
            throw std::runtime_error("Video output missing 'path' property");
        }
        return std::make_shared<VideoOutput>(path, imgSize);
    }
    else
    {
        auto& factories = getCustomFactories();
        auto it = factories.find(type);
        if (it != factories.end() && it->second.first)
        {
            return it->second.first(j, imgSize);
        }
        throw std::runtime_error("Unknown output type in JSON: " + type);
    }
}

std::shared_ptr<Output> Output::create(const ParametricString& s, cv::Size imgSize)
{
    if (!s.className.empty())
    {
        if (s.className == "highgui")
        {
            return std::make_shared<HighguiOutput>();
        }
        else
        {
            auto& factories = getCustomFactories();
            auto it = factories.find(s.className);
            if (it != factories.end() && it->second.second)
            {
                return it->second.second(s, imgSize);
            }
            throw std::runtime_error("Unknown video output: " + s.className);
        }
    }
    else
    {
        return std::make_shared<VideoOutput>(s.varArgs[0], imgSize);
    }
}

} // ::atv
