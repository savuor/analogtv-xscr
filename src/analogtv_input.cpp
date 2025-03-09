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

#include "precomp.hpp"
#include "analogtv_input.hpp"

namespace atv
{

void AnalogInput::setup_sync(int do_cb, int do_ssavi)
{
  int synclevel = do_ssavi ? ANALOGTV_WHITE_LEVEL : ANALOGTV_SYNC_LEVEL;

  for (int lineno = 0; lineno < ANALOGTV_V; lineno++)
  {
    int vsync = lineno >= 3 && lineno < 7;

    signed char *sig = this->sigMat[lineno];

    int p = ANALOGTV_SYNC_START;
    if (vsync)
    {
      while (p<ANALOGTV_BP_START) sig[p++] = ANALOGTV_BLANK_LEVEL;
      while (p<ANALOGTV_H)        sig[p++] = synclevel;
    }
    else
    {
      while (p<ANALOGTV_BP_START)  sig[p++] = synclevel;
      while (p<ANALOGTV_PIC_START) sig[p++] = ANALOGTV_BLANK_LEVEL;
      while (p<ANALOGTV_FP_START)  sig[p++] = ANALOGTV_BLACK_LEVEL;
    }
    while (p<ANALOGTV_H) sig[p++]=ANALOGTV_BLANK_LEVEL;

    if (do_cb)
    {
      /* 9 cycles of colorburst */
      for (int i = ANALOGTV_CB_START; i < ANALOGTV_CB_START + 36*ANALOGTV_SCALE; i+=4*ANALOGTV_SCALE)
      {
        sig[i+1] += ANALOGTV_CB_LEVEL;
        sig[i+3] -= ANALOGTV_CB_LEVEL;
      }
    }
  }
}


/*
  This takes a screen image and encodes it as a video camera would,
  including all the bandlimiting and YIQ modulation.
  This isn't especially tuned for speed.

  xoff, yoff: top left corner of rendered image, in window pixels.
  w, h: scaled size of rendered image, in window pixels.
  mask: BlackPixel means don't render (it's not full alpha)
*/

void AnalogInput::load_ximage(const cv::Mat4b& pic_im, const cv::Mat4b& mask_im,
                              int xoff, int yoff, int target_w, int target_h, int out_w, int out_h)
{
    int x_length=ANALOGTV_PIC_LEN;
    int y_overscan=5*ANALOGTV_SCALE; /* overscan this much top and bottom */
    int y_scanlength=ANALOGTV_VISLINES+2*y_overscan;

    if (target_w > 0) x_length     = x_length     * target_w / out_w;
    if (target_h > 0) y_scanlength = y_scanlength * target_h / out_h;

    int img_w = pic_im.cols;
    int img_h = pic_im.rows;

    xoff = ANALOGTV_PIC_LEN  * xoff / out_w;
    yoff = ANALOGTV_VISLINES * yoff / out_w;

    int multiq[ANALOGTV_PIC_LEN+4];
    for (int i=0; i<x_length+4; i++)
    {
        //TODO: check this, may be incorrect
        double phase = 90.0 + 90.0 * i;
        double ampl = 1.0;
        multiq[i] = (int)(cos(M_PI/180.0*(phase+33)) * 4096.0 * ampl);

        // double phase = 90.0 - 90.0 * i;
        // double ampl = 1.0;
        // multiq[i] = (int)(-cos(M_PI/180.0*(phase-303)) * 4096.0 * ampl);
    }

    for (int y = 0; y < y_scanlength; y++)
    {
        int picy1 = (y*img_h                 )/y_scanlength;
        int picy2 = (y*img_h + y_scanlength/2)/y_scanlength;

        cv::Vec4b col1[ANALOGTV_PIC_LEN];
        cv::Vec4b col2[ANALOGTV_PIC_LEN];
        char mask[ANALOGTV_PIC_LEN];

        const cv::Vec4b* rowIm1 = pic_im[picy1];
        const cv::Vec4b* rowIm2 = pic_im[picy2];

        uint32_t* rowMask1 = mask_im.data ? (uint32_t*)(mask_im.data + picy1 * mask_im.step) : nullptr;
        for (int x = 0; x < x_length; x++)
        {
            int picx = (x*img_w) / x_length;
            col1[x] = rowIm1[picx];
            col2[x] = rowIm2[picx];
            if (rowMask1)
                mask[x] = (rowMask1[picx] != 0);
            else
                mask[x] = 1;
        }

        int rowY[ANALOGTV_PIC_LEN];
        int rowI[ANALOGTV_PIC_LEN];
        int rowQ[ANALOGTV_PIC_LEN];
        for (int x = 0; x < x_length; x++)
        {
            int r1 = col1[x][2];
            int g1 = col1[x][1];
            int b1 = col1[x][0];
            int r2 = col2[x][2];
            int g2 = col2[x][1];
            int b2 = col2[x][0];

            /* Compute YIQ as:
            y=0.30 r + 0.59 g + 0.11 b
            i=0.60 r - 0.28 g - 0.32 b
            q=0.21 r - 0.52 g + 0.31 b
            The coefficients below are in .4 format */

            int rawy = (( 5*r1 + 11*g1 + 2*b1 +
                          5*r2 + 11*g2 + 2*b2) * 257) >>7;
            int rawi = ((10*r1 -  4*g1 - 5*b1 +
                         10*r2 -  4*g2 - 5*b2) * 257) >>7;
            int rawq = (( 3*r1 -  8*g1 + 5*b1 +
                          3*r2 -  8*g2 + 5*b2) * 257) >>7;

            rowY[x] = rawy;
            rowI[x] = rawi;
            rowQ[x] = rawq;
        }

        int fyx[7], fyy[7];
        int fix[4], fiy[4];
        int fqx[4], fqy[4];
        for (int i=0; i<7; i++) fyx[i]=fyy[i]=0;
        for (int i=0; i<4; i++) fix[i]=fiy[i]=fqx[i]=fqy[i]=0.0;

        signed char* sigRow = this->sigMat[y - y_overscan + ANALOGTV_TOP + yoff];
        for (int x = 0; x < x_length; x++)
        {
            if (!mask[x]) continue;

            int rawy = rowY[x];
            int rawi = rowI[x];
            int rawq = rowQ[x];

            /* Filter y at with a 4-pole low-pass Butterworth filter at 3.5 MHz
            with an extra zero at 3.5 MHz, from
            mkfilter -Bu -Lp -o 4 -a 2.1428571429e-01 0 -Z 2.5e-01 -l */

            fyx[0] = fyx[1]; fyx[1] = fyx[2]; fyx[2] = fyx[3];
            fyx[3] = fyx[4]; fyx[4] = fyx[5]; fyx[5] = fyx[6];
            fyx[6] = (rawy * 1897) >> 16;
            fyy[0] = fyy[1]; fyy[1] = fyy[2]; fyy[2] = fyy[3];
            fyy[3] = fyy[4]; fyy[4] = fyy[5]; fyy[5] = fyy[6];
            fyy[6] = (fyx[0]+fyx[6]) + 4*(fyx[1]+fyx[5]) + 7*(fyx[2]+fyx[4]) + 8*fyx[3]
                   + ((-151*fyy[2] + 8115*fyy[3] - 38312*fyy[4] + 36586*fyy[5]) >> 16);
            int filty = fyy[6];

            /* Filter I at 1.5 MHz. 3 pole Butterworth from
            mkfilter -Bu -Lp -o 3 -a 1.0714285714e-01 0 */

            fix[0] = fix[1]; fix[1] = fix[2]; fix[2] = fix[3];
            fix[3] = (rawi * 1413) >> 16;
            fiy[0] = fiy[1]; fiy[1] = fiy[2]; fiy[2] = fiy[3];
            fiy[3] = (fix[0]+fix[3]) + 3*(fix[1]+fix[2])
                   + ((16559*fiy[0] - 72008*fiy[1] + 109682*fiy[2]) >> 16);
            int filti = fiy[3];

            /* Filter Q at 0.5 MHz. 3 pole Butterworth from
            mkfilter -Bu -Lp -o 3 -a 3.5714285714e-02 0 -l */

            fqx[0] = fqx[1]; fqx[1] = fqx[2]; fqx[2] = fqx[3];
            fqx[3] = (rawq * 75) >> 16;
            fqy[0] = fqy[1]; fqy[1] = fqy[2]; fqy[2] = fqy[3];
            fqy[3] = (fqx[0]+fqx[3]) + 3 * (fqx[1]+fqx[2])
                   + ((2612*fqy[0] - 9007*fqy[1] + 10453 * fqy[2]) >> 12);
            int filtq = fqy[3];

            int composite = filty + ((multiq[x] * filti + multiq[x+3] * filtq)>>12);
            composite = ((composite*100)>>14) + ANALOGTV_BLACK_LEVEL;
            composite = std::clamp(composite, 0, 125);

            sigRow[x+ANALOGTV_PIC_START+xoff] = composite;
        }
    }
}


void AnalogInput::draw_solid(int left, int right, int top, int bot, int ntsc[4])
{
  left  = left  / 4;
  right = right / 4;

  right = std::max(right, left+1);
  bot   = std::max(bot,   top+1);

  typedef cv::Vec<int8_t, 4> Vec4c;
  Vec4c v(ntsc[0], ntsc[1], ntsc[2], ntsc[3]);
  for (int y = top; y < bot; y++)
  {
    Vec4c* sigRow = this->sigMat.ptr<Vec4c>(y);
    for (int x = left; x < right; x++)
    {
      sigRow[x] = v;
    }
  }
}

void analogtv_lcp_to_ntsc(double luma, double chroma, double phase, int ntsc[4])
{
  for (int i=0; i<4; i++)
  {
    double w=90.0*i + phase;
    double val=luma + chroma * (cos(M_PI/180.0*w));
    val = std::clamp(val, 0.0, 127.0);
    ntsc[i]=(int)val;
  }
}


void AnalogInput::draw_solid_rel_lcp(double left, double right, double top, double bot,
                                     double luma, double chroma, double phase)
{
  int ntsc[4];

  int topi   = (int)(ANALOGTV_TOP + ANALOGTV_VISLINES*top);
  int boti   = (int)(ANALOGTV_TOP + ANALOGTV_VISLINES*bot);
  int lefti  = (int)(ANALOGTV_VIS_START + ANALOGTV_VIS_LEN*left);
  int righti = (int)(ANALOGTV_VIS_START + ANALOGTV_VIS_LEN*right);

  analogtv_lcp_to_ntsc(luma, chroma, phase, ntsc);
  this->draw_solid(lefti, righti, topi, boti, ntsc);
}


} // ::atv