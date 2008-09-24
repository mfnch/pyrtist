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
unsigned int HT_Default_Hash(void *key, unsigned int key_size) {
  return hashlittle(key, key_size, 1);
}

/* Default comparison function */
int HT_Default_Cmp(void *key1, void *key2, unsigned int size1, unsigned int size2)
{
  if (size1 != size2)
    return 0;
  else {
    char *k1 = (char *) key1, *k2 = (char *) key2; /* NOTE: Non optimal!!! */
    int i;
    for(i = 0; i < size1; i++)
      if ( *k1++ != *k2++ ) return 0;
    return 1;
  }
}

int HT_Default_Action(HashItem *it, void *pass_data) {return 1;}

/* Create a new hashtable
 */
void HT_New(Hashtable **new_ht, unsigned int num_entries,
            HashFunction hash, HashComparison cmp) {
  Hashtable *ht;
  HashItem **hi;
  int i, mask;

  assert(num_entries > 0);
  i = 1;
  mask = 0;
  do {
    i <<= 1;
    mask = mask << 1 | 1;
  } while ((num_entries >>= 1) != 0);
  num_entries = i;

  ht = (Hashtable *) Mem_Alloc(sizeof(Hashtable));
  hi = (HashItem **) Mem_Alloc(sizeof(HashItem)*num_entries);
  *new_ht = ht;

#if DEBUG
  printf("Created hashtable of size %d, mask = %8x\n", num_entries, mask);
#endif

  for(i = 0; i < num_entries; i++) hi[i] = (HashItem *) NULL;
  ht->num_entries = num_entries;
  ht->mask = mask;
  ht->settings.copy_keys = 1;
  ht->settings.copy_objs = 1;
  ht->destroy = NULL;
  ht->item = hi;

  if ( hash == (HashFunction) NULL )
    ht->hash = HT_Default_Hash;
  else
    ht->hash = hash;

  if ( cmp == (HashComparison) NULL )
    ht->cmp = HT_Default_Cmp;
  else
    ht->cmp = cmp;
}

Task Destroy_Item(HashItem *item, void *destructor) {
  return ((Task (*)(HashItem *)) destructor)(item);
}

void HT_Destroy(Hashtable *ht) {
  int branch;
  HashItem *hi, *next;

  if (ht->destroy) (void) HT_Iter2(ht, -1, Destroy_Item, ht->destroy);

  /* First we deallocate all the HashItem-s */
  for(branch = 0; branch < ht->num_entries; branch++)
    for(hi = ht->item[branch]; hi != (HashItem *) NULL; hi = next) {
      next = hi->next;
      if ( hi->allocated.key ) Mem_Free(hi->key);
      if ( hi->allocated.obj ) Mem_Free(hi->object);
      Mem_Free(hi);
    }

  /* Now we deallocate the table of branches */
  Mem_Free(ht->item);
  /* And at the end we free the main Hashtable structure */
  Mem_Free(ht);
}

void HT_Destructor(Hashtable *ht, Task (*destroy)(HashItem *)) {
  ht->destroy = destroy;
}

/* Add a new element to the branch number 'branch' of the hashtable 'ht'.
 * This new element will have the attributes: 'key' and 'object'.
 * key and object will only be referenced by the hashtable (not copied),
 * so you should allocate/free by yourself if you need to do so.
 */
int HT_Add(Hashtable *ht, unsigned int branch,
           void *key, unsigned int key_size,
           void *object, unsigned int object_size) {
  HashItem *hi;
  assert(branch < ht->num_entries);
  hi = (HashItem *) Mem_Alloc(sizeof(HashItem));

  hi->key_size = key_size;
  if (ht->settings.copy_keys) {
    hi->key = Mem_Dup(key, key_size);
    hi->allocated.key = 1;
  } else {
    hi->key = key;
    hi->allocated.key = 0;
  }

  hi->object_size = object_size;
  if (ht->settings.copy_objs) {
    hi->object = Mem_Dup(object, object_size);
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

Task HT_Remove(Hashtable *ht, void *key, unsigned int key_size) {
  HashItem **hi_ptr, *hi;
  unsigned int branch = ht->mask & ht->hash(key, key_size);
  hi_ptr = & ht->item[branch];
  while( (hi = *hi_ptr) != (HashItem *) NULL ) {
    if ( ht->cmp(hi->key, key, hi->key_size, key_size) ) {
      *hi_ptr = hi->next;
      if ( hi->allocated.key ) Mem_Free(hi->key);
      if ( hi->allocated.obj ) Mem_Free(hi->object);
      Mem_Free(hi);
      return Success;
    }
    hi_ptr = & hi->next;
  }
  return Failed;
}

Task HT_Rename(Hashtable *ht, void *key, unsigned int key_size,
               void *new_key, unsigned int new_key_size) {
  HashItem *item;
  void *object;
  unsigned int object_size, allocated_obj;
  TASK( HT_Find(ht, key, key_size, & item) );
  object = item->object;
  object_size = item->object_size;
  allocated_obj = item->allocated.obj;
  item->allocated.obj = 0;
  TASK( HT_Remove(ht, key, key_size) );
  TASK( HT_Insert(ht, new_key, new_key_size) );
  TASK( HT_Find(ht, new_key, new_key_size, & item) );
  item->object = object;
  item->object_size = object_size;
  item->allocated.obj = allocated_obj;
  return Success;
}

/*
 * doCopy whether to copy the key when a new object is made
 */
void HT_Copy_Key(Hashtable *ht, int do_copy) {
  ht->settings.copy_keys = do_copy;
}

void HT_Copy_Obj(Hashtable *ht, int do_copy) {
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
int HT_Iter(Hashtable *ht, int branch,
            void *key, unsigned int key_size,
 HashItem **result, HTIterator it, void *pass_data)
{
  if ( branch < 0 ) {
    return 0;

  } else {
    HashItem *hi;
    for(hi = ht->item[branch]; hi != (HashItem *) NULL; hi = hi->next)
      if ( ht->cmp(hi->key, key, hi->key_size, key_size) ) {
        if ( it(hi, pass_data) ) {
          if (result != (HashItem **) NULL) *result = hi;
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
Task HT_Iter2(Hashtable *ht, int branch, HTIterator2 it2, void *pass_data) {
  if ( branch < 0 ) {
    int i;
    for(i = 0; i < ht->num_entries; i++) {
      TASK( HT_Iter2(ht, i, it2, pass_data) );
    }
    return Success;

  } else {
    HashItem *hi;
    for(hi = ht->item[branch]; hi != (HashItem *) NULL; hi = hi->next) {
      TASK( it2(hi, pass_data) );
    }
    return Success;
  }
}

static Task count_action(HashItem *hi, void *branch_size) {
  ++*((int *) branch_size);
  return Success;
}

void HT_Statistics(Hashtable *ht, FILE *out) {
  int i;
  int branch_size;
  fprintf(out, "--------------------\n");
  fprintf(out, "HASHTABLE STATISTICS:\n");
  fprintf(out, "number of branches %d\n", ht->num_entries);
  fprintf(out, "occupation status\n");
  for(i = 0; i < ht->num_entries; i++) {
    branch_size = 0;
    (void) HT_Iter2(ht, i, count_action, & branch_size);
    fprintf(out, "branch %d: %d\n", i, branch_size);
  }
  fprintf(out, "--------------------\n");
}

/******************************************************************************/

#if HASHTABLE_TEST
/* Test */
int main(void) {
  Hashtable *ht;
  HashItem *hi;

  (void) HT_New(& ht, 5, (HashFunction) NULL, (HashComparison) NULL);
  (void) HT_Insert(ht, "Ciao", 4);
  (void) HT_Insert(ht, "Matteo", 6);
  (void) HT_Insert(ht, "Franchin", 8);
  (void) HT_Insert(ht, "questo", 6);
  (void) HT_Insert(ht, "e'", 2);
  (void) HT_Insert(ht, "il", 2);
  (void) HT_Insert(ht, "mio", 3);
  (void) HT_Insert(ht, "nome", 4);
  (void) HT_Insert(ht, "e questa e' una piccola frase.", 30);
  (void) HT_Insert(ht, "due parole", 10);
  HT_Statistics(ht, stdout);
  if ( HT_Find(ht, "Matteo", 6, & hi) ) {
    printf("Item found\n");
  } else {
    printf("Item not found\n");
  }
  exit(EXIT_SUCCESS);
}
#endif
