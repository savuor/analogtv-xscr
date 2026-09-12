#pragma once

#include "precomp.hpp"
#include "utils.hpp"

namespace atv
{

struct Output
{
  Output() { }

  //TODO: send desired FPS
 /**
 * @brief Currently supported are: ":highgui" and video files
 * 
 * @param s Filename or output name
 * @param imgSize Image size to write
 * @return Output object
 */
  static std::shared_ptr<Output> create(const atv::ParametricString& s, cv::Size imgSize);
  static std::shared_ptr<Output> create(const nlohmann::json& j, cv::Size imgSize);

  virtual void send(const cv::Mat& m) = 0;

  virtual ~Output() { }
};

} // ::atv
