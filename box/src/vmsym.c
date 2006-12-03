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

#include "defaults.h"
#include "types.h"
#include "str.h"
#include "messages.h"
#include "virtmach.h"
#include "vmsym.h"

Task VM_Sym_Init(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
  VMSymTable *st = & vmp->sym_table;

  HT(& st->syms, VMSYM_SYM_HT_SIZE);
  TASK( Arr_New(& st->defs, sizeof(VMSym), VMSYM_DEF_ARR_SIZE) );
  TASK( Arr_New(& st->refs, sizeof(VMSym), VMSYM_REF_ARR_SIZE) );
  TASK( Arr_New(& st->names, sizeof(char), VMSYS_NAME_ARR_SIZE) );
  return Success;
}

void VM_Sym_Destroy(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
  VMSymTable *st = & vmp->sym_table;
  HT_Destroy(st->syms);
  Arr_Destroy(st->defs);
  Arr_Destroy(st->refs);
  Arr_Destroy(st->names);
}

void VM_Sym_Procedure(VMSym *s, Name *name, int sheet) {
  assert(s != (VMSym *) NULL);
  assert(sheet >= 0);
  s->is_definition = (sheet > 0);
  s->name = *name;
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
  VMSymTable *st = & vmp->sym_table;
  HashItem *hi;

  /* If the name of the symbol is not present in the hash table,
   * we insert it: this has to be done anyway!
   */
  if ( ! HT_Find(st->syms, s->name.text, s->name.length, & hi) ) {
    VMSymStuff s_stuff;
#ifdef DEBUG
    printf("VM_Sym_Add: Inserting new symbol '%s'\n", Name_To_Str(& s->name));
#endif
    s_stuff.def = -1;
    s_stuff.ref = -1;
    (void) HT_Insert_Obj(st->syms, s->name.text, s->name.length,
      (void *) & s_stuff, sizeof(s_stuff));
    if ( ! HT_Find(st->syms, s->name.text, s->name.length, & hi) ) {
      MSG_ERROR("Hashtable seems not to work (from VM_Sym_Add)");
      return Failed;
    }
  }

  if ( s->is_definition ) {
    VMSymStuff *s_stuff = (VMSymStuff *) hi->object;
    VMSym *s_in;
    if (s_stuff->def < 1) {
      MSG_ERROR("Double definition of the symbol '%s'",
       Name_To_Str(& s->name));
      return Failed;
    }
    TASK( Arr_Push(st->defs, s) );
    s_in = Arr_LastItemPtr(st->defs, VMSym);
    s_in->name.length = 0;
    s_stuff->def = Arr_NumItems(st->defs);
    return Success;

  } else {
    VMSymStuff *s_stuff = (VMSymStuff *) hi->object;
    VMSym *s_in;
    TASK( Arr_Push(st->refs, s) );
    s_in = Arr_LastItemPtr(st->refs, VMSym);
    s_in->next = s_stuff->ref;
    s_stuff->ref = Arr_NumItems(st->refs);
    s_in->name.length = 0;
    return Success;
  }
}

Task VM_Sym_Link(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
  return Failed;
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
