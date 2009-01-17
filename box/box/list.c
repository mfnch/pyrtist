/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/* $Id$ */

#include <string.h>

#include "types.h"
#include "messages.h"
#include "mem.h"
#include "str.h"
#include "list.h"

void List_New(List **l, UInt item_size) {
  List *nl = BoxMem_Alloc(sizeof(List));
  nl->item_size = item_size;
  nl->length = 0;
  nl->destructor = (ListDestructor) NULL;
  nl->head_tail.next = (ListItemHead *) NULL;
  nl->head_tail.previous = (ListItemHead *) NULL;
  *l = nl;
}

void List_Destroy(List *l) {
  ListItemHead *lih, *lih_next;
  for(lih=l->head_tail.next; lih != (ListItemHead *) NULL;) {
    if (l->destructor != (ListDestructor) NULL) {
      void *item = (void *) lih + sizeof(ListItemHead);
      l->destructor(item);
    }
    lih_next = lih->next;
    BoxMem_Free(lih);
    lih = lih_next;
  }
  BoxMem_Free(l);
}

UInt List_Length(List *l) {
  return l->length;
}

void List_Remove(List *l, void *item) {
  ListItemHead *lih = item - sizeof(ListItemHead);
  ListItemHead **prev_lih, **next_lih;

  prev_lih = (lih->previous == (ListItemHead *) NULL) ?
             & l->head_tail.next : & lih->previous->next;
  next_lih = (lih->next == (ListItemHead *) NULL) ?
             & l->head_tail.previous : & lih->next->previous;

  *prev_lih = lih->next;
  *next_lih = lih->previous;
  if (l->destructor != (ListDestructor) NULL) l->destructor(item);
  BoxMem_Free(lih);
}

void List_Insert_With_Size(List *l, void *item_where,
                           const void *item_what, UInt size) {
  ListItemHead **prev_lih, **next_lih;

  void *new_item = BoxMem_Alloc(sizeof(ListItemHead) + size);
  ListItemHead *new_lih = (ListItemHead *) new_item;
  new_item += sizeof(ListItemHead);
  (void) memcpy(new_item, item_what, size);

  if (item_where == NULL) {
    new_lih->previous = l->head_tail.previous;
    new_lih->next = (ListItemHead *) NULL;
    prev_lih = (l->head_tail.previous == (ListItemHead *) NULL) ?
               & l->head_tail.next : & l->head_tail.previous->next;
    next_lih = & l->head_tail.previous;

  } else {
    ListItemHead *lih = item_where - sizeof(ListItemHead);
    new_lih->next = lih;
    new_lih->previous = lih->previous;
    prev_lih = (lih->previous == (ListItemHead *) NULL) ?
               & l->head_tail.next : & lih->previous->next;
    next_lih = & lih->previous;
  }

  *prev_lih = *next_lih = new_lih;
  ++l->length;
}

Task List_Iter(List *l, ListIterator i, void *pass_data) {
  ListItemHead *lih;
  for(lih=l->head_tail.next; lih != (ListItemHead *) NULL; lih=lih->next) {
    void *item = (void *) lih + sizeof(ListItemHead);
    if IS_FAILED( i(item, pass_data) ) return Failed;
  }
  return Success;
}

Task List_Item_Get(List *l, void **item, UInt index) {
  if (index >= 1 && index <= l->length) {
    ListItemHead *lih;
    for(lih=l->head_tail.next; lih != (ListItemHead *) NULL; lih=lih->next) {
      if (--index == 0) {
        *item = (void *) lih + sizeof(ListItemHead);
        return Success;
      }
    }
    MSG_ERROR("List seems to have more elements than what I thought!");
    return Failed;
  } else {
    MSG_ERROR("Trying to get item with index %U of a list with %U elements",
     index, l->length);
    return Failed;
  }
}

void List_Append_Strings(List *l, const char *strings, char separator) {
  const char *s = strings, *string = s;
  UInt length = 0;
  while(1) {
    register char c = *s;
    if (c == '\0') {
      if (length > 0) List_Append_With_Size(l, string, length+1);
      return;
    } else if (c == separator) {
      if (length > 0) {
        char *s_copy = Str_Dup(string, length);
        List_Append_With_Size(l, s_copy, length+1);
        BoxMem_Free(s_copy);
      }
      string = ++s;
      length = 0;
    } else {
      ++s;
      ++length;
    }
  }
}

typedef struct {
  ListProduct product;
  void *pass;
  List *sublist;
  Int num_sublists;
  ListItemHead *item;
  Int sublist_idx;
  void **tuple;
} ListProductData;

static Task Product_Iter(ListProductData *state);
static Task Product_Iter(ListProductData *state);

/* Used internally (by Product_Iter) */
static Task Product_Sublist_Iter(void *item, void *pass) {
  ListProductData *my_state = (ListProductData *) pass;
  my_state->tuple[my_state->sublist_idx-1] = item;
  return Product_Iter(my_state);
}

/* Used internally (by List_Product_Iter) */
static Task Product_Iter(ListProductData *state) {
  if (state->sublist_idx >= state->num_sublists)
    /* The tuple has been fully filled: we can proceed
     * and invoke the function 'product'
     */
    return state->product(state->tuple, state->pass);
  else {
    ListProductData my_state = *state;
    List *this_sublist =
            *((List **) ((void *) state->item + sizeof(ListItemHead)));
    my_state.item = my_state.item->next;
    ++my_state.sublist_idx;
    return List_Iter(this_sublist, Product_Sublist_Iter, & my_state);
  }
}

/* We explain this function with an example: assume you have two lists:
 *   list1 = [1, 2, 3], list2 = [a, b, c]
 * You want to iterate over all the couples made by one element of list1
 * and one of list2:
 *   list1 x list2 = [(1, a), (1, b), (1, c), (2, a), (2, b), ...]
 * Then you use List_Product_Iter, where 'l' is a list containing
 * the two lists 'list1' and 'list2'. 'product' is a function which
 * receives an array of pointers to the items of the tuple, as first
 * argument, and a pointer provided by the user, as second argument.
 * This function will be invoked for each tuple of the product list.
 * pass is a pointer used just to pass to the 'product' function
 * extra data. pass is passed unchanged to the 'product' function.
 * NOTE: the function iterates over the product of an aribtrary
 *  number of two (despite the examples, where we use only two lists).
 */
Task List_Product_Iter(List *l, ListProduct product, void *pass) {
  UInt n = List_Length(l);
  if (n > 0) {
    Task status;
    ListProductData state;
    state.product = product;
    state.pass = pass;
    state.sublist = l;
    state.num_sublists = List_Length(l);
    state.item = l->head_tail.next;
    state.sublist_idx = 0;
    state.tuple = (void **) BoxMem_Alloc(n*sizeof(void *));
    status = Product_Iter(& state);
    BoxMem_Free(state.tuple);
    return status;

  } else
    return Success;
}

#if 0

Task Print_List_Items(void *item, void *pass) {
  printf("Item: '%s'\n", (char *) item);
  return Success;
}

Task Test_Product_Iter(void **tuple, void *pass) {
  char *format_string = (char *) pass;
  char *string1 = (char *) tuple[0],
       *string2 = (char *) tuple[1],
       *string3 = (char *) tuple[2];
  printf(format_string, string1, string2, string3);
  return Success;
}

int main(void) {
  int i;
  List *l;
  List_New(& l, 5);
  List_Append(l, "ciao");
  List_Append(l, "word");
  List_Append(l, "caro");
  List_Append(l, "judo");
  List_Append(l, "karo");
  List_Append(l, "razo");
  (void) List_Iter(l, Print_List_Items, NULL);
  printf("Removing elements!\n");
  for(i=0; i<20; i++) {
    int index = 3;
    void *item;
    if IS_FAILED( List_Item_Get(l, & item, index) ) break;
    printf("Removing item '%s' at position %d\n", (char *) item, index);
    List_Remove(l, item);
    printf("The followings items are still in the list:\n");
    (void) List_Iter(l, Print_List_Items, NULL);
  }
  List_Destroy(l);

  if (1) {
    List *l, *greetings, *people, *terminators;
    printf("*** Testing List_Product_Iter ***\n");
    List_New(& l, sizeof(List *));
    List_New(& greetings, 0);
    List_New(& people, 0);
    List_New(& terminators, 0);

    List_Append_String(greetings, "Hello");
    List_Append_String(greetings, "Bye");
    List_Append_String(greetings, "See you");

    List_Append_String(people, "Matteo");
    List_Append_String(people, "Terry");

    List_Append_String(terminators, ":-)");
    List_Append_String(terminators, ":-(");
    List_Append_String(terminators, "!");

    List_Append(l, & greetings);
    List_Append(l, & people);
    List_Append(l, & terminators);
    List_Product_Iter(l, Test_Product_Iter, "%s %s%s\n");

    List_Destroy(l);
    List_Destroy(greetings);
    List_Destroy(people);
    List_Destroy(terminators);
  }

  return 0;
}

#endif
