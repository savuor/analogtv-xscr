#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv_input.hpp"
#include "source.hpp"

namespace atv
{


struct BarsSource : Source
{
  BarsSource(const cv::Mat& _logoImg = { });

  void update(AnalogInput& input, double time) override;

  std::string getName() const override
  {
    return "SMPTE Color Bars #" + std::to_string(number);
  }

  cv::Mat logoImg, logoMask;
  int number;
};


struct ImageSource : Source
{
  ImageSource(const cv::Mat& _img = { });

  void update(AnalogInput& input, double time) override;

  std::string getName() const override
  {
    return "Image #" + std::to_string(number);
  }

  void resizeImage();

  cv::Mat img;
  cv::Mat resizedImg;
  cv::Point offset;
  int number;

  double prevVerticalUnderscan = 0.970;
  double prevHorizontalUnderscan = 0.815;
  int prevLineOverscan = 5;
};


} // ::atv