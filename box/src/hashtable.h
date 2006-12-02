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

/**
 * @file hashtable.h
 * @brief The definition of the hash table object.
 *
 * The hash table implementation used by the Box compiler, VM, etc.
 */

typedef unsigned int (*HashFunction)(void *key, unsigned int key_size);
typedef int (*HashComparison)(void *key1, void *key2,
 unsigned int size1, unsigned int size2);

typedef struct ht {
  struct ht *next;
  struct {
    unsigned int key: 1;
    unsigned int obj: 1;
  } allocated;
  void *key, *object;
  unsigned int key_size, object_size;
} HashItem;

/** Hast table object */
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
  /** function to get the hash from the key */
  HashFunction hash;
  /** function to compare two keys */
  HashComparison cmp;
  /** Pointer to the table */
  HashItem **item;
} Hashtable;

unsigned int HT_Default_Hash(void *key, unsigned int key_size);
int HT_Default_Cmp(void *key1, void *key2, unsigned int size1, unsigned int size2);
int HT_Default_Action(HashItem *hi);

/** Create a new hashtable.
 * @param ht where to put the pointer to the created hash table.
 * @param num_entries number of entries of the hash table
 *  (which should be approximatively equal to the number of expected
 *  elements to be inserted in the hashtable).
 * @param hash hash function to be used.
 * @param cmp comparison function to be used.
 */
void HT_New(Hashtable **ht, unsigned int num_entries,
 HashFunction hash, HashComparison cmp);

/** Destroy the hash table given as argument.
 */
void HT_Destroy(Hashtable *ht);

/** Most general function to add a new element to the hashtable.
 * key and object will be duplicated or not, depending on the settings
 * of the hash table (see HT_Copy_Key, HT_Copy_Obj).
 * @param ht the hash table.
 * @param branch number of the branch in the hash table (should be computed
 *  using the hash function). This is done by the macro HT_Insert.
 * @param key pointer to the key.
 * @param key_size size of the key.
 * @param object object corresponding to the key.
 * @param object_size size of the object.
 * @see HT_Insert, HT_Insert_Obj, HT_Copy_Key, HT_Copy_Obj
 */
int HT_Add(Hashtable *ht, unsigned int branch, void *key,
 unsigned int key_size, void *object, unsigned int object_size);

/** Iterate over one or all the branches of an hashtable 'ht'.
 *
 * If 'branch < 0' returns 0, otherwise iterate over
 * the branch number 'branch'. For every iteration the function cmp
 * will be used to determine if the current element is equal to 'item'.
 * For every element which matches, the function 'action' will be called.
 * If this function returns 0 the iteration will continue, if it returns 0,
 * then the iteration will end and the function will return the current
 * element inside *result.
 * RETURN VALUE: this function returns 1 if the item has been succesfully found
 *  ('action' returned with 1), 0 otherwise.
 */
int HT_Iter(Hashtable *ht, int branch, void *key, unsigned int key_size,
 HashItem **result, int (*action)(HashItem *));
int HT_Iter2(Hashtable *ht, int branch, int (*action)(HashItem *));
void HT_Statistics(Hashtable *ht, FILE *out);

/** When an object is added the key is copied by default (using malloc).
 * This function can be used to avoid this behaviour.
 */
void HT_Copy_Key(Hashtable *ht, int bool);

/** When an object is added the object is copied by default (using malloc).
 * This function can be used to avoid this behaviour.
 */
void HT_Copy_Obj(Hashtable *ht, int bool);

/** Create an hash table using the default hashing function
 * and the default comparison function
 * @see HT_New
 */
#define HT(ht, num_entries) \
 HT_New(ht, num_entries, (HashFunction) NULL, (HashComparison) NULL)

/** Insert with HT_Add an element in the hash table,
 * using the default hashing function and with no referenced object.
 * @see HT_Insert_Obj, HT_Add
 */
#define HT_Insert(ht, key, key_size) \
 HT_Add( \
    ht, \
    ht->mask & ht->hash(key, key_size), \
    key, key_size, \
    (void *) NULL, 0)

/** Insert with HT_Add an element in the hash table,
 * using the default hashing function.
 * @see HT_Insert, HT_Add
 */
#define HT_Insert_Obj(ht, key, key_size, object, object_size) \
 HT_Add( \
    ht, \
    ht->mask & ht->hash(key, key_size), \
    key, key_size, \
    object, object_size)

/** Uses HT_Iter to find the given key in the hash table.
 * item will be set with the pointer to found the HashItem.
 * @see HT_Iter
 */
#define HT_Find(ht, key, key_size, item) \
  HT_Iter( \
    ht, \
    ht->mask & ht->hash(key, key_size), \
    key, key_size, \
    item, \
    HT_Default_Action)
