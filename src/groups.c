/*
 * Copyright (C) 2000-2007 Carsten Haitzler, Geoff Harrison and various contributors
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
#include "E.h"
#include "borders.h"
#include "dialog.h"
#include "emodule.h"
#include "ewins.h"
#include "groups.h"
#include "list.h"
#include "settings.h"
#include "snaps.h"

#define DEBUG_GROUPS 0
#if DEBUG_GROUPS
#define Dprintf(fmt, ...) Eprintf("%s: " fmt, __func__, __VA_ARGS__)
#else
#define Dprintf(fmt...)
#endif

#define USE_GROUP_SHOWHIDE 1	/* Enable group borders */

#define SET_OFF    0
#define SET_ON     1
#define SET_TOGGLE 2

static              LIST_HEAD(group_list);

static struct {
   GroupConfig         dflt;
   char                swapmove;
} Conf_groups;

static struct {
   Group              *current;
} Mode_groups;

static void         AddEwinToGroup(EWin * ewin, Group * g);
static void         RemoveEwinFromGroup(EWin * ewin, Group * g);

int
GroupsGetSwapmove(void)
{
   return Conf_groups.swapmove;
}

static Group       *
GroupCreate(int gid)
{
   Group              *g;

   g = ECALLOC(Group, 1);
   if (!g)
      return NULL;

   LIST_APPEND(Group, &group_list, g);

   if (gid == -1)
     {
	/* Create new group id */
	/* ... using us time. Should really be checked for uniqueness. */
	g->index = (int)GetTimeUs();
     }
   else
     {
	/* Use given group id */
	g->index = gid;
     }
   g->cfg.iconify = Conf_groups.dflt.iconify;
   g->cfg.kill = Conf_groups.dflt.kill;
   g->cfg.move = Conf_groups.dflt.move;
   g->cfg.raise = Conf_groups.dflt.raise;
   g->cfg.set_border = Conf_groups.dflt.set_border;
   g->cfg.stick = Conf_groups.dflt.stick;
   g->cfg.shade = Conf_groups.dflt.shade;

   Dprintf("grp=%p gid=%d\n", g, g->index);
   return g;
}

static void
GroupDestroy(Group * g)
{
   if (!g)
      return;

   Dprintf("grp=%p gid=%d\n", g, g->index);
   LIST_REMOVE(Group, &group_list, g);

   if (g == Mode_groups.current)
      Mode_groups.current = NULL;
   Efree(g->members);

   Efree(g);
}

static int
GroupMatchId(const void *data, const void *match)
{
   return ((const Group *)data)->index != PTR2INT(match);
}

static Group       *
GroupFind(int gid)
{
   return LIST_FIND(Group, &group_list, GroupMatchId, INT2PTR(gid));
}

void
GroupRemember(int gid)
{
   Group              *g;

   g = GroupFind(gid);
   if (!g)
      return;

   g->save = 1;
}

static Group       *
GroupFind2(const char *groupid)
{
   int                 gid;

   if (groupid[0] == '*' || groupid[0] == '\0')
      return Mode_groups.current;

   gid = -1;
   sscanf(groupid, "%d", &gid);
   if (gid <= 0)
      return NULL;

   return GroupFind(gid);
}

static void
BreakWindowGroup(EWin * ewin, Group * g)
{
   int                 i, j;
   EWin               *ewin2;
   Group              *g2;

   Dprintf("ewin=%p group=%p gid=%d\n", ewin, g, g->index);
   if (!ewin || !ewin->groups)
      return;

   for (j = 0; j < ewin->num_groups; j++)
     {
	g2 = ewin->groups[j];
	if (g && g != g2)
	   continue;

	for (i = 0; i < g2->num_members; i++)
	  {
	     ewin2 = g2->members[0];
	     RemoveEwinFromGroup(ewin2, g2);
	     SnapshotEwinUpdate(ewin2, SNAP_USE_GROUPS);
	  }
     }
}

static void
BuildWindowGroup(EWin ** ewins, int num, int gid)
{
   int                 i;
   Group              *group;

   Mode_groups.current = group = GroupCreate(gid);

   for (i = 0; i < num; i++)
      AddEwinToGroup(ewins[i], group);
}

Group             **
GroupsGetList(int *pnum)
{
   return LIST_GET_ITEMS(Group, &group_list, pnum);
}

Group              *const *
EwinGetGroups(const EWin * ewin, int *num)
{
   *num = ewin->num_groups;
   return ewin->groups;
}

#if ENABLE_DIALOGS
static Group      **
ListWinGroups(const EWin * ewin, char group_select, int *num)
{
   Group             **groups;
   Group             **groups2;
   int                 i, j, n, killed;

   groups = NULL;
   *num = 0;

   switch (group_select)
     {
     case GROUP_SELECT_EWIN_ONLY:
	*num = n = ewin->num_groups;
	if (n <= 0)
	   break;
	groups = EMALLOC(Group *, n);
	if (!groups)
	   break;
	memcpy(groups, ewin->groups, n * sizeof(Group *));
	break;
     case GROUP_SELECT_ALL_EXCEPT_EWIN:
	groups2 = GroupsGetList(num);
	if (!groups2)
	   break;
	n = *num;
	for (i = killed = 0; i < n; i++)
	  {
	     for (j = 0; j < ewin->num_groups; j++)
	       {
		  if (ewin->groups[j] == groups2[i])
		    {
		       groups2[i] = NULL;
		       killed++;
		    }
	       }
	  }
	if (n - killed > 0)
	  {
	     groups = EMALLOC(Group *, n - killed);
	     if (groups)
	       {
		  for (i = j = 0; i < n; i++)
		     if (groups2[i])
			groups[j++] = groups2[i];
		  *num = n - killed;
	       }
	  }
	Efree(groups2);
	break;
     case GROUP_SELECT_ALL:
     default:
	groups = GroupsGetList(num);
	break;
     }

   return groups;
}
#endif /* ENABLE_DIALOGS */

static void
_GroupAddEwin(Group * g, EWin * ewin)
{
   int                 i;

   for (i = 0; i < ewin->num_groups; i++)
      if (ewin->groups[i] == g)
	 return;

   ewin->num_groups++;
   ewin->groups = EREALLOC(Group *, ewin->groups, ewin->num_groups);
   ewin->groups[ewin->num_groups - 1] = g;
   g->num_members++;
   g->members = EREALLOC(EWin *, g->members, g->num_members);
   g->members[g->num_members - 1] = ewin;
}

static void
AddEwinToGroup(EWin * ewin, Group * g)
{
   if (!ewin || !g)
      return;

   _GroupAddEwin(g, ewin);
   SnapshotEwinUpdate(ewin, SNAP_USE_GROUPS);
}

void
GroupsEwinAdd(EWin * ewin, const int *pgid, int ngid)
{
   Group              *g;
   int                 i, gid;

   for (i = 0; i < ngid; i++)
     {
	gid = pgid[i];
	g = GroupFind(gid);
	Dprintf("ewin=%p gid=%d grp=%p\n", ewin, gid, g);
	if (!g)
	  {
	     /* This should not happen, but may if group/snap configs are corrupted */
	     BuildWindowGroup(&ewin, 1, gid);
	  }
	else
	  {
	     _GroupAddEwin(g, ewin);
	  }
     }
   SnapshotEwinUpdate(ewin, SNAP_USE_GROUPS);
}

static int
EwinInGroup(const EWin * ewin, const Group * g)
{
   int                 i;

   if (ewin && g)
     {
	for (i = 0; i < g->num_members; i++)
	  {
	     if (g->members[i] == ewin)
		return 1;
	  }
     }
   return 0;
}

Group              *
EwinsInGroup(const EWin * ewin1, const EWin * ewin2)
{
   int                 i;

   if (ewin1 && ewin2)
     {
	for (i = 0; i < ewin1->num_groups; i++)
	  {
	     if (EwinInGroup(ewin2, ewin1->groups[i]))
		return ewin1->groups[i];
	  }
     }
   return NULL;
}

static void
RemoveEwinFromGroup(EWin * ewin, Group * g)
{
   int                 i, j, k, i2;

   if (!ewin || !g)
      return;

   for (k = 0; k < ewin->num_groups; k++)
     {
	/* is the window actually part of the given group */
	if (ewin->groups[k] != g)
	   continue;

	for (i = 0; i < g->num_members; i++)
	  {
	     if (g->members[i] != ewin)
		continue;

	     /* remove it from the group */
	     for (j = i; j < g->num_members - 1; j++)
		g->members[j] = g->members[j + 1];
	     g->num_members--;
	     if (g->num_members > 0)
		g->members = EREALLOC(EWin *, g->members, g->num_members);
	     else if (g->save)
	       {
		  Efree(g->members);
		  g->members = NULL;
	       }
	     else
	       {
		  GroupDestroy(g);
	       }

	     /* and remove the group from the groups that the window is in */
	     for (i2 = k; i2 < ewin->num_groups - 1; i2++)
		ewin->groups[i2] = ewin->groups[i2 + 1];
	     ewin->num_groups--;
	     if (ewin->num_groups <= 0)
	       {
		  Efree(ewin->groups);
		  ewin->groups = NULL;
		  ewin->num_groups = 0;
	       }
	     else
		ewin->groups =
		   EREALLOC(Group *, ewin->groups, ewin->num_groups);

	     GroupsSave();
	     return;
	  }
     }
}

void
GroupsEwinRemove(EWin * ewin)
{
   int                 num, i;

   num = ewin->num_groups;
   for (i = 0; i < num; i++)
      RemoveEwinFromGroup(ewin, ewin->groups[0]);
}

#if ENABLE_DIALOGS
static char       **
GetWinGroupMemberNames(Group ** groups, int num)
{
   int                 i, j, len;
   char              **group_member_strings;
   const char         *name;

   group_member_strings = ECALLOC(char *, num);

   if (!group_member_strings)
      return NULL;

   for (i = 0; i < num; i++)
     {
	group_member_strings[i] = EMALLOC(char, 1024);

	if (!group_member_strings[i])
	   break;

	len = 0;
	for (j = 0; j < groups[i]->num_members; j++)
	  {
	     name = EwinGetTitle(groups[i]->members[j]);
	     if (!name)		/* Should never happen */
		continue;
	     len += Esnprintf(group_member_strings[i] + len, 1024 - len,
			      "%s\n", name);
	     if (len >= 1024)
		break;
	  }
     }

   return group_member_strings;
}
#endif /* ENABLE_DIALOGS */

#if USE_GROUP_SHOWHIDE
static void
ShowHideWinGroups(EWin * ewin, int group_index, char onoff)
{
   EWin              **gwins;
   int                 i, num;
   const Border       *b = NULL;

   if (!ewin || group_index >= ewin->num_groups)
      return;

   if (group_index < 0)
     {
	gwins = ListWinGroupMembersForEwin(ewin, GROUP_ACTION_ANY, 0, &num);
     }
   else
     {
	gwins = ewin->groups[group_index]->members;
	num = ewin->groups[group_index]->num_members;
     }

   if (onoff == SET_TOGGLE)
      onoff = (ewin->border == ewin->normal_border) ? SET_ON : SET_OFF;

   for (i = 0; i < num; i++)
     {
	if (onoff == SET_ON)
	   b = BorderFind(gwins[i]->border->group_border_name);
	else
	   b = gwins[i]->normal_border;

	EwinBorderChange(gwins[i], b, 0);
     }
   if (group_index < 0)
      Efree(gwins);
}
#else

#define ShowHideWinGroups(ewin, group_index, onoff)

#endif /* USE_GROUP_SHOWHIDE */

void
GroupsSave(void)
{
   Group              *g;
   FILE               *f;
   char                s[1024];

   if (LIST_IS_EMPTY(&group_list))
      return;

   Esnprintf(s, sizeof(s), "%s.groups", EGetSavePrefix());
   f = fopen(s, "w");
   if (!f)
      return;

   LIST_FOR_EACH(Group, &group_list, g)
   {
      if (!g->save)
	 continue;

      fprintf(f, "NEW: %i\n", g->index);
      fprintf(f, "ICONIFY: %i\n", g->cfg.iconify);
      fprintf(f, "KILL: %i\n", g->cfg.kill);
      fprintf(f, "MOVE: %i\n", g->cfg.move);
      fprintf(f, "RAISE: %i\n", g->cfg.raise);
      fprintf(f, "SET_BORDER: %i\n", g->cfg.set_border);
      fprintf(f, "STICK: %i\n", g->cfg.stick);
      fprintf(f, "SHADE: %i\n", g->cfg.shade);
   }

   fclose(f);
}

static int
_GroupsLoad(FILE * fs)
{
   char                s[1024];
   Group              *g = NULL;

   while (fgets(s, sizeof(s), fs))
     {
	char                ss[128];
	int                 ii;

	if (strlen(s) > 0)
	   s[strlen(s) - 1] = 0;
	ii = 0;
	sscanf(s, "%100s %d", ss, &ii);

	if (!strcmp(ss, "NEW:"))
	  {
	     g = GroupCreate(ii);
	     continue;
	  }
	if (!g)
	   continue;

	if (!strcmp(ss, "ICONIFY:"))
	  {
	     g->cfg.iconify = ii;
	  }
	else if (!strcmp(ss, "KILL:"))
	  {
	     g->cfg.kill = ii;
	  }
	else if (!strcmp(ss, "MOVE:"))
	  {
	     g->cfg.move = ii;
	  }
	else if (!strcmp(ss, "RAISE:"))
	  {
	     g->cfg.raise = ii;
	  }
	else if (!strcmp(ss, "SET_BORDER:"))
	  {
	     g->cfg.set_border = ii;
	  }
	else if (!strcmp(ss, "STICK:"))
	  {
	     g->cfg.stick = ii;
	  }
	else if (!strcmp(ss, "SHADE:"))
	  {
	     g->cfg.shade = ii;
	  }
     }

   return 0;
}

void
GroupsLoad(void)
{
   char                s[4096];

   Esnprintf(s, sizeof(s), "%s.groups", EGetSavePrefix());

   ConfigFileLoad(s, NULL, _GroupsLoad, 0);
}

#if ENABLE_DIALOGS

#define GROUP_OP_ADD	1
#define GROUP_OP_DEL	2
#define GROUP_OP_BREAK	3

typedef struct {
   EWin               *ewin;
   int                 action;
   const char         *message;
   Group             **groups;
   int                 group_num;
   int                 cur_grp;	/* Current  group */
   int                 prv_grp;	/* Previous group */
} GroupSelDlgData;

static void
_DlgApplyGroupChoose(Dialog * d, int val __UNUSED__, void *data __UNUSED__)
{
   GroupSelDlgData    *dd = DLG_DATA_GET(d, GroupSelDlgData);

   if (!dd->groups)
      return;

   switch (dd->action)
     {
     case GROUP_OP_ADD:
	AddEwinToGroup(dd->ewin, dd->groups[dd->cur_grp]);
	break;
     case GROUP_OP_DEL:
	RemoveEwinFromGroup(dd->ewin, dd->groups[dd->cur_grp]);
	break;
     case GROUP_OP_BREAK:
	BreakWindowGroup(dd->ewin, dd->groups[dd->cur_grp]);
	break;
     default:
	break;
     }

   GroupsSave();
}

static void
_DlgExitGroupChoose(Dialog * d)
{
   GroupSelDlgData    *dd = DLG_DATA_GET(d, GroupSelDlgData);

   if (!dd->groups)
      return;
   ShowHideWinGroups(dd->ewin, dd->cur_grp, SET_OFF);
   Efree(dd->groups);
}

static void
GroupCallback(Dialog * d, int val, void *data __UNUSED__)
{
   GroupSelDlgData    *dd = DLG_DATA_GET(d, GroupSelDlgData);

   /* val is equal to dd->cur_grp */
   ShowHideWinGroups(dd->ewin, dd->prv_grp, SET_OFF);
   ShowHideWinGroups(dd->ewin, val, SET_ON);
   dd->prv_grp = val;
}

static void
_DlgFillGroupChoose(Dialog * d, DItem * table, void *data)
{
   GroupSelDlgData    *dd = DLG_DATA_GET(d, GroupSelDlgData);
   DItem              *di, *radio;
   int                 i, num_groups;
   char              **group_member_strings;

   *dd = *(GroupSelDlgData *) data;

   DialogItemTableSetOptions(table, 2, 0, 0, 0);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetColSpan(di, 2);
   DialogItemSetAlign(di, 0, 512);
   DialogItemSetText(di, dd->message);

   num_groups = dd->group_num;
   group_member_strings = GetWinGroupMemberNames(dd->groups, num_groups);
   if (!group_member_strings)
      return;			/* Silence clang - It should not be possible to go here */

   radio = NULL;		/* Avoid warning */
   for (i = 0; i < num_groups; i++)
     {
	di = DialogAddItem(table, DITEM_RADIOBUTTON);
	if (i == 0)
	   radio = di;
	DialogItemSetColSpan(di, 2);
	DialogItemSetCallback(di, GroupCallback, i, NULL);
	DialogItemSetText(di, group_member_strings[i]);
	DialogItemRadioButtonSetFirst(di, radio);
	DialogItemRadioButtonGroupSetVal(di, i);
     }
   DialogItemRadioButtonGroupSetValPtr(radio, &dd->cur_grp);

   StrlistFree(group_member_strings, num_groups);
}

static const DialogDef DlgGroupChoose = {
   "GROUP_SELECTION",
   NULL, N_("Window Group Selection"),
   sizeof(GroupSelDlgData),
   SOUND_SETTINGS_GROUP,
   "pix/group.png",
   N_("Enlightenment Window Group\n" "Selection Dialog"),
   _DlgFillGroupChoose,
   DLG_OC, _DlgApplyGroupChoose, _DlgExitGroupChoose
};

static void
ChooseGroupDialog(int action)
{
   int                 group_sel;
   GroupSelDlgData     gsdd, *dd = &gsdd;

   dd->ewin = GetContextEwin();
   if (!dd->ewin)
      return;

   dd->action = action;
   dd->cur_grp = dd->prv_grp = 0;

   switch (action)
     {
     default:
	return;
     case GROUP_OP_ADD:
	dd->message = _("Pick the group the window will belong to:");
	group_sel = GROUP_SELECT_ALL_EXCEPT_EWIN;
	break;
     case GROUP_OP_DEL:
	dd->message = _("Select the group to remove the window from:");
	group_sel = GROUP_SELECT_EWIN_ONLY;
	break;
     case GROUP_OP_BREAK:
	dd->message = _("Select the group to break:");
	group_sel = GROUP_SELECT_EWIN_ONLY;
	break;
     }

   dd->groups = ListWinGroups(dd->ewin, group_sel, &dd->group_num);

   if (!dd->groups)
     {
	if (action == GROUP_OP_BREAK || action == GROUP_OP_DEL)
	  {
	     DialogOK(_("Window Group Error"),
		      _
		      ("This window currently does not belong to any groups.\n"
		       "You can only destroy groups or remove windows from groups\n"
		       "through a window that actually belongs to at least one group."));
	     return;
	  }

	if (group_sel == GROUP_SELECT_ALL_EXCEPT_EWIN)
	  {
	     DialogOK(_("Window Group Error"),
		      _("Currently, no groups exist or this window\n"
			"already belongs to all existing groups.\n"
			"You have to start other groups first."));
	     return;
	  }

	DialogOK(_("Window Group Error"),
		 _
		 ("Currently, no groups exist. You have to start a group first."));
	return;
     }

   ShowHideWinGroups(dd->ewin, 0, SET_ON);

   DialogShowSimple(&DlgGroupChoose, dd);
}

typedef struct {
   EWin               *ewin;
   GroupConfig         cfg;	/* Dialog data for current group */
   GroupConfig        *cfgs;	/* Work copy of ewin group cfgs */
   int                 ngrp;
   int                 cur_grp;	/* Current  group */
   int                 prv_grp;	/* Previous group */
} EwinGroupDlgData;

static void
_DlgApplyGroups(Dialog * d, int val __UNUSED__, void *data __UNUSED__)
{
   EwinGroupDlgData   *dd = DLG_DATA_GET(d, EwinGroupDlgData);
   EWin               *ewin;
   int                 i;

   /* Check ewin */
   ewin = EwinFindByPtr(dd->ewin);
   if (ewin && ewin->num_groups != dd->ngrp)
      ewin = NULL;

   if (ewin)
     {
	dd->cfgs[dd->cur_grp] = dd->cfg;
	for (i = 0; i < ewin->num_groups; i++)
	   ewin->groups[i]->cfg = dd->cfgs[i];
     }

   autosave();
}

static void
_DlgExitGroups(Dialog * d)
{
   EwinGroupDlgData   *dd = DLG_DATA_GET(d, EwinGroupDlgData);
   EWin               *ewin;

   ewin = EwinFindByPtr(dd->ewin);
   ShowHideWinGroups(ewin, dd->cur_grp, SET_OFF);

   Efree(dd->cfgs);
}

static void
GroupSelectCallback(Dialog * d, int val, void *data __UNUSED__)
{
   EwinGroupDlgData   *dd = DLG_DATA_GET(d, EwinGroupDlgData);

   /* val is equal to dd->cur_grp */
   dd->cfgs[dd->prv_grp] = dd->cfg;
   dd->cfg = dd->cfgs[val];
   DialogRedraw(d);
   ShowHideWinGroups(dd->ewin, dd->prv_grp, SET_OFF);
   ShowHideWinGroups(dd->ewin, val, SET_ON);
   dd->prv_grp = val;
}

static void
_DlgFillGroups(Dialog * d, DItem * table, void *data)
{
   EwinGroupDlgData   *dd = DLG_DATA_GET(d, EwinGroupDlgData);
   EWin               *ewin = (EWin *) data;
   DItem              *radio, *di;
   int                 i;
   char              **group_member_strings;

   dd->ewin = ewin;
   dd->cfgs = EMALLOC(GroupConfig, ewin->num_groups);
   dd->ngrp = ewin->num_groups;
   dd->cur_grp = dd->prv_grp = 0;
   for (i = 0; i < ewin->num_groups; i++)
      dd->cfgs[i] = ewin->groups[i]->cfg;
   dd->cfg = dd->cfgs[0];

   ShowHideWinGroups(ewin, 0, SET_ON);

   DialogItemTableSetOptions(table, 2, 0, 0, 0);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetColSpan(di, 2);
   DialogItemSetAlign(di, 0, 512);
   DialogItemSetText(di, _("Pick the group to configure:"));

   group_member_strings =
      GetWinGroupMemberNames(ewin->groups, ewin->num_groups);
   if (!group_member_strings)
      return;			/* Silence clang - It should not be possible to go here */

   radio = NULL;		/* Avoid warning */
   for (i = 0; i < ewin->num_groups; i++)
     {
	di = DialogAddItem(table, DITEM_RADIOBUTTON);
	if (i == 0)
	   radio = di;
	DialogItemSetColSpan(di, 2);
	DialogItemSetCallback(di, GroupSelectCallback, i, d);
	DialogItemSetText(di, group_member_strings[i]);
	DialogItemRadioButtonSetFirst(di, radio);
	DialogItemRadioButtonGroupSetVal(di, i);
     }
   DialogItemRadioButtonGroupSetValPtr(radio, &dd->cur_grp);

   StrlistFree(group_member_strings, ewin->num_groups);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 2);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetColSpan(di, 2);
   DialogItemSetAlign(di, 0, 512);
   DialogItemSetText(di, _("The following actions are\n"
			   "applied to all group members:"));

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Changing Border Style"));
   DialogItemCheckButtonSetPtr(di, &(dd->cfg.set_border));

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Iconifying"));
   DialogItemCheckButtonSetPtr(di, &(dd->cfg.iconify));

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Killing"));
   DialogItemCheckButtonSetPtr(di, &(dd->cfg.kill));

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Moving"));
   DialogItemCheckButtonSetPtr(di, &(dd->cfg.move));

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Raising/Lowering"));
   DialogItemCheckButtonSetPtr(di, &(dd->cfg.raise));

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Sticking"));
   DialogItemCheckButtonSetPtr(di, &(dd->cfg.stick));

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Shading"));
   DialogItemCheckButtonSetPtr(di, &(dd->cfg.shade));
}

static const DialogDef DlgGroups = {
   "CONFIGURE_GROUP",
   NULL, N_("Window Group Settings"),
   sizeof(EwinGroupDlgData),
   SOUND_SETTINGS_GROUP,
   "pix/group.png",
   N_("Enlightenment Window Group\n" "Settings Dialog"),
   _DlgFillGroups,
   DLG_OAC, _DlgApplyGroups, _DlgExitGroups
};

static void
SettingsGroups(void)
{
   EWin               *ewin;

   ewin = GetContextEwin();
   if (!ewin)
      return;

   if (ewin->num_groups == 0)
     {
	DialogOK(_("Window Group Error"),
		 _("This window currently does not belong to any groups."));
	return;
     }

   DialogShowSimple(&DlgGroups, ewin);
}

typedef struct {
   GroupConfig         group_cfg;
   char                group_swap;
} GroupCfgDlgData;

static void
_DlgApplyGroupDefaults(Dialog * d, int val __UNUSED__, void *data __UNUSED__)
{
   GroupCfgDlgData    *dd = DLG_DATA_GET(d, GroupCfgDlgData);

   Conf_groups.dflt = dd->group_cfg;
   Conf_groups.swapmove = dd->group_swap;

   autosave();
}

static void
_DlgFillGroupDefaults(Dialog * d, DItem * table, void *data __UNUSED__)
{
   GroupCfgDlgData    *dd = DLG_DATA_GET(d, GroupCfgDlgData);
   DItem              *di;

   dd->group_cfg = Conf_groups.dflt;
   dd->group_swap = Conf_groups.swapmove;

   DialogItemTableSetOptions(table, 2, 0, 0, 0);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetColSpan(di, 2);
   DialogItemSetAlign(di, 0, 512);
   DialogItemSetText(di, _("Per-group settings:"));

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 2);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Changing Border Style"));
   DialogItemCheckButtonSetPtr(di, &dd->group_cfg.set_border);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Iconifying"));
   DialogItemCheckButtonSetPtr(di, &dd->group_cfg.iconify);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Killing"));
   DialogItemCheckButtonSetPtr(di, &dd->group_cfg.kill);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Moving"));
   DialogItemCheckButtonSetPtr(di, &dd->group_cfg.move);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Raising/Lowering"));
   DialogItemCheckButtonSetPtr(di, &dd->group_cfg.raise);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Sticking"));
   DialogItemCheckButtonSetPtr(di, &dd->group_cfg.stick);

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Shading"));
   DialogItemCheckButtonSetPtr(di, &dd->group_cfg.shade);

   di = DialogAddItem(table, DITEM_SEPARATOR);
   DialogItemSetColSpan(di, 2);

   di = DialogAddItem(table, DITEM_TEXT);
   DialogItemSetColSpan(di, 2);
   DialogItemSetAlign(di, 0, 512);
   DialogItemSetText(di, _("Global settings:"));

   di = DialogAddItem(table, DITEM_CHECKBUTTON);
   DialogItemSetColSpan(di, 2);
   DialogItemSetText(di, _("Swap Window Locations"));
   DialogItemCheckButtonSetPtr(di, &dd->group_swap);
}

const DialogDef     DlgGroupDefaults = {
   "CONFIGURE_DEFAULT_GROUP_CONTROL",
   N_("Groups"), N_("Default Group Control Settings"),
   sizeof(GroupCfgDlgData),
   SOUND_SETTINGS_GROUP,
   "pix/group.png",
   N_("Enlightenment Default\n" "Group Control Settings Dialog"),
   _DlgFillGroupDefaults,
   DLG_OAC, _DlgApplyGroupDefaults, NULL
};

static void
GroupsConfigure(const char *params)
{
   char                s[128];
   const char         *p;
   int                 l;

   p = params;
   l = 0;
   s[0] = '\0';
   sscanf(p, "%100s %n", s, &l);

   if (!strcmp(s, "group"))
     {
	SettingsGroups();
     }
   else if (!strcmp(s, "add"))
     {
	ChooseGroupDialog(GROUP_OP_ADD);
     }
   else if (!strcmp(s, "del"))
     {
	ChooseGroupDialog(GROUP_OP_DEL);
     }
   else if (!strcmp(s, "break"))
     {
	ChooseGroupDialog(GROUP_OP_BREAK);
     }
}
#endif /* ENABLE_DIALOGS */

/*
 * Groups module
 */

static void
GroupShow(Group * g)
{
   int                 j;

   for (j = 0; j < g->num_members; j++)
      IpcPrintf("%d: %s\n", g->index, EwinGetIcccmName(g->members[j]));

   IpcPrintf("        index: %d\n" "  num_members: %d\n"
	     "      iconify: %d\n" "         kill: %d\n"
	     "         move: %d\n" "        raise: %d\n"
	     "   set_border: %d\n" "        stick: %d\n"
	     "        shade: %d\n",
	     g->index, g->num_members,
	     g->cfg.iconify, g->cfg.kill,
	     g->cfg.move, g->cfg.raise,
	     g->cfg.set_border, g->cfg.stick, g->cfg.shade);
}

static void
IPC_GroupInfo(const char *params)
{
   Group              *group;

   if (params)
     {
	group = GroupFind2(params);
	if (group)
	   GroupShow(group);
	else
	   IpcPrintf("Error: no such group: %s\n", params);
     }
   else
     {
	IpcPrintf("Number of groups: %d\n", LIST_GET_COUNT(&group_list));
	LIST_FOR_EACH(Group, &group_list, group) GroupShow(group);
     }
}

static void
IPC_GroupOps(const char *params)
{
   Group              *group;
   char                windowid[128];
   char                operation[128];
   char                groupid[128];
   unsigned int        win;
   EWin               *ewin;

   if (!params)
     {
	IpcPrintf("Error: no window specified\n");
	return;
     }

   windowid[0] = operation[0] = groupid[0] = '\0';
   sscanf(params, "%100s %100s %100s", windowid, operation, groupid);
   win = 0;
   sscanf(windowid, "%x", &win);

   if (!operation[0])
     {
	IpcPrintf("Error: no operation specified\n");
	return;
     }

   ewin = EwinFindByExpr(windowid);
   if (!ewin)
     {
	IpcPrintf("Error: no such window: %s\n", windowid);
	return;
     }

   if (!strcmp(operation, "start"))
     {
	BuildWindowGroup(&ewin, 1, -1);
	IpcPrintf("start %8x\n", win);
     }
   else if (!strcmp(operation, "add"))
     {
	group = GroupFind2(groupid);
	AddEwinToGroup(ewin, group);
	IpcPrintf("add %8x\n", win);
     }
   else if (!strcmp(operation, "del"))
     {
	group = GroupFind2(groupid);
	RemoveEwinFromGroup(ewin, group);
	IpcPrintf("del %8x\n", win);
     }
   else if (!strcmp(operation, "break"))
     {
	group = GroupFind2(groupid);
	BreakWindowGroup(ewin, group);
	IpcPrintf("break %8x\n", win);
     }
   else if (!strcmp(operation, "showhide"))
     {
	ShowHideWinGroups(ewin, -1, SET_TOGGLE);
	IpcPrintf("showhide %8x\n", win);
     }
   else
     {
	IpcPrintf("Error: no such operation: %s\n", operation);
	return;
     }
   GroupsSave();
}

static void
IPC_Group(const char *params)
{
   char                groupid[128];
   char                operation[128];
   char                param1[128];
   Group              *group;
   int                 onoff;

   if (!params)
     {
	IpcPrintf("Error: no group specified\n");
	return;
     }

   groupid[0] = operation[0] = param1[0] = '\0';
   sscanf(params, "%100s %100s %100s", groupid, operation, param1);

   if (!operation[0])
     {
	IpcPrintf("Error: no operation specified\n");
	return;
     }

   group = GroupFind2(groupid);
   if (!group)
     {
	IpcPrintf("Error: no such group: %s\n", groupid);
	return;
     }

   if (!param1[0])
     {
	IpcPrintf("Error: no mode specified\n");
	return;
     }

   onoff = -1;
   if (!strcmp(param1, "on"))
      onoff = 1;
   else if (!strcmp(param1, "off"))
      onoff = 0;

   if (onoff == -1 && strcmp(param1, "?"))
     {
	IpcPrintf("Error: unknown mode specified\n");
     }
   else if (!strcmp(operation, "num_members"))
     {
	IpcPrintf("num_members: %d\n", group->num_members);
	onoff = -1;
     }
   else if (!strcmp(operation, "iconify"))
     {
	if (onoff >= 0)
	   group->cfg.iconify = onoff;
	else
	   onoff = group->cfg.iconify;
     }
   else if (!strcmp(operation, "kill"))
     {
	if (onoff >= 0)
	   group->cfg.kill = onoff;
	else
	   onoff = group->cfg.kill;
     }
   else if (!strcmp(operation, "move"))
     {
	if (onoff >= 0)
	   group->cfg.move = onoff;
	else
	   onoff = group->cfg.move;
     }
   else if (!strcmp(operation, "raise"))
     {
	if (onoff >= 0)
	   group->cfg.raise = onoff;
	else
	   onoff = group->cfg.raise;
     }
   else if (!strcmp(operation, "set_border"))
     {
	if (onoff >= 0)
	   group->cfg.set_border = onoff;
	else
	   onoff = group->cfg.set_border;
     }
   else if (!strcmp(operation, "stick"))
     {
	if (onoff >= 0)
	   group->cfg.stick = onoff;
	else
	   onoff = group->cfg.stick;
     }
   else if (!strcmp(operation, "shade"))
     {
	if (onoff >= 0)
	   group->cfg.shade = onoff;
	else
	   onoff = group->cfg.shade;
     }
   else
     {
	IpcPrintf("Error: no such operation: %s\n", operation);
	onoff = -1;
     }

   if (onoff == 1)
      IpcPrintf("%s: on\n", operation);
   else if (onoff == 0)
      IpcPrintf("%s: off\n", operation);
}

#if ENABLE_DIALOGS
static void
GroupsIpc(const char *params)
{
   const char         *p;
   char                cmd[128], prm[128];
   int                 len;

   cmd[0] = prm[0] = '\0';
   p = params;
   if (p)
     {
	len = 0;
	sscanf(p, "%100s %100s %n", cmd, prm, &len);
	p += len;
     }

   if (!p || cmd[0] == '?')
     {
	/* Show groups */
     }
   else if (!strncmp(cmd, "cfg", 2))
     {
	GroupsConfigure(prm);
     }
}
#endif /* ENABLE_DIALOGS */

static const IpcItem GroupsIpcArray[] = {
#if ENABLE_DIALOGS
   {
    GroupsIpc,
    "groups", "grp",
    "Configure window groups",
    "  groups cfg           Configure groups\n"}
   ,
#endif /* ENABLE_DIALOGS */
   {
    IPC_GroupInfo,
    "group_info", "gl",
    "Retrieve some info on groups",
    "use \"group_info [group_index]\"\n"}
   ,
   {
    IPC_GroupOps,
    "group_op", "gop",
    "Group operations",
    "use \"group_op <windowid> <property> [<value>]\" to perform "
    "group operations on a window.\n" "Available group_op commands are:\n"
    "  group_op <windowid> start\n"
    "  group_op <windowid> add [<group_index>]\n"
    "  group_op <windowid> del [<group_index>]\n"
    "  group_op <windowid> break [<group_index>]\n"
    "  group_op <windowid> showhide\n"}
   ,
   {
    IPC_Group,
    "group", "gc",
    "Group commands",
    "use \"group <groupid> <property> <value>\" to set group properties.\n"
    "Available group commands are:\n"
    "  group <groupid> num_members <on/off/?>\n"
    "  group <groupid> iconify <on/off/?>\n"
    "  group <groupid> kill <on/off/?>\n" "  group <groupid> move <on/off/?>\n"
    "  group <groupid> raise <on/off/?>\n"
    "  group <groupid> set_border <on/off/?>\n"
    "  group <groupid> stick <on/off/?>\n"
    "  group <groupid> shade <on/off/?>\n"}
   ,
};
#define N_IPC_FUNCS (sizeof(GroupsIpcArray)/sizeof(IpcItem))

/*
 * Configuration items
 */
static const CfgItem GroupsCfgItems[] = {
   CFG_ITEM_BOOL(Conf_groups, dflt.iconify, 1),
   CFG_ITEM_BOOL(Conf_groups, dflt.kill, 0),
   CFG_ITEM_BOOL(Conf_groups, dflt.move, 1),
   CFG_ITEM_BOOL(Conf_groups, dflt.raise, 0),
   CFG_ITEM_BOOL(Conf_groups, dflt.set_border, 1),
   CFG_ITEM_BOOL(Conf_groups, dflt.stick, 1),
   CFG_ITEM_BOOL(Conf_groups, dflt.shade, 1),
   CFG_ITEM_BOOL(Conf_groups, swapmove, 1),
};
#define N_CFG_ITEMS (sizeof(GroupsCfgItems)/sizeof(CfgItem))

extern const EModule ModGroups;

const EModule       ModGroups = {
   "groups", "grp",
   NULL,
   {N_IPC_FUNCS, GroupsIpcArray},
   {N_CFG_ITEMS, GroupsCfgItems}
};
