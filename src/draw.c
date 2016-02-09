/*
 * Copyright (C) 2000-2007 Carsten Haitzler, Geoff Harrison and various contributors
 * Copyright (C) 2007-2015 Kim Woelders
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
#include "config.h"

#include <X11/Xlib.h>

#include "E.h"
#include "desktops.h"
#include "eobj.h"
#include "ewins.h"
#include "shapewin.h"
#include "xwin.h"

#define MR_MODES_MOVE           0x47	/* MR_OPAQUE through MR_BOX and MR_TECH_OPAQUE */
#define MR_MODES_RESIZE         0x47	/* MR_OPAQUE through MR_BOX and MR_TECH_OPAQUE */

static Font         font = NoXID;	/* Used in mode 1 (technical) */

static void
draw_h_arrow(EX_Drawable dr, GC gc, int x1, int x2, int y1)
{
   char                str[32];

   if (x2 - x1 >= 12)
     {
	XDrawLine(disp, dr, gc, x1, y1, x1 + 6, y1 - 3);
	XDrawLine(disp, dr, gc, x1, y1, x1 + 6, y1 + 3);
	XDrawLine(disp, dr, gc, x2, y1, x2 - 6, y1 - 3);
	XDrawLine(disp, dr, gc, x2, y1, x2 - 6, y1 + 3);
     }
   if (x2 >= x1)
     {
	XDrawLine(disp, dr, gc, x1, y1, x2, y1);
	Esnprintf(str, sizeof(str), "%i", x2 - x1 + 1);
	XDrawString(disp, dr, gc, (x1 + x2) / 2, y1 - 10, str, strlen(str));
     }
}

static void
draw_v_arrow(EX_Drawable dr, GC gc, int y1, int y2, int x1)
{
   char                str[32];

   if (y2 - y1 >= 12)
     {
	XDrawLine(disp, dr, gc, x1, y1, x1 + 3, y1 + 6);
	XDrawLine(disp, dr, gc, x1, y1, x1 - 3, y1 + 6);
	XDrawLine(disp, dr, gc, x1, y2, x1 + 3, y2 - 6);
	XDrawLine(disp, dr, gc, x1, y2, x1 - 3, y2 - 6);
     }
   if (y2 >= y1)
     {
	XDrawLine(disp, dr, gc, x1, y1, x1, y2);
	Esnprintf(str, sizeof(str), "%i", y2 - y1 + 1);
	XDrawString(disp, dr, gc, x1 + 10, (y1 + y2) / 2, str, strlen(str));
     }
}

void
do_draw_technical(EX_Drawable dr, GC gc,
		  int a, int b, int c, int d, int bl, int br, int bt, int bb)
{
   if (!font)
      font = XLoadFont(disp, "-*-helvetica-medium-r-*-*-10-*-*-*-*-*-*-*");
   XSetFont(disp, gc, font);

   if (c < 3)
      c = 3;
   if (d < 3)
      d = 3;

   draw_h_arrow(dr, gc, a + bl, a + bl + c - 1, b + bt + d - 16);
   draw_h_arrow(dr, gc, 0, a - 1, b + bt + (d / 2));
   draw_h_arrow(dr, gc, a + c + bl + br, WinGetW(VROOT) - 1, b + bt + (d / 2));
   draw_v_arrow(dr, gc, b + bt, b + bt + d - 1, a + bl + 16);
   draw_v_arrow(dr, gc, 0, b - 1, a + bl + (c / 2));
   draw_v_arrow(dr, gc, b + d + bt + bb, WinGetH(VROOT) - 1, a + bl + (c / 2));

   XDrawLine(disp, dr, gc, a, 0, a, WinGetH(VROOT));
   XDrawLine(disp, dr, gc, a + c + bl + br - 1, 0,
	     a + c + bl + br - 1, WinGetH(VROOT));
   XDrawLine(disp, dr, gc, 0, b, WinGetW(VROOT), b);
   XDrawLine(disp, dr, gc, 0, b + d + bt + bb - 1,
	     WinGetW(VROOT), b + d + bt + bb - 1);

   XDrawRectangle(disp, dr, gc, a + bl + 1, b + bt + 1, c - 3, d - 3);
}

static void
do_draw_boxy(EX_Drawable dr, GC gc,
	     int a, int b, int c, int d, int bl, int br, int bt, int bb)
{
   if (c < 3)
      c = 3;
   if (d < 3)
      d = 3;
   XDrawRectangle(disp, dr, gc, a, b, c + bl + br - 1, d + bt + bb - 1);
   XDrawRectangle(disp, dr, gc, a + bl + 1, b + bt + 1, c - 3, d - 3);
}

typedef struct {
   EX_Window           root;
   GC                  gc;
   int                 xo, yo, wo, ho;
   int                 bl, br, bt, bb;
   ShapeWin           *shwin;
} ShapeData;

static void
_ShapeDrawNograb_tech_box(EWin * ewin, int md, int firstlast,
			  int xn, int yn, int wn, int hn, int seqno)
{
   ShapeData          *psd = (ShapeData *) ewin->shape_data;

   if (firstlast == 0 && !psd->shwin)
      psd->shwin = ShapewinCreate(md);
   if (!psd->shwin)
      return;

   ShapewinShapeSet(psd->shwin, md, xn, yn, wn, hn, psd->bl, psd->br, psd->bt,
		    psd->bb, seqno);
   EoMap(psd->shwin, 0);

   CoordsShow(ewin);

   if (firstlast == 2)
     {
	ShapewinDestroy(psd->shwin);
	psd->shwin = NULL;
     }
}

typedef void        (DrawFunc) (EX_Drawable dr, GC gc,
				int a, int b, int c, int d,
				int bl, int br, int bt, int bb);

static DrawFunc    *const draw_functions[] = {
   do_draw_technical, do_draw_boxy,
   NULL, NULL,
   NULL, do_draw_technical,
};

static void
_ShapeDrawNontranslucent(EWin * ewin, int md, int firstlast,
			 int xn, int yn, int wn, int hn)
{
   ShapeData          *psd = (ShapeData *) ewin->shape_data;
   DrawFunc           *drf;

   if (firstlast == 0)
     {
	XGCValues           gcv;

	gcv.function = GXxor;
	gcv.foreground = Dpy.pixel_white;
	if (gcv.foreground == 0)
	   gcv.foreground = Dpy.pixel_black;
	gcv.subwindow_mode = IncludeInferiors;
	psd->gc = EXCreateGC(psd->root,
			     GCFunction | GCForeground | GCSubwindowMode, &gcv);
     }

   drf = draw_functions[md - 1];

   if (firstlast > 0)
      drf(psd->root, psd->gc, psd->xo, psd->yo, psd->wo, psd->ho,
	  psd->bl, psd->br, psd->bt, psd->bb);

   CoordsShow(ewin);

   if (firstlast < 2)
      drf(psd->root, psd->gc, xn, yn, wn, hn,
	  psd->bl, psd->br, psd->bt, psd->bb);

   if (firstlast == 2)
     {
	EXFreeGC(psd->gc);
	psd->gc = NULL;
     }
}

void
DrawEwinShape(EWin * ewin, int md, int x, int y, int w, int h,
	      int firstlast, int seqno)
{
   ShapeData          *psd;
   int                 dx, dy;

   /* Quit if no change */
   if (firstlast == 1 &&
       (x == ewin->shape_x && y == ewin->shape_y &&
	(ewin->state.shaded || (w == ewin->shape_w && h == ewin->shape_h))))
      return;

   if ((md == MR_OPAQUE) || (md == MR_TECH_OPAQUE))
     {
	EwinOpMoveResize(ewin, OPSRC_USER, x, y, w, h);
	EwinShapeSet(ewin);
	CoordsShow(ewin);
	if (md == MR_OPAQUE)
	   goto done;
     }

   if (firstlast == 0)
     {
	EwinShapeSet(ewin);

	psd = ECALLOC(ShapeData, 1);
	ewin->shape_data = psd;
	if (!psd)
	   goto done;
	psd->root = WinGetXwin(VROOT);
	EwinBorderGetSize(ewin, &psd->bl, &psd->br, &psd->bt, &psd->bb);
     }
   psd = (ShapeData *) ewin->shape_data;
   if (!psd)
      goto done;

   dx = EoGetX(EoGetDesk(ewin));
   dy = EoGetY(EoGetDesk(ewin));
   ewin->shape_x = x;
   ewin->shape_y = y;
   x += dx;
   y += dy;

   if (!ewin->state.shaded)
     {
	ewin->shape_w = w;
	ewin->shape_h = h;
     }
   else
     {
	w = ewin->shape_w;
	h = ewin->shape_h;
     }

   if (((md <= MR_BOX) || (md == MR_TECH_OPAQUE)) &&
       Conf.movres.avoid_server_grab)
     {
	_ShapeDrawNograb_tech_box(ewin, md, firstlast, x, y, w, h, seqno);
	goto done;
     }

   switch (md)
     {
     case MR_TECHNICAL:
     case MR_TECH_OPAQUE:
     case MR_BOX:
	_ShapeDrawNontranslucent(ewin, md, firstlast, x, y, w, h);
	break;
     default:
	/* Fall back to opaque mode */
	Conf.movres.mode_move = MR_OPAQUE;
	break;
     }

   psd->xo = x;
   psd->yo = y;
   psd->wo = w;
   psd->ho = h;

 done:
   if (firstlast == 0 || firstlast == 2 || firstlast == 4)
     {
	ewin->req_x = ewin->shape_x;
	ewin->req_y = ewin->shape_y;
	if (firstlast == 2)
	  {
	     CoordsHide();
	     Efree(ewin->shape_data);
	     ewin->shape_data = NULL;
	  }
     }
}

void
DrawEwinShapeEnd(EWin * ewin)
{
   ShapeData          *psd = (ShapeData *) ewin->shape_data;

   if (!psd)
      return;
   if (psd->shwin)
      ShapewinDestroy(psd->shwin);
   Efree(psd);
   ewin->shape_data = NULL;
}

int
DrawEwinShapeNeedsGrab(int mode)
{
   if (mode == MR_OPAQUE)
      return 0;
   if ((mode <= MR_BOX) || (mode == MR_TECH_OPAQUE))
      return !Conf.movres.avoid_server_grab;
   return 1;
}

static int
_MoveResizeModeValidate(unsigned int valid, int md)
{
   if (md & ~0x1f)
      return MR_OPAQUE;
   if (valid & (1U << md))
      return md;
   return MR_OPAQUE;
}

int
MoveResizeModeValidateMove(int md)
{
   return _MoveResizeModeValidate(MR_MODES_MOVE, md);
}

int
MoveResizeModeValidateResize(int md)
{
   return _MoveResizeModeValidate(MR_MODES_RESIZE, md);
}
