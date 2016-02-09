/*
 * Copyright (C) 2013-2015 Kim Woelders
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

#include <stdlib.h>

#include "list.h"

#if LIST_NOINLINE_dlist_find
dlist_t            *
dlist_find(const dlist_t * head, dlist_match_func_t * match, const void *prm)
{
   dlist_t            *etmp;

   dlist_for_each(head, etmp)
   {
      if (match(etmp, prm) == 0)
	 return etmp;
   }

   return NULL;
}
#endif

#if LIST_NOINLINE_dlist_for_each_func
void
dlist_for_each_func(dlist_t * head, dlist_foreach_func_t * func, void *prm)
{
   dlist_t            *e, *tmp;

   dlist_for_each_safe(head, e, tmp)
   {
      func(e, prm);
   }
}
#endif

#if LIST_NOINLINE_dlist_get_index
int
dlist_get_index(const dlist_t * head, dlist_t * elem)
{
   const dlist_t      *e;
   int                 i;

   i = 0;
   dlist_for_each(head, e)
   {
      i++;
      if (e == elem)
	 return i;
   }

   return 0;
}
#endif

#if LIST_NOINLINE_dlist_get_by_index
dlist_t            *
dlist_get_by_index(const dlist_t * head, int ix)
{
   dlist_t            *etmp;
   int                 i;

   i = 0;
   dlist_for_each(head, etmp)
   {
      i++;
      if (i == ix)
	 return etmp;
   }

   return NULL;
}
#endif

#if LIST_NOINLINE_dlist_get_items
dlist_t           **
dlist_get_items(const dlist_t * head, int *pnum)
{
   dlist_t            *etmp;
   dlist_t           **lst;
   int                 i, num;

   lst = NULL;
   num = dlist_get_count(head);
   if (num <= 0)
      goto done;
   lst = (dlist_t **) malloc(num * sizeof(dlist_t *));

   i = 0;
   dlist_for_each(head, etmp)
   {
      lst[i++] = etmp;
   }

 done:
   *pnum = num;
   return lst;
}
#endif
