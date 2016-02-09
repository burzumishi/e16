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

#include <math.h>
#include <X11/Xlib.h>

#include "E.h"
#include "animation.h"
#include "desktops.h"
#include "dialog.h"
#include "ecompmgr.h"
#include "eimage.h"
#include "emodule.h"
#include "settings.h"
#include "util.h"
#include "xwin.h"

#define M_2PI_F ((float)(2 * M_PI))

#define FX_OP_ENABLE  1		/* Enable, start */
#define FX_OP_DISABLE 2		/* Disable, stop */
#define FX_OP_START   3		/* Start (if enabled) */
#define FX_OP_PAUSE   4
#define FX_OP_DESK    5

typedef struct {
   const char         *name;
   void                (*init_func) (const char *name);
   void                (*ops_func) (int op);
   char                enabled;
   char                active;
} FXHandler;

#if USE_COMPOSITE
/* As of composite 0.4 we need to set the clip region */
#define SET_GC_CLIP(eo, gc) ECompMgrWinClipToGC(eo, gc)
#else
#define SET_GC_CLIP(eo, gc)
#endif

/****************************** RIPPLES *************************************/

#define FX_RIPPLE_WATERH 64

typedef struct {
   Win                 win;
   EX_Pixmap           above;
   int                 count;
   float               incv, inch;
   GC                  gc1;
} fx_ripple_data_t;

static Animator    *fx_ripple = NULL;

static int
FX_ripple_timeout(EObj * eo __UNUSED__, int run __UNUSED__, void *state)
{
   fx_ripple_data_t   *d = (fx_ripple_data_t *) state;
   int                 y;
   EObj               *bgeo;
   XGCValues           xgcv;

   bgeo = DeskGetBackgroundObj(DesksGetCurrent());

   if (!d->above)
     {
	d->win = EobjGetWin(bgeo);
	d->above = ECreatePixmap(d->win, WinGetW(VROOT),
				 FX_RIPPLE_WATERH * 2, 0);
	xgcv.subwindow_mode = ClipByChildren;
	if (!d->gc1)
	   d->gc1 = EXCreateGC(WinGetXwin(d->win), GCSubwindowMode, &xgcv);
     }

   if (d->count == 0)
      EXCopyArea(WinGetXwin(d->win), d->above,
		 0, WinGetH(VROOT) - (FX_RIPPLE_WATERH * 3),
		 WinGetW(VROOT), FX_RIPPLE_WATERH * 2, 0, 0);

   d->count++;
   if (d->count > 32)
      d->count = 0;

   d->incv += 0.40f;
   if (d->incv > M_2PI_F)
      d->incv = 0;
   d->inch += 0.32f;
   if (d->inch > M_2PI_F)
      d->inch = 0;

   SET_GC_CLIP(bgeo, d->gc1);

   for (y = 0; y < FX_RIPPLE_WATERH; y++)
     {
	float               a, p;
	int                 xoff, yoff, yy;

	p = (float)(FX_RIPPLE_WATERH - y) / (float)FX_RIPPLE_WATERH;

	a = p * p * 48 + d->incv;
	yoff = y + (int)(sinf(a) * 7) + 8;	/* 0:63 + -7:7 + 8 = 1:78 */
	yy = (FX_RIPPLE_WATERH * 2) - yoff;	/* 128 - 1:78 = 127:50    */

	a = p * p * 64 + d->inch;
	xoff = (int)(sinf(a) * 10 * (1 - p));

	EXCopyAreaGC(d->above, WinGetXwin(d->win), d->gc1,
		     0, yy, WinGetW(VROOT), 1,
		     xoff, WinGetH(VROOT) - FX_RIPPLE_WATERH + y);
     }

   return 4;
}

static void
FX_Ripple_Init(const char *name __UNUSED__)
{
   fx_ripple_data_t    fxd;

   memset(&fxd, 0, sizeof(fxd));

   fx_ripple = AnimatorAdd(NULL, ANIM_FX_RIPPLES, FX_ripple_timeout, -1, 0,
			   sizeof(fxd), &fxd);
}

static void
FX_Ripple_Ops(int op)
{
   fx_ripple_data_t   *d;

   d = (fx_ripple_data_t *) AnimatorGetData(fx_ripple);
   if (!d)
      return;

   EFreePixmap(d->above);
   d->above = NoXID;
   d->count = 0;

   if (op != FX_OP_DISABLE)
      return;

   EClearArea(d->win, 0, WinGetH(VROOT) - FX_RIPPLE_WATERH,
	      WinGetW(VROOT), FX_RIPPLE_WATERH);
   EXFreeGC(d->gc1);

   AnimatorDel(NULL, fx_ripple);
   fx_ripple = NULL;
}

/****************************** WAVES ***************************************/
/* by tsade :)                                                              */
/****************************************************************************/

#define FX_WAVE_WATERH 64
#define FX_WAVE_WATERW 64
#define FX_WAVE_DEPTH  10
#define FX_WAVE_GRABH  (FX_WAVE_WATERH + FX_WAVE_DEPTH)
#define FX_WAVE_CROSSPERIOD 0.42f

typedef struct {
   Win                 win;
   EX_Pixmap           above;
   int                 count;
   float               incv, inch, incx;
   GC                  gc1;
} fx_waves_data_t;

static Animator    *fx_waves = NULL;

static int
FX_Wave_timeout(EObj * eo __UNUSED__, int run __UNUSED__, void *state)
{
   fx_waves_data_t    *d = (fx_waves_data_t *) state;
   float               incx2;
   int                 y;
   EObj               *bgeo;
   XGCValues           xgcv;

   bgeo = DeskGetBackgroundObj(DesksGetCurrent());

   /* Check to see if we need to create stuff */
   if (!d->above)
     {
	d->win = EobjGetWin(bgeo);
	d->above = ECreatePixmap(d->win, WinGetW(VROOT), FX_WAVE_WATERH * 2, 0);
	xgcv.subwindow_mode = ClipByChildren;
	if (!d->gc1)
	   d->gc1 = EXCreateGC(WinGetXwin(d->win), GCSubwindowMode, &xgcv);
     }

   /* On the zero, grab the desktop again. */
   if (d->count == 0)
      EXCopyArea(WinGetXwin(d->win), d->above,
		 0, WinGetH(VROOT) - (FX_WAVE_WATERH * 3),
		 WinGetW(VROOT), FX_WAVE_WATERH * 2, 0, 0);

   /* Increment and roll the counter */
   d->count++;
   if (d->count > 32)
      d->count = 0;

   /* Increment and roll some other variables */
   d->incv += 0.40f;
   if (d->incv > M_2PI_F)
      d->incv = 0;

   d->inch += 0.32f;
   if (d->inch > M_2PI_F)
      d->inch = 0;

   d->incx += 0.32f;
   if (d->incx > M_2PI_F)
      d->incx = 0;

   SET_GC_CLIP(bgeo, d->gc1);

   /* Copy the area to correct bugs */
   if (d->count == 0)
      EXCopyAreaGC(d->above, WinGetXwin(d->win), d->gc1,
		   0, WinGetH(VROOT) - FX_WAVE_GRABH,
		   WinGetW(VROOT), FX_WAVE_DEPTH * 2,
		   0, WinGetH(VROOT) - FX_WAVE_GRABH);

   /* Go through the bottom couple (FX_WAVE_WATERH) lines of the window */
   for (y = 0; y < FX_WAVE_WATERH; y++)
     {
	/* Variables */
	float               a, p;
	int                 xoff, yoff, yy;
	int                 x;

	/* Figure out the side-to-side movement */
	p = (float)(FX_WAVE_WATERH - y) / (float)FX_WAVE_WATERH;

	a = p * p * 48 + d->incv;
	yoff = y + (int)(sinf(a) * 7) + 8;	/* 0:63 + -7:7 + 8 = 1:78 */
	yy = (FX_WAVE_WATERH * 2) - yoff;	/* 128 - 1:78 = 127:50    */

	a = p * p * 64 + d->inch;
	xoff = (int)(sinf(a) * 10 * (1 - p));

	/* Set up the next part */
	incx2 = d->incx;

	/* Go through the width of the screen, in block sizes */
	for (x = 0; x < WinGetW(VROOT); x += FX_WAVE_WATERW)
	  {
	     /* Variables */
	     int                 sx;

	     /* Add something to incx2 and roll it */
	     incx2 += FX_WAVE_CROSSPERIOD;

	     if (incx2 > M_2PI_F)
		incx2 = 0;

	     /* Figure it out */
	     sx = (int)(sinf(incx2) * FX_WAVE_DEPTH);

	     /* Display this block */
	     EXCopyAreaGC(d->above, WinGetXwin(d->win), d->gc1,
			  x, yy, FX_WAVE_WATERW, 1,
			  xoff + x, WinGetH(VROOT) - FX_WAVE_WATERH + y + sx);
	  }
     }

   return 4;
}

static void
FX_Waves_Init(const char *name __UNUSED__)
{
   fx_waves_data_t     fxd;

   memset(&fxd, 0, sizeof(fxd));

   fx_waves = AnimatorAdd(NULL, ANIM_FX_WAVES, FX_Wave_timeout, -1, 0,
			  sizeof(fxd), &fxd);
}

static void
FX_Waves_Ops(int op)
{
   fx_waves_data_t    *d;

   d = (fx_waves_data_t *) AnimatorGetData(fx_waves);
   if (!d)
      return;

   EFreePixmap(d->above);
   d->above = NoXID;
   d->count = 0;

   if (op != FX_OP_DISABLE)
      return;

   EClearArea(d->win, 0, WinGetH(VROOT) - FX_WAVE_WATERH,
	      WinGetW(VROOT), FX_WAVE_WATERH);
   EXFreeGC(d->gc1);

   AnimatorDel(NULL, fx_waves);
   fx_waves = NULL;
}

/****************************************************************************/

#define fx_rip fx_handlers[0]
#define fx_wav fx_handlers[1]

static FXHandler    fx_handlers[] = {
   {"ripples", FX_Ripple_Init, FX_Ripple_Ops, 0, 0},
   {"waves", FX_Waves_Init, FX_Waves_Ops, 0, 0},
};
#define N_FX_HANDLERS (sizeof(fx_handlers)/sizeof(FXHandler))

/****************************** Effect handlers *****************************/

static void
FX_Op(FXHandler * fxh, int op)
{
   switch (op)
     {
     case FX_OP_ENABLE:
	if (fxh->enabled)
	   break;
	fxh->enabled = 1;
	goto do_start;

     case FX_OP_DISABLE:
	if (!fxh->enabled)
	   break;
	fxh->enabled = 0;
	goto do_stop;

     case FX_OP_START:
	if (!fxh->enabled)
	   break;
      do_start:
	if (fxh->active)
	   break;
	fxh->init_func(fxh->name);
	fxh->active = 1;
	break;

     case FX_OP_PAUSE:
	if (!fxh->enabled)
	   break;
      do_stop:
	if (!fxh->active)
	   break;
	fxh->ops_func(FX_OP_DISABLE);
	fxh->active = 0;
	break;

     case FX_OP_DESK:
	if (!fxh->enabled)
	   break;
	fxh->ops_func(FX_OP_DESK);
	break;
     }
}

static void
FX_OpForEach(int op)
{
   unsigned int        i;

   for (i = 0; i < N_FX_HANDLERS; i++)
      FX_Op(&fx_handlers[i], op);
}

static void
FxCfgFunc(void *item __UNUSED__, const char *value)
{
   FXHandler          *fxh = NULL;

   if (item == &fx_rip.enabled)
      fxh = &fx_rip;
   else if (item == &fx_wav.enabled)
      fxh = &fx_wav;
   if (!fxh)
      return;

   FX_Op(fxh, atoi(value) ? FX_OP_ENABLE : FX_OP_DISABLE);
}

/****************************************************************************/

/*
 * Fx Module
 */

static void
FxSighan(int sig, void *prm __UNUSED__)
{
   switch (sig)
     {
     case ESIGNAL_START:
	FX_OpForEach(FX_OP_START);
	break;
     case ESIGNAL_DESK_SWITCH_START:
	break;
     case ESIGNAL_DESK_SWITCH_DONE:
	FX_OpForEach(FX_OP_DESK);
	break;
     case ESIGNAL_ANIMATION_SUSPEND:
	FX_OpForEach(FX_OP_PAUSE);
	break;
     case ESIGNAL_ANIMATION_RESUME:
	FX_OpForEach(FX_OP_START);
	break;
     }
}

#if ENABLE_DIALOGS
static char         tmp_effect_ripples;
static char         tmp_effect_waves;

static void
_DlgApplyFx(Dialog * d __UNUSED__, int val __UNUSED__, void *data __UNUSED__)
{
   FX_Op(&fx_rip, tmp_effect_ripples ? FX_OP_ENABLE : FX_OP_DISABLE);
   FX_Op(&fx_wav, tmp_effect_waves ? FX_OP_ENABLE : FX_OP_DISABLE);

   autosave();
}

static void
_DlgFillFx(Dialog * d __UNUSED__, DItem * table, void *data __UNUSED__)
{
   DItem              *di;

   tmp_effect_ripples = fx_rip.enabled;
   tmp_effect_waves = fx_wav.enabled;

   DialogItemTableSetOptions(table, 1, 0, 0, 0);

   /* Effects */
   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetText(di, _("Effects"));
   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetText(di, _("Ripples"));
   DialogItemCheckButtonSetPtr(di, &tmp_effect_ripples);
   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetText(di, _("Waves"));
   DialogItemCheckButtonSetPtr(di, &tmp_effect_waves);
}

const DialogDef     DlgFx = {
   "CONFIGURE_FX",
   N_("FX"), N_("Special FX Settings"),
   0,
   SOUND_SETTINGS_FX,
   "pix/fx.png",
   N_("Enlightenment Special Effects\n" "Settings Dialog"),
   _DlgFillFx,
   DLG_OAC, _DlgApplyFx, NULL
};
#endif /* ENABLE_DIALOGS */

#define CFR_FUNC_BOOL(conf, name, dflt, func) \
    { #name, &conf, ITEM_TYPE_BOOL, dflt, func }

static const CfgItem FxCfgItems[] = {
   CFR_FUNC_BOOL(fx_handlers[0].enabled, ripples.enabled, 0, FxCfgFunc),
   CFR_FUNC_BOOL(fx_handlers[1].enabled, waves.enabled, 0, FxCfgFunc),
};
#define N_CFG_ITEMS (sizeof(FxCfgItems)/sizeof(CfgItem))

/*
 * Module descriptor
 */
extern const EModule ModEffects;

const EModule       ModEffects = {
   "effects", "fx",
   FxSighan,
   {0, NULL},
   {N_CFG_ITEMS, FxCfgItems}
};
