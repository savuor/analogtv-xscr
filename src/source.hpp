#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv_input.hpp"

namespace atv
{

struct Source
{
  Source() :
    outSize()
  { }

  static std::shared_ptr<Source> create(const atv::ParametricString& s);

  virtual void update(AnalogInput& input, double time) = 0;

  virtual cv::Size getImageSize() = 0;

  virtual void setOutSize(cv::Size size) = 0;

  // used for images only
  virtual void setSsavi(bool _do_ssavi) = 0;

  virtual ~Source() {}

  cv::Size outSize;
};

} // ::atv