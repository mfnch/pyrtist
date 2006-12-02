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
#include "virtmach.h"
#include "vmsym.h"

Task VM_Sym_Init(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
  VMSymTable *st = & vmp->sym_table;

  HT(& st->syms, VMSYM_SYM_HT_SIZE);
  Arr_New(& st->defs, sizeof(VMSym), VMSYM_DEF_ARR_SIZE);
  Arr_New(& st->refs, sizeof(VMSym), VMSYM_REF_ARR_SIZE);

  return Success;
#if 0
  Hashtable *ht;
  HashItem *hi;

  (void) HT_New(& ht, 5, (HashFunction) NULL, (HashComparison) NULL);
  (void) HT_Insert(ht, "Ciao", 4);
  (void) HT_Insert(ht, "Matteo", 6);
  (void) HT_Insert(ht, "Franchin", 8);
  (void) HT_Insert(ht, "questo", 6);
  (void) HT_Insert(ht, "e'", 2);
  (void) HT_Insert(ht, "il", 2);
  (void) HT_Insert(ht, "mio", 3);
  (void) HT_Insert(ht, "nome", 4);
  (void) HT_Insert(ht, "e questa e' una piccola frase.", 30);
  (void) HT_Insert(ht, "due parole", 10);
  HT_Statistics(ht, stdout);
  if ( HT_Find(ht, "Matteo", 6, & hi) ) {
    printf("Item found\n");
  } else {
    printf("Item not found\n");
  }
  exit(EXIT_SUCCESS);
#endif
}

void VM_Sym_Destroy(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
  VMSymTable *st = & vmp->sym_table;
  HT_Destroy(st->syms);
  Arr_Destroy(st->defs);
  Arr_Destroy(st->refs);
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
