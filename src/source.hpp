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
  static std::shared_ptr<Source> create(const nlohmann::json& j);

  virtual void update(AnalogInput& input, double time, bool do_ssavi, bool do_cb) = 0;

  virtual cv::Size getImageSize() = 0;

  virtual void setOutSize(cv::Size size) = 0;

  virtual std::string getName() const = 0;

  virtual ~Source() {}

  cv::Size outSize;
};

} // ::atv