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

struct AnalogReception
{
  AnalogInput input;

  double ofs;
  double level;
  double multipath;
  double freqerr;

  double ghostfir[ANALOGTV_GHOSTFIR_LEN];
  // used to update ghostfir
  double ghostfir2[ANALOGTV_GHOSTFIR_LEN];

  double hfloss;
  // used to update hfloss
  double hfloss2;

  void update(cv::RNG& rng);
};


struct Knobs
{
  double powerup;
  double brightness;
  double tint;
  double color;
  double contrast;
  double height;
  double width;
  double squish;

  bool useHashNoise;
  bool enableHashNoise;

  double horizontalDesync;
  double squeezeBottom;

  bool useFlutterHorizontalDesync;

  int channelChangeCycles;
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

public:

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

  AnalogTV(int seed = 0);
  void configure(int outWidth, int outHeight);
  void draw_signal(const std::vector<float>& rx_signal, double rx_signal_level, cv::Mat4b outBuffer);

  void set_knobs(const Knobs& knobs);

private:
  void  setup_frame(double rx_signal_level);
  void  ntsc_to_yiq(const std::vector<float>& rx_signal, int lineno, unsigned int signal_offset, int start, int end, struct analogtv_yiq_s *it_yiq) const;
  void  sync(const std::vector<float>& rx_signal);
  void  setup_levels(double avgheight);

  int   get_line(int lineno, int *slineno, int *ytop, int *ybot, unsigned *signal_offset) const;
  void  blast_imagerow(const std::vector<float>& rgbf, int ytop, int ybot);
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


void analogtv_lcp_to_ntsc(double luma, double chroma, double phase, int ntsc[4]);

} // ::atv
