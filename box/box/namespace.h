/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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

/** @file namespace.h
 * @brief Handling of names and namespaces in the Box compiler.
 *
 * A nice description...
 */

#ifndef _NAMESPACE_H
#  define _NAMESPACE_H

#  include "types.h"
#  include "array.h"
#  include "hashtable.h"
#  include "value.h"

/** Namespace floor. Each floor can have its own variables which live just
 * on that floor and are destroyed when the floor is destroyed.
 */
typedef size_t NmspFloor;

/** Used to access a name from the default floor (the higher one where having
 * such a name)
 */
#define NMSPFLOOR_DEFAULT -1

/** The namespace object */
typedef struct {
  BoxHT  ht;     /**< Hash table containing all the names of the namespace */
  BoxArr floors; /**< Array of pointers to the name chain for each floor */
} Namespace;

/** Possible types for item inserted into the namespace */
typedef enum {
  NMSPITEMTYPE_VALUE
} NmspItemType;

/** Item which can be inserted into the namespace */
typedef struct nmsp_item_s {
  struct nmsp_item_s
                *next;     /**< Chain of all the names in the same floor */
  BoxHTItem     *ht_item;  /**< Corresponding BoxHTItem (for quick removal) */
  NmspItemType  type;      /**< Type of item */
  void          *data;     /**< Pointer to the item data */
} NmspItem;

/** Initialise a new Namespace object, in the space already allocated
 * and pointed by ns. Namespace_Finish should be called to finalize
 * the object.
 */
void Namespace_Init(Namespace *ns);

/** Finalize an object initialized with Namespace_Init. */
void Namespace_Finish(Namespace *ns);

/** Allocate space for a Namespace object and call Namespace_Init
 * to initialize it. Namespace_Destroy should be called to destroy the object.
 */
Namespace *Namespace_New(void);

/** Destroy an object created with Namespace_New. */
void Namespace_Destroy(Namespace *ns);

/** Create a new floor. A floor is a container for the temporary things
 * which can be put in a namespace. When the floor is destroyed by calling
 * 'Namespace_Floor_Down', all the associated names are removed from the
 * namespace.
 */
void Namespace_Floor_Up(Namespace *ns);

/** Destroy the active floor in the given namespace. */
void Namespace_Floor_Down(Namespace *ns);

/** Add a new item with name 'item_name' to the given floor of the namespace
 * and return the pointer to the corresponding NmspItem object.
 */
NmspItem *Namespace_Add_Item(Namespace *ns, NmspFloor floor,
                             const char *item_name);

NmspItem *Namespace_Get_Item(Namespace *ns, NmspFloor floor,
                             const char *item_name);

/** Use 'Namespace_Add_Item' to add the value 'v' to the namespace. */
void Namespace_Add_Value(Namespace *ns, NmspFloor floor,
                         const char *item_name, Value *v);

/** Get an item 'item_name' from the namespace, assuming this must be a
 * Value.
 */
Value *Namespace_Get_Value(Namespace *ns, NmspFloor floor,
                           const char *item_name);

/*

void Namespace_Plug(Namespace *ns, const char *name, Namespace *ns_to_plug);

void Namespace_Unplug(Namespace *ns, const char *name);
*/

#endif /* _NAMESPACE_H */

