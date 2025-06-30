#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv_input.hpp"

namespace atv
{

struct Source
{
  Source() :
    outSize(), do_ssavi()
  { }

  static std::shared_ptr<Source> create(const atv::ParametricString& s);

  virtual void update(AnalogInput& input, double time) = 0;

  virtual cv::Size getImageSize() = 0;

  virtual void setOutSize(cv::Size size) = 0;

  virtual void setSsavi(bool _do_ssavi) = 0;

  virtual std::string getName() const = 0;

  virtual ~Source() {}

  cv::Size outSize;
  bool do_ssavi;
};

} // ::atv