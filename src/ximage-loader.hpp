/* ximage-loader.h --- converts XPM data to Pixmaps.
 * xscreensaver, Copyright (c) 1998-2018 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifndef _XIMAGE_LOADER_H_
#define _XIMAGE_LOADER_H_

#include "precomp.hpp"

extern XImage *file_to_ximage (const char *filename);

#endif /* _XIMAGE_LOADER_H_ */