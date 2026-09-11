#pragma once

#include "precomp.hpp"
#include "analogtv.hpp"

namespace atv
{

struct AppSettings
{
  int seed = 0;
  cv::Size size = {640, 480};
  int fps = 30;
  Knobs knobs;
  std::vector<std::string> sourceStrings;
  std::vector<std::string> outputStrings;

  struct ReceptionConfig
  {
    int sourceIndex = 0;
    double level = 0.3;
    double multipath = 0.0;
    double ofs = 0.0;
    double freqerr = 0.0;
  };

  struct ChannelConfig
  {
    double noise_level = 0.04;
    std::vector<ReceptionConfig> receptions;
  };

  std::vector<ChannelConfig> channels;
};

AppSettings loadSettings(const std::string& filePath);

} // ::atv
