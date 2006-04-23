/*
 * Copyright (C) 2006 Matteo Franchin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

struct {
  UInt key_size;
  void *key, *object;
  struct HashItem *next;
} HashItem;

typedef struct HashItem HashItem;

/* An hash-table is simply a collection of n lists. Let's call them "branches".
 * n is setted up with the function 'Hash_New', which creates the hash-table.
 * The user can then decide to add a new element to a particular branch,
 * using the function 'Hash_Insert'. The user can then search elements inside
 * a given branch, with the function 'Hash_Find'. This function implements
 * the usual stupid algorithm to find the element: it will run through all
 * the elements of the branch, until the right one is reached.
 * This formalism give a way to store elements and search them in a fast way.
 * The only thing to do is to define a function which returns a branch,
 * for every element given. For this purpose, a generic function is given
 * (Hash_Key), but the user can use his own better function.
 */
typedef struct {
  Array *data;
} HashTable;

typedef struct {
  int size;
  void *data;
  int (*compare)(HashKey *o1, HashKey *o2);
} HashKey;
