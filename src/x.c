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

#include <string.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/extensions/shape.h>
#if USE_XRENDER
#include <X11/extensions/Xrender.h>
#define RENDER_VERSION VERS(RENDER_MAJOR, RENDER_MINOR)
#endif
#if USE_XI2
#include <X11/extensions/XInput2.h>
#endif

#include "E.h"
#include "edebug.h"
#include "util.h"
#include "xwin.h"
#if USE_GLX
#include "eglx.h"
#endif

#define DEBUG_XWIN   0
#define DEBUG_PIXMAP 0

EDisplay            Dpy;
Display            *disp;

#if USE_COMPOSITE
static Visual      *argb_visual = NULL;
static EX_Colormap  argb_cmap = NoXID;
#endif

static XContext     xid_context = 0;

static Win          win_first = NULL;
static Win          win_last = NULL;

#define WinBgInvalidate(win) if (win->bg_owned > 0) win->bg_owned = -1

void
EXInit(void)
{
   memset(&Dpy, 0, sizeof(Dpy));
}

static              Win
_EXidCreate(void)
{
   Win                 win;

   win = ECALLOC(struct _xwin, 1);

   win->bgcol = 0xffffffff;

   return win;
}

static void
_EXidDestroy(Win win)
{
#if DEBUG_XWIN
   Eprintf("%s: %p %#x\n", __func__, win, win->xwin);
#endif
   if (win->rects)
      XFree(win->rects);
   Efree(win->cbl.lst);
   Efree(win);
}

static void
_EXidAdd(Win win)
{
#if DEBUG_XWIN
   Eprintf("%s: %p %#x\n", __func__, win, win->xwin);
#endif
   if (!xid_context)
      xid_context = XUniqueContext();

   XSaveContext(disp, win->xwin, xid_context, (XPointer) win);

   if (!win_first)
     {
	win_first = win_last = win;
     }
   else
     {
	win->prev = win_last;
	win_last->next = win;
	win_last = win;
     }
}

static void
_EXidDel(Win win)
{
#if DEBUG_XWIN
   Eprintf("%s: %p %#x\n", __func__, win, win->xwin);
#endif
   if (win == win_first)
     {
	if (win == win_last)
	  {
	     win_first = win_last = NULL;
	  }
	else
	  {
	     win_first = win->next;
	     win->next->prev = NULL;
	  }
     }
   else if (win == win_last)
     {
	win_last = win->prev;
	win->prev->next = NULL;
     }
   else
     {
	win->prev->next = win->next;
	win->next->prev = win->prev;
     }

   XDeleteContext(disp, win->xwin, xid_context);
   if (win->in_use)
      win->do_del = 1;
   else
      _EXidDestroy(win);
}

#define EXidLookup ELookupXwin

Win
EXidLookup(EX_Window xwin)
{
   Win                 win;
   XPointer            xp;

   if (!xid_context)
      return NULL;

   xp = NULL;
   if (XFindContext(disp, xwin, xid_context, &xp) == XCNOENT)
      xp = NULL;
   win = (Win) xp;

   return win;
}

static              Win
_EXidSet(EX_Window xwin, Win parent, int x, int y, int w, int h, int depth,
	 Visual * visual, EX_Colormap cmap)
{
   Win                 win;

   win = _EXidCreate();
   win->parent = parent;
   win->xwin = xwin;
   win->x = x;
   win->y = y;
   win->w = w;
   win->h = h;
   win->depth = depth;
   win->visual = visual;
   win->cmap = cmap;
   win->argb = depth == 32;
#if DEBUG_XWIN
   Eprintf("%s: %#x\n", __func__, win->xwin);
#endif
   _EXidAdd(win);

   return win;
}

void
EventCallbackRegister(Win win, EventCallbackFunc * func, void *prm)
{
   EventCallbackItem  *eci;

   if (!win)
      return;
#if 0
   Eprintf("%s: %p %#x: func=%p prm=%p\n", __func__, win, win->xwin, func, prm);
#endif

   win->cbl.num++;
   win->cbl.lst = EREALLOC(EventCallbackItem, win->cbl.lst, win->cbl.num);
   eci = win->cbl.lst + win->cbl.num - 1;
   eci->func = func;
   eci->prm = prm;
}

void
EventCallbackUnregister(Win win, EventCallbackFunc * func, void *prm)
{
   EventCallbackList  *ecl;
   EventCallbackItem  *eci;
   int                 i;

   if (!win)
      return;
#if 0
   Eprintf("%s: %p %#x: func=%p prm=%p\n", __func__, win, win->xwin, func, prm);
#endif

   ecl = &win->cbl;
   eci = ecl->lst;
   for (i = 0; i < ecl->num; i++, eci++)
      if (eci->func == func && eci->prm == prm)
	{
	   ecl->num--;
	   if (ecl->num)
	     {
		for (; i < ecl->num; i++, eci++)
		   *eci = *(eci + 1);
		win->cbl.lst =
		   EREALLOC(EventCallbackItem, win->cbl.lst, ecl->num);
	     }
	   else
	     {
		Efree(win->cbl.lst);
		win->cbl.lst = NULL;
	     }
	   return;
	}
}

void
EventCallbacksProcess(Win win, XEvent * ev)
{
   EventCallbackList  *ecl;
   EventCallbackItem  *eci;
   int                 i;

   if (!win)
      return;

   win->in_use = 1;
   ecl = &win->cbl;
   eci = ecl->lst;
   for (i = 0; i < ecl->num; i++, eci++)
     {
	if (EDebug(EDBUG_TYPE_DISPATCH))
	   Eprintf("%s: type=%d win=%#lx func=%p prm=%p\n", __func__,
		   ev->type, ev->xany.window, eci->func, eci->prm);
	eci->func(win, ev, eci->prm);
	if (win->do_del)
	  {
	     _EXidDestroy(win);
	     return;
	  }
     }
   win->in_use = 0;
}

Win
ECreateWindow(Win parent, int x, int y, int w, int h, int saveunder)
{
   Win                 win;
   EX_Window           xwin;
   XSetWindowAttributes attr;

   attr.backing_store = NotUseful;
   attr.override_redirect = False;
   attr.colormap = parent->cmap;
   attr.border_pixel = 0;
/*   attr.background_pixel = 0; */
   attr.background_pixmap = NoXID;
   if ((saveunder == 1) && (Conf.save_under))
      attr.save_under = True;
   else if (saveunder == 2)
      attr.save_under = True;
   else
      attr.save_under = False;

   xwin = XCreateWindow(disp, parent->xwin, x, y, w, h, 0,
			CopyFromParent, InputOutput, CopyFromParent,
			CWOverrideRedirect | CWSaveUnder | CWBackingStore |
			CWColormap | CWBackPixmap | CWBorderPixel, &attr);
   win = _EXidSet(xwin, parent, x, y, w, h, parent->depth, parent->visual,
		  parent->cmap);

   return win;
}

#if USE_COMPOSITE
static              Win
_ECreateWindowVDC(Win parent, int x, int y, int w, int h,
		  Visual * vis, unsigned int depth, EX_Colormap cmap)
{
   Win                 win;
   EX_Window           xwin;
   XSetWindowAttributes attr;

   attr.background_pixmap = NoXID;
   attr.border_pixel = 0;
   attr.backing_store = NotUseful;
   attr.save_under = False;
   attr.override_redirect = False;
   attr.colormap = cmap;

   xwin = XCreateWindow(disp, parent->xwin, x, y, w, h, 0,
			depth, InputOutput, vis,
			CWOverrideRedirect | CWSaveUnder | CWBackingStore |
			CWColormap | CWBackPixmap | CWBorderPixel, &attr);
   win = _EXidSet(xwin, parent, x, y, w, h, depth, vis, cmap);

   return win;
}

Win
ECreateArgbWindow(Win parent, int x, int y, int w, int h, Win cwin)
{
   int                 depth;
   Visual             *vis;
   EX_Colormap         cmap;

   if (cwin && Conf.testing.argb_clients_inherit_attr)
     {
	depth = cwin->depth;
	vis = cwin->visual;
	cmap = cwin->cmap;
     }
   else
     {
	if (!argb_visual)
	  {
	     argb_visual = EVisualFindARGB();
	     argb_cmap =
		XCreateColormap(disp, WinGetXwin(VROOT), argb_visual,
				AllocNone);
	  }
	depth = 32;
	vis = argb_visual;
	cmap = argb_cmap;
     }

   return _ECreateWindowVDC(parent, x, y, w, h, vis, depth, cmap);
}

#if USE_GLX
static              Win
_ECreateWindowVD(Win parent, int x, int y, int w, int h,
		 Visual * vis, unsigned int depth)
{
   EX_Colormap         cmap;

   if (!vis || depth == 0)
      return 0;

   cmap = XCreateColormap(disp, WinGetXwin(VROOT), vis, AllocNone);

   return _ECreateWindowVDC(parent, x, y, w, h, vis, depth, cmap);
}
#endif

Win
ECreateObjectWindow(Win parent, int x, int y, int w, int h, int saveunder,
		    int type, Win cwin)
{
   Win                 win;
   int                 argb = 0;

   switch (type)
     {
     default:
     case WIN_TYPE_NO_ARGB:
	break;
     case WIN_TYPE_CLIENT:
	if (Conf.testing.argb_clients || EVisualIsARGB(cwin->visual))
	   argb = 1;
	break;
     case WIN_TYPE_INTERNAL:
	if (Conf.testing.argb_internal_objects)
	   argb = 1;
	break;
#if USE_GLX
     case WIN_TYPE_GLX:	/* Internal GL */
	win =
	   _ECreateWindowVD(parent, x, y, w, h, EGlGetVisual(), EGlGetDepth());
	return win;
#endif
     }

   if (argb)
      win = ECreateArgbWindow(parent, x, y, w, h, cwin);
   else
      win = ECreateWindow(parent, x, y, w, h, saveunder);

   return win;
}

#else

Win
ECreateObjectWindow(Win parent, int x, int y, int w, int h, int saveunder,
		    int type __UNUSED__, Win cwin __UNUSED__)
{
   return ECreateWindow(parent, x, y, w, h, saveunder);
}

#endif /* USE_COMPOSITE */

Win
ECreateClientWindow(Win parent, int x, int y, int w, int h)
{
#if USE_COMPOSITE
   if (Conf.testing.argb_internal_clients)
      return ECreateArgbWindow(parent, x, y, w, h, NULL);
#endif

   return ECreateWindow(parent, x, y, w, h, 0);
}

Win
ECreateEventWindow(Win parent, int x, int y, int w, int h)
{
   Win                 win;
   EX_Window           xwin;
   XSetWindowAttributes attr;

   attr.override_redirect = False;

   xwin = XCreateWindow(disp, parent->xwin, x, y, w, h, 0, 0, InputOnly,
			CopyFromParent, CWOverrideRedirect, &attr);
   win = _EXidSet(xwin, parent, x, y, w, h, 0, NULL, NoXID);

   return win;
}

#if 0				/* Not used */
/*
 * create a window which will accept the keyboard focus when no other 
 * windows have it
 */
Win
ECreateFocusWindow(Win parent, int x, int y, int w, int h)
{
   Win                 win;
   XSetWindowAttributes attr;

   attr.backing_store = NotUseful;
   attr.override_redirect = False;
   attr.colormap = WinGetCmap(VROOT);
   attr.border_pixel = 0;
   attr.background_pixel = 0;
   attr.save_under = False;
   attr.event_mask = KeyPressMask | FocusChangeMask;

   EX_Window           xwin, xpar;

   win = XCreateWindow(disp, parent, x, y, w, h, 0, 0, InputOnly,
		       CopyFromParent,
		       CWOverrideRedirect | CWSaveUnder | CWBackingStore |
		       CWColormap | CWBackPixel | CWBorderPixel | CWEventMask,
		       &attr);

   XSetWindowBackground(disp, win, 0);
   XMapWindow(disp, win);
   XSetInputFocus(disp, win, RevertToParent, CurrentTime);

   return win;
}
#endif

void
EMoveWindow(Win win, int x, int y)
{
   if (!win)
      return;

#if 0
   Eprintf("%s: %p %#x: %d,%d %dx%d -> %d,%d\n", __func__,
	   win, win->xwin, win->x, win->y, win->w, win->h, x, y);
#endif
   if ((x == win->x) && (y == win->y))
      return;

   win->x = x;
   win->y = y;

   XMoveWindow(disp, win->xwin, x, y);
}

void
EResizeWindow(Win win, int w, int h)
{
   if (!win)
      return;

   if ((w == win->w) && (h == win->h))
      return;

   WinBgInvalidate(win);
   win->w = w;
   win->h = h;

   XResizeWindow(disp, win->xwin, w, h);
}

void
EMoveResizeWindow(Win win, int x, int y, int w, int h)
{
   if (!win)
      return;

#if 0
   Eprintf("%s: %p %#x: %d,%d %dx%d -> %d,%d %dx%d\n", __func__,
	   win, win->xwin, win->x, win->y, win->w, win->h, x, y, w, h);
#endif
   if ((w == win->w) && (h == win->h) && (x == win->x) && (y == win->y))
      return;

   if (w != win->w || h != win->h)
      WinBgInvalidate(win);

   win->x = x;
   win->y = y;
   win->w = w;
   win->h = h;

   XMoveResizeWindow(disp, win->xwin, x, y, w, h);
}

static int
_ExDelTree(Win win)
{
   Win                 win2;
   int                 nsub;

   win->do_del = -1;

   nsub = 0;
   for (win2 = win_first; win2; win2 = win2->next)
     {
	if (win2->parent != win)
	   continue;
	_ExDelTree(win2);
	nsub++;
     }

   return nsub;
}

void
EDestroyWindow(Win win)
{
   Win                 next;
   int                 nsub;

   if (!win)
      return;

#if DEBUG_XWIN
   Eprintf("%s: %p %#x\n", __func__, win, win->xwin);
#endif
   if (win->parent != NoXID)
     {
	EFreeWindowBackgroundPixmap(win);
	XDestroyWindow(disp, win->xwin);
     }

   /* Mark the ones to be deleted */
   nsub = _ExDelTree(win);
   if (nsub == 0)
     {
	/* No children */
	_EXidDel(win);
	return;
     }

   /* Delete entire tree */
   for (win = win_first; win; win = next)
     {
	next = win->next;
	if (win->do_del < 0)
	   _EXidDel(win);
     }
}

void
EWindowSync(Win win)
{
   Window              rr;
   int                 x, y;
   unsigned int        w, h, bw, depth;

   if (!win)
      return;

   XGetGeometry(disp, win->xwin, &rr, &x, &y, &w, &h, &bw, &depth);
#if 0
   Eprintf("%s: %p %#x: %d,%d %dx%d -> %d,%d %dx%d\n", __func__,
	   win, win->xwin, win->x, win->y, win->w, win->h, x, y, w, h);
#endif
   win->x = x;
   win->y = y;
   win->w = w;
   win->h = h;
   win->depth = depth;
}

void
EWindowSetGeometry(Win win, int x, int y, int w, int h, int bw)
{
   if (!win)
      return;

   win->x = x;
   win->y = y;
   win->w = w;
   win->h = h;
   win->bw = bw;
}

void
EWindowSetMapped(Win win, int mapped)
{
   if (!win)
      return;

   win->mapped = mapped;
}

EX_Window
EXWindowGetParent(EX_Window xwin)
{
   Window              parent, rt;
   Window             *pch = NULL;
   unsigned int        nch = 0;

   parent = NoXID;
   if (!XQueryTree(disp, xwin, &rt, &parent, &pch, &nch))
      parent = NoXID;
   else if (pch)
      XFree(pch);

   return parent;
}

Win
ECreateWinFromXwin(EX_Window xwin)
{
   Win                 win;
   Window              rr;
   int                 x, y;
   unsigned int        w, h, bw, depth;

   if (!XGetGeometry(disp, xwin, &rr, &x, &y, &w, &h, &bw, &depth))
      return NULL;

   win = _EXidCreate();
   if (!win)
      return NULL;

   win->xwin = xwin;
   win->x = x;
   win->y = y;
   win->w = w;
   win->h = h;
   win->depth = depth;
   win->visual = WinGetVisual(VROOT);
   win->cmap = WinGetCmap(VROOT);
#if DEBUG_XWIN
   Eprintf("%s: %p %#x\n", __func__, win, win->xwin);
#endif

   return win;
}

void
EDestroyWin(Win win)
{
   _EXidDestroy(win);
}

Win
ERegisterWindow(EX_Window xwin, XWindowAttributes * pxwa)
{
   Win                 win;
   XWindowAttributes   xwa;

   win = EXidLookup(xwin);
   if (win)
      goto done;

   if (!pxwa)
     {
	pxwa = &xwa;
	if (!EXGetWindowAttributes(xwin, pxwa))
	   goto done;
     }

#if 0
   Eprintf("%s: %p #%x %d+%d %dx%d\n", __func__, win, xwin,
	   pxwa->x, pxwa->y, pxwa->width, pxwa->height);
#endif
   win = _EXidSet(xwin, NoXID, pxwa->x, pxwa->y, pxwa->width, pxwa->height,
		  pxwa->depth, pxwa->visual, pxwa->colormap);
   win->mapped = pxwa->map_state != IsUnmapped;
   win->attached = 1;

 done:
   return win;
}

void
EUnregisterXwin(EX_Window xwin)
{
   Win                 win;

   win = EXidLookup(xwin);
   if (!win)
      return;

   /* FIXME - We shouldn't go here */
   _EXidDel(win);
#if 1				/* Debug - Fix code if we get here */
   Eprintf("*** FIXME - %s %#x\n", __func__, xwin);
#endif
}

void
EUnregisterWindow(Win win)
{
   if (!win)
      return;

   if (win->cbl.lst)
     {
	if (EDebug(1))
	   Eprintf("%s(%#x) Ignored (%d callbacks remain)\n",
		   __func__, win->xwin, win->cbl.num);
	return;
     }

   _EXidDel(win);
}

void
EMapWindow(Win win)
{
   if (!win)
      return;

   if (win->mapped)
      return;
   win->mapped = 1;

   XMapWindow(disp, win->xwin);
}

void
EUnmapWindow(Win win)
{
   if (!win)
      return;

   if (!win->mapped)
      return;
   win->mapped = 0;

   XUnmapWindow(disp, win->xwin);
}

void
EReparentWindow(Win win, Win parent, int x, int y)
{
   if (!win)
      return;

#if 0
   Eprintf
      ("%s: %p %#lx: %d %#lx->%#lx %d,%d %dx%d -> %d,%d\n", __func__,
       win, win->xwin, win->mapped, (win->parent) ? win->parent->xwin : NoXID,
       parent->xwin, win->x, win->y, win->w, win->h, x, y);
#endif
   if (parent == win->parent)
     {
	if ((x != win->x) || (y != win->y))
	  {
	     win->x = x;
	     win->y = y;
	     XMoveWindow(disp, win->xwin, x, y);
	  }
	return;
     }
   else
     {
	win->parent = parent;
	win->x = x;
	win->y = y;
     }

   XReparentWindow(disp, win->xwin, parent->xwin, x, y);
}

void
EMapRaised(Win win)
{
   if (!win)
      return;

   if (win->mapped)
     {
	XRaiseWindow(disp, win->xwin);
	return;
     }
   else
     {
	win->mapped = 1;
     }

   XMapRaised(disp, win->xwin);
}

int
EXGetWindowAttributes(EX_Window xwin, XWindowAttributes * pxwa)
{
   return XGetWindowAttributes(disp, xwin, pxwa);
}

int
EXGetGeometry(EX_Drawable draw, EX_Window * root_return, int *x, int *y,
	      int *w, int *h, int *bw, int *depth)
{
   int                 ok;
   Window              rr;
   int                 xx, yy;
   unsigned int        ww, hh, bb, dd;

   ok = XGetGeometry(disp, draw, &rr, &xx, &yy, &ww, &hh, &bb, &dd);
   if (!ok)
      goto done;

   if (root_return)
      *root_return = rr;
   if (x)
      *x = xx;
   if (y)
      *y = yy;
   if (w)
      *w = ww;
   if (h)
      *h = hh;
   if (bw)
      *bw = bb;
   if (depth)
      *depth = dd;

 done:
#if 0				/* Debug */
   if (!ok)
      Eprintf("%s win=%#x, error %d\n", __func__, (unsigned)win, ok);
#endif
   return ok;
}

int
EGetGeometry(Win win, EX_Window * root_return, int *x, int *y,
	     int *w, int *h, int *bw, int *depth)
{
   if (!win)
      return 0;

   if (x)
      *x = win->x;
   if (y)
      *y = win->y;
   if (w)
      *w = win->w;
   if (h)
      *h = win->h;
   if (bw)
      *bw = 0;
   if (depth)
      *depth = win->depth;
   if (root_return)
      *root_return = WinGetXwin(VROOT);

   return 1;
}

void
EGetWindowAttributes(Win win, XWindowAttributes * pxwa)
{
   if (!win)
      return;

   pxwa->x = win->x;
   pxwa->y = win->y;
   pxwa->width = win->w;
   pxwa->height = win->h;
   pxwa->border_width = win->bw;
   pxwa->depth = win->depth;
   pxwa->visual = win->visual;
   pxwa->colormap = win->cmap;
}

#if 0				/* Unused */
void
EConfigureWindow(Win win, unsigned int mask, XWindowChanges * wc)
{
   char                doit = 0;

   if (!win)
      return;

   if ((mask & CWX) && (wc->x != win->x))
     {
	win->x = wc->x;
	doit = 1;
     }
   if ((mask & CWY) && (wc->y != win->y))
     {
	win->y = wc->y;
	doit = 1;
     }
   if ((mask & CWWidth) && (wc->width != win->w))
     {
	WinBgInvalidate(win);
	win->w = wc->width;
	doit = 1;
     }
   if ((mask & CWHeight) && (wc->height != win->h))
     {
	WinBgInvalidate(win);
	win->h = wc->height;
	doit = 1;
     }

   if ((doit) || (mask & (CWBorderWidth | CWSibling | CWStackMode)))
      XConfigureWindow(disp, win->xwin, mask, wc);
}
#endif

void
ESetWindowBackgroundPixmap(Win win, EX_Pixmap pmap, int kept)
{
   if (!win)
      return;

   if (win->bgpmap && win->bg_owned)
      EFreeWindowBackgroundPixmap(win);
   win->bgpmap = kept ? pmap : NoXID;
   win->bg_owned = 0;		/* Don't manage pixmap */
   win->bgcol = 0xffffffff;	/* Hmmm.. */

   XSetWindowBackgroundPixmap(disp, win->xwin, pmap);
}

EX_Pixmap
EGetWindowBackgroundPixmap(Win win)
{
   EX_Pixmap           pmap;

   if (!win)
      return NoXID;

   if (win->bg_owned < 0)	/* Free if invalidated */
      EFreeWindowBackgroundPixmap(win);
   else if (win->bgpmap)
      return win->bgpmap;

   /* Allocate/set new */
   pmap = ECreatePixmap(win, win->w, win->h, 0);
   ESetWindowBackgroundPixmap(win, pmap, 1);
   win->bg_owned = 1;		/* Manage pixmap */

   return pmap;
}

void
EFreeWindowBackgroundPixmap(Win win)
{
   if (!win || !win->bgpmap)
      return;

   if (win->bg_owned)
      EFreePixmap(win->bgpmap);
   win->bgpmap = NoXID;
   win->bg_owned = 0;
}

void
ESetWindowBackground(Win win, unsigned int col)
{
   if (!win)
      return;

   if (win->bgpmap)
     {
	EFreeWindowBackgroundPixmap(win);
	win->bgcol = col;
     }
   else if (win->bgcol != col)
     {
	win->bgcol = col;
     }
   else
      return;

   XSetWindowBackground(disp, win->xwin, col);
}

#if USE_XI2
void
EXIMaskSetup(EXIEventMask * em, int dev, unsigned int event_mask)
{
   em->em.mask_len = sizeof(em->mb);
   em->em.mask = em->mb;
   memset(em->mb, 0, sizeof(em->mb));

   em->em.deviceid = dev;

   if (event_mask & KeyPressMask)
      XISetMask(em->mb, XI_KeyPress);
   if (event_mask & KeyReleaseMask)
      XISetMask(em->mb, XI_KeyRelease);

   if (event_mask & ButtonPressMask)
      XISetMask(em->mb, XI_ButtonPress);
   if (event_mask & ButtonReleaseMask)
      XISetMask(em->mb, XI_ButtonRelease);
   if (event_mask & PointerMotionMask)
      XISetMask(em->mb, XI_Motion);

   if (event_mask & EnterWindowMask)
      XISetMask(em->mb, XI_Enter);
   if (event_mask & LeaveWindowMask)
      XISetMask(em->mb, XI_Leave);

   if (event_mask & FocusChangeMask)
     {
	XISetMask(em->mb, XI_FocusIn);
	XISetMask(em->mb, XI_FocusOut);
     }
}
#endif

void
ESelectInput(Win win, unsigned int event_mask)
{
#if USE_XI2
   unsigned int        evold_mask;

#define EVENTS_TO_XI_KBD (KeyPressMask | KeyReleaseMask)
#define EVENTS_TO_XI_PTR (ButtonPressMask | ButtonReleaseMask | PointerMotionMask)
#define EVENTS_TO_XI_WEL (EnterWindowMask | LeaveWindowMask)
#define EVENTS_TO_XI_WFO (FocusChangeMask)
#define EVENTS_TO_XI \
   (EVENTS_TO_XI_KBD | EVENTS_TO_XI_PTR | EVENTS_TO_XI_WEL | EVENTS_TO_XI_WFO)

   evold_mask = win->event_mask;
   win->event_mask = event_mask;

   if (((event_mask ^ evold_mask) & EVENTS_TO_XI) && XEXT_AVAILABLE(XEXT_XI))
     {
	EXIEventMask        em;

	EXIMaskSetup(&em, XIAllMasterDevices, event_mask & EVENTS_TO_XI);
	XISelectEvents(disp, win->xwin, &em.em, 1);

	event_mask &= ~EVENTS_TO_XI;
	evold_mask &= ~EVENTS_TO_XI;
     }

   if (event_mask ^ evold_mask)
     {
	XSelectInput(disp, win->xwin, event_mask);
     }
#else
   XSelectInput(disp, win->xwin, event_mask);
#endif
}

void
ESelectInputChange(Win win, unsigned int set, unsigned int clear)
{
#if USE_XI2
   ESelectInput(win, (win->event_mask | set) & ~clear);
#else
   XWindowAttributes   xwa;

   EXGetWindowAttributes(win->xwin, &xwa);
   xwa.your_event_mask |= set;
   xwa.your_event_mask &= ~clear;
   XSelectInput(disp, win->xwin, xwa.your_event_mask);
#endif
}

void
EChangeWindowAttributes(Win win, unsigned int mask, XSetWindowAttributes * attr)
{
   XChangeWindowAttributes(disp, win->xwin, mask, attr);
}

void
ESetWindowBorderWidth(Win win, unsigned int bw)
{
   XSetWindowBorderWidth(disp, win->xwin, bw);
}

void
ERaiseWindow(Win win)
{
   XRaiseWindow(disp, win->xwin);
}

void
ELowerWindow(Win win)
{
   XLowerWindow(disp, win->xwin);
}

void
EXRestackWindows(EX_Window * windows, int nwindows)
{
#if SIZEOF_INT == SIZEOF_LONG
   XRestackWindows(disp, (Window *) windows, nwindows);
#else
   int                 i;
   Window             *_wins;

   _wins = EMALLOC(Window, nwindows);
   if (!_wins)
      return;

   for (i = 0; i < nwindows; i++)
      _wins[i] = windows[i];
   XRestackWindows(disp, _wins, nwindows);

   Efree(_wins);
#endif
}

void
EClearWindow(Win win)
{
   XClearWindow(disp, win->xwin);
}

void
EClearWindowExpose(Win win)
{
   XClearArea(disp, win->xwin, 0, 0, 0, 0, True);
}

void
EClearArea(Win win, int x, int y, unsigned int w, unsigned int h)
{
   XClearArea(disp, win->xwin, x, y, w, h, False);
}

int
ETranslateCoordinates(Win src_w, Win dst_w, int src_x, int src_y,
		      int *dest_x_return, int *dest_y_return,
		      EX_Window * child_return)
{
   Window              child;
   Bool                rc;

   rc = XTranslateCoordinates(disp, src_w->xwin, dst_w->xwin, src_x, src_y,
			      dest_x_return, dest_y_return, &child);

   if (child_return)
      *child_return = child;

   return rc;
}

void
EXWarpPointer(EX_Window xwin, int x, int y)
{
   XWarpPointer(disp, NoXID, xwin, 0, 0, 0, 0, x, y);
}

void
EWarpPointer(Win win, int x, int y)
{
   EXWarpPointer(win ? win->xwin : NoXID, x, y);
}

int
EXQueryPointer(EX_Window xwin, int *px, int *py, EX_Window * pchild,
	       unsigned int *pmask)
{
   Window              root, child;
   int                 root_x, root_y;
   unsigned int        mask;
   Bool                rc;

   if (xwin == NoXID)
      xwin = WinGetXwin(VROOT);

   if (!px)
      px = &root_x;
   if (!py)
      py = &root_y;
   if (!pmask)
      pmask = &mask;

   rc = XQueryPointer(disp, xwin, &root, &child, &root_x, &root_y, px, py,
		      pmask);

   if (pchild)
      *pchild = child;

   return rc;
}

int
EQueryPointer(Win win, int *px, int *py, EX_Window * pchild,
	      unsigned int *pmask)
{
   EX_Window           xwin;

   xwin = (win) ? win->xwin : WinGetXwin(VROOT);

   return EXQueryPointer(xwin, px, py, pchild, pmask);
}

int
EXDrawableOk(EX_Drawable draw)
{
   if (draw == NoXID)
      return 0;

   return EXGetGeometry(draw, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

int
EXWindowOk(EX_Window xwin)
{
   XWindowAttributes   xwa;

   if (xwin == NoXID)
      return 0;

   return EXGetWindowAttributes(xwin, &xwa);
}

EX_KeyCode
EKeysymToKeycode(EX_KeySym keysym)
{
   return XKeysymToKeycode(disp, keysym);
}

EX_KeyCode
EKeynameToKeycode(const char *name)
{
   return XKeysymToKeycode(disp, XStringToKeysym(name));
}

#define DEBUG_SHAPE_OPS       0
#define DEBUG_SHAPE_PROPAGATE 0

#if DEBUG_SHAPE_OPS || DEBUG_SHAPE_PROPAGATE
static void
_EShapeShow(const char *txt, EX_Window xwin, XRectangle * pr, int nr)
{
   int                 i;

   Eprintf("%s %#x nr=%d\n", txt, xwin, nr);
   for (i = 0; i < nr; i++)
      Eprintf(" %3d - %4d,%4d %4dx%4d\n", i,
	      pr[i].x, pr[i].y, pr[i].width, pr[i].height);
}
#endif

int
EShapeUpdate(Win win)
{
   if (win->rects)
     {
	XFree(win->rects);
	win->num_rect = 0;
     }

   win->rects =
      XShapeGetRectangles(disp, win->xwin, ShapeBounding, &(win->num_rect),
			  &(win->ord));
   if (win->rects)
     {
	if (win->num_rect == 1)
	  {
	     if ((win->rects[0].x == 0) && (win->rects[0].y == 0)
		 && (win->rects[0].width == win->w)
		 && (win->rects[0].height == win->h))
	       {
		  win->num_rect = 0;
		  XFree(win->rects);
		  win->rects = NULL;
		  XShapeCombineMask(disp, win->xwin, ShapeBounding, 0, 0,
				    NoXID, ShapeSet);
	       }
	  }
	else if (win->num_rect > 4096)
	  {
	     Eprintf("*** %s: nrect=%d - Not likely, ignoring.\n", __func__,
		     win->num_rect);
	     XShapeCombineMask(disp, win->xwin, ShapeBounding, 0, 0, NoXID,
			       ShapeSet);
	     win->num_rect = 0;
	     XFree(win->rects);
	     win->rects = NULL;
	  }
     }
   else
     {
	win->num_rect = -1;
     }
#if DEBUG_SHAPE_OPS
   _EShapeShow(__func__, win->xwin, win->rects, win->num_rect);
#endif
   return win->num_rect != 0;
}

static void
_EShapeCombineMask(Win win, int dest, int x, int y, EX_Pixmap pmap, int op)
{
   char                wasshaped = 0;

   if (!win)
      return;

   if (win->rects || win->num_rect < 0)
     {
	win->num_rect = 0;
	if (win->rects)
	   XFree(win->rects);
	win->rects = NULL;
	wasshaped = 1;
     }
#if DEBUG_SHAPE_OPS
   Eprintf("%s %#x %d,%d %dx%d mask=%#x wassh=%d\n", __func__,
	   win->xwin, win->x, win->y, win->w, win->h, pmap, wasshaped);
#endif
   if (pmap)
     {
	XShapeCombineMask(disp, win->xwin, dest, x, y, pmap, op);
	EShapeUpdate(win);
     }
   else if (wasshaped)
      XShapeCombineMask(disp, win->xwin, dest, x, y, pmap, op);
}

static void
_EShapeCombineMaskTiled(Win win, int dest, int x, int y,
			EX_Pixmap pmap, int op, int w, int h)
{
   XGCValues           gcv;
   GC                  gc;
   EX_Window           tm;

   gcv.fill_style = FillTiled;
   gcv.tile = pmap;
   gcv.ts_x_origin = 0;
   gcv.ts_y_origin = 0;
   tm = ECreatePixmap(win, w, h, 1);
   gc = EXCreateGC(tm, GCFillStyle | GCTile |
		   GCTileStipXOrigin | GCTileStipYOrigin, &gcv);
   XFillRectangle(disp, tm, gc, 0, 0, w, h);
   EXFreeGC(gc);
   _EShapeCombineMask(win, dest, x, y, tm, op);
   EFreePixmap(tm);
}

static void
_EShapeCombineRectangles(Win win, int dest, int x, int y,
			 XRectangle * rect, int n_rects, int op, int ordering)
{
   if (!win)
      return;
#if DEBUG_SHAPE_OPS
   Eprintf("%s %#x %d\n", __func__, win->xwin, n_rects);
#endif

   if (n_rects == 1 && op == ShapeSet)
     {
	if ((rect[0].x == 0) && (rect[0].y == 0) &&
	    (rect[0].width == win->w) && (rect[0].height == win->h))
	  {
	     win->num_rect = 0;
	     XFree(win->rects);
	     win->rects = NULL;
	     XShapeCombineMask(disp, win->xwin, dest, x, y, NoXID, op);
	     return;
	  }
     }
   XShapeCombineRectangles(disp, win->xwin, dest, x, y, rect, n_rects, op,
			   ordering);
   if (n_rects > 1)
     {
	/* Limit shape to window extents */
	XRectangle          r;

	r.x = r.y = 0;
	r.width = win->w;
	r.height = win->h;
	XShapeCombineRectangles(disp, win->xwin, ShapeBounding, 0, 0, &r,
				1, ShapeIntersect, Unsorted);
     }
   EShapeUpdate(win);
}

static void
_EShapeCombineShape(Win win, int dest, int x, int y,
		    Win src_win, int src_kind, int op)
{
   XShapeCombineShape(disp, win->xwin, dest, x, y, src_win->xwin, src_kind, op);
   EShapeUpdate(win);
}

int
EShapePropagate(Win win)
{
   Win                 xch;
   unsigned int        num_rects;
   int                 k, rn;
   int                 x, y, w, h;
   XRectangle         *rects, *rectsn, *rl;

   if (!win || win->w <= 0 || win->h <= 0)
      return 0;

#if DEBUG_SHAPE_PROPAGATE
   Eprintf("%s %#x %d,%d %dx%d\n", __func__, win->xwin,
	   win->x, win->y, win->w, win->h);
#endif

   num_rects = 0;
   rects = NULL;

   /* go through all child windows and create/inset spans */
   for (xch = win_first; xch; xch = xch->next)
     {
	if (xch->parent != win)
	   continue;

#if DEBUG_SHAPE_PROPAGATE > 1
	Eprintf("%#x(%d/%d): %4d,%4d %4dx%4d\n",
		xch->xwin, xch->mapped, xch->num_rect,
		xch->x, xch->y, xch->w, xch->h);
#endif
	if (!xch->mapped)
	   continue;

	x = xch->x;
	y = xch->y;
	w = xch->w;
	h = xch->h;
	if (x >= win->w || y >= win->h || x + w < 0 || y + h < 0)
	   continue;

	rn = xch->num_rect;

	if (rn > 0)
	  {
	     rl = xch->rects;
	     rectsn = EREALLOC(XRectangle, rects, num_rects + rn);
	     if (!rectsn)
		goto bail_out;
	     rects = rectsn;

	     /* go through all clip rects in this window's shape */
	     for (k = 0; k < rn; k++)
	       {
		  /* for each clip rect, add it to the rect list */
		  rects[num_rects].x = x + rl[k].x;
		  rects[num_rects].y = y + rl[k].y;
		  rects[num_rects].width = rl[k].width;
		  rects[num_rects].height = rl[k].height;
#if DEBUG_SHAPE_PROPAGATE > 1
		  Eprintf(" - %x %d: %4d,%4d %4dx%4d\n", xch->xwin, k,
			  rects[num_rects].x, rects[num_rects].y,
			  rects[num_rects].width, rects[num_rects].height);
#endif
		  num_rects++;
	       }
	  }
	else if (rn == 0)
	  {
	     /* Unshaped */
	     rectsn = EREALLOC(XRectangle, rects, num_rects + 1);
	     if (!rectsn)
		goto bail_out;
	     rects = rectsn;

	     rects[num_rects].x = x;
	     rects[num_rects].y = y;
	     rects[num_rects].width = w;
	     rects[num_rects].height = h;
#if DEBUG_SHAPE_PROPAGATE > 1
	     Eprintf(" - %x  : %4d,%4d %4dx%4d\n", xch->xwin,
		     rects[num_rects].x, rects[num_rects].y,
		     rects[num_rects].width, rects[num_rects].height);
#endif
	     num_rects++;
	  }
     }

#if DEBUG_SHAPE_PROPAGATE
   _EShapeShow(__func__, win->xwin, rects, num_rects);
#endif

   /* set the rects as the shape mask */
   if (rects)
     {
	_EShapeCombineRectangles(win, ShapeBounding, 0, 0, rects,
				 num_rects, ShapeSet, Unsorted);
	Efree(rects);
     }
   else
     {
	/* Empty shape */
	_EShapeCombineRectangles(win, ShapeBounding, 0, 0, NULL, 0, ShapeSet,
				 Unsorted);
     }

   return win->num_rect;

 bail_out:
   Efree(rects);
   _EShapeCombineMask(win, ShapeBounding, 0, 0, NoXID, ShapeSet);
   return 0;
}

int
EShapeCheck(Win win)
{
   if (!win)
      return 0;

   return win->num_rect;
}

void
EShapeSetMask(Win win, int x, int y, EX_Pixmap mask)
{
   _EShapeCombineMask(win, ShapeBounding, x, y, mask, ShapeSet);
}

void
EShapeUnionMask(Win win, int x, int y, EX_Pixmap mask)
{
   _EShapeCombineMask(win, ShapeBounding, x, y, mask, ShapeUnion);
}

void
EShapeSetMaskTiled(Win win, int x, int y, EX_Pixmap mask, int w, int h)
{
   _EShapeCombineMaskTiled(win, ShapeBounding, x, y, mask, ShapeSet, w, h);
}

void
EShapeSetRects(Win win, int x, int y, XRectangle * rect, int n_rects)
{
   _EShapeCombineRectangles(win, ShapeBounding, x, y, rect, n_rects,
			    ShapeSet, Unsorted);
}

void
EShapeUnionRects(Win win, int x, int y, XRectangle * rect, int n_rects)
{
   _EShapeCombineRectangles(win, ShapeBounding, x, y, rect, n_rects,
			    ShapeUnion, Unsorted);
}

int
EShapeSetShape(Win win, int x, int y, Win src_win)
{
   _EShapeCombineShape(win, ShapeBounding, x, y,
		       src_win, ShapeBounding, ShapeSet);
   return win->num_rect != 0;
}

static              EX_Pixmap
_EWindowGetShapePixmap(Win win, unsigned int fg, unsigned int bg)
{
   EX_Pixmap           mask;
   GC                  gc;
   int                 i;
   const XRectangle   *rect;

   if (win->num_rect == 0)	/* Not shaped */
      return NoXID;

   mask = ECreatePixmap(win, win->w, win->h, 1);
   gc = EXCreateGC(mask, 0, NULL);

   XSetForeground(disp, gc, bg);
   XFillRectangle(disp, mask, gc, 0, 0, win->w, win->h);

   XSetForeground(disp, gc, fg);
   rect = win->rects;
   for (i = 0; i < win->num_rect; i++)
      XFillRectangle(disp, mask, gc, rect[i].x, rect[i].y,
		     rect[i].width, rect[i].height);

   EXFreeGC(gc);

   return mask;
}

/* Build mask from window shape rects */
EX_Pixmap
EWindowGetShapePixmap(Win win)
{
   return _EWindowGetShapePixmap(win, 1, 0);
}

/* Build inverted mask from window shape rects */
EX_Pixmap
EWindowGetShapePixmapInverted(Win win)
{
   return _EWindowGetShapePixmap(win, 0, 1);
}

EX_Pixmap
ECreatePixmap(Win win, unsigned int width, unsigned int height,
	      unsigned int depth)
{
   EX_Pixmap           pmap;

   if (depth == 0)
      depth = win->depth;

   pmap = XCreatePixmap(disp, win->xwin, width, height, depth);
#if DEBUG_PIXMAP
   Eprintf("%s: %#x\n", __func__, pmap);
#endif
   return pmap;
}

void
EFreePixmap(EX_Pixmap pmap)
{
#if DEBUG_PIXMAP
   Eprintf("%s: %#x\n", __func__, pmap);
#endif
   XFreePixmap(disp, pmap);
}

EX_Pixmap
EXCreatePixmapCopy(EX_Pixmap src, unsigned int w, unsigned int h,
		   unsigned int depth)
{
   EX_Pixmap           pmap;
   GC                  gc;

   pmap = XCreatePixmap(disp, src, w, h, depth);
   gc = EXCreateGC(src, 0, NULL);
   XCopyArea(disp, src, pmap, gc, 0, 0, w, h, 0, 0);
   EXFreeGC(gc);
#if DEBUG_PIXMAP
   Eprintf("%s: %#x\n", __func__, pmap);
#endif
   return pmap;
}

void
EXCopyAreaGC(EX_Drawable src, EX_Drawable dst, GC gc, int sx, int sy,
	     unsigned int w, unsigned int h, int dx, int dy)
{
   XCopyArea(disp, src, dst, gc, sx, sy, w, h, dx, dy);
}

void
EXCopyArea(EX_Drawable src, EX_Drawable dst, int sx, int sy,
	   unsigned int w, unsigned int h, int dx, int dy)
{
   GC                  gc = (GC) Dpy.root_gc;

   XCopyArea(disp, src, dst, gc, sx, sy, w, h, dx, dy);
}

void
EXCopyAreaTiled(EX_Drawable src, EX_Pixmap mask, EX_Drawable dst,
		int sx, int sy, unsigned int w, unsigned int h, int dx, int dy)
{
   GC                  gc;
   XGCValues           gcv;

   gcv.fill_style = FillTiled;
   gcv.tile = src;
   gcv.ts_x_origin = sx;
   gcv.ts_y_origin = sy;
   gcv.clip_mask = mask;
   gc = EXCreateGC(dst, GCFillStyle |
		   GCTile | GCTileStipXOrigin | GCTileStipYOrigin | GCClipMask,
		   &gcv);
   XFillRectangle(disp, dst, gc, dx, dy, w, h);
   EXFreeGC(gc);
}

void
EXFillAreaSolid(EX_Drawable dst, int x, int y, unsigned int w, unsigned int h,
		unsigned int pixel)
{
   GC                  gc;
   XGCValues           gcv;

   gcv.foreground = pixel;
   gc = EXCreateGC(dst, GCForeground, &gcv);
   XFillRectangle(disp, dst, gc, x, y, w, h);
   EXFreeGC(gc);
}

static void
_EXDrawRectangle(EX_Drawable dst, GC gc, int x, int y,
		 unsigned int w, unsigned int h, unsigned int pixel)
{
   XSetForeground(disp, gc, pixel);
   XDrawRectangle(disp, dst, gc, x, y, w, h);
}

static void
_EXFillRectangle(EX_Drawable dst, GC gc, int x, int y,
		 unsigned int w, unsigned int h, unsigned int pixel)
{
   XSetForeground(disp, gc, pixel);
   XFillRectangle(disp, dst, gc, x, y, w, h);
}

void
EXPaintRectangle(EX_Drawable dst, int x, int y,
		 unsigned int w, unsigned int h,
		 unsigned int fg, unsigned int bg)
{
   GC                  gc;

   if (w == 0 || h == 0)
      return;
   gc = EXCreateGC(dst, 0, NULL);
   _EXDrawRectangle(dst, gc, x, y, w - 1, h - 1, fg);
   if (w > 2 && h > 2)
      _EXFillRectangle(dst, gc, x + 1, y + 1, w - 2, h - 2, bg);
   EXFreeGC(gc);
}

GC
EXCreateGC(EX_Drawable draw, unsigned int mask, XGCValues * val)
{
   XGCValues           xgcv;

   if (val)
     {
	mask |= GCGraphicsExposures;
	val->graphics_exposures = False;
     }
   else
     {
	mask = GCSubwindowMode | GCGraphicsExposures;
	val = &xgcv;
	val->subwindow_mode = IncludeInferiors;
	val->graphics_exposures = False;
     }
   return XCreateGC(disp, draw, mask, val);
}

void
EXFreeGC(GC gc)
{
   if (gc)
      XFreeGC(disp, gc);
}

void
EXSendEvent(EX_Window xwin, unsigned int event_mask, XEvent * ev)
{
   XSendEvent(disp, xwin, False, event_mask, ev);
}

unsigned int
EAllocColor(EX_Colormap cmap, unsigned int argb)
{
   XColor              xc;

   COLOR32_TO_RGB16(argb, xc.red, xc.green, xc.blue);
   XAllocColor(disp, cmap, &xc);

   return xc.pixel;
}

/*
 * Display
 */

int
EDisplayOpen(const char *dstr, int scr)
{
   char                dbuf[256], *s;
   unsigned int        ddpy, dscr;

   if (!dstr)
      goto do_open;

   Esnprintf(dbuf, sizeof(dbuf), "%s", dstr);
   s = strchr(dbuf, ':');
   if (!s)
      return -1;
   s++;

   ddpy = dscr = 0;
   sscanf(s, "%u.%u", &ddpy, &dscr);
   if (scr >= 0)		/* Override screen */
      dscr = scr;
   Esnprintf(s, sizeof(dbuf) - (s - dbuf), "%u.%u", ddpy, dscr);
   dstr = dbuf;

 do_open:
   disp = XOpenDisplay(dstr);

   return (disp) ? 0 : -1;
}

void
EDisplayClose(void)
{
   if (!disp)
      return;
   XCloseDisplay(disp);
   XSetErrorHandler(NULL);
   XSetIOErrorHandler(NULL);
   disp = NULL;
}

void
EDisplayDisconnect(void)
{
   if (!disp)
      return;
   close(ConnectionNumber(disp));
   XSetErrorHandler(NULL);
   XSetIOErrorHandler(NULL);

   disp = NULL;
}

static EXErrorHandler *EXErrorFunc = NULL;
static EXIOErrorHandler *EXIOErrorFunc = NULL;

static int
_HandleXError(Display * dpy __UNUSED__, XErrorEvent * ev)
{
   if (EDebug(1) && EXErrorFunc)
      EXErrorFunc((XEvent *) ev);

   Dpy.last_error_code = ev->error_code;

   return 0;
}

static int
_HandleXIOError(Display * dpy __UNUSED__)
{
   disp = NULL;

   if (EXIOErrorFunc)
      EXIOErrorFunc();

   return 0;
}

void
EDisplaySetErrorHandlers(EXErrorHandler * error, EXIOErrorHandler * fatal)
{
   /* set up an error handler for then E would normally have fatal X errors */
   EXErrorFunc = error;
   XSetErrorHandler(_HandleXError);

   /* set up a handler for when the X Connection goes down */
   EXIOErrorFunc = fatal;
   XSetIOErrorHandler(_HandleXIOError);
}

/*
 * Server
 */

void
EGrabServer(void)
{
   if (Dpy.server_grabbed <= 0)
     {
	if (EDebug(EDBUG_TYPE_GRABS))
	   Eprintf("%s\n", __func__);
	XGrabServer(disp);
     }
   Dpy.server_grabbed++;
}

void
EUngrabServer(void)
{
   if (Dpy.server_grabbed == 1)
     {
	XUngrabServer(disp);
	XFlush(disp);
	if (EDebug(EDBUG_TYPE_GRABS))
	   Eprintf("%s\n", __func__);
     }
   Dpy.server_grabbed--;
   if (Dpy.server_grabbed < 0)
      Dpy.server_grabbed = 0;
}

int
EServerIsGrabbed(void)
{
   return Dpy.server_grabbed;
}

void
EFlush(void)
{
   XFlush(disp);
}

void
ESync(unsigned int mask)
{
   if (mask & Conf.testing.no_sync_mask)
      return;
   XSync(disp, False);
}

/*
 * Visuals
 */

#if USE_XRENDER

Visual             *
EVisualFindARGB(void)
{
   XVisualInfo        *xvi, xvit;
   int                 i, num;
   Visual             *vis;

   xvit.screen = Dpy.screen;
   xvit.depth = 32;
#if __cplusplus
   xvit.c_class = TrueColor;
#else
   xvit.class = TrueColor;
#endif

   xvi = XGetVisualInfo(disp,
			VisualScreenMask | VisualDepthMask | VisualClassMask,
			&xvit, &num);
   if (!xvi)
      return NULL;

   for (i = 0; i < num; i++)
     {
	if (EVisualIsARGB(xvi[i].visual))
	   break;
     }

   vis = (i < num) ? xvi[i].visual : NULL;

   XFree(xvi);

   return vis;
}

int
EVisualIsARGB(Visual * vis)
{
   XRenderPictFormat  *pictfmt;

   pictfmt = XRenderFindVisualFormat(disp, vis);
   if (!pictfmt)
      return 0;

#if 0
   Eprintf("Visual ID=%#lx Type=%d, alphamask=%d\n", vis->visualid,
	   pictfmt->type, pictfmt->direct.alphaMask);
#endif
   return pictfmt->type == PictTypeDirect && pictfmt->direct.alphaMask;
}

#endif /* USE_XRENDER */

/*
 * Misc
 */

EX_Time
EGetTimestamp(void)
{
   static EX_Window    win_ts = NoXID;
   XSetWindowAttributes attr;
   XEvent              ev;

   if (win_ts == NoXID)
     {
	attr.override_redirect = False;
	win_ts = XCreateWindow(disp, WinGetXwin(VROOT), -100, -100, 1, 1, 0,
			       CopyFromParent, InputOnly, CopyFromParent,
			       CWOverrideRedirect, &attr);
	XSelectInput(disp, win_ts, PropertyChangeMask);
     }

   XChangeProperty(disp, win_ts, XA_WM_NAME, XA_STRING, 8,
		   PropModeAppend, (unsigned char *)"", 0);
   XWindowEvent(disp, win_ts, PropertyChangeMask, &ev);

   return ev.xproperty.time;
}

#if USE_COMPOSITE

#include <X11/extensions/Xcomposite.h>

EX_Pixmap
EWindowGetPixmap(const Win win)
{
   XWindowAttributes   xwa;

   if (EXGetWindowAttributes(win->xwin, &xwa) == 0 ||
       xwa.map_state == IsUnmapped)
      return NoXID;

   return XCompositeNameWindowPixmap(disp, WinGetXwin(win));
}

#endif /* USE_COMPOSITE */

#if USE_XRENDER

/*
 * Pictures
 */
#define _R(x) (((x) >> 16) & 0xff)
#define _G(x) (((x) >>  8) & 0xff)
#define _B(x) (((x)      ) & 0xff)

EX_Picture
EPictureCreate(Win win, EX_Drawable draw)
{
   EX_Picture          pict;
   XRenderPictFormat  *pictfmt;

   if (!win)
      win = VROOT;
   pictfmt = XRenderFindVisualFormat(disp, WinGetVisual(win));
   pict = XRenderCreatePicture(disp, draw, pictfmt, 0, NULL);

   return pict;
}

EX_Picture
EPictureCreateII(Win win, EX_Drawable draw)
{
   EX_Picture          pict;
   XRenderPictFormat  *pictfmt;
   XRenderPictureAttributes pa;

   pictfmt = XRenderFindVisualFormat(disp, WinGetVisual(win));
   pa.subwindow_mode = IncludeInferiors;
   pict = XRenderCreatePicture(disp, draw, pictfmt, CPSubwindowMode, &pa);

   return pict;
}

EX_Picture
EPictureCreateSolid(EX_Window xwin, int argb, unsigned int a, unsigned int rgb)
{
   Display            *dpy = disp;
   XRenderColor        c;
   EX_Picture          pict;

   c.alpha = (unsigned short)(a * 0x101);
   c.red = (unsigned short)(_R(rgb) * 0x101);
   c.green = (unsigned short)(_G(rgb) * 0x101);
   c.blue = (unsigned short)(_B(rgb) * 0x101);

#if RENDER_VERSION >= VERS(0, 11)
   /* Version 0.10 should be good but apparently sometimes isn't
    * (or is it some broken driver?).
    * Anyway, let's require 0.11 and avoid some trouble. */
   if (ExtVersion(XEXT_RENDER) >= VERS(0, 11))
     {
	pict = XRenderCreateSolidFill(dpy, &c);
     }
   else
#endif
     {
	EX_Pixmap           pmap;
	XRenderPictFormat  *pictfmt;
	XRenderPictureAttributes pa;

	pmap = XCreatePixmap(dpy, xwin, 1, 1, argb ? 32 : 8);
	pictfmt = XRenderFindStandardFormat(dpy,
					    argb ? PictStandardARGB32 :
					    PictStandardA8);
	pa.repeat = True;
	pict = XRenderCreatePicture(dpy, pmap, pictfmt, CPRepeat, &pa);
	XRenderFillRectangle(dpy, PictOpSrc, pict, &c, 0, 0, 1, 1);
	XFreePixmap(dpy, pmap);
     }

   return pict;
}

EX_Picture
EPictureCreateBuffer(Win win, int w, int h, int argb, EX_Pixmap * ppmap)
{
   EX_Picture          pict;
   EX_Pixmap           pmap;
   XRenderPictFormat  *pictfmt;
   int                 depth;

   depth = argb ? 32 : WinGetDepth(win);
   pictfmt = argb ?
      XRenderFindStandardFormat(disp, PictStandardARGB32) :
      XRenderFindVisualFormat(disp, WinGetVisual(win));
   pmap = XCreatePixmap(disp, WinGetXwin(win), w, h, depth);
   pict = XRenderCreatePicture(disp, pmap, pictfmt, 0, NULL);
   if (ppmap)
      *ppmap = pmap;
   else
      XFreePixmap(disp, pmap);

   return pict;
}

void
EPictureDestroy(EX_Picture pict)
{
   XRenderFreePicture(disp, pict);
}

void
EPictureFillRect(EX_Picture pict, int x, int y, int w, int h,
		 unsigned int color)
{
   XRenderColor        c;

   COLOR32_TO_ARGB16(color, c.alpha, c.red, c.green, c.blue);
   XRenderFillRectangle(disp, PictOpSrc, pict, &c, x, y, w, h);
}

#endif /* USE_XRENDER */

#if USE_COMPOSITE

void
EPictureSetClip(EX_Picture pict, EX_SrvRegion clip)
{
   XFixesSetPictureClipRegion(disp, pict, 0, 0, clip);
}

/*
 * Regions
 */
#define DEBUG_REGIONS 0

#if DEBUG_REGIONS
static int          n_rgn_c = 0;
static int          n_rgn_d = 0;
#endif

EX_SrvRegion
ERegionCreate(void)
{
   EX_SrvRegion        rgn;

   rgn = XFixesCreateRegion(disp, NULL, 0);

#if DEBUG_REGIONS
   n_rgn_c++;
   Eprintf("%s: %#x %d %d %d\n", __func__, rgn,
	   n_rgn_c - n_rgn_d, n_rgn_c, n_rgn_d);
#endif
   return rgn;
}

EX_SrvRegion
ERegionCreateRect(int x, int y, int w, int h)
{
   EX_SrvRegion        rgn;
   XRectangle          rct;

   rct.x = x;
   rct.y = y;
   rct.width = w;
   rct.height = h;
   rgn = XFixesCreateRegion(disp, &rct, 1);

#if DEBUG_REGIONS
   n_rgn_c++;
   Eprintf("%s: %#x %d %d %d\n", __func__, rgn,
	   n_rgn_c - n_rgn_d, n_rgn_c, n_rgn_d);
#endif
   return rgn;
}

#if USE_DESK_EXPOSE
EX_SrvRegion
ERegionCreateFromRects(XRectangle * rectangles, int nrectangles)
{
   EX_SrvRegion        rgn;

   rgn = XFixesCreateRegion(disp, rectangles, nrectangles);

#if DEBUG_REGIONS
   n_rgn_c++;
   Eprintf("%s: %#x %d %d %d\n", __func__, rgn,
	   n_rgn_c - n_rgn_d, n_rgn_c, n_rgn_d);
#endif
   return rgn;
}
#endif

EX_SrvRegion
ERegionCreateFromWindow(Win win)
{
   EX_SrvRegion        rgn;

   rgn =
      XFixesCreateRegionFromWindow(disp, WinGetXwin(win), WindowRegionBounding);

#if DEBUG_REGIONS
   n_rgn_c++;
   Eprintf("%s: %#x %d %d %d\n", __func__, rgn,
	   n_rgn_c - n_rgn_d, n_rgn_c, n_rgn_d);
#endif
   return rgn;
}

EX_SrvRegion
ERegionCreateFromBitmap(EX_Pixmap mask)
{
   return XFixesCreateRegionFromBitmap(disp, mask);
}

EX_SrvRegion
ERegionCopy(EX_SrvRegion rgn, EX_SrvRegion src)
{
   XFixesCopyRegion(disp, rgn, src);
   return rgn;
}

EX_SrvRegion
ERegionClone(EX_SrvRegion src)
{
   EX_SrvRegion        rgn;

   rgn = ERegionCreate();
   ERegionCopy(rgn, src);

   return rgn;
}

void
ERegionDestroy(EX_SrvRegion rgn)
{
#if DEBUG_REGIONS
   n_rgn_d++;
   Eprintf("%s: %#x %d %d %d\n", __func__, rgn,
	   n_rgn_c - n_rgn_d, n_rgn_c, n_rgn_d);
#endif
   XFixesDestroyRegion(disp, rgn);
}

void
ERegionEmpty(EX_SrvRegion rgn)
{
   XFixesSetRegion(disp, rgn, NULL, 0);
}

void
ERegionSetRect(EX_SrvRegion rgn, int x, int y, int w, int h)
{
   XRectangle          rct;

   rct.x = x;
   rct.y = y;
   rct.width = w;
   rct.height = h;
   XFixesSetRegion(disp, rgn, &rct, 1);
}

void
ERegionTranslate(EX_SrvRegion rgn, int dx, int dy)
{
   if (dx == 0 && dy == 0)
      return;
   XFixesTranslateRegion(disp, rgn, dx, dy);
}

void
ERegionIntersect(EX_SrvRegion dst, EX_SrvRegion src)
{
   XFixesIntersectRegion(disp, dst, dst, src);
}

void
ERegionUnion(EX_SrvRegion dst, EX_SrvRegion src)
{
   XFixesUnionRegion(disp, dst, dst, src);
}

void
ERegionSubtract(EX_SrvRegion dst, EX_SrvRegion src)
{
   XFixesSubtractRegion(disp, dst, dst, src);
}

void
ERegionIntersectOffset(EX_SrvRegion dst, int dx, int dy, EX_SrvRegion src,
		       EX_SrvRegion tmp)
{
   Display            *dpy = disp;
   EX_SrvRegion        rgn;

   rgn = src;
   if (dx != 0 || dy != 0)
     {
	rgn = ERegionCopy(tmp, src);
	XFixesTranslateRegion(dpy, rgn, dx, dy);
     }
   XFixesIntersectRegion(dpy, dst, dst, rgn);
}

void
ERegionSubtractOffset(EX_SrvRegion dst, int dx, int dy, EX_SrvRegion src,
		      EX_SrvRegion tmp)
{
   Display            *dpy = disp;
   EX_SrvRegion        rgn;

   rgn = src;
   if (dx != 0 || dy != 0)
     {
	rgn = ERegionCopy(tmp, src);
	XFixesTranslateRegion(dpy, rgn, dx, dy);
     }
   XFixesSubtractRegion(dpy, dst, dst, rgn);
}

#if 0				/* Unused */
void
ERegionUnionOffset(EX_SrvRegion dst, int dx, int dy, EX_SrvRegion src,
		   EX_SrvRegion tmp)
{
   Display            *dpy = disp;
   EX_SrvRegion        rgn;

   rgn = src;
   if (dx != 0 || dy != 0)
     {
	rgn = ERegionCopy(tmp, src);
	XFixesTranslateRegion(dpy, rgn, dx, dy);
     }
   XFixesUnionRegion(dpy, dst, dst, rgn);
}
#endif

#if 0				/* Unused (for debug) */
int
ERegionIsEmpty(EX_SrvRegion rgn)
{
   int                 nr;
   XRectangle         *pr;

   pr = XFixesFetchRegion(disp, rgn, &nr);
   if (pr)
      XFree(pr);
   return nr == 0;
}
#endif

void
ERegionShow(const char *txt, EX_SrvRegion rgn,
	    void (*prf) (const char *fmt, ...))
{
   int                 i, nr;
   XRectangle         *pr;

   prf = (prf) ? prf : Eprintf;

   if (rgn == NoXID)
     {
	prf(" - region: %s %#x is None\n", txt, rgn);
	return;
     }

   pr = XFixesFetchRegion(disp, rgn, &nr);
   if (!pr || nr <= 0)
     {
	prf(" - region: %s %#x is empty\n", txt, rgn);
	goto done;
     }

   prf(" - region: %s %#x:\n", txt, rgn);
   for (i = 0; i < nr; i++)
      prf("%4d: %4d+%4d %4dx%4d\n", i, pr[i].x, pr[i].y, pr[i].width,
	  pr[i].height);

 done:
   if (pr)
      XFree(pr);
}

#endif /* USE_COMPOSITE */
