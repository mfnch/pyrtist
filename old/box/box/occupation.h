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

/**
 * @file occupation.h
 * @brief A BoxOcc is an BoxArr whose elements can be occupied and released.
 *
 * This file defines the object BoxOcc (occupation array) which is an
 * extension of the object BoxArr.
 * With respect to the BoxArr object, the BoxOcc object adds the possibility
 * of occupying and releasing an object. For example:
 *   You can insert three elements: BoxOcc_Occupy will return the indices
 *   1, 2 and 3, then you can release the element 2 with BoxOcc_Release.
 *   Now when you insert another element with BoxOcc_Occupy, it will
 *   be put at the position previously occupied by 2: the most recently
 *   released item (i.e. BoxOcc_Occupy will return 2).
 *   This object is mainly used to keep track of the occupied registers
 *   for the Box VM.
 */

#ifndef _BOX_OCCUPATION_H
#  define _BOX_OCCUPATION_H

#  include <box/types.h>
#  include <box/error.h>
#  include <box/array.h>

typedef void (*BoxOccFinalizer)(void *);

typedef struct {
  BoxErr          err;     /**< Error state of the object */
  BoxArr          array;   /**< Array obj of which BoxOcc is an extension */
  BoxUInt         chain,   /**< Chain of released objects */
                  max_idx, /**< Max index ever returned by BoxOcc_Occupy */
                  elsize;  /**< Element size */
  BoxOccFinalizer fin;     /**< BoxOcc finalizer */
} BoxOcc;

/** Initialise the BoxOcc object into the already allocated space pointed
 * by 'occ'. See BoxArr_Init for meaning of 'element_size' and 'initial_size'.
 */
void BoxOcc_Init(BoxOcc *occ, BoxUInt element_size, BoxUInt initial_size);

/** Finalize the BoxOcc object without deallocating the memory where it lies
 */
void BoxOcc_Finish(BoxOcc *occ);

/** Allocate and initialize a BoxOcc object */
BoxOcc *BoxOcc_New(BoxUInt element_size, BoxUInt initial_size);

/** Finalize and deallocates a BoxOcc object */
void BoxOcc_Destroy(BoxOcc *occ);

/** Set the finalizer for a BoxOcc object (called when releasing items
 * or when finalizing the BoxOcc object)
 */
void BoxOcc_Set_Finalizer(BoxOcc *occ, BoxOccFinalizer fin);

/** Occupies a item in a BoxOcc object and return the corresponding index */
BoxUInt BoxOcc_Occupy(BoxOcc *occ, void *item);

/** Release the item corresponding to the index 'item_index' for the given
 * BoxOcc object.
 */
void BoxOcc_Release(BoxOcc *occ, BoxUInt item_index);

/** Return the pointer to the data corresponding to 'item_index' */
void *BoxOcc_Item_Ptr(BoxOcc *occ, BoxUInt item_index);

/** Return the maximum index ever returned by BoxOcc_Occupy */
BoxUInt BoxOcc_Max_Index(BoxOcc *occ);

#endif /* _BOX_OCCUPATION_H */
