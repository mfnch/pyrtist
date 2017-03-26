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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "messages.h"
#include "msgbase.h"
#include "array.h"
#include "srcpos.h"
#include "print.h"

BoxSrcName *BoxSrcName_Create(void)
{
  BoxSrcName *srcname = Box_Mem_Safe_Alloc(sizeof(BoxSrcName));
  BoxSrcName_Init(srcname);
  return srcname;
}

void BoxSrcName_Destroy(BoxSrcName *srcname)
{
  BoxSrcName_Finish(srcname);
  Box_Mem_Free(srcname);
}

static void My_SrcName_Finalizer(void *ptr)
{
  Box_Mem_Free(*((char **) ptr));
}

void BoxSrcName_Init(BoxSrcName *srcname)
{
  BoxArr_Init(& srcname->names, sizeof(char *), 8);
  BoxArr_Set_Finalizer(& srcname->names, My_SrcName_Finalizer);
}

void BoxSrcName_Finish(BoxSrcName *srcname)
{
  BoxArr_Finish(& srcname->names);
}

const char *BoxSrcName_Add(BoxSrcName *srcname, const char *name)
{
  if (name) {
    char *name_copy = Box_Mem_Strdup(name);
    BoxArr_Push(& srcname->names, & name_copy);
    return name_copy;
  } else
    return NULL;
}

static void My_File_Name_Finalizer(void *ptr)
{
  Box_Mem_Free(*((char **) ptr));
}

void BoxSrcPosTable_Init(BoxSrcPosTable *pt)
{
  BoxArr_Init(& pt->file_names, sizeof(char *), 2);
  BoxArr_Set_Finalizer(& pt->file_names, My_File_Name_Finalizer);
  BoxArr_Init(& pt->assoc_table, sizeof(BoxSrcAssoc), 5);
}

void BoxSrcPosTable_Finish(BoxSrcPosTable *pt)
{
  BoxArr_Finish(& pt->file_names);
  BoxArr_Finish(& pt->assoc_table);
}

void BoxSrcPosTable_Clear(BoxSrcPosTable *pt)
{
  BoxSrcPosTable_Finish(pt);
  BoxSrcPosTable_Init(pt);
}

void BoxSrcPosTable_Compactify(BoxSrcPosTable *pt)
{
  BoxArr_Compactify(& pt->file_names);
  BoxArr_Compactify(& pt->assoc_table);
}

static int My_Cmp_Names(void *left, void *right, void *pass_data)
{
  return strcmp(*((char **) left), *((char **) right));
}

static const char *My_New_Filename(BoxSrcPosTable *pt,
                                   const char *file_name)
{
  if (file_name == NULL)
    return NULL;

  else {
    BoxArr *fns = & pt->file_names;
    size_t idx = BoxArr_Find(fns, & file_name, My_Cmp_Names, NULL);
    if (idx > 0) {
      return *((char **) BoxArr_Item_Ptr(fns, idx));

    } else {
      char *s = Box_Mem_Strdup(file_name);
      BoxArr_Push(fns, & s);
      return s;
    }
  }
}

void BoxSrcPosTable_Associate(BoxSrcPosTable *pt,
                              BoxOutPos op, BoxSrcPos *sp)
{
  BoxArr *at = & pt->assoc_table;
  BoxSrcAssoc *sa = NULL;

  if (BoxArr_Num_Items(at) > 0) {
    BoxSrcAssoc *last_sa = (BoxSrcAssoc *) BoxArr_Last_Item_Ptr(at);
    if (last_sa->out_pos > op) {
      MSG_FATAL("BoxSrcPosTable_Associate: out positions should be entered "
                "from the lower to the greater.");
      assert(0);
    }

    if (last_sa->out_pos == op)
      sa = last_sa; /* Last item will be overwritten */
  }

  if (sa == NULL)
    sa = (BoxSrcAssoc *) BoxArr_Push(at, NULL);

  sa->src_pos = *sp;
  sa->src_pos.file_name = My_New_Filename(pt, sp->file_name);
  sa->out_pos = op;
}

void BoxSrcPosTable_Print(BoxSrcPosTable *pt, FILE *out)
{
  BoxArr *at = & pt->assoc_table;
  BoxSrcAssoc *sa = BoxArr_First_Item_Ptr(at);
  size_t n = BoxArr_Num_Items(at), i;

  fprintf(out, "BoxSrcPosTable: content:\n");
  for(i = 0; i < n; i++) {
    char *src_pos_str = BoxSrcPos_To_Str(& sa[i].src_pos);
    fprintf(out, "  out_pos=%ld, src_pos=\"%s\"\n",
            sa[i].out_pos, src_pos_str);
    Box_Mem_Free(src_pos_str);
  }
  fprintf(out, "BoxSrcPosTable: end.\n");
}

BoxSrcPos *BoxSrcPosTable_Get_Src_Of(BoxSrcPosTable *pt, BoxOutPos op)
{
  BoxArr *at = & pt->assoc_table;
  BoxSrcAssoc *sa = BoxArr_First_Item_Ptr(at);
  size_t n = BoxArr_Num_Items(at);

  /* First let's check that op is in the available interval */
  if (n < 1)
    return NULL;

  if (sa[0].out_pos > op)
    return NULL;

  if (sa[n - 1].out_pos <= op)
    return & sa[n - 1].src_pos;

  {
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

void BoxSrcPos_Init(BoxSrcPos *pos, const char *file_name)
{
  pos->file_name = file_name;
  pos->line = 1;
  pos->col = 1;
}

char *BoxSrcPos_To_Str(BoxSrcPos *pos)
{
  if (pos->file_name == NULL)
    return printdup("line %ld", pos->line);
  else
    return printdup("line %ld of file \"%s\"", pos->line, pos->file_name);
}

void BoxSrcPos_Next_Line(BoxSrcPos *pos)
{
  if (pos->line > 0) {
    pos->line += 1;
    pos->col = 1;
  }
}

int BoxSrcPos_Is_Undef(BoxSrcPos *pos)
{
  return pos->line < 1;
}

void BoxSrcPos_Set_Undef(BoxSrcPos *pos)
{
  pos->line = 0;
  pos->col = 0;
}

void BoxSrc_Init(BoxSrc *src)
{
  src->begin = src->end = 0;
}

char *BoxSrc_To_Str(BoxSrc *loc)
{
#if 1
  return Box_SPrintF("idx %d-%d", (int) loc->begin, (int) loc->end);
#else
  long bl = loc->begin.line, bc = loc->begin.col,
       el = loc->end.line, ec = loc->end.col;
  const char *fn =
    (loc->begin.file_name != NULL) ?
    Box_SPrintF("\"%s\", ", loc->begin.file_name) : Box_Mem_Strdup("");

  if (bl == 0)
    return Box_SPrintF("%~stext ending at line %ld col %ld", fn, el, ec);

  else if (el == 0)
    return Box_SPrintF("%~sfrom line %ld col %ld", fn, bl, bc);

  else if (bl == el) {
    if (loc->begin.col >= loc->end.col - 1)
      return Box_SPrintF("%~sline %ld col %ld", fn, bl, bc);
    else
      return Box_SPrintF("%~sline %ld cols %ld-%ld", fn, bl, bc, ec);
  } else {
    return Box_SPrintF("%~sline %ld-%ld cols %ld-%ld", fn, bl, el, bc, ec);
  }
#endif
}

void BoxSrc_Merge(BoxSrc *result, BoxSrc *fst, BoxSrc *snd)
{
  result->begin = (fst->begin <= snd->begin) ? fst->begin : snd->begin;
  result->end = (fst->end >= snd->end) ? fst->end : snd->end;
}
