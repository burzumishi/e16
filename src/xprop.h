/*
 * Copyright (C) 2004-2014 Kim Woelders
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
#ifndef _XPROP_H_
#define _XPROP_H_

#include "xtypes.h"

EX_Atom             ex_atom_get(const char *name);
void                ex_atoms_get(const char *const *names,
				 unsigned int num, EX_Atom * atoms);

int                 ex_client_message32_send(EX_Window win, EX_Atom type,
					     unsigned int mask, unsigned int d0,
					     unsigned int d1, unsigned int d2,
					     unsigned int d3, unsigned int d4);

void                ex_window_prop_del(EX_Window win, EX_Atom atom);

void                ex_window_prop_card32_set(EX_Window win, EX_Atom atom,
					      const unsigned int *val,
					      unsigned int num);
int                 ex_window_prop_card32_get(EX_Window win, EX_Atom atom,
					      unsigned int *val,
					      unsigned int len);
int                 ex_window_prop_card32_list_get(EX_Window win, EX_Atom atom,
						   unsigned int **plst);

void                ex_window_prop_xid_set(EX_Window win, EX_Atom atom,
					   EX_Atom type,
					   const EX_ID * lst, unsigned int num);
int                 ex_window_prop_xid_get(EX_Window win, EX_Atom atom,
					   EX_Atom type,
					   EX_ID * lst, unsigned int len);
int                 ex_window_prop_xid_list_get(EX_Window win, EX_Atom atom,
						EX_Atom type, EX_ID ** plst);
void                ex_window_prop_xid_list_change(EX_Window win, EX_Atom atom,
						   EX_Atom type,
						   EX_ID item, int op);
void                ex_window_prop_atom_set(EX_Window win, EX_Atom atom,
					    const EX_Atom * val,
					    unsigned int num);
int                 ex_window_prop_atom_get(EX_Window win, EX_Atom atom,
					    EX_Atom * val, unsigned int len);
int                 ex_window_prop_atom_list_get(EX_Window win, EX_Atom atom,
						 EX_Atom ** plst);
void                ex_window_prop_atom_list_change(EX_Window win, EX_Atom atom,
						    EX_Atom item, int op);
void                ex_window_prop_window_set(EX_Window win, EX_Atom atom,
					      const EX_Window * val,
					      unsigned int num);
int                 ex_window_prop_window_get(EX_Window win, EX_Atom atom,
					      EX_Window * val,
					      unsigned int len);
int                 ex_window_prop_window_list_get(EX_Window win, EX_Atom atom,
						   EX_Window ** plst);

void                ex_window_prop_string_set(EX_Window win, EX_Atom atom,
					      const char *str);
char               *ex_window_prop_string_get(EX_Window win, EX_Atom atom);

/* Misc. */
#include <X11/Xatom.h>
extern EX_Atom      atoms_icccm[];

#define EX_ATOM_UTF8_STRING		atoms_icccm[8]

/* ICCCM */
#define EX_ATOM_WM_STATE		atoms_icccm[0]
#define EX_ATOM_WM_WINDOW_ROLE		atoms_icccm[1]
#define EX_ATOM_WM_CLIENT_LEADER	atoms_icccm[2]
#define EX_ATOM_WM_COLORMAP_WINDOWS	atoms_icccm[3]
#define EX_ATOM_WM_CHANGE_STATE		atoms_icccm[4]
#define EX_ATOM_WM_PROTOCOLS		atoms_icccm[5]
#define EX_ATOM_WM_DELETE_WINDOW	atoms_icccm[6]
#define EX_ATOM_WM_TAKE_FOCUS		atoms_icccm[7]

#define CHECK_COUNT_ICCCM 9

#define EX_ATOM_WM_CLASS		XA_WM_CLASS
#define EX_ATOM_WM_NAME			XA_WM_NAME
#define EX_ATOM_WM_COMMAND		XA_WM_COMMAND
#define EX_ATOM_WM_ICON_NAME		XA_WM_ICON_NAME
#define EX_ATOM_WM_CLIENT_MACHINE	XA_WM_CLIENT_MACHINE
#define EX_ATOM_WM_HINTS		XA_WM_HINTS
#define EX_ATOM_WM_NORMAL_HINTS		XA_WM_NORMAL_HINTS
#define EX_ATOM_WM_TRANSIENT_FOR	XA_WM_TRANSIENT_FOR

void                ex_icccm_init(void);

void                ex_icccm_delete_window_send(EX_Window win, EX_Time ts);
void                ex_icccm_take_focus_send(EX_Window win, EX_Time ts);

void                ex_icccm_title_set(EX_Window win, const char *title);
char               *ex_icccm_title_get(EX_Window win);
void                ex_icccm_name_class_set(EX_Window win,
					    const char *name, const char *clss);
void                ex_icccm_name_class_get(EX_Window win,
					    char **name, char **clss);

/* NETWM (EWMH) */
extern EX_Atom      atoms_netwm[];

/* Window manager info */
#define EX_ATOM_NET_SUPPORTED			atoms_netwm[0]
#define EX_ATOM_NET_SUPPORTING_WM_CHECK		atoms_netwm[1]

/* Desktop status/requests */
#define EX_ATOM_NET_NUMBER_OF_DESKTOPS		atoms_netwm[2]
#define EX_ATOM_NET_VIRTUAL_ROOTS		atoms_netwm[3]
#define EX_ATOM_NET_DESKTOP_GEOMETRY		atoms_netwm[4]
#define EX_ATOM_NET_DESKTOP_NAMES		atoms_netwm[5]
#define EX_ATOM_NET_DESKTOP_VIEWPORT		atoms_netwm[6]
#define EX_ATOM_NET_WORKAREA			atoms_netwm[7]
#define EX_ATOM_NET_CURRENT_DESKTOP		atoms_netwm[8]
#define EX_ATOM_NET_SHOWING_DESKTOP		atoms_netwm[9]

#define EX_ATOM_NET_ACTIVE_WINDOW		atoms_netwm[10]
#define EX_ATOM_NET_CLIENT_LIST			atoms_netwm[11]
#define EX_ATOM_NET_CLIENT_LIST_STACKING	atoms_netwm[12]

/* Client window props/client messages */
#define EX_ATOM_NET_WM_NAME			atoms_netwm[13]
#define EX_ATOM_NET_WM_VISIBLE_NAME		atoms_netwm[14]
#define EX_ATOM_NET_WM_ICON_NAME		atoms_netwm[15]
#define EX_ATOM_NET_WM_VISIBLE_ICON_NAME	atoms_netwm[16]

#define EX_ATOM_NET_WM_DESKTOP			atoms_netwm[17]

#define EX_ATOM_NET_WM_WINDOW_TYPE		atoms_netwm[18]
#define EX_ATOM_NET_WM_WINDOW_TYPE_DESKTOP	atoms_netwm[19]
#define EX_ATOM_NET_WM_WINDOW_TYPE_DOCK		atoms_netwm[20]
#define EX_ATOM_NET_WM_WINDOW_TYPE_TOOLBAR	atoms_netwm[21]
#define EX_ATOM_NET_WM_WINDOW_TYPE_MENU		atoms_netwm[22]
#define EX_ATOM_NET_WM_WINDOW_TYPE_UTILITY	atoms_netwm[23]
#define EX_ATOM_NET_WM_WINDOW_TYPE_SPLASH	atoms_netwm[24]
#define EX_ATOM_NET_WM_WINDOW_TYPE_DIALOG	atoms_netwm[25]
#define EX_ATOM_NET_WM_WINDOW_TYPE_NORMAL	atoms_netwm[26]

#define EX_ATOM_NET_WM_STATE			atoms_netwm[27]
#define EX_ATOM_NET_WM_STATE_MODAL		atoms_netwm[28]
#define EX_ATOM_NET_WM_STATE_STICKY		atoms_netwm[29]
#define EX_ATOM_NET_WM_STATE_MAXIMIZED_VERT	atoms_netwm[30]
#define EX_ATOM_NET_WM_STATE_MAXIMIZED_HORZ	atoms_netwm[31]
#define EX_ATOM_NET_WM_STATE_SHADED		atoms_netwm[32]
#define EX_ATOM_NET_WM_STATE_SKIP_TASKBAR	atoms_netwm[33]
#define EX_ATOM_NET_WM_STATE_SKIP_PAGER		atoms_netwm[34]
#define EX_ATOM_NET_WM_STATE_HIDDEN		atoms_netwm[35]
#define EX_ATOM_NET_WM_STATE_FULLSCREEN		atoms_netwm[36]
#define EX_ATOM_NET_WM_STATE_ABOVE		atoms_netwm[37]
#define EX_ATOM_NET_WM_STATE_BELOW		atoms_netwm[38]
#define EX_ATOM_NET_WM_STATE_DEMANDS_ATTENTION	atoms_netwm[39]

#define EX_ATOM_NET_WM_ALLOWED_ACTIONS		atoms_netwm[40]
#define EX_ATOM_NET_WM_ACTION_MOVE		atoms_netwm[41]
#define EX_ATOM_NET_WM_ACTION_RESIZE		atoms_netwm[42]
#define EX_ATOM_NET_WM_ACTION_MINIMIZE		atoms_netwm[43]
#define EX_ATOM_NET_WM_ACTION_SHADE		atoms_netwm[44]
#define EX_ATOM_NET_WM_ACTION_STICK		atoms_netwm[45]
#define EX_ATOM_NET_WM_ACTION_MAXIMIZE_HORZ	atoms_netwm[46]
#define EX_ATOM_NET_WM_ACTION_MAXIMIZE_VERT	atoms_netwm[47]
#define EX_ATOM_NET_WM_ACTION_FULLSCREEN	atoms_netwm[48]
#define EX_ATOM_NET_WM_ACTION_CHANGE_DESKTOP	atoms_netwm[49]
#define EX_ATOM_NET_WM_ACTION_CLOSE		atoms_netwm[50]
#define EX_ATOM_NET_WM_ACTION_ABOVE		atoms_netwm[51]
#define EX_ATOM_NET_WM_ACTION_BELOW		atoms_netwm[52]

#define EX_ATOM_NET_WM_STRUT			atoms_netwm[53]
#define EX_ATOM_NET_WM_STRUT_PARTIAL		atoms_netwm[54]

#define EX_ATOM_NET_FRAME_EXTENTS		atoms_netwm[55]

#define EX_ATOM_NET_WM_ICON			atoms_netwm[56]

#define EX_ATOM_NET_WM_USER_TIME		atoms_netwm[57]
#define EX_ATOM_NET_WM_USER_TIME_WINDOW		atoms_netwm[58]

#if 0				/* Not used */
#define EX_ATOM_NET_WM_ICON_GEOMETRY		atoms_netwm[0]
#define EX_ATOM_NET_WM_PID			atoms_netwm[0]
#define EX_ATOM_NET_WM_HANDLED_ICONS		atoms_netwm[0]

#define EX_ATOM_NET_WM_PING			atoms_netwm[0]
#endif
#define EX_ATOM_NET_WM_SYNC_REQUEST		atoms_netwm[59]
#define EX_ATOM_NET_WM_SYNC_REQUEST_COUNTER	atoms_netwm[60]

#define EX_ATOM_NET_WM_WINDOW_OPACITY		atoms_netwm[61]

/* Misc window ops */
#define EX_ATOM_NET_CLOSE_WINDOW		atoms_netwm[62]
#define EX_ATOM_NET_MOVERESIZE_WINDOW		atoms_netwm[63]
#define EX_ATOM_NET_WM_MOVERESIZE		atoms_netwm[64]
#define EX_ATOM_NET_RESTACK_WINDOW		atoms_netwm[65]

#if 0				/* Not yet implemented */
#define EX_ATOM_NET_REQUEST_FRAME_EXTENTS	atoms_netwm[0]
#endif

/* Startup notification */
#define EX_ATOM_NET_STARTUP_ID			atoms_netwm[66]
#define EX_ATOM_NET_STARTUP_INFO_BEGIN		atoms_netwm[67]
#define EX_ATOM_NET_STARTUP_INFO		atoms_netwm[68]

#define CHECK_COUNT_NETWM 69

void                ex_netwm_init(void);

void                ex_netwm_wm_identify(EX_Window root,
					 EX_Window check, const char *wm_name);

void                ex_netwm_desk_count_set(EX_Window root,
					    unsigned int n_desks);
void                ex_netwm_desk_roots_set(EX_Window root,
					    const EX_Window * vroots,
					    unsigned int n_desks);
void                ex_netwm_desk_names_set(EX_Window root,
					    const char **names,
					    unsigned int n_desks);
void                ex_netwm_desk_size_set(EX_Window root,
					   unsigned int width,
					   unsigned int height);
void                ex_netwm_desk_workareas_set(EX_Window root,
						const unsigned int *areas,
						unsigned int n_desks);
void                ex_netwm_desk_current_set(EX_Window root,
					      unsigned int desk);
void                ex_netwm_desk_viewports_set(EX_Window root,
						const unsigned int *origins,
						unsigned int n_desks);
void                ex_netwm_showing_desktop_set(EX_Window root, int on);

void                ex_netwm_client_list_set(EX_Window root,
					     const EX_Window * p_clients,
					     unsigned int n_clients);
void                ex_netwm_client_list_stacking_set(EX_Window root,
						      const EX_Window *
						      p_clients,
						      unsigned int n_clients);
void                ex_netwm_client_active_set(EX_Window root, EX_Window win);
void                ex_netwm_name_set(EX_Window win, const char *name);
int                 ex_netwm_name_get(EX_Window win, char **name);
void                ex_netwm_icon_name_set(EX_Window win, const char *name);
int                 ex_netwm_icon_name_get(EX_Window win, char **name);
void                ex_netwm_visible_name_set(EX_Window win, const char *name);
int                 ex_netwm_visible_name_get(EX_Window win, char **name);
void                ex_netwm_visible_icon_name_set(EX_Window win,
						   const char *name);
int                 ex_netwm_visible_icon_name_get(EX_Window win, char **name);

void                ex_netwm_desktop_set(EX_Window win, unsigned int desk);
int                 ex_netwm_desktop_get(EX_Window win, unsigned int *desk);

int                 ex_netwm_user_time_get(EX_Window win, unsigned int *ts);

void                ex_netwm_opacity_set(EX_Window win, unsigned int opacity);
int                 ex_netwm_opacity_get(EX_Window win, unsigned int *opacity);

void                ex_netwm_startup_id_set(EX_Window win, const char *id);
int                 ex_netwm_startup_id_get(EX_Window win, char **id);

void                ex_icccm_state_set_iconic(EX_Window win);
void                ex_icccm_state_set_normal(EX_Window win);
void                ex_icccm_state_set_withdrawn(EX_Window win);

void                ex_window_prop_string_list_set(EX_Window win, EX_Atom atom,
						   char **lst, int num);
int                 ex_window_prop_string_list_get(EX_Window win, EX_Atom atom,
						   char ***plst);

#endif /* _XPROP_H_ */
