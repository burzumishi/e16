/*
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

#include "E.h"
#include "container.h"
#include "events.h"
#include "ewins.h"
#include "hints.h"
#include "xprop.h"
#include "xwin.h"

#define DEBUG_SYSTRAY 0

/* Systray object info */
typedef struct {
   Win                 win;
   char                mapped;
} SWin;

#define StObjGetWin(o) (((SWin*)(o))->win)
#define StObjIsMapped(o) (((SWin*)(o))->mapped)

/* XEmbed atoms */
static EX_Atom      E_XA__XEMBED = 0;
static EX_Atom      E_XA__XEMBED_INFO = 0;

/* Systray atoms */
static EX_Atom      _NET_SYSTEM_TRAY_OPCODE = 0;
static EX_Atom      _NET_SYSTEM_TRAY_MESSAGE_DATA = 0;

/* Systray selection */
static ESelection  *systray_sel = NULL;

static void         SystrayItemEvent(Win win, XEvent * ev, void *prm);

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

/* _XEMBED client message */
#define XEMBED_EMBEDDED_NOTIFY      0

/* _XEMBED_INFO property */
#define XEMBED_MAPPED               (1 << 0)

static int
SystrayGetXembedInfo(EX_Window xwin, unsigned int *info)
{
   int                 num;

   EGrabServer();

   if (!EXWindowOk(xwin))
     {
	/* Invalid window */
	num = -1;
	goto done;
     }

   num = ex_window_prop_xid_get(xwin, E_XA__XEMBED_INFO,
				E_XA__XEMBED_INFO, info, 2);

   if (num < 2)
     {
	/* Property invalid or not there. */
	info[0] = 0;		/* Set protocol version 0 */
	info[1] = XEMBED_MAPPED;	/* Set mapped */
	num = 0;
     }

 done:
   EUngrabServer();

   return num;
}

/*
 * Return index, -1 if not found.
 */
static int
SystrayObjFind(Container * ct, EX_Window xwin)
{
   int                 i;

   for (i = 0; i < ct->num_objs; i++)
      if (xwin == WinGetXwin(StObjGetWin(ct->objs[i].obj)))
	 return i;

   return -1;
}

static              Win
SystrayObjManage(Container * ct, EX_Window xwin)
{
   Win                 win;

#if DEBUG_SYSTRAY
   Eprintf("%s: %#x\n", __func__, xwin);
#endif
   win = ERegisterWindow(xwin, NULL);
   if (!win)
      return win;

   ESelectInput(win, StructureNotifyMask | PropertyChangeMask);
   EventCallbackRegister(win, SystrayItemEvent, ct);
   EReparentWindow(win, ct->icon_win, 0, 0);
   XAddToSaveSet(disp, xwin);

   return win;
}

static void
SystrayObjUnmanage(Container * ct __UNUSED__, Win win, int gone)
{
#if DEBUG_SYSTRAY
   Eprintf("%s: %#x gone=%d\n", __func__, WinGetXwin(win), gone);
#endif

   if (!gone)
     {
	ESelectInput(win, NoEventMask);
	EUnmapWindow(win);
	EReparentWindow(win, VROOT, 0, 0);
	XRemoveFromSaveSet(disp, WinGetXwin(win));
     }
   EventCallbackUnregister(win, SystrayItemEvent, ct);
   EUnregisterWindow(win);
}

static void
SystrayObjAdd(Container * ct, EX_Window xwin)
{
   SWin               *swin = NULL;
   Win                 win;
   unsigned int        xembed_info[2];

   /* Not if already there */
   if (SystrayObjFind(ct, xwin) >= 0)
      return;

   EGrabServer();

   switch (SystrayGetXembedInfo(xwin, xembed_info))
     {
     case -1:			/* Error - assume invalid window */
	Eprintf("%s: %#x: Hmm.. Invalid window? Ignoring.\n", __func__, xwin);
	goto bail_out;
     case 0:			/* Assume broken - proceed anyway */
	Eprintf("%s: %#x: Hmm.. No _XEMBED_INFO?\n", __func__, xwin);
	break;
     default:
	if (EDebug(EDBUG_TYPE_ICONBOX))
	   Eprintf("%s: %#x: _XEMBED_INFO: %u %u\n", __func__, xwin,
		   xembed_info[0], xembed_info[1]);
	break;
     }

   swin = EMALLOC(SWin, 1);
   if (!swin)
      goto bail_out;

   if (ContainerObjectAdd(ct, swin) < 0)
      goto bail_out;

   win = SystrayObjManage(ct, xwin);
   if (!win)
      goto bail_out;

   swin->win = win;
   swin->mapped = (xembed_info[1] & XEMBED_MAPPED) != 0;
   if (swin->mapped)
      EMapWindow(win);

   /* TBD - Always set protocol version as reported by client */
   ex_client_message32_send(xwin, E_XA__XEMBED, NoEventMask,
			    CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0,
			    xwin, xembed_info[0]);

   EUngrabServer();

   return;			/* Success */

 bail_out:
   EUngrabServer();
   if (!swin)
      return;
   ContainerObjectDel(ct, swin);
   Efree(swin);
}

static void
SystrayObjDel(Container * ct, Win win, int gone)
{
   int                 i;
   SWin               *swin;

   i = SystrayObjFind(ct, WinGetXwin(win));
   if (i < 0)
      return;

   if (EDebug(EDBUG_TYPE_ICONBOX))
      Eprintf("%s: %#x\n", __func__, WinGetXwin(win));

   swin = (SWin *) ct->objs[i].obj;

   ContainerObjectDel(ct, swin);

   if (disp)
      SystrayObjUnmanage(ct, swin->win, gone);

   Efree(swin);
}

static void
SystrayObjMapUnmap(Container * ct, EX_Window xwin)
{
   int                 i, map;
   SWin               *swin;
   unsigned int        xembed_info[2];

   i = SystrayObjFind(ct, xwin);
   if (i < 0)
      return;

   swin = (SWin *) ct->objs[i].obj;

   if (SystrayGetXembedInfo(xwin, xembed_info) >= 0)
     {
	if (EDebug(EDBUG_TYPE_ICONBOX))
	   Eprintf("%s: %#x: _XEMBED_INFO: %u %u\n", __func__, xwin,
		   xembed_info[0], xembed_info[1]);

	map = (xembed_info[1] & XEMBED_MAPPED) != 0;
	if (map == swin->mapped)
	   return;

	if (map)
	   EMapWindow(swin->win);
	else
	   EUnmapWindow(swin->win);
     }
   else
     {
	if (EDebug(EDBUG_TYPE_ICONBOX))
	   Eprintf("%s: %#x: _XEMBED_INFO: gone?\n", __func__, xwin);

	map = 0;
	if (map == swin->mapped)
	   return;
     }

   swin->mapped = map;
   ContainerRedraw(ct);
}

static void
SystrayEventClientMessage(Container * ct, XClientMessageEvent * ev)
{
   EX_Window           xwin;

   if (EDebug(EDBUG_TYPE_ICONBOX))
      Eprintf("%s: ev->type=%ld ev->data.l: %#lx %#lx %#lx %#lx\n", __func__,
	      ev->message_type,
	      ev->data.l[0], ev->data.l[1], ev->data.l[2], ev->data.l[3]);

   if (ev->message_type == _NET_SYSTEM_TRAY_OPCODE)
     {
	xwin = ev->data.l[2];
	if (xwin == NoXID)
	   goto done;

	SystrayObjAdd(ct, xwin);
     }
   else if (ev->message_type == _NET_SYSTEM_TRAY_MESSAGE_DATA)
     {
	if (EDebug(EDBUG_TYPE_ICONBOX))
	   Eprintf("%s: Got data message\n", __func__);
     }
 done:
   ;
}

static void
SystrayEventClientProperty(Container * ct, XPropertyEvent * ev)
{
   if (EDebug(EDBUG_TYPE_ICONBOX))
      Eprintf("%s: %#lx %ld\n", __func__, ev->window, ev->atom);

   if (ev->atom == E_XA__XEMBED_INFO)
     {
	SystrayObjMapUnmap(ct, ev->window);
     }
}

static void
SystraySelectionEvent(Win win __UNUSED__, XEvent * ev, void *prm)
{
   if (EDebug(EDBUG_TYPE_ICONBOX))
      Eprintf("%s: %2d %#lx\n", __func__, ev->type, ev->xany.window);

   switch (ev->type)
     {
     default:
	Eprintf("%s: ??? %2d %#lx\n", __func__, ev->type, ev->xany.window);
	break;

     case SelectionClear:
	DialogOK(_("Systray Error!"), _("Systray went elsewhere?!?"));
	SelectionRelease(systray_sel);
	systray_sel = NoXID;
	EwinHide(((Container *) prm)->ewin);
	break;

     case ClientMessage:
	SystrayEventClientMessage((Container *) prm, &(ev->xclient));
	break;
     }
}

static void
SystrayEvent(Win _win __UNUSED__, XEvent * ev, void *prm __UNUSED__)
{
   if (EDebug(EDBUG_TYPE_ICONBOX))
      Eprintf("%s: %2d %#lx\n", __func__, ev->type, ev->xany.window);

#if 0				/* FIXME - Need this one at all? ConfigureRequest? */
   EX_Window           xwin;

   switch (ev->type)
     {
     case MapNotify:
	EWindowSync(ELookupXwin(ev->xmap.window));
	ContainerRedraw(prm);
	break;

     case DestroyNotify:
	xwin = ev->xdestroywindow.window;
	goto do_terminate;

     case ReparentNotify:
     case EX_EVENT_REPARENT_GONE:
	/* Terminate if reparenting away from systray */
	if (ev->xreparent.parent == ev->xreparent.event)
	   break;
	xwin = ev->xreparent.window;
	goto do_terminate;

      do_terminate:
	SystrayObjDel(prm, xwin);
	break;
     }
#endif
}

static void
SystrayItemEvent(Win win, XEvent * ev, void *prm)
{
   Container          *ct = (Container *) prm;

   if (EDebug(EDBUG_TYPE_ICONBOX))
      Eprintf("%s: %2d %#lx\n", __func__, ev->type, ev->xany.window);

   switch (ev->type)
     {
     case MapNotify:
	EWindowSync(win);
	ContainerRedraw(ct);
	break;

     case DestroyNotify:
	goto do_terminate;

     case ReparentNotify:
     case EX_EVENT_REPARENT_GONE:
	/* Terminate if reparenting away from systray */
	if (ev->xreparent.parent == WinGetXwin(ct->icon_win))
	   break;
	goto do_terminate;

     case ClientMessage:
	SystrayEventClientMessage(ct, &(ev->xclient));
	break;

     case PropertyNotify:
	SystrayEventClientProperty(ct, &(ev->xproperty));
	break;

      do_terminate:
	SystrayObjDel(ct, win, ev->type != ReparentNotify);
	ContainerRedraw(ct);
	break;
     }
}

static void
SystrayInit(Container * ct)
{
   Win                 win;

   E_XA__XEMBED = ex_atom_get("_XEMBED");
   E_XA__XEMBED_INFO = ex_atom_get("_XEMBED_INFO");
   _NET_SYSTEM_TRAY_OPCODE = ex_atom_get("_NET_SYSTEM_TRAY_OPCODE");
   _NET_SYSTEM_TRAY_MESSAGE_DATA = ex_atom_get("_NET_SYSTEM_TRAY_MESSAGE_DATA");

   /* Acquire selection */
   if (systray_sel)
     {
	DialogOK(_("Systray Error!"), _("Only one systray is allowed"));
	return;
     }

   systray_sel =
      SelectionAcquire("_NET_SYSTEM_TRAY_S", SystraySelectionEvent, ct);
   if (!systray_sel)
     {
	DialogOK(_("Systray Error!"), _("Could not activate systray"));
	return;
     }

   win = ct->icon_win;
   ESelectInputChange(win, SubstructureRedirectMask, 0);
   EventCallbackRegister(win, SystrayEvent, ct);

   /* Container parameter setup */
   ct->wm_name = "Systray";
   ct->menu_title = _("Systray Options");
   ct->dlg_title = _("Systray Settings");
   ct->iconsize = 16;
}

static void
SystrayExit(Container * ct, int wm_exit __UNUSED__)
{
   SelectionRelease(systray_sel);
   systray_sel = NoXID;

   EventCallbackUnregister(ct->win, SystrayEvent, ct);

   while (ct->num_objs)
     {
	SystrayObjDel(ct, StObjGetWin(ct->objs[0].obj), 0);
     }
}

static void
SystrayObjSizeCalc(Container * ct, ContainerObject * cto)
{
   /* Inner size */
   if (StObjIsMapped(cto->obj))
      cto->wi = cto->hi = ct->iconsize;
   else
      cto->wi = cto->hi = 0;
}

static void
SystrayObjPlace(Container * ct __UNUSED__, ContainerObject * cto,
		EImage * im __UNUSED__)
{
   if (StObjIsMapped(cto->obj))
     {
	EMoveResizeWindow(StObjGetWin(cto->obj), cto->xi, cto->yi, cto->wi,
			  cto->hi);
	/* This seems to fix rendering for ceratin apps which seem to expect
	 * expose events after resize (e.g. opera) */
	ESync(0);
	EClearWindowExpose(StObjGetWin(cto->obj));
     }
}

extern const ContainerOps SystrayOps;

const ContainerOps  SystrayOps = {
   SystrayInit,
   SystrayExit,
   NULL,
   NULL,
   SystrayObjSizeCalc,
   SystrayObjPlace,
};
