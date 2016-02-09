/*
 * Copyright (C) 2008-2014 Kim Woelders
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _XTYPES_H_
#define _XTYPES_H_

#include <X11/Xlib.h>

#define EX_ID           unsigned int
#define EX_Atom         EX_ID
#define EX_Colormap     EX_ID
#define EX_Cursor       EX_ID
#define EX_Drawable     EX_ID
#define EX_KeySym       EX_ID
#define EX_Picture      EX_ID
#define EX_Pixmap       EX_ID
#define EX_SrvRegion    EX_ID
#define EX_Window       EX_ID

#define EX_KeyCode      unsigned char
#define EX_Time         unsigned int

typedef struct _xwin *Win;

#define NoXID   0U

#endif /* _XTYPES_H_ */
