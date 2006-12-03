/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* #define DEBUG */

#include "hashtable.h"
#include "hashfunc.h"
#include "str.h"

static void *xmalloc(size_t size) {
  void *p = malloc(size);
  if ( p == NULL ) {
    fprintf(stderr, "Fatal error: malloc failed!\n");
    exit(EXIT_FAILURE);
  }
  return p;
}

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

int HT_Default_Action(HashItem *hi) {return 1;}

/* Create a new hashtable
 */
void HT_New(Hashtable **new_ht, unsigned int num_entries,
 HashFunction hash, HashComparison cmp)
{
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

  ht = (Hashtable *) xmalloc(sizeof(Hashtable));
  hi = (HashItem **) xmalloc(sizeof(HashItem)*num_entries);
  *new_ht = ht;

#if DEBUG
  printf("Created hashtable of size %d, mask = %8x\n", num_entries, mask);
#endif

  for(i = 0; i < num_entries; i++) hi[i] = (HashItem *) NULL;
  ht->num_entries = num_entries;
  ht->mask = mask;
  ht->settings.copy_keys = 1;
  ht->settings.copy_objs = 1;
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

void HT_Destroy(Hashtable *ht) {
  int branch;
  HashItem *hi, *next;

  /* First we deallocate all the HashItem-s */
  for(branch = 0; branch < ht->num_entries; branch++)
    for(hi = ht->item[branch]; hi != (HashItem *) NULL; hi = next) {
      next = hi->next;
      if ( hi->allocated.key ) free(hi->key);
      if ( hi->allocated.obj ) free(hi->object);
      free(hi);
    }

  /* Now we deallocate the table of branches */
  free(ht->item);
  /* And at the end we free the main Hashtable structure */
  free(ht);
}

/* Add a new element to the branch number 'branch' of the hashtable 'ht'.
 * This new element will have the attributes: 'key' and 'object'.
 * key and object will only be referenced by the hashtable (not copied),
 * so you should allocate/free by yourself if you need to do so.
 */
int HT_Add(
 Hashtable *ht,
 unsigned int branch,
 void *key,
 unsigned int key_size,
 void *object,
 unsigned int object_size)
{
  HashItem *hi;
  assert(branch < ht->num_entries);
  hi = (HashItem *) xmalloc(sizeof(HashItem));

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

void HT_Copy_Key(Hashtable *ht, int bool) {
  ht->settings.copy_keys = bool;
}

void HT_Copy_Obj(Hashtable *ht, int bool) {
  ht->settings.copy_objs = bool;
}

/* Iterate over one or all the branches of an hashtable 'ht':
 * if 'branch < 0' returns 0, otherwise iterate over
 * the branch number 'branch'. For every iteration the function cmp
 * will be used to determine if the current element is equal to 'item'.
 * For every element which matches, the function 'action' will be called.
 * If this function returns 0 the iteration will continue, if it returns 0,
 * then the iteration will end and the function will return the current
 * element inside *result.
 * RETURN VALUE: this function returns 1 if the item has been succesfully found
 *  ('action' returned with 1), 0 otherwise.
 */
int HT_Iter(Hashtable *ht, int branch,
 void *key, unsigned int key_size,
 HashItem **result, int (*action)(HashItem *))
{
  if ( branch < 0 ) {
    return 0;

  } else {
    HashItem *hi;
    for(hi = ht->item[branch]; hi != (HashItem *) NULL; hi = hi->next)
      if ( ht->cmp(hi->key, key, hi->key_size, key_size) ) {
        if ( action(hi) ) {
          *result = hi;
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
 * If this function returns 0 the iteration will continue, if it returns 0,
 * then the iteration will end.
 * RETURN VALUE: this function returns 1 if the item has been succesfully found
 *  ('action' returned with 1), 0 otherwise.
 */
int HT_Iter2(Hashtable *ht, int branch, int (*action)(HashItem *)) {
  if ( branch < 0 ) {
    int i;
    for(i = 0; i < ht->num_entries; i++)
      if ( HT_Iter2(ht, i, action) ) return 1;
    return 0;

  } else {
    HashItem *hi;
    for(hi = ht->item[branch]; hi != (HashItem *) NULL; hi = hi->next)
      if ( action(hi) ) return 1;
    return 0;
  }
}

static int branch_size;
static int count_action(HashItem *hi) {++branch_size; return 0;}

void HT_Statistics(Hashtable *ht, FILE *out) {
  int i;
  fprintf(out, "--------------------\n");
  fprintf(out, "HASHTABLE STATISTICS:\n");
  fprintf(out, "number of branches %d\n", ht->num_entries);
  fprintf(out, "occupation status\n");
  for(i = 0; i < ht->num_entries; i++) {
    branch_size = 0;
    (void) HT_Iter2(ht, i, count_action);
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
