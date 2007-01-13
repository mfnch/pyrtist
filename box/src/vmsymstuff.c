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

#include "types.h"
#include "virtmach.h"
#include "vmsym.h"
#include "vmsymstuff.h"

/*** call references ********************************************************/
/* This is the function which assembles the code for the function call */
static Task Assemble_Call(VMProgram *vmp, UInt sym_num, UInt sym_type,
 int defined, void *def, UInt def_size, void *ref, UInt ref_size) {
  UInt call_num = 0;
  assert(sym_type == VM_SYM_CALL);
  if (defined && def != NULL) {
    assert(def_size=sizeof(UInt));
    call_num = *((UInt *) def);
  }
  VM_Assemble(vmp, ASM_CALL_I, CAT_IMM, call_num);
  return Success;
}

Task VM_Sym_New_Call(VMProgram *vmp, UInt *sym_num) {
  return VM_Sym_New(vmp, sym_num, VM_SYM_CALL, sizeof(UInt));
}

Task VM_Sym_Def_Call(VMProgram *vmp, UInt sym_num, UInt proc_num) {
  return VM_Sym_Def(vmp, sym_num, & proc_num);
}

Task VM_Sym_Call(VMProgram *vmp, UInt sym_num) {
  return VM_Sym_Code_Ref(vmp, sym_num, Assemble_Call);
}
