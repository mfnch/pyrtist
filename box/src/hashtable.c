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

static void *xmalloc(size_t size) {
  void *p = malloc(size);
  if ( p == NULL ) {
    fprintf(stderr, "Fatal error: malloc failed!\n");
    exit(EXIT_FAILURE);
  }
  return p;
}

/* Generic hash-function (invented by me: could be bad!) */
unsigned int default_hash(void *key, unsigned int key_size,
 unsigned int ht_size) {
  unsigned long h = 0;
  unsigned char *c = (unsigned char *) key;
  int i;

  for(i = 0; i < key_size; i++)
    h = ( h << 3 ) + *c++;

  return h % ht_size;
}

/* Generic comparison function */
int default_cmp(void *key1, void *key2, unsigned int size1, unsigned int size2)
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

int default_action(HashItem *hi) {return 1;}

/* Create a new hashtable
 */
Hashtable *hashtable_new(
 unsigned int num_entries,
 HashFunction hash,
 HashComparison cmp)
{
  Hashtable *ht;
  HashItem **hi;
  int i;

  assert(num_entries > 0);
  ht = (Hashtable *) xmalloc(sizeof(Hashtable));
  hi = (HashItem **) xmalloc(sizeof(HashItem)*num_entries);

  for(i = 0; i < num_entries; i++) hi[i] = (HashItem *) NULL;
  ht->num_entries = num_entries;
  ht->item = hi;

  if ( hash == (HashFunction) NULL )
    ht->hash = default_hash;
  else
    ht->hash = hash;

  if ( cmp == (HashComparison) NULL )
    ht->cmp = default_cmp;
  else
    ht->cmp = cmp;

  return ht;
}

void hashtable_destroy(Hashtable *ht) {
  int i, branch;
  HashItem *hi, *next;

  /* First we deallocate all the HashItem-s */
  for(branch = 0; i < ht->num_entries; i++)
    for(hi = ht->item[branch]; hi != (HashItem *) NULL; hi = next) {
      next = hi->next;
      free(hi);
    }

  /* Now we deallocate the table of branches */
  free(ht->item);
  /* And at the end we free the main Hashtable structure */
  free(ht);
}

/* Add a new element to the branch number 'branch' of the hashtable 'ht'.
 * This new element will have the attributes: 'key' and 'object'.
 * These object will only be referenced by the hashtable (not copied),
 * so you should allocate/free by yourself if you need to do so.
 */
int hashtable_add(
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
  hi->key = key;
  hi->object = object;
  hi->key_size = key_size;
  hi->object_size = object_size;

  /* Add the new item to the list */
  hi->next = ht->item[branch];
  ht->item[branch] = hi;
#ifdef DEBUG
  fprintf(stderr, "Adding item to branch %d\n", branch);
#endif
  return 1;
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
int hashtable_iter(
 Hashtable *ht,
 int branch,
 void *key,
 unsigned int key_size,
 HashItem **result,
 int (*action)(HashItem *))
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
int hashtable_iter2(Hashtable *ht, int branch, int (*action)(HashItem *)) {
  if ( branch < 0 ) {
    int i;
    for(i = 0; i < ht->num_entries; i++)
      if ( hashtable_iter2(ht, i, action) ) return 1;
    return 0;

  } else {
    HashItem *hi;
    for(hi = ht->item[branch]; hi != (HashItem *) NULL; hi = hi->next)
      if ( action(hi) ) return 1;
    return 0;
  }
}

static int branch_size;

int count_action(HashItem *hi) {++branch_size; return 0;}

void hashtable_statistics(Hashtable *ht, FILE *out) {
  int i;
  HashItem *hi;
  fprintf(out, "--------------------\n");
  fprintf(out, "HASHTABLE STATISTICS:\n");
  fprintf(out, "number of branches %d\n", ht->num_entries);
  fprintf(out, "occupation status\n");
  for(i = 0; i < ht->num_entries; i++) {
    branch_size = 0;
    (void) hashtable_iter2(ht, i, count_action);
    fprintf(out, "branch %d: %d\n", i, branch_size);
  }
  fprintf(out, "--------------------\n");
}

/******************************************************************************/

#if 0
/* Test */
int main(void) {
  Hashtable *ht;
  HashItem *hi;

  ht = hashtable_new(1000, (HashFunction) NULL, (HashComparison) NULL);
  (void) hashtable_insert(ht, "Ciao", NULL);
  (void) hashtable_insert(ht, "Matteo", NULL);
  (void) hashtable_insert(ht, "Franchin", NULL);
  if ( hashtable_find(ht, "Matteo", & hi) ) {
    printf("Item found\n");
  } else {
    printf("Item not found\n");
  }
  exit(EXIT_SUCCESS);
}
#endif
