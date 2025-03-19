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

namespace atv
{

struct AnalogInput
{
  cv::Mat_<int8_t> sigMat;

  AnalogInput() : sigMat(ANALOGTV_V + 1, ANALOGTV_H) { }

  void setup_sync(int do_cb, int do_ssavi);

  void draw_solid(int left, int right, int top, int bot, int ntsc[4]);

  void draw_solid_rel_lcp(double left, double right,
                          double top, double bot,
                          double luma, double chroma, double phase);

  void load_ximage(const cv::Mat4b& pic_im, const cv::Mat4b& mask_im,
                   int xoff, int yoff, int target_w, int target_h, int out_w, int out_h);

  void finish_frame();
};

} // ::atv