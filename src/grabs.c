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
#if USE_XI2
#include <X11/extensions/XInput2.h>
#define DEV_PTR Mode.events.xi2_ptr
#define DEV_KBD Mode.events.xi2_kbd
#endif

#include "E.h"
#include "cursors.h"
#include "grabs.h"
#include "xwin.h"

static int
_GrabKeyboard(Win win, int sync_kbd)
{
   int                 rc;

#if USE_XI2
   EXIEventMask        em;

   EXIMaskSetup(&em, DEV_KBD, KeyPressMask | KeyReleaseMask);
   rc = XIGrabDevice(disp, DEV_KBD, WinGetXwin(win), CurrentTime, NoXID,
		     GrabModeAsync, sync_kbd ? GrabModeSync : GrabModeAsync,
		     False, &em.em);
#else
   rc = XGrabKeyboard(disp, WinGetXwin(win), False,
		      GrabModeAsync, sync_kbd ? GrabModeSync : GrabModeAsync,
		      CurrentTime);
#endif

#if 0
   Eprintf("%s: %#lx sync=%d rc=%d\n", __func__, WinGetXwin(win), sync_kbd, rc);
#endif

   return rc;
}

int
GrabKeyboardSet(Win win)
{
   return _GrabKeyboard(win, 0);
}

int
GrabKeyboardFreeze(Win win)
{
   return _GrabKeyboard(win, 1);
}

int
GrabKeyboardRelease(void)
{
   int                 rc;

#if USE_XI2
   rc = XIUngrabDevice(disp, DEV_KBD, CurrentTime);
#else
   rc = XUngrabKeyboard(disp, CurrentTime);
#endif

#if 0
   Eprintf("%s: %d\n", __func__, rc);
#endif
   return rc;
}

int
GrabPointerSet(Win win, unsigned int csr, int confine __UNUSED__)
{
   int                 rc;

#if USE_XI2
   EXIEventMask        em;

   EXIMaskSetup(&em, DEV_PTR,
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
   rc = XIGrabDevice(disp, DEV_PTR, WinGetXwin(win), CurrentTime, ECsrGet(csr),
		     GrabModeAsync, GrabModeAsync, False, &em.em);
#else
   EX_Window           confine_to = (confine) ? WinGetXwin(VROOT) : NoXID;

   rc = XGrabPointer(disp, WinGetXwin(win), False,
		     ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
		     ButtonMotionMask | EnterWindowMask | LeaveWindowMask,
		     GrabModeAsync, GrabModeAsync, confine_to, ECsrGet(csr),
		     CurrentTime);
#endif

   Mode.grabs.pointer_grab_window = WinGetXwin(win);
   Mode.grabs.pointer_grab_active = 1;

   if (EDebug(EDBUG_TYPE_GRABS))
      Eprintf("%s: %#x, rc=%d\n", __func__, Mode.grabs.pointer_grab_window, rc);

   return rc;
}

void
GrabPointerRelease(void)
{
#if USE_XI2
   XIUngrabDevice(disp, DEV_PTR, CurrentTime);
#else
   XUngrabPointer(disp, CurrentTime);
#endif

   if (EDebug(EDBUG_TYPE_GRABS))
      Eprintf("%s: %#x\n", __func__, Mode.grabs.pointer_grab_window);

   Mode.grabs.pointer_grab_active = 0;
   Mode.grabs.pointer_grab_window = NoXID;
}

void
GrabPointerThaw(void)
{
#if USE_XI2
   XIAllowEvents(disp, DEV_PTR, XIReplayDevice, CurrentTime);
#else
   XAllowEvents(disp, ReplayPointer, CurrentTime);
#endif
}

void
GrabButtonSet(unsigned int button, unsigned int modifiers, Win win,
	      unsigned int event_mask, unsigned int csr, int confine __UNUSED__)
{
   Bool                owner_events = False;
   int                 pointer_mode = GrabModeSync;
   int                 keyboard_mode = GrabModeAsync;
   int                 i;

#if USE_XI2
   EXIEventMask        em;
   XIGrabModifiers     modifiers_inouts[8];
   int                 num_modifiers;

   EXIMaskSetup(&em, DEV_PTR, event_mask);

   if (modifiers == AnyModifier)
     {
	num_modifiers = 1;
	modifiers_inouts[0].modifiers = XIAnyModifier;
	modifiers_inouts[0].status = 0;
     }
   else
     {
	num_modifiers = 0;
	for (i = 0; i < 8; i++)
	  {
	     if (i && !Mode.masks.mod_combos[i])
		continue;
	     modifiers_inouts[num_modifiers].modifiers =
		modifiers | Mode.masks.mod_combos[i];
	     modifiers_inouts[num_modifiers].status = 0;
	     num_modifiers++;
	  }
     }
   XIGrabButton(disp, DEV_PTR, button, WinGetXwin(win), ECsrGet(csr),
		pointer_mode, keyboard_mode, owner_events,
		&em.em, num_modifiers, modifiers_inouts);
#else
   EX_Window           confine_to = (confine) ? WinGetXwin(win) : NoXID;

   if (modifiers == AnyModifier)
     {
	XGrabButton(disp, button, modifiers,
		    WinGetXwin(win), owner_events, event_mask, pointer_mode,
		    keyboard_mode, confine_to, ECsrGet(csr));
	return;
     }

   for (i = 0; i < 8; i++)
     {
	if (i && !Mode.masks.mod_combos[i])
	   continue;
	XGrabButton(disp, button, modifiers | Mode.masks.mod_combos[i],
		    WinGetXwin(win), owner_events, event_mask, pointer_mode,
		    keyboard_mode, confine_to, ECsrGet(csr));
     }
#endif
}

void
GrabButtonRelease(unsigned int button, unsigned int modifiers, Win win)
{
   int                 i;

#if USE_XI2
   XIGrabModifiers     modifiers_inouts[8];
   int                 num_modifiers;

   if (modifiers == AnyModifier)
     {
	num_modifiers = 1;
	modifiers_inouts[0].modifiers = XIAnyModifier;
	modifiers_inouts[0].status = 0;
     }
   else
     {
	num_modifiers = 0;
	for (i = 0; i < 8; i++)
	  {
	     if (i && !Mode.masks.mod_combos[i])
		continue;
	     modifiers_inouts[num_modifiers].modifiers =
		modifiers | Mode.masks.mod_combos[i];
	     modifiers_inouts[num_modifiers].status = 0;
	     num_modifiers++;
	  }
     }
   XIUngrabButton(disp, DEV_PTR, button, WinGetXwin(win),
		  num_modifiers, modifiers_inouts);
#else
   if (modifiers == AnyModifier)
     {
	XUngrabButton(disp, button, modifiers, WinGetXwin(win));
	return;
     }

   for (i = 0; i < 8; i++)
     {
	if (i && !Mode.masks.mod_combos[i])
	   continue;
	XUngrabButton(disp, button, modifiers | Mode.masks.mod_combos[i],
		      WinGetXwin(win));
     }
#endif
}

void
GrabKeySet(unsigned int keycode, unsigned int modifiers, Win win)
{
   Bool                owner_events = False;
   int                 pointer_mode = GrabModeAsync;
   int                 keyboard_mode = GrabModeSync;
   int                 i;

#if USE_XI2
   EXIEventMask        em;
   XIGrabModifiers     modifiers_inouts[8];
   int                 num_modifiers;

   EXIMaskSetup(&em, DEV_KBD, KeyPressMask | KeyReleaseMask);

   if (modifiers == AnyModifier)
     {
	num_modifiers = 1;
	modifiers_inouts[0].modifiers = XIAnyModifier;
	modifiers_inouts[0].status = 0;
     }
   else
     {
	num_modifiers = 0;
	for (i = 0; i < 8; i++)
	  {
	     if (i && !Mode.masks.mod_combos[i])
		continue;
	     modifiers_inouts[num_modifiers].modifiers =
		modifiers | Mode.masks.mod_combos[i];
	     modifiers_inouts[num_modifiers].status = 0;
	     num_modifiers++;
	  }
     }
   XIGrabKeycode(disp, DEV_KBD, keycode, WinGetXwin(win),
		 keyboard_mode, pointer_mode, owner_events,
		 &em.em, num_modifiers, modifiers_inouts);
#else

   if (modifiers == AnyModifier)
     {
	XGrabKey(disp, keycode, modifiers, WinGetXwin(win), owner_events,
		 pointer_mode, keyboard_mode);
	return;
     }

   for (i = 0; i < 8; i++)
     {
	if (i && !Mode.masks.mod_combos[i])
	   continue;
	XGrabKey(disp, keycode, modifiers | Mode.masks.mod_combos[i],
		 WinGetXwin(win), owner_events, pointer_mode, keyboard_mode);
     }
#endif
}

void
GrabKeyRelease(unsigned int keycode, unsigned int modifiers, Win win)
{
   int                 i;

#if USE_XI2
   XIGrabModifiers     modifiers_inouts[8];
   int                 num_modifiers;

   if (modifiers == AnyModifier)
     {
	num_modifiers = 1;
	modifiers_inouts[0].modifiers = XIAnyModifier;
	modifiers_inouts[0].status = 0;
     }
   else
     {
	num_modifiers = 0;
	for (i = 0; i < 8; i++)
	  {
	     if (i && !Mode.masks.mod_combos[i])
		continue;
	     modifiers_inouts[num_modifiers].modifiers =
		modifiers | Mode.masks.mod_combos[i];
	     modifiers_inouts[num_modifiers].status = 0;
	     num_modifiers++;
	  }
     }
   XIUngrabKeycode(disp, DEV_KBD, keycode, WinGetXwin(win),
		   num_modifiers, modifiers_inouts);
#else

   if (modifiers == AnyModifier)
     {
	XUngrabKey(disp, keycode, modifiers, WinGetXwin(win));
	return;
     }

   for (i = 0; i < 8; i++)
     {
	if (i && !Mode.masks.mod_combos[i])
	   continue;
	XUngrabKey(disp, keycode, modifiers | Mode.masks.mod_combos[i],
		   WinGetXwin(win));
     }
#endif
}
