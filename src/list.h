/*
 * Copyright (C) 2013 Kim Woelders
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
#ifndef LIST_H
#define LIST_H

#include <stddef.h>

typedef struct _dlist_t dlist_t;

struct _dlist_t {
   struct _dlist_t    *next, *prev;
};

#define dlist_for_each(head, elem) \
    for (elem = (head)->next; elem != (head); elem = elem->next)

#define dlist_for_each_safe(head, elem, temp) \
    for (elem = (head)->next, temp = elem->next; elem != (head); elem = temp, temp = elem->next)

static inline void
dlist_init(dlist_t * head)
{
   head->next = head->prev = head;
}

static inline void
_dlist_insert(dlist_t * elem, dlist_t * prev, dlist_t * next)
{
   next->prev = elem;
   elem->next = next;
   elem->prev = prev;
   prev->next = elem;
}

static inline void
dlist_append(dlist_t * head, dlist_t * elem)
{
   _dlist_insert(elem, head->prev, head);
}

static inline void
dlist_prepend(dlist_t * head, dlist_t * elem)
{
   _dlist_insert(elem, head, head->next);
}

static inline void
_dlist_remove(dlist_t * prev, dlist_t * next)
{
   next->prev = prev;
   prev->next = next;
}

static inline dlist_t *
dlist_remove(dlist_t * elem)
{
   if (elem)
      _dlist_remove(elem->prev, elem->next);

   return elem;
}

static inline int
dlist_is_empty(const dlist_t * head)
{
   return head->next == head;
}

static inline unsigned int
dlist_get_count(const dlist_t * head)
{
   unsigned int        count = 0;
   dlist_t            *e;

   dlist_for_each(head, e)
   {
      count++;
   }

   return count;
}

#define LIST_NOINLINE_dlist_find 0

typedef int         (dlist_match_func_t) (const void *elem, const void *prm);

#if LIST_NOINLINE_dlist_find
dlist_t            *dlist_find(const dlist_t * head,
			       dlist_match_func_t * match, const void *prm);
#else
static inline dlist_t *
dlist_find(const dlist_t * head, dlist_match_func_t * match, const void *prm)
{
   dlist_t            *e;

   dlist_for_each(head, e)
   {
      if (match(e, prm) == 0)
	 return e;
   }

   return NULL;
}
#endif

#define LIST_NOINLINE_dlist_for_each_func 0

typedef void        (dlist_foreach_func_t) (void *elem, void *prm);

#if LIST_NOINLINE_dlist_for_each_func
void                dlist_for_each_func(dlist_t * head,
					dlist_foreach_func_t * func, void *prm);
#else
static inline void
dlist_for_each_func(dlist_t * head, dlist_foreach_func_t * func, void *prm)
{
   dlist_t            *e, *tmp;

   dlist_for_each_safe(head, e, tmp)
   {
      func(e, prm);
   }
}
#endif

static inline dlist_t *
dlist_check(const dlist_t * head, dlist_t * elem)
{
   dlist_t            *e;

   dlist_for_each(head, e)
   {
      if (e == elem)
	 return e;
   }

   return NULL;
}

#define LIST_NOINLINE_dlist_get_index 0

#if LIST_NOINLINE_dlist_get_index
int                 dlist_get_index(const dlist_t * head, dlist_t * elem);
#else
static inline int
dlist_get_index(const dlist_t * head, dlist_t * elem)
{
   const dlist_t      *e;
   int                 i;

   i = 0;
   dlist_for_each(head, e)
   {
      if (e == elem)
	 return i;
      i++;
   }

   return -1;
}
#endif

#define LIST_NOINLINE_dlist_get_by_index 0

#if LIST_NOINLINE_dlist_get_by_index
dlist_t            *dlist_get_by_index(const dlist_t * head, int ix);
#else
static inline dlist_t *
dlist_get_by_index(const dlist_t * head, int ix)
{
   dlist_t            *etmp;
   int                 i;

   i = 0;
   dlist_for_each(head, etmp)
   {
      if (i == ix)
	 return etmp;
      i++;
   }

   return NULL;
}
#endif

#define LIST_NOINLINE_dlist_get_items 1

#if LIST_NOINLINE_dlist_get_items
dlist_t           **dlist_get_items(const dlist_t * head, int *pnum);
#else
static inline dlist_t **
dlist_get_items(const dlist_t * head, int *pnum)
{
   dlist_t            *etmp;
   dlist_t           **lst;
   int                 i, num;

   lst = NULL;
   num = dlist_get_count(head);
   if (num <= 0)
      goto done;
   lst = (dlist_t **) malloc(num * sizeof(void *));

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

#define LIST_HEAD(head) \
    dlist_t head = { &head, &head }

#define LIST_INIT(type, head) \
    dlist_init(head)

#define LIST_APPEND(type, head, elem) \
    dlist_append(head, &((elem)->list))

#define LIST_PREPEND(type, head, elem) \
    dlist_prepend(head, &((elem)->list))

#define LIST_REMOVE(type, head, elem) \
    (type*)dlist_remove(&((elem)->list))

#define LIST_FIND(type, head, func, prm) \
    (type*)dlist_find(head, func, prm)

#define LIST_CHECK(type, head, elem) \
    (type*)dlist_check(head, &((elem)->list))

#define LIST_GET_INDEX(type, head, elem) \
    dlist_get_index(head, &((elem)->list))

#define LIST_GET_BY_INDEX(type, head, ix) \
    (type*)dlist_get_by_index(head, ix)

#define LIST_IS_EMPTY(head) \
    dlist_is_empty(head)

#define LIST_GET_COUNT(head) \
    dlist_get_count(head)

#define LIST_NEXT(type, head, elem) \
    ((elem->list.next != head) ? (type*)(elem)->list.next : NULL)

#define LIST_PREV(type, head, elem) \
    ((elem->list.prev != head) ? (type*)(elem)->list.prev : NULL)

#define LIST_GET_ITEMS(type, head, pnum) \
    (type**)dlist_get_items(head, pnum)

#define LIST_FOR_EACH(type, head, elem) \
    for (elem = (type*)((head)->next); \
         &(elem)->list != (head); \
         elem = (type*)((elem)->list.next))

#define LIST_FOR_EACH_REV(type, head, elem) \
    for (elem = (type*)((head)->prev); \
         &(elem)->list != (head); \
         elem = (type*)((elem)->list.prev))

#define LIST_FOR_EACH_SAFE(type, head, elem, temp) \
    for (elem = (type*)((head)->next), temp = (type*)((elem)->list.next); \
         &(elem)->list != (head); \
         elem = temp, temp = (type*)((elem)->list.next))

#define LIST_FOR_EACH_FUNC(type, head, func, prm) \
    dlist_for_each_func(head, func, prm)

#endif /* LIST_H */
