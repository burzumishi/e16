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
#ifndef _XWIN_H_
#define _XWIN_H_

#include "util.h"
#include "xtypes.h"

typedef struct {
   char               *name;
   int                 screens;
   int                 screen;
   unsigned int        pixel_black;
   unsigned int        pixel_white;

   Win                 rroot;	/* Real root window */
   Win                 vroot;	/* Virtual root window */

   void               *root_gc;

   int                 server_grabbed;

   unsigned char       last_error_code;
} EDisplay;

__EXPORT__ extern EDisplay Dpy;
__EXPORT__ extern Display *disp;

#define RROOT Dpy.rroot
#define VROOT Dpy.vroot

void                EXInit(void);

int                 EDisplayOpen(const char *dstr, int scr);
void                EDisplayClose(void);
void                EDisplayDisconnect(void);

typedef void        (EXErrorHandler) (const XEvent *);
typedef void        (EXIOErrorHandler) (void);
void                EDisplaySetErrorHandlers(EXErrorHandler * error,
					     EXIOErrorHandler * fatal);

void                EGrabServer(void);
void                EUngrabServer(void);
int                 EServerIsGrabbed(void);
void                EFlush(void);

#define ESYNC_MAIN	0x0001
#define ESYNC_DESKS	0x0002
#define ESYNC_MENUS	0x0004
#define ESYNC_MOVRES	0x0008
#define ESYNC_FOCUS	0x0010
#define ESYNC_SLIDEOUT	0x0080
void                ESync(unsigned int mask);

int                 EVisualIsARGB(Visual * vis);
Visual             *EVisualFindARGB(void);

EX_Time             EGetTimestamp(void);

typedef void        (EventCallbackFunc) (Win win, XEvent * ev, void *prm);

typedef struct {
   EventCallbackFunc  *func;
   void               *prm;
} EventCallbackItem;

typedef struct {
   int                 num;
   EventCallbackItem  *lst;
} EventCallbackList;

struct _xwin {
   struct _xwin       *next;
   struct _xwin       *prev;
   EventCallbackList   cbl;
   EX_Window           xwin;
   Win                 parent;
   int                 x, y, w, h;
   short               depth;
   unsigned short      bw;
   char                argb;
   char                mapped;
   char                in_use;
   signed char         do_del;
   char                attached;
   signed char         bg_owned;	/* bgpmap "owned" by Win */
   int                 num_rect;
   int                 ord;
   XRectangle         *rects;
   Visual             *visual;
   EX_Colormap         cmap;
   EX_Pixmap           bgpmap;
   unsigned int        bgcol;
#if USE_XI2
   unsigned int        event_mask;
#endif
};

Win                 ELookupXwin(EX_Window xwin);

#define             WinGetXwin(win)		((win)->xwin)
#define             WinGetPmap(win)		((win)->bgpmap)
#define             WinGetX(win)		((win)->x)
#define             WinGetY(win)		((win)->y)
#define             WinGetW(win)		((win)->w)
#define             WinGetH(win)		((win)->h)
#define             WinGetBorderWidth(win)	((win)->bw)
#define             WinGetDepth(win)		((win)->depth)
#define             WinGetVisual(win)		((win)->visual)
#define             WinGetCmap(win)		((win)->cmap)
#define             WinGetNumRect(win)		((win)->num_rect)
#define             WinIsMapped(win)		((win)->mapped != 0)
#define             WinIsShaped(win)		((win)->num_rect != 0)

Win                 ECreateWinFromXwin(EX_Window xwin);
void                EDestroyWin(Win win);

Win                 ERegisterWindow(EX_Window xwin, XWindowAttributes * pxwa);
void                EUnregisterWindow(Win win);
void                EUnregisterXwin(EX_Window xwin);
void                EventCallbackRegister(Win win, EventCallbackFunc * func,
					  void *prm);
void                EventCallbackUnregister(Win win, EventCallbackFunc * func,
					    void *prm);
void                EventCallbacksProcess(Win win, XEvent * ev);

Win                 ECreateWindow(Win parent, int x, int y, int w, int h,
				  int saveunder);
Win                 ECreateArgbWindow(Win parent, int x, int y, int w, int h,
				      Win cwin);
Win                 ECreateClientWindow(Win parent, int x, int y, int w, int h);

#define WIN_TYPE_CLIENT     0
#define WIN_TYPE_INTERNAL   1
#define WIN_TYPE_NO_ARGB    2
#define WIN_TYPE_GLX        3
Win                 ECreateObjectWindow(Win parent, int x, int y, int w,
					int h, int saveunder, int type,
					Win cwin);
Win                 ECreateEventWindow(Win parent, int x, int y, int w, int h);
Win                 ECreateFocusWindow(Win parent, int x, int y, int w, int h);
void                EWindowSync(Win win);
void                EWindowSetGeometry(Win win, int x, int y, int w, int h,
				       int bw);
void                EWindowSetMapped(Win win, int mapped);

void                EMoveWindow(Win win, int x, int y);
void                EResizeWindow(Win win, int w, int h);
void                EMoveResizeWindow(Win win, int x, int y, int w, int h);
void                EDestroyWindow(Win win);
void                EMapWindow(Win win);
void                EMapRaised(Win win);
void                EUnmapWindow(Win win);
void                EReparentWindow(Win win, Win parent, int x, int y);
int                 EGetGeometry(Win win, EX_Window * root_return,
				 int *x, int *y, int *w, int *h, int *bw,
				 int *depth);
void                EGetWindowAttributes(Win win, XWindowAttributes * pxwa);
void                EConfigureWindow(Win win, unsigned int mask,
				     XWindowChanges * wc);
void                ESetWindowBackgroundPixmap(Win win, EX_Pixmap pmap,
					       int kept);
EX_Pixmap           EGetWindowBackgroundPixmap(Win win);
void                EFreeWindowBackgroundPixmap(Win win);
void                ESetWindowBackground(Win win, unsigned int col);
int                 ETranslateCoordinates(Win src_w, Win dst_w,
					  int src_x, int src_y,
					  int *dest_x_return,
					  int *dest_y_return,
					  EX_Window * child_return);
int                 EXDrawableOk(EX_Drawable draw);
int                 EXWindowOk(EX_Window xwin);

void                ESelectInput(Win win, unsigned int event_mask);
void                ESelectInputChange(Win win, unsigned int set,
				       unsigned int clear);
void                EChangeWindowAttributes(Win win, unsigned int mask,
					    XSetWindowAttributes * attr);
void                ESetWindowBorderWidth(Win win, unsigned int bw);
void                ERaiseWindow(Win win);
void                ELowerWindow(Win win);
void                EClearWindow(Win win);
void                EClearWindowExpose(Win win);
void                EClearArea(Win win, int x, int y,
			       unsigned int w, unsigned int h);

EX_Pixmap           ECreatePixmap(Win win, unsigned int width,
				  unsigned int height, unsigned int depth);
void                EFreePixmap(EX_Pixmap pixmap);

int                 EShapeUpdate(Win win);
void                EShapeSetMask(Win win, int x, int y, EX_Pixmap mask);
void                EShapeUnionMask(Win win, int x, int y, EX_Pixmap mask);
void                EShapeSetMaskTiled(Win win, int x, int y, EX_Pixmap mask,
				       int w, int h);
void                EShapeSetRects(Win win, int x, int y,
				   XRectangle * rect, int n_rects);
void                EShapeUnionRects(Win win, int x, int y,
				     XRectangle * rect, int n_rects);
int                 EShapeSetShape(Win win, int x, int y, Win src_win);
int                 EShapePropagate(Win win);
int                 EShapeCheck(Win win);
EX_Pixmap           EWindowGetShapePixmap(Win win);
EX_Pixmap           EWindowGetShapePixmapInverted(Win win);

void                EWarpPointer(Win win, int x, int y);
int                 EQueryPointer(Win win, int *px, int *py,
				  EX_Window * pchild, unsigned int *pmask);

unsigned int        EAllocColor(EX_Colormap cmap, unsigned int argb);

#define _A(x)   (((x) >> 24) & 0xff)
#define _R(x)   (((x) >> 16) & 0xff)
#define _G(x)   (((x) >>  8) & 0xff)
#define _B(x)   (((x)      ) & 0xff)
#define _A16(x) (((x) >> 16) & 0xff00)
#define _R16(x) (((x) >>  8) & 0xff00)
#define _G16(x) (((x)      ) & 0xff00)
#define _B16(x) (((x) <<  8) & 0xff00)

#define COLOR32_FROM_RGB(c, r, g, b) \
    c = (0xff000000 | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff))
#define COLOR32_TO_RGB(c, r, g, b) \
  do { r = _R(c); g = _G(c); b = _B(c); } while (0)
#define COLOR32_TO_ARGB(c, a, r, g, b) \
  do { a = _A(c); r = _R(c); g = _G(c); b = _B(c); } while (0)
#define COLOR32_TO_RGB16(c, r, g, b) \
  do { r = _R16(c); g = _G16(c); b = _B16(c); } while (0)
#define COLOR32_TO_ARGB16(c, a, r, g, b) \
  do { a = _A16(c); r = _R16(c); g = _G16(c); b = _B16(c); } while (0)

EX_Window           EXWindowGetParent(EX_Window xwin);
int                 EXGetWindowAttributes(EX_Window xwin,
					  XWindowAttributes * pxwa);
int                 EXGetGeometry(EX_Window xwin, EX_Window * root_return,
				  int *x, int *y, int *w, int *h, int *bw,
				  int *depth);

void                EXRestackWindows(EX_Window * windows, int nwindows);

void                EXCopyAreaGC(EX_Drawable src, EX_Drawable dst, GC gc,
				 int sx, int sy, unsigned int w, unsigned int h,
				 int dx, int dy);
void                EXCopyArea(EX_Drawable src, EX_Drawable dst, int sx,
			       int sy, unsigned int w, unsigned int h, int dx,
			       int dy);
void                EXCopyAreaTiled(EX_Drawable src, EX_Pixmap mask,
				    EX_Drawable dst, int sx, int sy,
				    unsigned int w, unsigned int h, int dx,
				    int dy);
void                EXFillAreaSolid(EX_Drawable dst, int x, int y,
				    unsigned int w, unsigned int h,
				    unsigned int pixel);
void                EXPaintRectangle(EX_Drawable dst, int x, int y,
				     unsigned int w, unsigned int h,
				     unsigned int fg, unsigned int bg);

void                EXWarpPointer(EX_Window xwin, int x, int y);
int                 EXQueryPointer(EX_Window xwin, int *px, int *py,
				   EX_Window * pchild, unsigned int *pmask);

EX_Pixmap           EXCreatePixmapCopy(EX_Pixmap src, unsigned int w,
				       unsigned int h, unsigned int depth);

GC                  EXCreateGC(EX_Drawable draw, unsigned int mask,
			       XGCValues * val);
void                EXFreeGC(GC gc);

void                EXSendEvent(EX_Window xwin, unsigned int event_mask,
				XEvent * ev);

EX_KeyCode          EKeysymToKeycode(EX_KeySym keysym);
EX_KeyCode          EKeynameToKeycode(const char *name);

typedef struct {
   char                type;
   char                depth;
   EX_Pixmap           pmap;
   EX_Pixmap           mask;
   unsigned short      w, h;
} PmapMask;

void                PmapMaskInit(PmapMask * pmm, Win win, int w, int h);
void                PmapMaskFree(PmapMask * pmm);

#if USE_XRENDER
EX_Picture          EPictureCreate(Win win, EX_Drawable draw);
EX_Picture          EPictureCreateII(Win win, EX_Drawable draw);
EX_Picture          EPictureCreateSolid(EX_Window xwin, int argb,
					unsigned int a, unsigned int rgb);
EX_Picture          EPictureCreateBuffer(Win win, int w, int h, int argb,
					 EX_Pixmap * ppmap);
void                EPictureDestroy(EX_Picture pict);
void                EPictureFillRect(EX_Picture pict, int x, int y,
				     int w, int h, unsigned int color);

#endif /* USE_XRENDER */

#if USE_COMPOSITE

EX_SrvRegion        ERegionCreate(void);
EX_SrvRegion        ERegionCreateRect(int x, int y, int w, int h);

#if USE_DESK_EXPOSE
EX_SrvRegion        ERegionCreateFromRects(XRectangle * rectangles,
					   int nrectangles);
#endif
EX_SrvRegion        ERegionCreateFromWindow(Win win);
EX_SrvRegion        ERegionCreateFromBitmap(EX_Pixmap mask);
EX_SrvRegion        ERegionCopy(EX_SrvRegion rgn, EX_SrvRegion src);
EX_SrvRegion        ERegionClone(EX_SrvRegion src);
void                ERegionDestroy(EX_SrvRegion rgn);
void                ERegionEmpty(EX_SrvRegion rgn);
void                ERegionSetRect(EX_SrvRegion rgn, int x, int y, int w,
				   int h);
void                ERegionTranslate(EX_SrvRegion rgn, int dx, int dy);
void                ERegionIntersect(EX_SrvRegion dst, EX_SrvRegion src);
void                ERegionSubtract(EX_SrvRegion dst, EX_SrvRegion src);
void                ERegionUnion(EX_SrvRegion dst, EX_SrvRegion src);
void                ERegionIntersectOffset(EX_SrvRegion dst, int dx, int dy,
					   EX_SrvRegion src, EX_SrvRegion tmp);
void                ERegionSubtractOffset(EX_SrvRegion dst, int dx, int dy,
					  EX_SrvRegion src, EX_SrvRegion tmp);
void                ERegionUnionOffset(EX_SrvRegion dst, int dx, int dy,
				       EX_SrvRegion src, EX_SrvRegion tmp);
#if 0				/* Unused (for debug) */
int                 ERegionIsEmpty(EX_SrvRegion rgn);
#endif
void                ERegionShow(const char *txt, EX_SrvRegion rgn,
				void (*prf) (const char *fmt, ...));

void                EPictureSetClip(EX_Picture pict, EX_SrvRegion clip);

EX_Pixmap           EWindowGetPixmap(const Win win);

#endif /* USE_COMPOSITE */

#if USE_XI2
#include <X11/extensions/XInput2.h>

typedef struct {
   XIEventMask         em;
   unsigned char       mb[(XI_LASTEVENT + 8) / 8];	/* Mask bits */
} EXIEventMask;

void                EXIMaskSetup(EXIEventMask * em, int dev,
				 unsigned int event_mask);
#endif

#endif /* _XWIN_H_ */
