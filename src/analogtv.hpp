/* analogtv, Copyright (c) 2003-2018 Trevor Blackwell <tlb@tlb.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#pragma once

#include "precomp.hpp"
#include "analogtv_common.hpp"
#include "analogtv_input.hpp"

#include "utils.hpp"

namespace atv
{

enum class ParamType
{
  Double,
  Bool,
  Int
};

struct ParamInfo
{
  ParamType type;
  double min;
  double max;
  double defaultValue;
  std::string description;
  void* value;
};

struct AnalogReception
{
  AnalogInput input;

  double ofs;       // offset in samples, 0 to ANALOGTV_SIGNAL_LEN-1, default 0
  double level;     // 0.05 to 2.0, default 0.3
  double multipath; // 0.0 to 1.0, default 0.0
  double freqerr;   // -3.0 to 3.0, default 0.0, only for ghosting stations

  double ghostfir[ANALOGTV_GHOSTFIR_LEN];
  // used to update ghostfir
  double ghostfir2[ANALOGTV_GHOSTFIR_LEN];

  double hfloss;
  // used to update hfloss
  double hfloss2;

  void update(cv::RNG& rng);

  const std::map<std::string, ParamInfo> paramInfos;

public:
  AnalogReception()
    : ofs(), level(), multipath(), freqerr(),
      hfloss(), hfloss2(),
      paramInfos()
  {
    const_cast<std::map<std::string, ParamInfo>&>(paramInfos) = {
      {"ofs",       {ParamType::Double,  0.0 , ANALOGTV_SIGNAL_LEN-1.0,  0.0, "Offset in samples", &ofs}},
      {"level",     {ParamType::Double,  0.05,                     2.0,  0.3, "Signal level",      &level}},
      {"multipath", {ParamType::Double,  0.0 ,                     1.0,  0.0, "Multipath",         &multipath}},
      {"freqerr",   {ParamType::Double, -3.0 ,                     3.0,  0.0, "Frequency error",   &freqerr}},
    };
    
    for (const auto& p : paramInfos)
    {
      *(static_cast<double*>(p.second.value)) = p.second.defaultValue;
    }

    std::fill_n(ghostfir, ANALOGTV_GHOSTFIR_LEN, 0.0);
    std::fill_n(ghostfir2, ANALOGTV_GHOSTFIR_LEN, 0.0);
  }

  AnalogReception& operator=(const AnalogReception& other)
  {
    if (this != &other)
    {
      ofs = other.ofs;
      level = other.level;
      multipath = other.multipath;
      freqerr = other.freqerr;
      hfloss = other.hfloss;
      hfloss2 = other.hfloss2;
      std::copy_n(other.ghostfir, ANALOGTV_GHOSTFIR_LEN, ghostfir);
      std::copy_n(other.ghostfir2, ANALOGTV_GHOSTFIR_LEN, ghostfir2);
      // paramInfos pointers already point to this->members, no need to reinit
    }
    return *this;
  }

  AnalogReception(const AnalogReception& other)
      : AnalogReception()
  {
    *this = other;
  }

  std::pair<double, double> getRange(const std::string& param) const
  {
    auto it = paramInfos.find(param);
    if (it != paramInfos.end()) return {it->second.min, it->second.max};
    return {0.0, 0.0};
  }

   double getDefault(const std::string& param) const
   {
     auto it = paramInfos.find(param);
     if (it != paramInfos.end()) return it->second.defaultValue;
     else return 0.0;
   }

  void setValue(const std::string& param, double value)
  {
    auto it = paramInfos.find(param);
    if (it != paramInfos.end() && it->second.value)
    {
      *(static_cast<double*>(it->second.value)) = value;
    }
  }
};


struct Knobs
{
  double powerup;    // default 1000.0, time to power up the TV, in ms
  double brightness; // brightness: -0.75 to 1.0, default 1.5 or 3.0 (?)
  double tint;       // 0 to 360, default 5
  double color;      // 0 to 4.0 or 5.0, default 0.7
  double contrast;   // 0 to 5.0, default 1.5
  double height;     // default 1.0
  double width;      // default 1.0
  double squish;     // default 0.0

  bool useHashNoise;    // default 0
  bool enableHashNoise; // default 1

  double horizontalDesync; // -5.0 to 5.0, default 0.0
  double squeezeBottom;    // -1.0 to 4.0, default 0.0

  bool useFlutterHorizontalDesync; // default false

  int channelChangeCycles; // default 200000, number of cycles to change channel

  const std::map<std::string, ParamInfo> paramInfos;

  //TODO: check all these ranges
  Knobs()
    : powerup(1000.0), brightness(1.5), tint(5.0), color(0.7), contrast(1.5),
      height(1.0), width(1.0), squish(0.0),
      useHashNoise(false), enableHashNoise(true),
      horizontalDesync(0.0), squeezeBottom(0.0),
      useFlutterHorizontalDesync(false),
      channelChangeCycles(200000),
      paramInfos {
        //                                        type       min         max     default  description                     pointer
        {"powerup",                    {ParamType::Double,   0.0,     5000.0,    1000.0, "Power up time, ms",             &powerup}},
        {"brightness",                 {ParamType::Double, -0.75,        1.0,       1.5, "Brightness",                    &brightness}},
        {"tint",                       {ParamType::Double,   0.0,      360.0,       5.0, "Tint",                          &tint}},
        {"color",                      {ParamType::Double,   0.0,        5.0,       0.7, "Color",                         &color}},
        {"contrast",                   {ParamType::Double,   0.0,        5.0,       1.5, "Contrast",                      &contrast}},
        {"height",                     {ParamType::Double,   0.5,        2.0,       1.0, "Height",                        &height}},
        {"width",                      {ParamType::Double,   0.5,        2.0,       1.0, "Width",                         &width}},
        {"squish",                     {ParamType::Double,   0.0,        1.0,       0.0, "Squish",                        &squish}},
        {"useHashNoise",               {ParamType::Bool,     0.0,        1.0,       0.0, "Use hash noise",                &useHashNoise}},
        {"enableHashNoise",            {ParamType::Bool,     0.0,        1.0,       1.0, "Enable hash noise",             &enableHashNoise}},
        {"horizontalDesync",           {ParamType::Double,  -5.0,        5.0,       0.0, "Horizontal desync",             &horizontalDesync}},
        {"squeezeBottom",              {ParamType::Double,  -1.0,        4.0,       0.0, "Squeeze bottom",                &squeezeBottom}},
        {"useFlutterHorizontalDesync", {ParamType::Bool,     0.0,        1.0,       0.0, "Use flutter horizontal desync", &useFlutterHorizontalDesync}},
        {"channelChangeCycles",        {ParamType::Int,      0.0, 1000'000.0, 200'000.0, "Channel change cycles",         &channelChangeCycles}}
      }
  {
    for (const auto& p : paramInfos)
    {
      this->setValue(p.first, p.second.defaultValue);
    }
  }

  Knobs(const Knobs& other)
    : Knobs()
  {
    *this = other;
  }

  Knobs& operator=(const Knobs& other)
  {
    if (this != &other)
    {
      powerup = other.powerup;
      brightness = other.brightness;
      tint = other.tint;
      color = other.color;
      contrast = other.contrast;
      height = other.height;
      width = other.width;
      squish = other.squish;
      useHashNoise = other.useHashNoise;
      enableHashNoise = other.enableHashNoise;
      horizontalDesync = other.horizontalDesync;
      squeezeBottom = other.squeezeBottom;
      useFlutterHorizontalDesync = other.useFlutterHorizontalDesync;
      channelChangeCycles = other.channelChangeCycles;
    }
    return *this;
  }

  std::pair<double, double> getRange(const std::string& param) const
  {
    auto it = paramInfos.find(param);
    if (it != paramInfos.end()) return {it->second.min, it->second.max};
    return {0.0, 0.0};
  }

  double getDefault(const std::string& param) const
  {
    auto it = paramInfos.find(param);
    if (it != paramInfos.end()) return it->second.defaultValue;
    else return 0.0;
  }

  ParamType getType(const std::string& param) const
  {
    auto it = paramInfos.find(param);
    if (it != paramInfos.end()) return it->second.type;
    else return ParamType::Double;
  }

  void setValue(const std::string& param, double value)
  {
    auto it = paramInfos.find(param);
    if (it != paramInfos.end() && it->second.value)
    {
      switch (it->second.type)
      {
        case ParamType::Double:
          *static_cast<double*>(it->second.value) = value;
          break;
        case ParamType::Bool:
          *static_cast<bool*>(it->second.value) = (std::abs(value) > std::numeric_limits<double>::epsilon());
          break;
        case ParamType::Int:
          *static_cast<int*>(it->second.value) = static_cast<int>(value);
          break;
      }
    }
  }
};


struct Receiver
{
public:
  cv::RNG rng;

  // if set, some portion of noise is added before channel switch
  int channel_change_cycles;

  Receiver(int seed);

  // returns rx_signal_level
  void receive(double noiselevel, bool switchChannel, const std::vector<AnalogReception>& receptions, std::vector<float>& rx_signal);
  static double get_rx_signal_level(double noiselevel, const std::vector<AnalogReception>& receptions);

private:
  static void init_signal(double noiselevel, unsigned start, unsigned end, unsigned randVal, std::vector<float>& rx_signal);
  static void transit_channels(const AnalogReception& rec, unsigned start, int skip, unsigned randVal, std::vector<float>& rx_signal);
  static void add_signal(const AnalogReception& rec, unsigned start, unsigned end, int skip, std::vector<float>& rx_signal);
};


/*
  The rest of this should be considered mostly opaque to the analogtv module.
 */

struct AnalogTV
{
private:
#if 0
  unsigned int onscreen_signature[ANALOGTV_V];
#endif

  // these params are set and used internally

  float agclevel;

  int usewidth, useheight, xrepl, subwidth;
  cv::Mat4b image; /* usewidth * useheight */
  int outWidth, outHeight;

  int shrinkpulse;

  float crtload[ANALOGTV_V];

  unsigned int intensity_values[ANALOGTV_CV_MAX];

  float tint_i, tint_q;

  int cur_hsync;
  int line_hsync[ANALOGTV_V];
  int cur_vsync;
  double cb_phase[4];
  double line_cb_phase[ANALOGTV_V][4];

  struct {
    int index;
    double value;
  } leveltable[ANALOGTV_MAX_LINEHEIGHT+1][ANALOGTV_MAX_LINEHEIGHT+1];

  float puheight;

  cv::RNG rng;

  // can be set individually for every frame from outside
  float tint_control, color_control, brightness_control, contrast_control;
  float height_control, width_control, squish_control;
  float horiz_desync;
  float squeezebottom;
  float powerup;

  /* For fast display, set fakeit_top, fakeit_bot to
     the scanlines (0..ANALOGTV_V) that can be preserved on screen.
     fakeit_scroll is the number of scan lines to scroll it up,
     or 0 to not scroll at all. It will DTRT if asked to scroll from
     an offscreen region.
  */
  // int fakeit_top;
  // int fakeit_bot;
  // int fakeit_scroll;
  // int redraw_all;

  int flutter_horiz_desync;
  //int flutter_tint;

  /* Add hash (in the radio sense, not the programming sense.) These
     are the small white streaks that appear in quasi-regular patterns
     all over the screen when someone is running the vacuum cleaner or
     the blender. We also set shrinkpulse for one period which
     squishes the image horizontally to simulate the temporary line
     voltate drop when someone turns on a big motor */
 // double hashnoise_rpm;
 // int hashnoise_counter;
 // int hashnoise_times[ANALOGTV_V];
 // int hashnoise_signal[ANALOGTV_V];
  int hashnoise_on;
  int hashnoise_enable;

public:

  AnalogTV(int seed = 0);
  void configure(int outWidth, int outHeight);
  void draw_signal(const std::vector<float>& rx_signal, double rx_signal_level, cv::Mat4b outBuffer);

  void set_knobs(const Knobs& knobs);

private:

  struct analogtv_yiq_s
  {
    float y,i,q;
  };

  void  setup_frame(double rx_signal_level);
  void  ntsc_to_yiq(const std::vector<float>& rx_signal, int lineno, unsigned int signal_offset, int start, int end,
                    std::vector<analogtv_yiq_s>& it_yiq) const;
  void  sync(const std::vector<float>& rx_signal);
  void  setup_levels(double avgheight);

  int   get_line(int lineno, int& slineno, int& ytop, int& ybot, unsigned& signal_offset) const;
  void  blast_imagerow(const std::vector<cv::Vec3f>& rgbf, int ytop, int ybot);
  void  parallel_for_draw_lines(const cv::Range& r, const std::vector<float>& rx_signal);
};


struct SetTopBox
{
public:
  atv::AnalogTV tv;
  atv::Receiver receiver;
  // pre-allocated received signal and rendered image frame
  std::vector<float> rxSignal;
  cv::Mat4b outBuffer;

  SetTopBox(int seed, int outWidth, int outHeight);

  void setKnobs(const Knobs& knobs);
  cv::Mat4b draw(double noiselevel, bool switchChannel, const std::vector<AnalogReception>& receptions);
};

} // ::atv
