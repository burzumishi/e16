/*
 * Copyright (C) 2014 Kim Woelders
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
#ifndef _SHAPEWIN_H_
#define _SHAPEWIN_H_

#include "config.h"

#include "eobj.h"
#include "xwin.h"

typedef struct _ShapeWin ShapeWin;
struct _ShapeWin {
   EObj                o;
   EX_Pixmap           mask;
   GC                  gc;
};

ShapeWin           *ShapewinCreate(int md);
void                ShapewinDestroy(ShapeWin * sw);
void                ShapewinShapeSet(ShapeWin * sw, int md, int x, int y, int w,
				     int h, int bl, int br, int bt, int bb,
				     int seqno);

void                do_draw_technical(EX_Drawable dr, GC gc,
				      int a, int b, int c, int d, int bl,
				      int br, int bt, int bb);

#endif /* _SHAPEWIN_H_ */
