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

/* $Id$ */

#include "types.h"
#include "typesys.h"
#include "messages.h"

Task TS_Init(TS **ts) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

void TS_Destroy(TS *ts) {
  MSG_ERROR("Stil not implemented!");
}

Task TS_Intrinsic_New(TS *ts, Type *i) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Alias_New(TS *ts, Type *a, Type t) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Link_New(TS *ts, Type *l, Type t) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Structure_Begin(TS *ts, Type *s) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Structure_Add(TS *ts, Type s, Type m, char *m_name) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Array_New(TS *ts, Type *a, Type t) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Species_Begin(TS *ts, Type *s) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Species_Add(TS *ts, Type s, Type m) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Enum_Begin(TS *ts, Type *e) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Enum_Add(TS *ts, Type e, Type t) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, Data *dv) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}
