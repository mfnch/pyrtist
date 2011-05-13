/****************************************************************************
 * Copyright (C) 2011 by Matteo Franchin                                    *
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <box/types.h>
#include <box/mem.h>
#include <box/str.h>

void BoxStr_Init(BoxStr *s) {
  s->ptr = NULL;
  s->length = 0;
  s->buffer_size = 0;
}

void BoxStr_Finish(BoxStr *s) {
  if (s->ptr != (char *) NULL) {
    BoxMem_Free(s->ptr);
    s->ptr = (char *) NULL;
    s->length = 0;
    s->buffer_size = 0;
  }
}

BoxTask BoxStr_Large_Enough(BoxStr *s, Int length) {
  size_t len;
  assert(s->length >= 0 && length >= 0);

  len = s->length + length + 1;
  len = len + (len+1)/2;
  assert(len > length);
  s->ptr = (char *) BoxMem_Realloc(s->ptr, len);
  s->buffer_size = len;
  return BOXTASK_OK;
}

BoxTask BoxStr_Concat_C_String(BoxStr *s, const char *ca) {
  Int len = strlen(ca);
  if (len < 1) return BOXTASK_OK;
  if (s->buffer_size - s->length - 1 < len)
    BoxStr_Large_Enough(s, len);
  assert(s->buffer_size - s->length - 1 >= len);
  (void) strcpy(s->ptr + s->length, ca);
  s->length += len;
  return BOXTASK_OK;
}

BoxTask BoxStr_Concat(BoxStr *dest, const BoxStr *src) {
  if (src->length > 0)
    return BoxStr_Concat_C_String(dest, src->ptr);
  return BOXTASK_OK;
}

BoxTask BoxStr_Init_From(BoxStr *new_str, const BoxStr *src) {
  BoxStr_Init(new_str);
  return BoxStr_Concat(new_str, src);
}

char *BoxStr_To_C_String(BoxStr *s) {
  if (s->length == 0)
    return BoxMem_Strdup((s->ptr == NULL) ?
                         "" : "<broken Str: s->ptr != NULL>");

  else {
    if (s->ptr == NULL)
      return BoxMem_Strdup("<broken Str: s->ptr == NULL>");

    else {
      size_t l = strlen(s->ptr), lp1 = l + 1;
      char *cs;
      Box_Fatal_Error_If_Not(lp1 > l); /* buffer overflow */
      cs = BoxMem_Safe_Alloc(lp1);
      strncpy(cs, s->ptr, l);
      cs[l] = '\0';
      return cs;
    }
  }
}

char *BoxStr_Get_Ptr(BoxStr *s) {return s->ptr;}

size_t BoxStr_Get_Size(BoxStr *s) {return s->length;}
