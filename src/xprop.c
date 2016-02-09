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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* Window property change actions (must match _NET_WM_STATE_... ones) */
#define EX_PROP_LIST_REMOVE    0
#define EX_PROP_LIST_ADD       1
#define EX_PROP_LIST_TOGGLE    2

#define DEBUG_CHECK 0
#if DEBUG_CHECK
#include <assert.h>
#endif
#include "xprop.h"
#include "xwin.h"

#define _ex_disp disp

#define N_ITEMS(x) (sizeof(x)/sizeof(x[0]))

/*
 * General stuff
 */

EX_Atom
ex_atom_get(const char *name)
{
   return XInternAtom(_ex_disp, name, False);
}

void
ex_atoms_get(const char *const *names, unsigned int num, EX_Atom * atoms)
{
#if SIZEOF_INT == SIZEOF_LONG
   XInternAtoms(_ex_disp, (char **)names, num, False, (Atom *) atoms);
#else
   unsigned int        i;
   Atom               *_atoms;

   _atoms = EMALLOC(Atom, num);
   if (!_atoms)
      return;

   XInternAtoms(_ex_disp, (char **)names, num, False, _atoms);
   for (i = 0; i < num; i++)
      atoms[i] = _atoms[i];

   Efree(_atoms);
#endif
}

/*
 * Send client message (format 32)
 */
int
ex_client_message32_send(EX_Window win, EX_Atom type,
			 unsigned int mask,
			 unsigned int d0, unsigned int d1,
			 unsigned int d2, unsigned int d3, unsigned int d4)
{
   XEvent              xev;

   xev.xclient.type = ClientMessage;
   xev.xclient.window = win;
   xev.xclient.message_type = type;
   xev.xclient.format = 32;
   xev.xclient.data.l[0] = d0;
   xev.xclient.data.l[1] = d1;
   xev.xclient.data.l[2] = d2;
   xev.xclient.data.l[3] = d3;
   xev.xclient.data.l[4] = d4;

   return XSendEvent(_ex_disp, win, False, mask, &xev);
}

/*
 * Set size 32 item (array) property
 */
static void
_ex_window_prop32_set(EX_Window win, EX_Atom atom,
		      EX_Atom type, const unsigned int *val, int num)
{
#if SIZEOF_INT == SIZEOF_LONG
   XChangeProperty(_ex_disp, win, atom, type, 32, PropModeReplace,
		   (unsigned char *)val, num);
#else
   unsigned long      *pl;
   int                 i;

   pl = (unsigned long *)Emalloc(num * sizeof(long));
   if (!pl)
      return;
   for (i = 0; i < num; i++)
      pl[i] = val[i];
   XChangeProperty(_ex_disp, win, atom, type, 32, PropModeReplace,
		   (unsigned char *)pl, num);
   Efree(pl);
#endif
}

/*
 * Get size 32 item (array) property
 *
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * The returned array must be freed with free().
 * Note: Return value 0 means that the property exists but has no elements.
 */
static int
_ex_window_prop32_list_get(EX_Window win, EX_Atom atom,
			   EX_Atom type, unsigned int **val, int num)
{
   unsigned char      *prop_ret;
   Atom                type_ret;
   unsigned long       bytes_after, num_ret;
   int                 format_ret;
   unsigned int       *lst;
   int                 i;

   prop_ret = NULL;
   if (XGetWindowProperty(_ex_disp, win, atom, 0, 0x7fffffff, False,
			  type, &type_ret, &format_ret, &num_ret,
			  &bytes_after, &prop_ret) != Success)
      return -1;

   if (type_ret != type || format_ret != 32)
     {
	num = -1;
     }
   else if (num_ret == 0 || !prop_ret)
     {
	num = 0;
     }
   else
     {
	if (num >= 0)
	  {
	     if ((int)num_ret < num)
		num = (int)num_ret;
	     lst = *val;
	  }
	else
	  {
	     num = (int)num_ret;
	     lst = (unsigned int *)Emalloc(num * sizeof(unsigned int));
	     *val = lst;
	     if (!lst)
		return 0;
	  }
	for (i = 0; i < num; i++)
	   lst[i] = ((unsigned long *)prop_ret)[i];
     }
   if (prop_ret)
      XFree(prop_ret);

   return num;
}

void
ex_window_prop_del(EX_Window win, EX_Atom atom)
{
   XDeleteProperty(_ex_disp, win, atom);
}

/*
 * Set CARD32 (array) property
 */
void
ex_window_prop_card32_set(EX_Window win, EX_Atom atom,
			  const unsigned int *val, unsigned int num)
{
   _ex_window_prop32_set(win, atom, XA_CARDINAL, val, (int)num);
}

/*
 * Get CARD32 (array) property
 *
 * At most len items are returned in val.
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_card32_get(EX_Window win, EX_Atom atom,
			  unsigned int *val, unsigned int len)
{
   return _ex_window_prop32_list_get(win, atom, XA_CARDINAL, &val, (int)len);
}

/*
 * Get CARD32 (array) property of any length
 *
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_card32_list_get(EX_Window win, EX_Atom atom, unsigned int **plst)
{
   return _ex_window_prop32_list_get(win, atom, XA_CARDINAL, plst, -1);
}

/*
 * Set simple string list property
 */
void
ex_window_prop_string_list_set(EX_Window win, EX_Atom atom, char **lst, int num)
{
   XTextProperty       xtp;

   if (XmbTextListToTextProperty(_ex_disp, lst, num,
				 XStdICCTextStyle, &xtp) != Success)
      return;
   XSetTextProperty(_ex_disp, win, &xtp, atom);
   XFree(xtp.value);
}

/*
 * Get simple string list property
 *
 * If the property was successfully fetched the number of items stored in
 * lst is returned, otherwise -1 is returned.
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_string_list_get(EX_Window win, EX_Atom atom, char ***plst)
{
   char              **pstr = NULL;
   XTextProperty       xtp;
   int                 i, items;
   char              **list;
   Status              s;

   *plst = NULL;

   if (!XGetTextProperty(_ex_disp, win, &xtp, atom))
      return -1;

   if (xtp.format == 8)
     {
	s = XmbTextPropertyToTextList(_ex_disp, &xtp, &list, &items);
	if (s == Success)
	  {
	     if (items > 0)
	       {
		  pstr = (char **)Emalloc(items * sizeof(char *));
		  if (!pstr)
		     goto done;
		  for (i = 0; i < items; i++)
		     pstr[i] = (list[i] && (*list[i] || i < items - 1)) ?
			Estrdup(list[i]) : NULL;
	       }
	     if (list)
		XFreeStringList(list);
	     goto done;
	  }
     }

   /* Bad format or XmbTextPropertyToTextList failed - Now what? */
   items = 1;
   pstr = (char **)Emalloc(sizeof(char *));
   if (!pstr)
      goto done;
   pstr[0] = (xtp.value) ? Estrdup((char *)xtp.value) : NULL;

 done:
   XFree(xtp.value);

   *plst = pstr;
   if (!pstr)
      items = 0;
   return items;
}

/*
 * Set simple string property
 */
void
ex_window_prop_string_set(EX_Window win, EX_Atom atom, const char *str)
{
   ex_window_prop_string_list_set(win, atom, (char **)(&str), 1);
}

/*
 * Get simple string property
 */
char               *
ex_window_prop_string_get(EX_Window win, EX_Atom atom)
{
   XTextProperty       xtp;
   char               *str;
   int                 items;
   char              **list;
   Status              s;

   if (!XGetTextProperty(_ex_disp, win, &xtp, atom))
      return NULL;

   if (xtp.format == 8)
     {
	s = XmbTextPropertyToTextList(_ex_disp, &xtp, &list, &items);
	if ((s == Success) && (items > 0))
	  {
	     str = (*list) ? Estrdup(*list) : NULL;
	     XFreeStringList(list);
	  }
	else
	   str = (xtp.value) ? Estrdup((char *)xtp.value) : NULL;
     }
   else
      str = (xtp.value) ? Estrdup((char *)xtp.value) : NULL;

   XFree(xtp.value);

   return str;
}

/*
 * Set UTF-8 string property
 */
static void
_ex_window_prop_string_utf8_set(EX_Window win, EX_Atom atom, const char *str)
{
   XChangeProperty(_ex_disp, win, atom, EX_ATOM_UTF8_STRING, 8,
		   PropModeReplace, (unsigned char *)str, strlen(str));
}

/*
 * Get UTF-8 string property
 */
static char        *
_ex_window_prop_string_utf8_get(EX_Window win, EX_Atom atom)
{
   char               *str;
   unsigned char      *prop_ret;
   Atom                type_ret;
   unsigned long       bytes_after, num_ret;
   int                 format_ret;

   str = NULL;
   prop_ret = NULL;
   XGetWindowProperty(_ex_disp, win, atom, 0, 0x7fffffff, False,
		      EX_ATOM_UTF8_STRING, &type_ret,
		      &format_ret, &num_ret, &bytes_after, &prop_ret);
   if (prop_ret && num_ret > 0 && format_ret == 8)
     {
	str = (char *)Emalloc(num_ret + 1);
	if (str)
	  {
	     memcpy(str, prop_ret, num_ret);
	     str[num_ret] = '\0';
	  }
     }
   if (prop_ret)
      XFree(prop_ret);

   return str;
}

/*
 * Set X ID (array) property
 */
void
ex_window_prop_xid_set(EX_Window win, EX_Atom atom, EX_Atom type,
		       const EX_ID * lst, unsigned int num)
{
   _ex_window_prop32_set(win, atom, type, lst, (int)num);
}

/*
 * Get X ID (array) property
 *
 * At most len items are returned in val.
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_xid_get(EX_Window win, EX_Atom atom, EX_Atom type,
		       EX_ID * lst, unsigned int len)
{
   return _ex_window_prop32_list_get(win, atom, type, &lst, (int)len);
}

/*
 * Get X ID (array) property
 *
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * The returned array must be freed with free().
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_xid_list_get(EX_Window win, EX_Atom atom,
			    EX_Atom type, EX_ID ** val)
{
   return _ex_window_prop32_list_get(win, atom, type, val, -1);
}

/*
 * Remove/add/toggle X ID list item.
 */
void
ex_window_prop_xid_list_change(EX_Window win, EX_Atom atom,
			       EX_Atom type, EX_ID item, int op)
{
   EX_ID              *lst, *lst_r;
   int                 i, num;

   num = ex_window_prop_xid_list_get(win, atom, type, &lst);
   if (num < 0)
      return;			/* Error - assuming invalid window */

   /* Is it there? */
   for (i = 0; i < num; i++)
     {
	if (lst[i] == item)
	   break;
     }

   if (i < num)
     {
	/* Was in list */
	if (op == EX_PROP_LIST_ADD)
	   goto done;
	/* Remove it */
	num--;
	for (; i < num; i++)
	   lst[i] = lst[i + 1];
     }
   else
     {
	/* Was not in list */
	if (op == EX_PROP_LIST_REMOVE)
	   goto done;
	/* Add it */
	num++;
	lst_r = (EX_ID *) realloc(lst, num * sizeof(EX_ID));
	if (!lst_r)
	   goto done;
	lst = lst_r;
	lst[i] = item;
     }

   ex_window_prop_xid_set(win, atom, type, lst, num);

 done:
   Efree(lst);
}

/*
 * Set Atom (array) property
 */
void
ex_window_prop_atom_set(EX_Window win, EX_Atom atom,
			const EX_Atom * lst, unsigned int num)
{
   ex_window_prop_xid_set(win, atom, XA_ATOM, lst, num);
}

/*
 * Get Atom (array) property
 *
 * At most len items are returned in val.
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_atom_get(EX_Window win, EX_Atom atom,
			EX_Atom * lst, unsigned int len)
{
   return ex_window_prop_xid_get(win, atom, XA_ATOM, lst, len);
}

/*
 * Get Atom (array) property
 *
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * The returned array must be freed with free().
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_atom_list_get(EX_Window win, EX_Atom atom, EX_Atom ** plst)
{
   return ex_window_prop_xid_list_get(win, atom, XA_ATOM, plst);
}

/*
 * Remove/add/toggle atom list item.
 */
void
ex_window_prop_atom_list_change(EX_Window win, EX_Atom atom,
				EX_Atom item, int op)
{
   ex_window_prop_xid_list_change(win, atom, XA_ATOM, item, op);
}

/*
 * Set Window (array) property
 */
void
ex_window_prop_window_set(EX_Window win, EX_Atom atom,
			  const EX_Window * lst, unsigned int num)
{
   ex_window_prop_xid_set(win, atom, XA_WINDOW, lst, num);
}

/*
 * Get Window (array) property
 *
 * At most len items are returned in val.
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_window_get(EX_Window win, EX_Atom atom,
			  EX_Window * lst, unsigned int len)
{
   return ex_window_prop_xid_get(win, atom, XA_WINDOW, lst, len);
}

/*
 * Get Window (array) property
 *
 * If the property was successfully fetched the number of items stored in
 * val is returned, otherwise -1 is returned.
 * The returned array must be freed with free().
 * Note: Return value 0 means that the property exists but has no elements.
 */
int
ex_window_prop_window_list_get(EX_Window win, EX_Atom atom, EX_Window ** plst)
{
   return ex_window_prop_xid_list_get(win, atom, XA_WINDOW, plst);
}

/*
 * ICCCM stuff
 */

static const char  *const atoms_icccm_names[] = {
/* *INDENT-OFF* */

   /* ICCCM */
   "WM_STATE",
   "WM_WINDOW_ROLE",
   "WM_CLIENT_LEADER",
   "WM_COLORMAP_WINDOWS",
   "WM_CHANGE_STATE",
   "WM_PROTOCOLS",
   "WM_DELETE_WINDOW",
   "WM_TAKE_FOCUS",
#if 0
   "WM_SAVE_YOURSELF",
#endif

   /* Misc. */
   "UTF8_STRING",

/* *INDENT-ON* */
};
EX_Atom             atoms_icccm[CHECK_COUNT_ICCCM];

void
ex_icccm_init(void)
{
#if DEBUG_CHECK
   assert(CHECK_COUNT_ICCCM == N_ITEMS(atoms_icccm));
#endif
   ex_atoms_get(atoms_icccm_names, N_ITEMS(atoms_icccm), atoms_icccm);
}

static void
ex_icccm_state_set(EX_Window win, unsigned int state)
{
   unsigned long       c[2];

   c[0] = state;
   c[1] = 0;
   XChangeProperty(_ex_disp, win, EX_ATOM_WM_STATE, EX_ATOM_WM_STATE,
		   32, PropModeReplace, (unsigned char *)c, 2);
}

void
ex_icccm_state_set_iconic(EX_Window win)
{
   ex_icccm_state_set(win, IconicState);
}

void
ex_icccm_state_set_normal(EX_Window win)
{
   ex_icccm_state_set(win, NormalState);
}

void
ex_icccm_state_set_withdrawn(EX_Window win)
{
   ex_icccm_state_set(win, WithdrawnState);
}

static void
ex_icccm_client_message_send(EX_Window win, EX_Atom atom, EX_Time ts)
{
   ex_client_message32_send(win, EX_ATOM_WM_PROTOCOLS, NoEventMask,
			    atom, ts, 0, 0, 0);
}

void
ex_icccm_delete_window_send(EX_Window win, EX_Time ts)
{
   ex_icccm_client_message_send(win, EX_ATOM_WM_DELETE_WINDOW, ts);
}

void
ex_icccm_take_focus_send(EX_Window win, EX_Time ts)
{
   ex_icccm_client_message_send(win, EX_ATOM_WM_TAKE_FOCUS, ts);
}

#if 0
void
ex_icccm_save_yourself_send(EX_Window win, EX_Time ts)
{
   ex_icccm_client_message_send(win, EX_ATOM_WM_SAVE_YOURSELF, ts);
}
#endif

void
ex_icccm_title_set(EX_Window win, const char *title)
{
   ex_window_prop_string_set(win, EX_ATOM_WM_NAME, title);
}

char               *
ex_icccm_title_get(EX_Window win)
{
   return ex_window_prop_string_get(win, EX_ATOM_WM_NAME);
}

void
ex_icccm_name_class_set(EX_Window win, const char *name, const char *clss)
{
   XClassHint         *xch;

   xch = XAllocClassHint();
   if (!xch)
      return;
   xch->res_name = (char *)name;
   xch->res_class = (char *)clss;
   XSetClassHint(_ex_disp, win, xch);
   XFree(xch);
}

void
ex_icccm_name_class_get(EX_Window win, char **name, char **clss)
{
   XClassHint          xch;

   *name = *clss = NULL;
   xch.res_name = NULL;
   xch.res_class = NULL;
   if (XGetClassHint(_ex_disp, win, &xch))
     {
	if (name && xch.res_name)
	   *name = Estrdup(xch.res_name);
	if (clss && xch.res_class)
	   *clss = Estrdup(xch.res_class);
	XFree(xch.res_name);
	XFree(xch.res_class);
     }
}

/*
 * _NET_WM hints (EWMH)
 */

static const char  *const atoms_netwm_names[] = {
/* *INDENT-OFF* */

   /* Window manager info */
   "_NET_SUPPORTED",
   "_NET_SUPPORTING_WM_CHECK",

   /* Desktop status/requests */
   "_NET_NUMBER_OF_DESKTOPS",
   "_NET_VIRTUAL_ROOTS",
   "_NET_DESKTOP_GEOMETRY",
   "_NET_DESKTOP_NAMES",
   "_NET_DESKTOP_VIEWPORT",
   "_NET_WORKAREA",
   "_NET_CURRENT_DESKTOP",
   "_NET_SHOWING_DESKTOP",

   "_NET_ACTIVE_WINDOW",
   "_NET_CLIENT_LIST",
   "_NET_CLIENT_LIST_STACKING",

   /* Client window props/client messages */
   "_NET_WM_NAME",
   "_NET_WM_VISIBLE_NAME",
   "_NET_WM_ICON_NAME",
   "_NET_WM_VISIBLE_ICON_NAME",

   "_NET_WM_DESKTOP",

   "_NET_WM_WINDOW_TYPE",
   "_NET_WM_WINDOW_TYPE_DESKTOP",
   "_NET_WM_WINDOW_TYPE_DOCK",
   "_NET_WM_WINDOW_TYPE_TOOLBAR",
   "_NET_WM_WINDOW_TYPE_MENU",
   "_NET_WM_WINDOW_TYPE_UTILITY",
   "_NET_WM_WINDOW_TYPE_SPLASH",
   "_NET_WM_WINDOW_TYPE_DIALOG",
   "_NET_WM_WINDOW_TYPE_NORMAL",

   "_NET_WM_STATE",
   "_NET_WM_STATE_MODAL",
   "_NET_WM_STATE_STICKY",
   "_NET_WM_STATE_MAXIMIZED_VERT",
   "_NET_WM_STATE_MAXIMIZED_HORZ",
   "_NET_WM_STATE_SHADED",
   "_NET_WM_STATE_SKIP_TASKBAR",
   "_NET_WM_STATE_SKIP_PAGER",
   "_NET_WM_STATE_HIDDEN",
   "_NET_WM_STATE_FULLSCREEN",
   "_NET_WM_STATE_ABOVE",
   "_NET_WM_STATE_BELOW",
   "_NET_WM_STATE_DEMANDS_ATTENTION",

   "_NET_WM_ALLOWED_ACTIONS",
   "_NET_WM_ACTION_MOVE",
   "_NET_WM_ACTION_RESIZE",
   "_NET_WM_ACTION_MINIMIZE",
   "_NET_WM_ACTION_SHADE",
   "_NET_WM_ACTION_STICK",
   "_NET_WM_ACTION_MAXIMIZE_HORZ",
   "_NET_WM_ACTION_MAXIMIZE_VERT",
   "_NET_WM_ACTION_FULLSCREEN",
   "_NET_WM_ACTION_CHANGE_DESKTOP",
   "_NET_WM_ACTION_CLOSE",
   "_NET_WM_ACTION_ABOVE",
   "_NET_WM_ACTION_BELOW",

   "_NET_WM_STRUT",
   "_NET_WM_STRUT_PARTIAL",

   "_NET_FRAME_EXTENTS",

   "_NET_WM_ICON",

   "_NET_WM_USER_TIME",
   "_NET_WM_USER_TIME_WINDOW",

#if 0				/* Not used */
   "_NET_WM_ICON_GEOMETRY",
   "_NET_WM_PID",
   "_NET_WM_HANDLED_ICONS",

   "_NET_WM_PING",
#endif
   "_NET_WM_SYNC_REQUEST",
   "_NET_WM_SYNC_REQUEST_COUNTER",

   "_NET_WM_WINDOW_OPACITY",

   /* Misc window ops */
   "_NET_CLOSE_WINDOW",
   "_NET_MOVERESIZE_WINDOW",
   "_NET_WM_MOVERESIZE",
   "_NET_RESTACK_WINDOW",

#if 0				/* Not yet implemented */
   "_NET_REQUEST_FRAME_EXTENTS",
#endif

   /* Startup notification */
   "_NET_STARTUP_ID",
   "_NET_STARTUP_INFO_BEGIN",
   "_NET_STARTUP_INFO",

/* *INDENT-ON* */
};
EX_Atom             atoms_netwm[CHECK_COUNT_NETWM];

void
ex_netwm_init(void)
{
#if DEBUG_CHECK
   assert(CHECK_COUNT_NETWM == N_ITEMS(atoms_netwm));
#endif
   ex_atoms_get(atoms_netwm_names, N_ITEMS(atoms_netwm), atoms_netwm);
}

/*
 * WM identification
 */
void
ex_netwm_wm_identify(EX_Window root, EX_Window check, const char *wm_name)
{
   ex_window_prop_window_set(root, EX_ATOM_NET_SUPPORTING_WM_CHECK, &check, 1);
   ex_window_prop_window_set(check, EX_ATOM_NET_SUPPORTING_WM_CHECK, &check, 1);
   _ex_window_prop_string_utf8_set(check, EX_ATOM_NET_WM_NAME, wm_name);
   /* This one isn't mandatory */
   _ex_window_prop_string_utf8_set(root, EX_ATOM_NET_WM_NAME, wm_name);
}

/*
 * Desktop configuration and status
 */

void
ex_netwm_desk_count_set(EX_Window root, unsigned int n_desks)
{
   ex_window_prop_card32_set(root, EX_ATOM_NET_NUMBER_OF_DESKTOPS, &n_desks, 1);
}

void
ex_netwm_desk_roots_set(EX_Window root, const EX_Window * vroots,
			unsigned int n_desks)
{
   ex_window_prop_window_set(root, EX_ATOM_NET_VIRTUAL_ROOTS, vroots, n_desks);
}

void
ex_netwm_desk_names_set(EX_Window root, const char **names,
			unsigned int n_desks)
{
   char                ss[32], *buf, *buf_r;
   const char         *s;
   unsigned int        i;
   int                 l, len;

   buf = NULL;
   len = 0;

   for (i = 0; i < n_desks; i++)
     {
	s = (names) ? names[i] : NULL;
	if (!s)
	  {
	     /* Default to "Desk-<number>" */
	     sprintf(ss, "Desk-%u", i);
	     s = ss;
	  }

	l = strlen(s) + 1;
	buf_r = (char *)realloc(buf, len + l);
	if (!buf_r)
	   goto done;
	buf = buf_r;
	memcpy(buf + len, s, l);
	len += l;
     }

   XChangeProperty(_ex_disp, root, EX_ATOM_NET_DESKTOP_NAMES,
		   EX_ATOM_UTF8_STRING, 8, PropModeReplace,
		   (unsigned char *)buf, len);

 done:
   Efree(buf);
}

void
ex_netwm_desk_size_set(EX_Window root, unsigned int width, unsigned int height)
{
   unsigned int        size[2];

   size[0] = width;
   size[1] = height;
   ex_window_prop_card32_set(root, EX_ATOM_NET_DESKTOP_GEOMETRY, size, 2);
}

void
ex_netwm_desk_workareas_set(EX_Window root, const unsigned int *areas,
			    unsigned int n_desks)
{
   ex_window_prop_card32_set(root, EX_ATOM_NET_WORKAREA, areas, 4 * n_desks);
}

void
ex_netwm_desk_current_set(EX_Window root, unsigned int desk)
{
   ex_window_prop_card32_set(root, EX_ATOM_NET_CURRENT_DESKTOP, &desk, 1);
}

void
ex_netwm_desk_viewports_set(EX_Window root, const unsigned int *origins,
			    unsigned int n_desks)
{
   ex_window_prop_card32_set(root, EX_ATOM_NET_DESKTOP_VIEWPORT,
			     origins, 2 * n_desks);
}

void
ex_netwm_showing_desktop_set(EX_Window root, int on)
{
   unsigned int        val;

   val = (on) ? 1 : 0;
   ex_window_prop_card32_set(root, EX_ATOM_NET_SHOWING_DESKTOP, &val, 1);
}

/*
 * Client status
 */

/* Mapping order */
void
ex_netwm_client_list_set(EX_Window root, const EX_Window * p_clients,
			 unsigned int n_clients)
{
   ex_window_prop_window_set(root, EX_ATOM_NET_CLIENT_LIST,
			     p_clients, n_clients);
}

/* Stacking order */
void
ex_netwm_client_list_stacking_set(EX_Window root,
				  const EX_Window * p_clients,
				  unsigned int n_clients)
{
   ex_window_prop_window_set(root, EX_ATOM_NET_CLIENT_LIST_STACKING,
			     p_clients, n_clients);
}

void
ex_netwm_client_active_set(EX_Window root, EX_Window win)
{
   ex_window_prop_window_set(root, EX_ATOM_NET_ACTIVE_WINDOW, &win, 1);
}

/*
 * Client window properties
 */

void
ex_netwm_name_set(EX_Window win, const char *name)
{
   _ex_window_prop_string_utf8_set(win, EX_ATOM_NET_WM_NAME, name);
}

int
ex_netwm_name_get(EX_Window win, char **name)
{
   char               *s;

   s = _ex_window_prop_string_utf8_get(win, EX_ATOM_NET_WM_NAME);
   *name = s;

   return ! !s;
}

void
ex_netwm_visible_name_set(EX_Window win, const char *name)
{
   _ex_window_prop_string_utf8_set(win, EX_ATOM_NET_WM_VISIBLE_NAME, name);
}

int
ex_netwm_visible_name_get(EX_Window win, char **name)
{
   char               *s;

   s = _ex_window_prop_string_utf8_get(win, EX_ATOM_NET_WM_VISIBLE_NAME);
   *name = s;

   return ! !s;
}

void
ex_netwm_icon_name_set(EX_Window win, const char *name)
{
   _ex_window_prop_string_utf8_set(win, EX_ATOM_NET_WM_ICON_NAME, name);
}

int
ex_netwm_icon_name_get(EX_Window win, char **name)
{
   char               *s;

   s = _ex_window_prop_string_utf8_get(win, EX_ATOM_NET_WM_ICON_NAME);
   *name = s;

   return ! !s;
}

void
ex_netwm_visible_icon_name_set(EX_Window win, const char *name)
{
   _ex_window_prop_string_utf8_set(win, EX_ATOM_NET_WM_VISIBLE_ICON_NAME, name);
}

int
ex_netwm_visible_icon_name_get(EX_Window win, char **name)
{
   char               *s;

   s = _ex_window_prop_string_utf8_get(win, EX_ATOM_NET_WM_VISIBLE_ICON_NAME);
   *name = s;

   return ! !s;
}

void
ex_netwm_desktop_set(EX_Window win, unsigned int desk)
{
   ex_window_prop_card32_set(win, EX_ATOM_NET_WM_DESKTOP, &desk, 1);
}

int
ex_netwm_desktop_get(EX_Window win, unsigned int *desk)
{
   return ex_window_prop_card32_get(win, EX_ATOM_NET_WM_DESKTOP, desk, 1);
}

int
ex_netwm_user_time_get(EX_Window win, unsigned int *ts)
{
   return ex_window_prop_card32_get(win, EX_ATOM_NET_WM_USER_TIME, ts, 1);
}

void
ex_netwm_opacity_set(EX_Window win, unsigned int opacity)
{
   ex_window_prop_card32_set(win, EX_ATOM_NET_WM_WINDOW_OPACITY, &opacity, 1);
}

int
ex_netwm_opacity_get(EX_Window win, unsigned int *opacity)
{
   return ex_window_prop_card32_get(win, EX_ATOM_NET_WM_WINDOW_OPACITY,
				    opacity, 1);
}

#if 0				/* Not used */
void
ex_netwm_startup_id_set(EX_Window win, const char *id)
{
   _ex_window_prop_string_utf8_set(win, EX_ATOM_NET_STARTUP_ID, id);
}
#endif

int
ex_netwm_startup_id_get(EX_Window win, char **id)
{
   char               *s;

   s = _ex_window_prop_string_utf8_get(win, EX_ATOM_NET_STARTUP_ID);
   *id = s;

   return ! !s;
}
