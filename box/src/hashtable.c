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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* #define DEBUG */

#include "types.h"
#include "hashtable.h"
#include "hashfunc.h"
#include "mem.h"
#include "str.h"

#include <ctype.h>


/* Default hash-function */
unsigned int BoxHT_Default_Hash(void *key, size_t key_size) {
  return hashlittle(key, key_size, 1);
}

/* Default comparison function */
int BoxHT_Default_Cmp(void *key1, void *key2, size_t size1, size_t size2) {
  if (size1 != size2)
    return 0;
  else {
    char *k1 = (char *) key1, *k2 = (char *) key2; /* NOTE: Non optimal!!! */
    size_t i;
    for(i = 0; i < size1; i++)
      if (*k1++ != *k2++) return 0;
    return 1;
  }
}

int BoxHT_Default_Action(BoxHTItem *it, void *pass_data) {return 1;}

/* Create a new hashtable
 */
BoxHT *BoxHT_New(unsigned int num_entries, BoxHTFunc hash, BoxHTCmp cmp) {
  BoxHT *ht = BoxMem_Alloc(sizeof(BoxHT));
  if (ht == NULL) return NULL;
  BoxHT_Init(ht, num_entries, hash, cmp);
  return ht;
}

void BoxHT_Init(BoxHT *ht, unsigned int num_entries,
                BoxHTFunc hash, BoxHTCmp cmp) {
  BoxHTItem **hi;
  int i, mask;

  assert(num_entries > 0);
  i = 1;
  mask = 0;
  do {
    i <<= 1;
    mask = mask << 1 | 1;
  } while ((num_entries >>= 1) != 0);
  num_entries = i;

  hi = BoxMem_Alloc(sizeof(BoxHTItem)*num_entries);

#if DEBUG
  printf("Created hashtable of size %d, mask = %8x\n", num_entries, mask);
#endif

  for(i = 0; i < num_entries; i++) hi[i] = NULL;
  ht->num_entries = num_entries;
  ht->mask = mask;
  ht->settings.copy_keys = 1;
  ht->settings.copy_objs = 1;
  ht->destroy = NULL;
  ht->item = hi;
  ht->hash = (hash == NULL) ? BoxHT_Default_Hash : hash;
  ht->cmp = (cmp == NULL) ? BoxHT_Default_Cmp : cmp;
}

static Task Destroy_Item(BoxHTItem *item, void *destructor) {
  return ((Task (*)(BoxHTItem *)) destructor)(item);
}

void BoxHT_Destroy(BoxHT *ht) {
  BoxHT_Finish(ht);
  BoxMem_Free(ht);
}

void BoxHT_Finish(BoxHT *ht) {
  int branch;
  BoxHTItem *hi, *next;

  if (ht->destroy)
    (void) BoxHT_Iter2(ht, -1, Destroy_Item, ht->destroy);

  /* First we deallocate all the BoxHTItem-s */
  for(branch = 0; branch < ht->num_entries; branch++)
    for(hi = ht->item[branch]; hi != NULL; hi = next) {
      next = hi->next;
      if (hi->allocated.key) BoxMem_Free(hi->key);
      if (hi->allocated.obj) BoxMem_Free(hi->object);
      BoxMem_Free(hi);
    }

  /* Now we deallocate the table of branches */
  BoxMem_Free(ht->item);
}

void BoxHT_Destructor(BoxHT *ht, Task (*destroy)(BoxHTItem *)) {
  ht->destroy = destroy;
}

/* Add a new element to the branch number 'branch' of the hashtable 'ht'.
 * This new element will have the attributes: 'key' and 'object'.
 * key and object will only be referenced by the hashtable (not copied),
 * so you should allocate/free by yourself if you need to do so.
 */
int BoxHT_Add(BoxHT *ht, unsigned int branch,
              void *key, size_t key_size,
              void *object, size_t object_size) {
  BoxHTItem *hi;
  assert(branch < ht->num_entries);
  hi = BoxMem_Alloc(sizeof(BoxHTItem));

  hi->key_size = key_size;
  if (ht->settings.copy_keys) {
    hi->key = BoxMem_Dup(key, key_size);
    hi->allocated.key = 1;
  } else {
    hi->key = key;
    hi->allocated.key = 0;
  }

  hi->object_size = object_size;
  if (ht->settings.copy_objs) {
    hi->object = BoxMem_Dup(object, object_size);
    hi->allocated.obj = 1;
  } else {
    hi->object = object;
    hi->allocated.obj = 0;
  }

  /* Add the new item to the list */
  hi->next = ht->item[branch];
  ht->item[branch] = hi;
#ifdef DEBUG
  fprintf(stderr, "Adding item to branch %d\n", branch);
#endif
  return 1;
}

Task BoxHT_Remove(BoxHT *ht, void *key, unsigned int key_size) {
  BoxHTItem **hi_ptr, *hi;
  unsigned int branch = ht->mask & ht->hash(key, key_size);
  hi_ptr = & ht->item[branch];
  while((hi = *hi_ptr) != NULL) {
    if ( ht->cmp(hi->key, key, hi->key_size, key_size) ) {
      *hi_ptr = hi->next;
      if ( hi->allocated.key ) BoxMem_Free(hi->key);
      if ( hi->allocated.obj ) BoxMem_Free(hi->object);
      BoxMem_Free(hi);
      return Success;
    }
    hi_ptr = & hi->next;
  }
  return Failed;
}

Task BoxHT_Rename(BoxHT *ht, void *key, unsigned int key_size,
                  void *new_key, unsigned int new_key_size) {
  BoxHTItem *item;
  void *object;
  unsigned int object_size, allocated_obj;
  TASK( BoxHT_Find(ht, key, key_size, & item) );
  object = item->object;
  object_size = item->object_size;
  allocated_obj = item->allocated.obj;
  item->allocated.obj = 0;
  TASK( BoxHT_Remove(ht, key, key_size) );
  TASK( BoxHT_Insert(ht, new_key, new_key_size) );
  TASK( BoxHT_Find(ht, new_key, new_key_size, & item) );
  item->object = object;
  item->object_size = object_size;
  item->allocated.obj = allocated_obj;
  return Success;
}

/*
 * do_copy: whether to copy the key when a new object is made
 */
void BoxHT_Copy_Key(BoxHT *ht, int do_copy) {
  ht->settings.copy_keys = do_copy;
}

void BoxHT_Copy_Obj(BoxHT *ht, int do_copy) {
  ht->settings.copy_objs = do_copy;
}

/* Iterate over one or all the branches of an hashtable 'ht':
 * if 'branch < 0' returns 0, otherwise iterate over
 * the branch number 'branch'. For every iteration the function cmp
 * will be used to determine if the current element is equal to 'item'.
 * For every element which matches, the function 'it' will be called.
 * If this function returns 0 the iteration will continue, if it returns 0,
 * then the iteration will end and the function will return the current
 * element inside *result.
 * RETURN VALUE: this function returns 1 if the item has been succesfully found
 *  ('action' returned with 1), 0 otherwise.
 */
int BoxHT_Iter(BoxHT *ht, int branch,
               void *key, size_t key_size,
               BoxHTItem **result, BoxHTIterator it, void *pass_data) {
  if ( branch < 0 ) {
    return 0;

  } else {
    BoxHTItem *hi;
    for(hi = ht->item[branch]; hi != NULL; hi = hi->next)
      if ( ht->cmp(hi->key, key, hi->key_size, key_size) ) {
        if ( it(hi, pass_data) ) {
          if (result != NULL) *result = hi;
          return 1;
        }
      }
    return 0;
  }
}

/* Iterate over one or all the branches of an hashtable 'ht':
 * if 'branch < 0' iterate over all the branches, otherwise iterate over
 * the branch number 'branch'.
 * For every element encountered, the function 'action' will be called.
 * If this function returns 'Success' the iteration will continue,
 * if it returns 'Failed', then the iteration will end.
 * RETURN VALUE: this function returns 1 if the item has been succesfully found
 *  ('action' returned with 1), 0 otherwise.
 */
Task BoxHT_Iter2(BoxHT *ht, int branch, BoxHTIterator2 it2, void *pass_data) {
  if ( branch < 0 ) {
    int i;
    for(i = 0; i < ht->num_entries; i++) {
      TASK( BoxHT_Iter2(ht, i, it2, pass_data) );
    }
    return Success;

  } else {
    BoxHTItem *hi;
    for(hi = ht->item[branch]; hi != NULL; hi = hi->next) {
      TASK( it2(hi, pass_data) );
    }
    return Success;
  }
}

static Task count_action(BoxHTItem *hi, void *branch_size) {
  ++*((int *) branch_size);
  return Success;
}

void BoxHT_Statistics(BoxHT *ht, FILE *out) {
  int i;
  int branch_size;
  fprintf(out, "--------------------\n");
  fprintf(out, "HASHTABLE STATISTICS:\n");
  fprintf(out, "number of branches %d\n", ht->num_entries);
  fprintf(out, "occupation status\n");
  for(i = 0; i < ht->num_entries; i++) {
    branch_size = 0;
    (void) BoxHT_Iter2(ht, i, count_action, & branch_size);
    fprintf(out, "branch %d: %d\n", i, branch_size);
  }
  fprintf(out, "--------------------\n");
}

/******************************************************************************/

#if HASHTABLE_TEST
/* Test */
int main(void) {
  BoxHT *ht;
  BoxHTItem *hi;

  (void) BoxHT_New(& ht, 5, (BoxHTFunc) NULL, (BoxHTCmp) NULL);
  (void) BoxHT_Insert(ht, "Ciao", 4);
  (void) BoxHT_Insert(ht, "Matteo", 6);
  (void) BoxHT_Insert(ht, "Franchin", 8);
  (void) BoxHT_Insert(ht, "questo", 6);
  (void) BoxHT_Insert(ht, "e'", 2);
  (void) BoxHT_Insert(ht, "il", 2);
  (void) BoxHT_Insert(ht, "mio", 3);
  (void) BoxHT_Insert(ht, "nome", 4);
  (void) BoxHT_Insert(ht, "e questa e' una piccola frase.", 30);
  (void) BoxHT_Insert(ht, "due parole", 10);
  BoxHT_Statistics(ht, stdout);
  if ( BoxHT_Find(ht, "Matteo", 6, & hi) ) {
    printf("Item found\n");
  } else {
    printf("Item not found\n");
  }
  exit(EXIT_SUCCESS);
}
#endif
