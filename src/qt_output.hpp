#pragma once

#include "output.hpp"

class QWidget;
class QLabel;

namespace atv
{

struct QtOutput : Output
{
  explicit QtOutput(cv::Size imgSize);

  void send(const cv::Mat& m) override;

  ~QtOutput() override;

private:
  QWidget* window = nullptr;
  QLabel* label = nullptr;
};

} // ::atv
