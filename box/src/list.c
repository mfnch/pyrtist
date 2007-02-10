/***************************************************************************
 *   Copyright (C) 2007 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

#include <string.h>

#include "types.h"
#include "messages.h"
#include "mem.h"
#include "list.h"

void List_New(List **l, UInt item_size) {
  List *nl = Mem_Alloc(sizeof(List));
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
    Mem_Free(lih);
    lih = lih_next;
  }
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
  Mem_Free(lih);
}

void List_Insert_With_Size(List *l, void *item_where,
 void *item_what, UInt size) {
  ListItemHead **prev_lih, **next_lih;

  void *new_item = Mem_Alloc(sizeof(ListItemHead) + size);
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

#if 0

Task Print_List_Items(void *item, void *) {
  printf("Item: '%s'\n", (char *) item);
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
  return 0;
}

#endif
