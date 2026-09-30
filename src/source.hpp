#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv_input.hpp"

namespace atv
{

struct Source
{
public:
  bool displayTimestamp = false;
  double vertUnderscan = 1.0;
  double horizUnderscan = 1.0;
  int lineOverscan = 0;
  bool filter_enabled = true;
  bool vertical_smoothing = true;
  bool do_ssavi = false;
  bool do_cb = true;

  const Properties properties;

  Source()
    : properties({
      //              data type                 gui type   min    max  default             description              pointer
      {"displayTimestamp",
        {    PropertyType::Bool,   ControlType::CheckBox,  0.0,   1.0,     0.0,    "Display timestamp",   &displayTimestamp}},
      {"vertUnderscan",
        {  PropertyType::Double, ControlType::DoubleSpin,  0.1,   2.0,     1.0,   "Vertical underscan",      &vertUnderscan}},
      {"horizUnderscan",
        {  PropertyType::Double, ControlType::DoubleSpin,  0.1,   2.0,     1.0, "Horizontal underscan",     &horizUnderscan}},
      {"lineOverscan",
        {     PropertyType::Int,    ControlType::IntSpin,  0.0, 100.0,     0.0,        "Line overscan",       &lineOverscan}},
      {"filter_enabled",
        {    PropertyType::Bool,   ControlType::CheckBox,  0.0,   1.0,     1.0,       "Filter enabled",     &filter_enabled}},
      {"vertical_smoothing",
        {    PropertyType::Bool,   ControlType::CheckBox,  0.0,   1.0,     1.0,   "Vertical smoothing", &vertical_smoothing}},
      {"do_ssavi",
        {    PropertyType::Bool,   ControlType::CheckBox,  0.0,   1.0,     0.0,          "Apply SSAVI",           &do_ssavi}},
      {"do_cb",
        {    PropertyType::Bool,   ControlType::CheckBox,  0.0,   1.0,     1.0,    "Apply color burst",              &do_cb}},
    })
  { }

  static std::shared_ptr<Source> create(const atv::ParametricString& s);
  static std::shared_ptr<Source> create(const nlohmann::json& j);

  virtual void update(AnalogInput& input, double time) = 0;

  virtual std::string getName() const = 0;

  virtual ~Source() {}

  void setupSync(AnalogInput& input);
  void loadXImage(AnalogInput& input, const cv::Mat& img, const cv::Mat& mask, int xoff, int yoff);
  void finishFrame(AnalogInput& input, double time = 0.0);
};

} // ::atv