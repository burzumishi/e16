/*
 * Copyright (C) 2000-2007 Carsten Haitzler, Geoff Harrison and various contributors
 * Copyright (C) 2003-2014 Kim Woelders
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
#include "ewins.h"
#include "hints.h"
#include "xprop.h"

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS           (1L << 0)
#define MWM_HINTS_DECORATIONS         (1L << 1)
#define MWM_HINTS_INPUT_MODE          (1L << 2)
#define MWM_HINTS_STATUS              (1L << 3)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

/* bit definitions for MwmHints.inputMode */
#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3

#define PROP_MWM_HINTS_ELEMENTS             5
#define PROP_MWM_HINTS_ELEMENTS_MIN         4

static EX_Atom      _MOTIF_WM_HINTS = 0;

/* Motif window hints */
typedef struct {
   unsigned int        flags;
   unsigned int        functions;
   unsigned int        decorations;
   unsigned int        inputMode;
   unsigned int        status;
} MWMHints;

void
MWM_GetHints(EWin * ewin, EX_Atom atom_change)
{
   int                 num;
   MWMHints            mhs, *mwmhints = &mhs;

   if (EwinIsInternal(ewin))
      return;

   if (!_MOTIF_WM_HINTS)
      _MOTIF_WM_HINTS = ex_atom_get("_MOTIF_WM_HINTS");

   if (atom_change && atom_change != _MOTIF_WM_HINTS)
      return;

   ewin->mwm.valid = 1;
   ewin->mwm.decor_border = 1;
   ewin->mwm.decor_resizeh = 1;
   ewin->mwm.decor_title = 1;
   ewin->mwm.decor_menu = 1;
   ewin->mwm.decor_minimize = 1;
   ewin->mwm.decor_maximize = 1;
   ewin->mwm.func_resize = 1;
   ewin->mwm.func_move = 1;
   ewin->mwm.func_minimize = 1;
   ewin->mwm.func_maximize = 1;
   ewin->mwm.func_close = 1;

   num = ex_window_prop_xid_get(EwinGetClientXwin(ewin), _MOTIF_WM_HINTS,
				_MOTIF_WM_HINTS, &mhs.flags, 5);
   if (num < PROP_MWM_HINTS_ELEMENTS_MIN)
      return;

   if (mwmhints->flags & MWM_HINTS_DECORATIONS)
     {
	ewin->mwm.decor_border = 0;
	ewin->mwm.decor_resizeh = 0;
	ewin->mwm.decor_title = 0;
	ewin->mwm.decor_menu = 0;
	ewin->mwm.decor_minimize = 0;
	ewin->mwm.decor_maximize = 0;
	if (mwmhints->decorations & MWM_DECOR_ALL)
	  {
	     ewin->mwm.decor_border = 1;
	     ewin->mwm.decor_resizeh = 1;
	     ewin->mwm.decor_title = 1;
	     ewin->mwm.decor_menu = 1;
	     ewin->mwm.decor_minimize = 1;
	     ewin->mwm.decor_maximize = 1;
	  }
	if (mwmhints->decorations & MWM_DECOR_BORDER)
	   ewin->mwm.decor_border = 1;
	if (mwmhints->decorations & MWM_DECOR_RESIZEH)
	   ewin->mwm.decor_resizeh = 1;
	if (mwmhints->decorations & MWM_DECOR_TITLE)
	   ewin->mwm.decor_title = 1;
	if (mwmhints->decorations & MWM_DECOR_MENU)
	   ewin->mwm.decor_menu = 1;
	if (mwmhints->decorations & MWM_DECOR_MINIMIZE)
	   ewin->mwm.decor_minimize = 1;
	if (mwmhints->decorations & MWM_DECOR_MAXIMIZE)
	   ewin->mwm.decor_maximize = 1;
     }

   if (mwmhints->flags & MWM_HINTS_FUNCTIONS)
     {
	ewin->mwm.func_resize = 0;
	ewin->mwm.func_move = 0;
	ewin->mwm.func_minimize = 0;
	ewin->mwm.func_maximize = 0;
	ewin->mwm.func_close = 0;
	if (mwmhints->functions & MWM_FUNC_ALL)
	  {
	     ewin->mwm.func_resize = 1;
	     ewin->mwm.func_move = 1;
	     ewin->mwm.func_minimize = 1;
	     ewin->mwm.func_maximize = 1;
	     ewin->mwm.func_close = 1;
	  }
	if (mwmhints->functions & MWM_FUNC_RESIZE)
	   ewin->mwm.func_resize = 1;
	if (mwmhints->functions & MWM_FUNC_MOVE)
	   ewin->mwm.func_move = 1;
	if (mwmhints->functions & MWM_FUNC_MINIMIZE)
	   ewin->mwm.func_minimize = 1;
	if (mwmhints->functions & MWM_FUNC_MAXIMIZE)
	   ewin->mwm.func_maximize = 1;
	if (mwmhints->functions & MWM_FUNC_CLOSE)
	   ewin->mwm.func_close = 1;
     }

   if (!ewin->mwm.decor_title && !ewin->mwm.decor_border)
      ewin->props.no_border = 1;
}

void
MWM_SetInfo(void)
{
   EX_Atom             atom;
   EX_Window           xwin;
   EX_ID               mwminfo[2];

   atom = ex_atom_get("_MOTIF_WM_INFO");
   xwin = WinGetXwin(VROOT);
   mwminfo[0] = 2;
   mwminfo[1] = xwin;
   ex_window_prop_xid_set(xwin, atom, atom, mwminfo, 2);
}
