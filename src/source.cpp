#include "precomp.hpp"

#include "source.hpp"
#include "source_types.hpp"
#include "video_source.hpp"

//DEBUG
#include <opencv2/highgui.hpp>

namespace atv
{

void Source::setupSync(AnalogInput& input)
{
  input.setup_sync(this->do_cb, this->do_ssavi);
}

void Source::loadXImage(AnalogInput& input, const cv::Mat& img, const cv::Mat& mask, int xoff, int yoff)
{
  input.load_ximage(img, mask, xoff, yoff, this->filter_enabled, this->vertical_smoothing);
}

void Source::finishFrame(AnalogInput& input, double time)
{
  if (this->displayTimestamp)
  {
    cv::Mat osd = drawTime(time);
    cv::Mat resizedOsd;
    const double scale = 1.0;
    cv::resize(osd, resizedOsd, cv::Size(), signalStretch * scale, scale, cv::INTER_LINEAR);

    this->loadXImage(input, resizedOsd, cv::Mat4b(), 16, 16);
  }

  input.finish_frame();
}


static int barsSourceCount = 0;

BarsSource::BarsSource(const cv::Mat& _logoImg) :
  Source(),
  logoImg()
{
  this->number = barsSourceCount++;

  if (_logoImg.empty())
    return;

  // stretch image to signal resolution
  const double scale = 0.3;
  cv::resize(_logoImg, this->logoImg, cv::Size(), signalStretch * scale, scale, cv::INTER_LINEAR);

  /* Pull the alpha out of the logo and make a separate mask ximage. */
  this->logoMask = cv::Mat(logoImg.size(), CV_8UC4, cv::Scalar(0));
  std::vector<cv::Mat> logoCh;
  cv::split(logoImg, logoCh);
  cv::Mat z = cv::Mat(logoImg.size(), CV_8UC1, cv::Scalar(0));
  cv::merge(std::vector<cv::Mat> {logoCh[0], logoCh[1], logoCh[2], z}, logoImg);
  cv::merge(std::vector<cv::Mat> {z, z, z, logoCh[3]}, this->logoMask);
}


void BarsSource::update(AnalogInput& input, double time)
{
  // original name: update_smpte_colorbars()

  /* 
     SMPTE is the society of motion picture and television engineers, and
     these are the standard color bars in the US. Following the partial spec
     at http://broadcastengineering.com/ar/broadcasting_inside_color_bars/
     These are luma, chroma, and phase numbers for each of the 7 bars.
  */
  double top_cb_table[7][3]={
    {75, 0, 0.0},    /* gray */
    {69, 31, 167.0}, /* yellow */
    {56, 44, 283.5}, /* cyan */
    {48, 41, 240.5}, /* green */
    {36, 41, 60.5},  /* magenta */
    {28, 44, 103.5}, /* red */
    {15, 31, 347.0}  /* blue */
  };
  double mid_cb_table[7][3]={
    {15, 31, 347.0}, /* blue */
    {7, 0, 0},       /* black */
    {36, 41, 60.5},  /* magenta */
    {7, 0, 0},       /* black */
    {56, 44, 283.5}, /* cyan */
    {7, 0, 0},       /* black */
    {75, 0, 0.0}     /* gray */
  };

  this->setupSync(input);

  for (int col = 0; col < 7; col++)
  {
    input.draw_solid_rel_lcp(col*(1.0/7.0),
                             (col+1)*(1.0/7.0),
                             0.00, 0.68,
                             top_cb_table[col][0],
                             top_cb_table[col][1],
                             top_cb_table[col][2]);

    input.draw_solid_rel_lcp(col*(1.0/7.0),
                             (col+1)*(1.0/7.0),
                             0.68, 0.75,
                             mid_cb_table[col][0],
                             mid_cb_table[col][1],
                             mid_cb_table[col][2]);
  }

  //                            left      right   top  bottom  luma  chroma  phase
  input.draw_solid_rel_lcp(      0.0,   1.0/6.0, 0.75,   1.00,    7,     40,   303);   /* -I       */
  input.draw_solid_rel_lcp(  1.0/6.0,   2.0/6.0, 0.75,   1.00,  100,      0,     0);   /* white    */
  input.draw_solid_rel_lcp(  2.0/6.0,   3.0/6.0, 0.75,   1.00,    7,     40,    33);   /* +Q       */
  input.draw_solid_rel_lcp(  3.0/6.0,   4.0/6.0, 0.75,   1.00,    7,      0,     0);   /* black    */
  input.draw_solid_rel_lcp(12.0/18.0, 13.0/18.0, 0.75,   1.00,    3,      0,     0);   /* black -4 */
  input.draw_solid_rel_lcp(13.0/18.0, 14.0/18.0, 0.75,   1.00,    7,      0,     0);   /* black    */
  input.draw_solid_rel_lcp(14.0/18.0, 15.0/18.0, 0.75,   1.00,   11,      0,     0);   /* black +4 */
  input.draw_solid_rel_lcp(  5.0/6.0,   6.0/6.0, 0.75,   1.00,    7,      0,     0);   /* black    */

  if (!this->logoImg.empty())
  {
    //TODO: fix this by calculating xoff and yoff properly
    int xoff = (ANALOGTV_VIS_LEN - this->logoImg.cols) / 2;
    int yoff = (ANALOGTV_VISLINES - this->logoImg.rows) / 2;
    this->loadXImage(input, this->logoImg, this->logoMask, xoff, yoff);
  }

  this->finishFrame(input, time);
}


cv::Size Source::fitSize(const cv::Size& imgSize, double vertUnderscan, double horizUnderscan,
                         int lineOverscan)
{
  assert(imgSize.width > 0 && imgSize.height > 0);

  // here we change the original image resizing algorithm from XAnalogTV
  // to avoid incorrect resampling and allow finer control over the image fitting

  cv::Size outSize(ANALOGTV_VISLINES * 4 / 3, ANALOGTV_VISLINES);
  cv::Size2d fittedSize;
  double ro = (double) (4.0 / 3.0);
  double ri = (double) imgSize.width / imgSize.height;
  if (ro > ri)
  {
    fittedSize = { (double)outSize.height * ri, (double)outSize.height };
  }
  else
  {
    fittedSize = { (double)outSize.width, (double)outSize.width / ri };
  }

  // stretch image to signal resolution
  const int overscan = lineOverscan * ANALOGTV_SCALE; /* overscan this much top and bottom */
  const double overscanFactor = (double) ( ANALOGTV_VISLINES + 2 * overscan) / ANALOGTV_VISLINES;

  fittedSize.width  = fittedSize.width * signalStretch * horizUnderscan;
  fittedSize.height = fittedSize.height * vertUnderscan * overscanFactor;

  CV_Assert(fittedSize.width > 0 && fittedSize.height > 0);
  return cv::Size(static_cast<int>(fittedSize.width), static_cast<int>(fittedSize.height));
}


static int imageSourceCount = 0;


ImageSource::ImageSource(const cv::Mat& _img) :
  Source(),
  img(_img),
  resizedImg(),
  offset(),
  number(imageSourceCount++)
{
  this->resizeImage();
}

void ImageSource::resizeImage()
{
  if (std::abs(this->vertUnderscan - this->prevVerticalUnderscan) > 1e-6 ||
      std::abs(this->horizUnderscan - this->prevHorizontalUnderscan) > 1e-6 ||
      this->lineOverscan != this->prevLineOverscan ||
      this->img.size() != this->resizedImg.size())
  {
    this->prevVerticalUnderscan = this->vertUnderscan;
    this->prevHorizontalUnderscan = this->horizUnderscan;
    this->prevLineOverscan = this->lineOverscan;

    cv::Size fittedSize = fitSize(this->img.size(), this->vertUnderscan, this->horizUnderscan, this->lineOverscan);

    this->offset.x = (ANALOGTV_VIS_LEN  - fittedSize.width) / 2;
    this->offset.y = (ANALOGTV_VISLINES - fittedSize.height) / 2;

    cv::resize(this->img, this->resizedImg, fittedSize, 0, 0, cv::INTER_LINEAR);
  }
}


void ImageSource::update(AnalogInput& input, double time)
{
  this->resizeImage();

  this->setupSync(input);

  this->loadXImage(input, this->resizedImg, cv::Mat4b(), offset.x, offset.y);

  this->finishFrame(input, time);
}

// the sources can be tuned later for different size or other params
std::shared_ptr<Source> Source::create(const nlohmann::json& j)
{
  if (j.is_string())
  {
    return create(atv::ParametricString::parse(j.get<std::string>()));
  }

  if (!j.is_object())
  {
    throw std::runtime_error("Invalid source JSON: expected object or string");
  }

  std::string type = j.value("type", "");

  std::shared_ptr<Source> src;
  if (type == "bars")
  {
    std::string logoPath = j.value("logo", "");
    cv::Mat logo;
    if (!logoPath.empty())
    {
      logo = loadImage(logoPath);
    }
    src = std::make_shared<BarsSource>(logo);
  }
  else if (type == "camera" || type == "cam")
  {
    int id = j.value("id", 0);
    src = std::make_shared<CameraSource>(id);
  }
  else if (type == "video")
  {
    std::string path = j.value("path", j.value("file", ""));
    if (path.empty())
    {
      throw std::runtime_error("Video source missing 'path' property");
    }
    src = std::make_shared<VideoFileSource>(path);
  }
  else if (type == "image")
  {
    std::string path = j.value("path", j.value("file", ""));
    if (path.empty())
    {
      throw std::runtime_error("Image source missing 'path' property");
    }
    cv::Mat img = loadImage(path);
    src = std::make_shared<ImageSource>(img);
  }
  else
  {
    throw std::runtime_error("Unknown source type in JSON: " + type);
  }

  for (const auto& [propName, _] : src->properties)
  {
    PropertyType propType = src->properties.getType(propName);
    double defaultValue = src->properties.getDefault(propName);
    switch (propType)
    {
      case PropertyType::Double:
        src->properties.setValue(propName, j.value(propName, defaultValue));
        break;
      case PropertyType::Int:
        src->properties.setValue(propName, j.value(propName, static_cast<int>(defaultValue)));
        break;
      case PropertyType::Bool:
        src->properties.setValue(propName, j.value(propName, defaultValue > 0) ? 1.0 : 0.0);
        break;
      default:
        break;
    }
  }

  return src;
}

std::shared_ptr<Source> Source::create(const atv::ParametricString& desc)
{
  std::shared_ptr<Source> src;

  if (!desc.className.empty())
  {
    std::string arg = desc.varArgs[0];

    // should be like ":bars" or ":bars:/path/to/image:params"
    if (desc.className == "bars")
    {
      cv::Mat logo;
      if (!arg.empty())
      {
        logo = loadImage(arg);
      }
      src = std::make_shared<BarsSource>(logo);
    }
    // should be like ":cam" or ":cam:number"
    else if (desc.className == "cam")
    {
      int nCam = arg.empty() ? 0 : parseInt(arg).value_or(0);
      src = std::make_shared<CameraSource>(nCam);
    }
    // should be like ":video:/path/to/video:params"
    else if (desc.className == "video")
    {
      src = std::make_shared<VideoFileSource>(arg);
    }
    // should be like ":image:/path/to/image/:params"
    else if (desc.className == "image")
    {
      cv::Mat img = loadImage(arg);
      src = std::make_shared<ImageSource>(img);
    }
    else
    {
        throw std::runtime_error("Unknown source type: " + desc.className);
    }
    src->displayTimestamp = desc.kvArgs.count("timestamp") > 0;
  }
  else
  {
    std::string srcStr = desc.varArgs[0];
    const std::set<std::string> knownVideoExtensions = {
      "h264", "h265",
      "mpeg2", "mpeg4", "mp4", "mjpeg", "mpg",
      "vp8", "mov", "wmv", "flv",
      "avi", "mkv"
    };
    int extIdx = srcStr.find_last_of(".");
    std::string ext = srcStr.substr(extIdx + 1, srcStr.length() - extIdx - 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (knownVideoExtensions.count(ext))
    {
      src = std::make_shared<VideoFileSource>(srcStr);
    }
    else
    {
      cv::Mat img = loadImage(srcStr);
      src = std::make_shared<ImageSource>(img);
    }
  }

  return src;
}

} // ::atv
