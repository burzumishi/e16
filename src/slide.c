/*
 * Copyright (C) 2013-2014 Kim Woelders
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
#include "E.h"
#include "animation.h"
#include "eobj.h"
#include "ewins.h"
#include "focus.h"
#include "slide.h"
#include "xwin.h"

/*
 * EObj sliding functions
 */

typedef struct {
   int                 fx, fy, fw, fh;
   int                 tx, ty, tw, th;
} eobj_slide_params;

static int
_EobjSlideSizeTo(EObj * eo, int remaining, void *state)
{
   eobj_slide_params  *p = (eobj_slide_params *) state;
   int                 k = 1024 - remaining, x, y, w, h;

   x = (p->fx * (1024 - k) + p->tx * k) >> 10;
   y = (p->fy * (1024 - k) + p->ty * k) >> 10;
   w = (p->fw * (1024 - k) + p->tw * k) >> 10;
   h = (p->fh * (1024 - k) + p->th * k) >> 10;
   EobjMoveResize(eo, x, y, w, h);

   return 0;
}

void
EobjSlideSizeTo(EObj * eo, int fx, int fy, int tx, int ty, int fw, int fh,
		int tw, int th, int speed)
{
   eobj_slide_params   p;
   int                 duration;

   p.fx = fx;
   p.fy = fy;
   p.fw = fw;
   p.fh = fh;
   p.tx = tx;
   p.ty = ty;
   p.tw = tw;
   p.th = th;

   if (speed < SPEED_MIN)
      speed = SPEED_MIN;
   duration = 1000000 / speed;

   AnimatorAdd(eo, ANIM_SLIDE, _EobjSlideSizeTo, duration, 1, sizeof(p), &p);
}

/*
 * EWin sliding functions
 */

typedef struct {
   int                 fx, fy, fw, fh;
   int                 tx, ty, tw, th;
   int                 mode;
   char                warp;
   char                firstlast;
} ewin_slide_params;

static int
_EwinSlideSizeTo(EObj * eo, int remaining, void *state)
{
   ewin_slide_params  *p = (ewin_slide_params *) state;
   EWin               *ewin = (EWin *) eo;
   int                 k = 1024 - remaining, x, y, w, h;

   x = (p->fx * (1024 - k) + p->tx * k) >> 10;
   y = (p->fy * (1024 - k) + p->ty * k) >> 10;
   w = (p->fw * (1024 - k) + p->tw * k) >> 10;
   h = (p->fh * (1024 - k) + p->th * k) >> 10;

   if (p->mode == MR_OPAQUE)
      EwinMoveResize(ewin, x, y, w, h, MRF_KEEP_MAXIMIZED);
   else
      DrawEwinShape(ewin, p->mode, x, y, w, h, p->firstlast, 0);
   if (p->firstlast == 0)
      p->firstlast = 1;

   if (!remaining)
     {
	ewin->state.sliding = 0;
	if (p->mode != MR_OPAQUE)
	   DrawEwinShape(ewin, p->mode, p->tx, p->ty,
			 ewin->client.w, ewin->client.h, 2, 0);
	EwinMove(ewin, p->tx, p->ty, MRF_NOCHECK_ONSCREEN | MRF_KEEP_MAXIMIZED);
	if (p->warp)
	  {
	     EwinWarpTo(ewin, 1);
	     FocusToEWin(ewin, FOCUS_SET);
	  }
     }

   return 0;
}

Animator           *
EwinSlideSizeTo(EWin * ewin, int tx, int ty, int tw, int th,
		int speed, int mode, int flags)
{
   Animator           *an;
   ewin_slide_params   p;
   int                 duration, warp, mx, my;

   /* Warp pointer back into window (and focus) if SLIDE_WARP and window
    * is focused on start and pointer lands outside window after resize.
    * Manual pointer moves during animation are not considered for now. */
   warp = (flags & SLIDE_WARP) && ewin->state.active;
   if (warp)
     {
	EQueryPointer(NULL, &mx, &my, NULL, NULL);
	warp = mx < tx || mx >= tx + tw || my < ty || my >= ty + th;
     }

   if (speed == 0)
     {
	EwinMoveResize(ewin, tx, ty, tw, th, MRF_KEEP_MAXIMIZED);
	if (warp)
	  {
	     EwinWarpTo(ewin, 1);
	     FocusToEWin(ewin, FOCUS_SET);
	  }
	return NULL;
     }

   ewin->state.sliding = 1;

   p.fx = EoGetX(ewin);
   p.fy = EoGetY(ewin);
   p.fw = ewin->client.w;
   p.fh = ewin->client.h;
   p.tx = tx;
   p.ty = ty;
   p.tw = tw;
   p.th = th;
   p.mode = DrawEwinShapeNeedsGrab(mode) ? MR_OPAQUE : mode;
   p.firstlast = 0;
   p.warp = warp;

   if (speed < SPEED_MIN)
      speed = SPEED_MIN;
   duration = 1000000 / speed;

   an = AnimatorAdd((EObj *) ewin, ANIM_SLIDE, _EwinSlideSizeTo, duration, 0,
		    sizeof(p), &p);
   if (flags & SLIDE_SOUND)
      AnimatorSetSound(an, SOUND_WINDOW_SLIDE, SOUND_WINDOW_SLIDE_END);

   return an;
}

Animator           *
EwinSlideTo(EWin * ewin, int fx __UNUSED__, int fy __UNUSED__, int tx, int ty,
	    int speed, int mode, int flags)
{
   return EwinSlideSizeTo(ewin, tx, ty, ewin->client.w, ewin->client.h,
			  speed, mode, flags);
}

Animator           *
EwinsSlideTo(EWin ** ewin, int *fx, int *fy, int *tx, int *ty, int num_wins,
	     int speed, int mode, int flags)
{
   Animator           *an = NULL;
   int                 i;

   for (i = 0; i < num_wins; i++)
     {
	an =
	   EwinSlideTo(ewin[i], fx[i], fy[i], tx[i], ty[i], speed, mode, flags);
	flags |= SLIDE_SOUND;
     }

   return an;
}
