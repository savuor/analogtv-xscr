/* xscreensaver, Copyright (c) 1992-2014 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifndef __FIXED_FUNCS_H__
#define __FIXED_FUNCS_H__

#include "precomp.h"

// from resources.h
extern Bool get_boolean_resource (char*);
extern int get_integer_resource (char*);
extern double get_float_resource (char*);
extern unsigned int get_pixel_resource (char*);

// from visual.h
extern int visual_depth (Screen *, Visual *);
extern int visual_pixmap_depth (Screen *, Visual *);
extern int visual_class (Screen *, Visual *);
extern int visual_cells (Screen *, Visual *);

extern void visual_rgb_masks (Screen *screen, Visual *visual,
                              unsigned long *red_mask,
                              unsigned long *green_mask,
                              unsigned long *blue_mask);

// from xshm.h

extern XImage *create_xshm_image (Display *dpy, Visual *visual,
                                  unsigned int depth,
                                  int format,
                                  unsigned int width, unsigned int height);
extern Bool put_xshm_image (Display *dpy, Drawable d, GC gc, XImage *image,
                            int src_x, int src_y, int dest_x, int dest_y,
                            unsigned int width, unsigned int height);
extern void destroy_xshm_image (Display *dpy, XImage *image);

// from analogtv-cli.c
Status dummy_XAllocColor (Display *dpy, Colormap cmap, XColor *c);
int dummy_XClearArea (Display *dpy, Window win, int x, int y, unsigned int w, unsigned int h, Bool exp);
int dummy_XClearWindow (Display *dpy, Window window);
GC dummy_XCreateGC(Display *dpy, Drawable d, unsigned long mask, XGCValues *gcv);
XImage * custom_XCreateImage (Display *dpy, Visual *v, unsigned int depth,
                    int format, int offset, char *data,
                    unsigned int width, unsigned int height,
                    int bitmap_pad, int bytes_per_line);
int custom_XDestroyImage (XImage *ximage);
Pixmap dummy_XCreatePixmap (Display *dpy, Drawable d, unsigned int width,
               unsigned int height, unsigned int depth);
int dummy_XFreeGC (Display *dpy, GC gc);
int dummy_XFreePixmap (Display *dpy, Pixmap p);
Status custom_XGetWindowAttributes (Display *dpy, Window w, XWindowAttributes *xgwa);
int custom_XPutImage (Display *dpy, Drawable d, GC gc, XImage *image, 
                        int src_x, int src_y, int dest_x, int dest_y,
                        unsigned int w, unsigned int h);
int custom_XQueryColor (Display *dpy, Colormap cmap, XColor *color);
int custom_XQueryColors (Display *dpy, Colormap cmap, XColor *c, int n);
int dummy_XSetWindowBackground (Display *dpy, Window win, unsigned long bg);


#endif /* __FIXED_FUNCS_H__ */
