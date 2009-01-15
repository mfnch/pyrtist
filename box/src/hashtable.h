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

/**
 * @file hashtable.h
 * @brief The definition of the hash table object.
 *
 * The hash table implementation used by the Box compiler, VM, etc.
 */

#ifndef _BOX_HASHTABLE_H
#  define _BOX_HASHTABLE_H

#  include <stdlib.h>
#  include <stdio.h>

#  include "types.h"

typedef unsigned int (*BoxHTFunc)(void *key, size_t key_size);
typedef int (*BoxHTCmp)(void *key1, void *key2, size_t size1, size_t size2);

typedef struct ht {
  struct ht *next;
  struct {
    unsigned int key: 1;
    unsigned int obj: 1;
  } allocated;
  void *key, *object;
  size_t key_size, object_size;
} BoxHTItem;

/** @brief Hash table object */
typedef struct {
  /** Size of the hash table, should be a power of 2 */
  int num_entries;
  /** num_entries is a power of two, mask is num_entries-1 */
  int mask;
  /** Hash table settings */
  struct {
    /** Should we copy the key (using malloc)? */
    unsigned int copy_keys: 1;
    /** Should we copy the key (using malloc)? */
    unsigned int copy_objs: 1;
  } settings;
  /** Used to finalize elements before destruction */
  Task (*destroy)(BoxHTItem *);
  /** function to get the hash from the key */
  BoxHTFunc hash;
  /** function to compare two keys */
  BoxHTCmp cmp;
  /** Pointer to the table */
  BoxHTItem **item;
} BoxHT;

typedef int (*BoxHTIterator)(BoxHTItem *item, void *pass_data);
typedef Task (*BoxHTIterator2)(BoxHTItem *item, void *pass_data);

unsigned int BoxHT_Default_Hash(void *key, size_t key_size);
int BoxHT_Default_Cmp(void *key1, void *key2, size_t size1, size_t size2);
int BoxHT_Default_Action(BoxHTItem *hi, void *pass_data);

/** Create a new hashtable.
 * @param ht where to put the pointer to the created hash table.
 * @param num_entries number of entries of the hash table
 *  (which should be approximatively equal to the number of expected
 *  elements to be inserted in the hashtable).
 * @param hash hash function to be used.
 * @param cmp comparison function to be used.
 */
BoxHT *BoxHT_New(unsigned int num_entries, BoxHTFunc hash, BoxHTCmp cmp);

/** Similar to BoxHT_New, but put the hashtable structure in ht, assuming
 * ht points to an allocated region of memory capable of containing
 * a BoxHT object.
 * @see BoxHT_New, BoxHT_Finish
 */
void BoxHT_Init(BoxHT *ht, unsigned int num_entries,
                BoxHTFunc hash, BoxHTCmp cmp);

/** Destroy an hashtable created with BoxHT_Init
 * @see BoxHT_Init
 */
void BoxHT_Finish(BoxHT *ht);

/** Destroy an hashtable created with BoxHT_New
 * @see BoxHT_New
 */
void BoxHT_Destroy(BoxHT *ht);

/** Gives a function used to destroy objects when 'BoxHT_Destroy' is called */
void BoxHT_Destructor(BoxHT *ht, Task (*destroy)(BoxHTItem *));

/** Most general function to add a new element to the hashtable.
 * key and object will be duplicated or not, depending on the settings
 * of the hash table (see BoxHT_Copy_Key, BoxHT_Copy_Obj).
 * @param ht the hash table.
 * @param branch number of the branch in the hash table (should be computed
 *  using the hash function). This is done by the macro BoxHT_Insert.
 * @param key pointer to the key.
 * @param key_size size of the key.
 * @param object object corresponding to the key.
 * @param object_size size of the object.
 * @see BoxHT_Insert, BoxHT_Insert_Obj, BoxHT_Copy_Key, BoxHT_Copy_Obj
 */
int BoxHT_Add(BoxHT *ht, unsigned int branch, void *key,
              size_t key_size, void *object, size_t object_size);

/** Remove the element matching the given key from the hash-table.
 */
Task BoxHT_Remove(BoxHT *ht, void *key, unsigned int key_size);

/** Rename the key keeping the old associated object:
 * The couple (old_key, old_object) is deleted and the new couple
 * (new_key, old_object) is inserted.
 * This function may be used to avoid deleting the data contained
 * in old_object.
 */
Task BoxHT_Rename(BoxHT *ht, void *key, unsigned int key_size,
                  void *new_key, unsigned int new_key_size);

/** Iterate over one branch of an hashtable 'ht'.
 *
 * If 'branch < 0' returns 0, otherwise iterate over
 * the branch number 'branch'. For every iteration the function cmp
 * will be used to determine if the current element is equal to 'item'.
 * For every element which matches, the function 'action' will be called.
 * If this function returns 0 the iteration will continue, if it returns 1,
 * then the iteration will end and the function will return the current
 * element inside *result.
 * RETURN VALUE: this function returns 1 if the item has been succesfully
 *  found ('action' returned with 1), 0 otherwise.
 */
int BoxHT_Iter(BoxHT *ht, int branch, void *key, size_t key_size,
               BoxHTItem **result, BoxHTIterator it, void *pass_data);

/** Iterate over one or all the branches of an hashtable 'ht':
 * if 'branch < 0' iterate over all the branches, otherwise iterate over
 * the branch number 'branch'.
 * For every element encountered, the function 'action' will be called.
 * If this function returns 0 the iteration will continue, if it returns 0,
 * then the iteration will end.
 * RETURN VALUE: this function returns 1 if the item has been succesfully
 *  found ('action' returned with 1), 0 otherwise.
 */
Task BoxHT_Iter2(BoxHT *ht, int branch, BoxHTIterator2 it2, void *pass_data);

/** Prints some statistics about the usage of an hash table.
 */
void BoxHT_Statistics(BoxHT *ht, FILE *out);

/** When an object is added the key is copied by default (using malloc).
 * This function can be used to avoid this behaviour.
 */
void BoxHT_Copy_Key(BoxHT *ht, int do_copy);

/** When an object is added the object is copied by default (using malloc).
 * This function can be used to avoid this behaviour.
 */
void BoxHT_Copy_Obj(BoxHT *ht, int do_copy);

/** Create an hash table using the default hashing function
 * and the default comparison function
 * @see BoxHT_New
 */
#define BoxHT_New_Default(ht, num_entries) \
 BoxHT_New((num_entries), (BoxHTFunc) NULL, (BoxHTCmp) NULL)

/** Create an hash table using the default hashing function
 * and the default comparison function
 * @see BoxHT_Init
 */
#define BoxHT_Init_Default(ht, num_entries) \
 BoxHT_Init((ht), (num_entries), (BoxHTFunc) NULL, (BoxHTCmp) NULL)

/** Insert with BoxHT_Add an element in the hash table,
 * using the default hashing function and with no referenced object.
 * @see BoxHT_Insert_Obj, BoxHT_Add
 */
#define BoxHT_Insert(ht, key, key_size) \
 BoxHT_Add((ht), (ht)->mask & (ht)->hash((key), (key_size)), \
           (key), (key_size), NULL, 0)

/** Insert with BoxHT_Add an element in the hash table,
 * using the default hashing function.
 * @see BoxHT_Insert, BoxHT_Add
 */
#define BoxHT_Insert_Obj(ht, key, key_size, object, object_size) \
 BoxHT_Add((ht), (ht)->mask & (ht)->hash((key), (key_size)), \
           (key), (key_size), (object), (object_size))

/** Uses BoxHT_Iter to find the given key in the hash table.
 * item will be set with the pointer to found the BoxHTItem.
 * @see BoxHT_Iter
 */
#define BoxHT_Find(ht, key, key_size, item) \
  BoxHT_Iter((ht), (ht)->mask & (ht)->hash((key), (key_size)), \
             (key), (key_size), (item), BoxHT_Default_Action, NULL)

#endif /* _BOX_HASHTABLE_H */
