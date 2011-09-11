/****************************************************************************
 * Copyright (C) 2010-2011 by Matteo Franchin                               *
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

#ifndef _BOX_LIBG_OBJ_H
#  define _BOX_LIBG_OBJ_H

#  include <stdlib.h>

#  include <box/types.h>
#  include <box/array.h>
#  include <box/str.h>

typedef enum {
  BOXGOBJKIND_EMPTY,
  BOXGOBJKIND_VOID,
  BOXGOBJKIND_CHAR,
  BOXGOBJKIND_INT,
  BOXGOBJKIND_REAL,
  BOXGOBJKIND_POINT,
  BOXGOBJKIND_OBJECT,
  BOXGOBJKIND_STR,
  BOXGOBJKIND_COMPOSITE,
  BOXGOBJKIND_TYPE
} BoxGObjKind;

typedef union {
  BoxChar     v_char;
  BoxInt      v_int;
  BoxReal     v_real;
  BoxPoint    v_point;
  BoxStr      v_str;
  BoxArr      v_composite;
} BoxGObjValue;

typedef struct {
  BoxGObjKind  kind;
  BoxGObjValue value;
} BoxGObj;

typedef BoxGObj *BoxGObjPtr;

/** C counterpart of the Box Obj object. */
typedef BoxGObj *BoxLibGObj;

/** Initialisation routine for BoxGObj objects. */
void BoxGObj_Init(BoxGObj *gobj);

/** Finalisation routine for BoxGObj objects. */
void BoxGObj_Finish(BoxGObj *gobj);

BoxGObj *BoxGObj_New(void);

void BoxGObj_Destroy(BoxGObj *gobj);

/** Create a copy of gobj_src in gobj_dest. gobj_dest should not contain
 * a proper BoxGObj object. In other words it should be either uninitalized
 * or be a BOXGOBJKIND_EMPTY object (just BoxGObj_Init has been called on it).
 */
void BoxGObj_Init_From(BoxGObj *gobj_dest, const BoxGObj *gobj_src);

/** Retrieve a subobject of the given BoxGObj object.
 * Return NULL if the index it out of bounds.
 */
BoxGObj *BoxGObj_Get(BoxGObj *gobj, BoxInt idx);

/** Get the number of subobject contained in gobj. */
size_t BoxGObj_Get_Num(BoxGObj *gobj);

#define BoxGObj_Get_Length BoxGObj_Get_Num

/** Get an integer identifying the type of the subobject at index idx. */
BoxInt BoxGObj_Get_Type(BoxGObj *gobj, BoxInt idx);

/** If the object has the given kind, return a pointer to its content,
 * otherwise return NULL. BoxGObj_To(NULL, whatever) returns NULL.
 * This allows safely using: BoxGObj_To(BoxGObj_Get(gobj, idx), kind).
 * Indeed, if the index 'idx' is out of bounds, then BoxGObj_Get returns NULL,
 * which is happily propagated by BoxGObj_To.
 */
void *BoxGObj_To(BoxGObj *gobj, BoxGObjKind kind);

/** Append a C value to 'gobj'. 'content' is the pointer to the C value and
 * 'kind' is its type. Note that only simple C values can be appended, meaning
 * that 'kind != BOXGOBJKIND_COMPOSITE'.
 */
void BoxGObj_Append_C_Value(BoxGObj *gobj, BoxGObjKind kind, void *content);

/** Append a BoxGObj composite object to 'gobj' and return the corresponding
 * pointer. 'num_items' is the expected number of the composite object.
 * This allows to add sub-objects to a give object and populate them.
 * NOTE: 'gobj' is guaranteed to be non-NULL.
 * For example, let's say you want to create something like Obj[1, Obj[2, 3]].
 * You can then use the following code:
 *  
 *   BoxGObj gobj = BoxGObj_New();
 *   BoxGObj_Append_C_Value(gobj, BOXGOBJKIND_INT, & ((BoxInt) 1));
 *   BoxGObj sub_gobj = BoxGObj_Append_Composite(gobj, 2);
 *   BoxGObj_Append_C_Value(sub_gobj, BOXGOBJKIND_INT, & ((BoxInt) 2));
 *   BoxGObj_Append_C_Value(sub_gobj, BOXGOBJKIND_INT, & ((BoxInt) 3));
 */
BoxGObj *BoxGObj_Append_Composite(BoxGObj *gobj, size_t num_items);

/** Add an object gobj_src to another object gobj_dest.
 * This function is equivalent to the Box source gobj_dest[gobj_src],
 * when both gobj_src and gobj_dest are Obj objects.
 */
void BoxGObj_Append_Obj(BoxGObj *gobj_dest, BoxGObj *gobj_src);

/** Function used to map a single BoxGObj object from 'gobj_src' to
 * 'gobj_dest'.
 */
typedef void BoxGObjFilter(BoxGObj *gobj_dest, const BoxGObj *gobj_src,
                           void *pass);

/** Copy gobj_src into gobj_dest, calling the provided function 'filter'
 * to copy the single values. 'pass' is passed as a last argument for
 * 'filter'.
 * @see BoxGObjFilter
 */
void BoxGObj_Filter(BoxGObj *gobj_dest, BoxGObj *gobj_src,
                    BoxGObjFilter filter, void *pass);

/** Filter 'gobj_src' through the given function 'filter' and merge the
 * result into 'gobj_dest'. 'pass' is passed as last argument for filter.
 * @see BoxGObj_Merge
 */
void BoxGObj_Merge_Filtered(BoxGObj *gobj_dest, BoxGObj *gobj_src,
                            BoxGObjFilter filter, void *pass);

/** Merge 'gobj_src' into 'gobj_dest'. This uses BoxGObj_Merge_Filtered
 * using a simple copy as a filter. If 'gobj_dest' is Obj[1, 2] and 'gobj_src'
 * is Obj[3, 4], then this function will append 3 and 4 to 'gobj_desc' which
 * after the call, will be Obj[1, 2, 3, 4].
 * @see BoxGObj_Merge_Filtered
 */
void BoxGObj_Merge(BoxGObj *gobj_dest, BoxGObj *gobj_src);

#define BoxGObj_Is_Single(gobj) \
  ((gobj)->kind != BOXGOBJKIND_COMPOSITE && (gobj)->kind != BOXGOBJKIND_EMPTY)

/** Extract 'num' objects from 'gobj' starting at index 'start_idx'.
 * The extracted objects are assumed to be of type 'kind' and the pointers
 * to their raw data are stored in 'out' (which needs to be allocated by the
 * user so that it can contain 'num' pointers). If there are objects which
 * cannot be extracted (wrong type or 'gobj' has not enough items), then
 * a NULL pointer is stored in the corresponding place in the 'out' array.
 * In that case, BOXTASK_FAILURE is returned. If all the objects are extracted
 * successfully BOXTASK_OK is returned.
 */
BoxTask BoxGObj_Extract_Array(BoxGObj *gobj, BoxGObjKind kind,
                              size_t start_idx, size_t num,
                              void **out);

typedef BoxTask (*BoxGObjIterator)(size_t relative_idx, BoxGObjKind k,
                                   BoxGObj *sub_obj, void *pass);

/** Iterate over the subobjects of 'gobj' starting at index 'start_idx' and
 * going over '*num_args' objects. If 'num_args' is NULL, then run over all
 * the subobjects available from index 'start_idx'. For each of them the
 * function 'iter' is called, passing the relative index (the first call
 * always receives 0), the pointer to the subobject, and the the user provided
 * pointer 'pass'. If the function returns 'BOXTASK_OK' then the iteration
 * continues to the next element, otherwise the iteration is stopped and
 * the 'BoxGObj_Iter' whatever 'iter' returned. Before returning, '*num_args'
 * is set to the total number of arguments over which the iteration was done
 * (which would be 1 if the first call to 'iter' does not return BOXTASK_OK).
 * NOTE: 'gobj' is not supposed to change in size while the iteration is done.
 */
BoxTask BoxGObj_Iter(BoxGObj *gobj, size_t start_idx, size_t *num_args,
                     BoxGObjIterator iter, void *pass);

#endif
