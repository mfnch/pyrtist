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

#include <assert.h>

#include "virtmach.h"
#include "vmsym.h"

Task VM_Sym_Begin(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
  VMSymTable *st = & vmp->sym_table;
  return Failed;
}

Task VM_Sym_End(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
}

void VM_Sym_Procedure(VMSym *s, char *name, int sheet) {
  assert(s != (VMSym *) NULL);
  assert(sheet >= 0);
  s->is_definition = (sheet > 0);
  s->name = name;
  s->type = VMSYM_PROCEDURE;
  s->value.def_procedure = sheet;
}

void VM_Sym_Reference(VMSym *s, int sheet, int position, int length) {
  assert(s != (VMSym *) NULL);
  assert(!s->is_definition);
  s->value.ref.sheet = sheet;
  s->value.ref.position = position;
  s->value.ref.length = length;
}

Task VM_Sym_Add(VMProgram *vmp, VMSym *s) {
  assert(vmp != (VMProgram *) NULL);
}

Task VM_Sym_Link(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
}

#if 0
/* Reference definition */
VMSym s;
VM_Sym_Procedure(& s, "my_procedure", 0);
VM_Sym_Reference(& s, 1, 10, 2);

/* Procedure definition */
VMSym s;
VM_Sym_Procedure(& s, "my_procedure", 1);
#endif
