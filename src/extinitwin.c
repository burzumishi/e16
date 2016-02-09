/*
 * Copyright (C) 2000-2007 Carsten Haitzler, Geoff Harrison and various contributors
 * Copyright (C) 2004-2015 Kim Woelders
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
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#define USE_EIWC_WINDOW 1
#if USE_XRENDER
#define USE_EIWC_RENDER 1
#include <X11/extensions/Xrender.h>
#endif

#include "E.h"
#include "eimage.h"
#include "xprop.h"
#include "xwin.h"

typedef struct {
   EX_Cursor           curs;
   XSetWindowAttributes attr;
   EX_Window           cwin;
} EiwData;

typedef void        (EiwLoopFunc) (EX_Window win, EImage * im, EiwData * d);

#if USE_EIWC_RENDER
#include <Imlib2.h>

static void         _eiw_render_loop(EX_Window win, EImage * im, EiwData * d);

static EiwLoopFunc *
_eiw_render_init(EX_Window win __UNUSED__, EiwData * d)
{
   Visual             *vis;

   /* Quit if no ARGB visual.
    * If we have, assume(?) a colored XRenderCreateCursor is available. */
   vis = EVisualFindARGB();
   if (!vis)
      return NULL;

   imlib_context_set_visual(vis);
   d->curs = NoXID;

   return _eiw_render_loop;
}

static void
_eiw_render_loop(EX_Window win, EImage * im, EiwData * d)
{
   int                 w, h;
   XRenderPictFormat  *pictfmt;
   EX_Pixmap           pmap;
   EX_Picture          pict;

   EImageGetSize(im, &w, &h);

   pictfmt = XRenderFindStandardFormat(disp, PictStandardARGB32);
   pmap = XCreatePixmap(disp, WinGetXwin(VROOT), w, h, 32);
   imlib_context_set_image(im);
   imlib_context_set_drawable(pmap);
   imlib_render_image_on_drawable(0, 0);
   pict = XRenderCreatePicture(disp, pmap, pictfmt, 0, 0);
   XFreePixmap(disp, pmap);

   if (d->curs != NoXID)
      XFreeCursor(disp, d->curs);
   d->curs = XRenderCreateCursor(disp, pict, w / 2, h / 2);
   XRenderFreePicture(disp, pict);

   XDefineCursor(disp, win, d->curs);
}

#endif /* USE_EIWC_RENDER */

#if USE_EIWC_WINDOW

static void         _eiw_window_loop(EX_Window win, EImage * im, EiwData * d);

static EiwLoopFunc *
_eiw_window_init(EX_Window win, EiwData * d)
{
   EX_Pixmap           pmap, mask;
   XColor              cl;

   d->cwin = XCreateWindow(disp, win, 0, 0, 32, 32, 0, CopyFromParent,
			   InputOutput, CopyFromParent,
			   CWOverrideRedirect | CWBackingStore | CWColormap |
			   CWBackPixel | CWBorderPixel, &d->attr);

   pmap = XCreatePixmap(disp, d->cwin, 16, 16, 1);
   EXFillAreaSolid(pmap, 0, 0, 16, 16, 0);

   mask = XCreatePixmap(disp, d->cwin, 16, 16, 1);
   EXFillAreaSolid(mask, 0, 0, 16, 16, 0);

   d->curs = XCreatePixmapCursor(disp, pmap, mask, &cl, &cl, 0, 0);
   XDefineCursor(disp, win, d->curs);
   XDefineCursor(disp, d->cwin, d->curs);

   return _eiw_window_loop;
}

static void
_eiw_window_loop(EX_Window win, EImage * im, EiwData * d)
{
   EX_Pixmap           pmap, mask;
   Window              ww;
   int                 dd, x, y, w, h;
   unsigned int        mm;

   EImageRenderPixmaps(im, NULL, 0, &pmap, &mask, 0, 0);
   EImageGetSize(im, &w, &h);
   XShapeCombineMask(disp, d->cwin, ShapeBounding, 0, 0, mask, ShapeSet);
   XSetWindowBackgroundPixmap(disp, d->cwin, pmap);
   EImagePixmapsFree(pmap, mask);
   XClearWindow(disp, d->cwin);
   XQueryPointer(disp, win, &ww, &ww, &dd, &dd, &x, &y, &mm);
   XMoveResizeWindow(disp, d->cwin, x - w / 2, y - h / 2, w, h);
   XMapWindow(disp, d->cwin);
}

#endif /* USE_EIWC_WINDOW */

static              EX_Window
ExtInitWinMain(void)
{
   int                 i, loop, err;
   EX_Window           win;
   EX_Pixmap           pmap;
   EX_Atom             a;
   EiwData             eiwd;
   EiwLoopFunc        *eiwc_loop_func;

   if (EDebug(EDBUG_TYPE_SESSION))
      Eprintf("%s: enter\n", __func__);

   err = EDisplayOpen(NULL, -1);
   if (err)
      return NoXID;

   EGrabServer();

   EImageInit();

   eiwd.attr.backing_store = NotUseful;
   eiwd.attr.override_redirect = True;
   eiwd.attr.colormap = WinGetCmap(VROOT);
   eiwd.attr.border_pixel = 0;
   eiwd.attr.background_pixel = 0;
   eiwd.attr.save_under = True;
   win = XCreateWindow(disp, WinGetXwin(VROOT),
		       0, 0, WinGetW(VROOT), WinGetH(VROOT),
		       0, CopyFromParent, InputOutput, CopyFromParent,
		       CWOverrideRedirect | CWSaveUnder | CWBackingStore |
		       CWColormap | CWBackPixel | CWBorderPixel, &eiwd.attr);

   pmap = XCreatePixmap(disp, win,
			WinGetW(VROOT), WinGetH(VROOT), WinGetDepth(VROOT));
   EXCopyArea(WinGetXwin(VROOT), pmap,
	      0, 0, WinGetW(VROOT), WinGetH(VROOT), 0, 0);
   XSetWindowBackgroundPixmap(disp, win, pmap);
   XMapRaised(disp, win);
   XFreePixmap(disp, pmap);

   a = ex_atom_get("ENLIGHTENMENT_RESTART_SCREEN");
   ex_window_prop_window_set(WinGetXwin(VROOT), a, &win, 1);

   XSelectInput(disp, win, StructureNotifyMask);

   EUngrabServer();
   ESync(0);

#if USE_EIWC_WINDOW && USE_EIWC_RENDER
   eiwc_loop_func = _eiw_render_init(win, &eiwd);
   if (!eiwc_loop_func)
      eiwc_loop_func = _eiw_window_init(win, &eiwd);
#elif USE_EIWC_RENDER
   eiwc_loop_func = _eiw_render_init(win, &eiwd);
#elif USE_EIWC_WINDOW
   eiwc_loop_func = _eiw_window_init(win, &eiwd);
#endif
   if (!eiwc_loop_func)
      return NoXID;

   {
      XWindowAttributes   xwa;
      char                s[1024];
      EImage             *im;

      for (i = loop = 1;; i++, loop++)
	{
	   if (i > 12)
	      i = 1;

	   /* If we get unmapped we are done */
	   XGetWindowAttributes(disp, win, &xwa);
	   if (xwa.map_state == IsUnmapped)
	      break;

	   Esnprintf(s, sizeof(s), "pix/wait%i.png", i);
	   if (EDebug(EDBUG_TYPE_SESSION) > 1)
	      Eprintf("%s: child %s\n", __func__, s);

	   im = ThemeImageLoad(s);
	   if (im)
	     {
		eiwc_loop_func(win, im, &eiwd);
		EImageFree(im);
	     }
	   ESync(0);
	   SleepUs(50000);

	   /* If we still are here after 5 sec something is wrong. */
	   if (loop > 100)
	      break;
	}
   }

   if (EDebug(EDBUG_TYPE_SESSION))
      Eprintf("%s: exit\n", __func__);

   EDisplayClose();

   exit(0);
}

EX_Window
ExtInitWinCreate(void)
{
   EX_Window           win_ex;	/* Hmmm.. */
   EX_Window           win;
   EX_Atom             a;

   if (EDebug(EDBUG_TYPE_SESSION))
      Eprintf("%s\n", __func__);

   a = ex_atom_get("ENLIGHTENMENT_RESTART_SCREEN");
   ESync(0);

   if (fork())
     {
	/* Parent */
	EUngrabServer();

	for (;;)
	  {
	     if (EDebug(EDBUG_TYPE_SESSION))
		Eprintf("%s: parent\n", __func__);

	     /* Hack to give the child some space. Not foolproof. */
	     sleep(1);

	     if (ex_window_prop_window_get
		 (WinGetXwin(VROOT), a, &win_ex, 1) > 0)
		break;
	  }

	win = win_ex;
	if (EDebug(EDBUG_TYPE_SESSION))
	   Eprintf("%s: parent - %#x\n", __func__, win);

	return win;
     }

   /* Child - Create the init window */

   if (EDebug(EDBUG_TYPE_SESSION))
      Eprintf("%s: child\n", __func__);

   /* Clean up inherited stuff */

   SignalsRestore();

   EImageExit(0);
   EDisplayDisconnect();

   ExtInitWinMain();

   /* We will never get here */
   return NoXID;
}

static EX_Window    init_win_ext = NoXID;

void
ExtInitWinSet(EX_Window win)
{
   init_win_ext = win;
}

EX_Window
ExtInitWinGet(void)
{
   return init_win_ext;
}

void
ExtInitWinKill(void)
{
   if (!disp || init_win_ext == NoXID)
      return;

   if (EDebug(EDBUG_TYPE_SESSION))
      Eprintf("%s: %#x\n", __func__, init_win_ext);
   XUnmapWindow(disp, init_win_ext);
   init_win_ext = NoXID;
}
