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
  char         *name;

};
