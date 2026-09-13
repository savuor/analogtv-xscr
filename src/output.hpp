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
 * @brief Currently supported are: ":highgui", ":qt", and video files
 * 
 * @param s Filename or output name
 * @param imgSize Image size to write
 * @return Output object
 */
  static std::shared_ptr<Output> create(const atv::ParametricString& s, cv::Size imgSize);
  static std::shared_ptr<Output> create(const nlohmann::json& j, cv::Size imgSize);

  using JsonFactory = std::function<std::shared_ptr<Output>(const nlohmann::json&, cv::Size)>;
  using ParametricFactory = std::function<std::shared_ptr<Output>(const ParametricString&, cv::Size)>;

  static void registerFactory(const std::string& type, JsonFactory jsonFactory, ParametricFactory paramFactory);

  virtual void send(const cv::Mat& m) = 0;

  virtual ~Output() { }
};

} // ::atv
