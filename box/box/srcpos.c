/****************************************************************************
 * Copyright (C) 2010 by Matteo Franchin                                    *
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

#include <assert.h>
#include <string.h>

#include "types.h"
#include "messages.h"
#include "msgbase.h"
#include "array.h"
#include "srcpos.h"

void BoxSrcPosTable_Init(BoxSrcPosTable *pt) {
  BoxArr_Init(& pt->file_names, sizeof(char *), 2);
  BoxArr_Init(& pt->assoc_table, sizeof(BoxSrcAssoc), 5);
}

void BoxSrcPosTable_Finish(BoxSrcPosTable *pt) {
  BoxArr_Finish(& pt->file_names);
  BoxArr_Finish(& pt->assoc_table);
}

static int My_Cmp_Names(void *left, void *right, void *pass_data) {
  return strcmp(*((char **) left), *((char **) right));
}

static const char *My_New_Filename(BoxSrcPosTable *pt,
                                   const char *file_name) {
  BoxArr *fns = & pt->file_names;
  size_t idx = BoxArr_Find(fns, & file_name, My_Cmp_Names, NULL);
  if (idx > 0) {
    return *((char **) BoxArr_Item_Ptr(fns, idx));
    
  } else {
    char *s = BoxMem_Strdup(file_name);
    BoxArr_Push(fns, & s);
    return s;
  }
}

void BoxSrcPosTable_Associate(BoxSrcPosTable *pt,
                              BoxOutPos op, BoxSrcPos *sp) {
  BoxArr *at = & pt->assoc_table;
  BoxSrcAssoc *sa;

  if (BoxArr_Num_Items(at) > 0) {
    BoxSrcAssoc *last_sa = BoxArr_Last_Item_Ptr(at);
    if (last_sa->out_pos > op) {
      MSG_FATAL("BoxSrcPosTable_Associate: out positions should be entered "
                "from the lower to the greater.");
      assert(0);
    }
  }

  sa = (BoxSrcAssoc *) BoxArr_Push(at, NULL);
  sa->src_pos = *sp;
  sa->src_pos.file_name = My_New_Filename(pt, sp->file_name);
  sa->out_pos = op;
}

BoxSrcPos *BoxSrcPosTable_Get_Src_Of(BoxSrcPosTable *pt, BoxOutPos op) {
  BoxArr *at = & pt->assoc_table;
  BoxSrcAssoc *sa = BoxArr_First_Item_Ptr(at);
  size_t n = BoxArr_Num_Items(at) - 1;

  /* First let's check that op is in the available interval */
  if (n < 1)
    return NULL;

  else if (sa[0].out_pos > op)
    return NULL;

  else if (sa[n - 1].out_pos <= op)
    return & sa[n - 1].src_pos;

  else {
    /* Here we know that:
     *
     *   sa[0].out_pos <= op < sa[n - 1].out_pos
     *
     * and in the loop the following will hold:
     *
     *   sa[first].out_pos <= op < sa[last].out_pos
     *
     */
    size_t first = 0, last = n - 1;

    do {
      size_t mid = (first + last)/2;
      if (op < sa[mid].out_pos)
        last = mid;
      else
        first = mid;

    } while(last - first > 1);
    return & sa[first].src_pos;
  }
}
