/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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

#include <box/ntypes.h>

typedef enum {
  BOXTYPECLASS_PARAMETER,
  BOXTYPECLASS_BASIC,
  BOXTYPECLASS_INTRINSIC,
  BOXTYPECLASS_ALIAS,
  BOXTYPECLASS_RAISED,
  BOXTYPECLASS_STRUCTURE,
  BOXTYPECLASS_SPECIES,
  BOXTYPECLASS_ENUM,
  BOXTYPECLASS_FUNCTION,
  BOXTYPECLASS_POINTER,
  BOXTYPECLASS_ANY

} BoxTypeClass;

struct BoxTypeDesc_struct {
  BoxTypeClass type_class;
  size_t       size,
               alignment;
};

typedef struct {
  char *name;
  size_t size,
         alignment;
} BoxTypeIntrinsic;

typedef struct {
  char *name;
  size_t size,
         alignment;
} BoxTypeAlias;


static void *MyType_Alloc(BoxType *t, BoxTypeClass tc) {
  size_t additional = 0, total;

  switch (tc) {
  case BOXTYPECLASS_INTRINSIC: additional = sizeof(BoxTypeAlias); break;
  case BOXTYPECLASS_ALIAS: additional = sizeof(BoxTypeAlias); break;
  default:
    MSG_FATAL("Unknown type class in MyType_Alloc");
    abort();
  }

  if (Box_Mem_X_Plus_Y(& total, additional, sizeof(BoxTypeDesc))) {
    BoxTypeDesc *td = Box_Mem_RC_Safe_Alloc(total);
    *t = td;
    return (void *) td + sizeof(BoxTypeDesc);
  }   

  MSG_FATAL("Integer overflow in MyType_Alloc");
  abort();
  return NULL;
}

BoxType BoxType_New_Intrinsic(size_t size, size_t alignment,
                              const char *name) {
  BoxType t;
  BoxTypeIntrinsic *ti = MyType_Alloc(& t, BOXTYPECLASS_INTRINSIC);
  ti->name = name;
  ti->size = size;
  ti->alignment = alignment;  
  return t;
}

BoxType BoxType_New_Alias(BoxType source, const char *name) {
  BoxType t;
  BoxTypeAlias *ta = MyType_Alloc(& t, BOXTYPECLASS_ALIAS);
  ta->name = name;
  ta->source = source;
}
