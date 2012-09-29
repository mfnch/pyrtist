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

#define BOX_ABBREV

#include <assert.h>

#include "types.h"
#include "mem.h"
#include "vmalloc.h"
#include "messages.h"

#include "bltinarray.h"

/* Creation of an array a[10][20][30] is supposed to happen through the
   following calls:

     BoxArray a;
     BoxArray_Init(& a, 3);
     BoxArray_Set_Size(& a, 10);
     BoxArray_Set_Size(& a, 20);
     BoxArray_Set_Size(& a, 30);

   The array is now usable! Raw access to the data of the array is obtained
   with BoxArray_Access. This function gives the extended pointer to the
   data of the array at the given address (the user must provide an address
   in bytes from the beginning of the array).

   The VM provides 5 instructions to deal with BoxArray objects:
   arinit, arsize, araddr, arget, arnext. For example:

     arinit 3         # create a[10][20][30]
     arsize 10
     arsize 20
     arsize 30

     mov ri0, 9       # access a[9][19][28]; array in ro0
     araddr 0, 19
     araddr 1, 29
     arget ro1, ro0

     arnext ro2, ro1  # array in ro0, ro2 contains element after ro1
 */

BoxTask BoxArray_Init(BoxArray *a, BoxInt num_dim) {
  assert(num_dim >= 1 && num_dim <= 127);
  a->num_dim = num_dim;
  BoxPtr_Nullify(& a->data);
  a->sizes = BoxMem_Alloc(sizeof(BoxInt)*num_dim);
  if (a->sizes == NULL) return BOXTASK_FAILURE;
  a->sizes[num_dim - 1] = 0; /* Used by BoxArray_Set_Size to detect the
                                state of initialisation of the object */
  return BOXTASK_OK;
}

void BoxArray_Finish(BoxVM *vm, BoxArray *a) {
  BoxMem_Free(a->sizes);
  BoxVM_Obj_Unlink(vm, & a->data);
  a->sizes = NULL;               /* Just to easily detect errors... */
  BoxPtr_Nullify(& a->data);
  a->num_dim = 0;
}

BoxTask BoxArray_Set_Size(BoxArray *a, BoxInt size) {
  BoxInt num_given_sizes, last_size_index;
  assert(a->num_dim >= 1 && a->num_dim <= 127);
  assert(BoxPtr_Is_Null(& a->data) && a->sizes != NULL);
  last_size_index = a->num_dim - 1;
  num_given_sizes = a->sizes[last_size_index];
  if (num_given_sizes < last_size_index) {
    a->sizes[num_given_sizes] = size;
    a->sizes[last_size_index] = num_given_sizes + 1;
    return BOXTASK_OK;

  } else {
    int i;
    size_t total_data_size;
    a->sizes[last_size_index] = size;

    total_data_size = size;
    for(i = 0; i < last_size_index; i++)
      assert( BoxMem_AX(& total_data_size, total_data_size, a->sizes[i]) );

    assert( BoxMem_AX(& total_data_size, total_data_size, a->item_size) );
    assert(0);

#if 0
    /* Temporarily disabled */
    BoxVM_Alloc(& a->data, total_data_size, 0 /* ??? */);
#endif
    if (BoxPtr_Is_Null(& a->data)) return BOXTASK_FAILURE;
    return BOXTASK_OK;
  }
}

void BoxArray_Access(BoxArray *a, BoxPtr *item, size_t addr) {
  assert(!BoxPtr_Is_Null(& a->data));
  *item = a->data;
  BoxPtr_Add_To_Ptr(item, addr);
}

Task BoxArray_Calc_Address(BoxArray *a, size_t *addr,
                           BoxInt dim, BoxInt index) {
  size_t new_addr;
  assert(!BoxPtr_Is_Null(& a->data) && a->sizes != NULL);
  assert(dim >= 0 && dim < a->num_dim - 1);
  if (index < 0 || index >= a->sizes[dim + 1]) {
    MSG_ERROR("Index out of bounds when accessing Array object.");
    return BOXTASK_FAILURE;
  }
  BoxMem_AX(& new_addr, *addr, a->sizes[dim]);
  BoxMem_x_Plus_y(& new_addr, new_addr, index);
  *addr = new_addr;
  return BOXTASK_OK;
}

