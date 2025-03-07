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

namespace atv
{

/* To simulate an NTSC CRT monitor with way more scanlines, and thus
   apply an ahistorical tv-like effect to a larger image, increase
   this resolution multiplier.
 */
#ifndef ANALOGTV_SCALE
# define ANALOGTV_SCALE 1
#endif

/*
  You'll need these to generate standard NTSC TV signals
 */
enum {
  /* We don't handle interlace here */
  ANALOGTV_V=262*ANALOGTV_SCALE,
  ANALOGTV_TOP=30*ANALOGTV_SCALE,
  ANALOGTV_VISLINES=200*ANALOGTV_SCALE,
  ANALOGTV_BOT=ANALOGTV_TOP + ANALOGTV_VISLINES,

  /* This really defines our sampling rate, 4x the colorburst
     frequency. Handily equal to the Apple II's dot clock.
     You could also make a case for using 3x the colorburst freq,
     but 4x isn't hard to deal with. */
  ANALOGTV_H=912*ANALOGTV_SCALE,

  /* Each line is 63500 nS long. The sync pulse is 4700 nS long, etc.
     Define sync, back porch, colorburst, picture, and front porch
     positions */
  ANALOGTV_SYNC_START=0,
  ANALOGTV_BP_START=4700*ANALOGTV_H/63500,
  ANALOGTV_CB_START=5800*ANALOGTV_H/63500,
  /* signal[row][ANALOGTV_PIC_START] is the first displayed pixel */
  ANALOGTV_PIC_START=9400*ANALOGTV_H/63500,
  ANALOGTV_PIC_LEN=52600*ANALOGTV_H/63500,
  ANALOGTV_FP_START=62000*ANALOGTV_H/63500,
  ANALOGTV_PIC_END=ANALOGTV_FP_START,

  /* TVs scan past the edges of the picture tube, so normally you only
     want to use about the middle 3/4 of the nominal scan line.
  */
  ANALOGTV_VIS_START=ANALOGTV_PIC_START + (ANALOGTV_PIC_LEN*1/8),
  ANALOGTV_VIS_END=ANALOGTV_PIC_START + (ANALOGTV_PIC_LEN*7/8),
  ANALOGTV_VIS_LEN=ANALOGTV_VIS_END-ANALOGTV_VIS_START,

  ANALOGTV_HASHNOISE_LEN=6*ANALOGTV_SCALE,

  ANALOGTV_GHOSTFIR_LEN=4,

  /* analogtv.signal is in IRE units, as defined below: */
  ANALOGTV_WHITE_LEVEL=100,
  ANALOGTV_GRAY50_LEVEL=55,
  ANALOGTV_GRAY30_LEVEL=35,
  ANALOGTV_BLACK_LEVEL=10,
  ANALOGTV_BLANK_LEVEL=0,
  ANALOGTV_SYNC_LEVEL=-40,
  ANALOGTV_CB_LEVEL=20,

  ANALOGTV_SIGNAL_LEN=ANALOGTV_V*ANALOGTV_H,

  /* The number of intensity levels we deal with for gamma correction &c */
  ANALOGTV_CV_MAX=1024,

  /* MAX_LINEHEIGHT corresponds to 2400 vertical pixels, beyond which
     it interpolates extra black lines. */
  ANALOGTV_MAX_LINEHEIGHT=12
};

} // ::atv