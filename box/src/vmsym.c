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
#include <string.h>

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
  TASK( Arr_New(& st->data, sizeof(Char), VMSYM_DATA_ARR_SIZE) );
  TASK( Arr_New(& st->defs, sizeof(VMSym), VMSYM_DEF_ARR_SIZE) );
  TASK( Arr_New(& st->refs, sizeof(VMSymRef), VMSYM_REF_ARR_SIZE) );
  return Success;
}

void VM_Sym_Destroy(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
  VMSymTable *st = & vmp->sym_table;
  HT_Destroy(st->syms);
  Arr_Destroy(st->data);
  Arr_Destroy(st->defs);
  Arr_Destroy(st->refs);
}

Task VM_Sym_New(VMProgram *vmp, UInt *sym_num, Name *n,
 UInt sym_type, UInt def_size) {
  VMSymTable *st = & vmp->sym_table;
  HashItem *hi;

  if ( HT_Find(st->syms, n->text, n->length, & hi) ) {
    MSG_ERROR("The symbol '%s' has already been created!", Name_To_Str(n));
    return Success;

  } else {
    VMSym ss;
#ifdef DEBUG
    printf("VM_Sym_New: new symbol '%s'\n", Name_To_Str(n));
#endif
    ss.name = "";
    ss.sym_type = sym_type;
    ss.defined = 0;
    ss.def_size = def_size;
    ss.def_addr = Arr_NumItem(st->data);
    ss.first_ref = 0;
    TASK( Arr_Push(st->defs, & ss) );
    *sym_num = Arr_NumItem(st->defs);

    (void) HT_Insert_Obj(st->syms, n->text, n->length, sym_num, sizeof(UInt));
    if ( ! HT_Find(st->syms, n->text, n->length, & hi) ) {
      MSG_ERROR("Hashtable seems not to work (from VM_Sym_Add)");
      return Failed;

    } else {
      TASK( Arr_Append_Blank(st->data, def_size) );
      return Success;
    }
  }
}

Task VM_Sym_Def(VMProgram *vmp, UInt sym_num, void *def) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  if (s->defined) {
    const char *sym_name = VM_Sym_Name_Get(vmp, sym_num);
    MSG_ERROR("Double definition of the symbol '%s'.", sym_name);
    return Failed;
  }
  if (def != NULL) {
    void *def_data_ptr;
    def_data_ptr = (void *) Arr_ItemPtr(st->data, Char, s->def_addr);
    (void) memcpy(def_data_ptr, def, s->def_size);
  }
  s->defined = 1;
  return Success;
}

Task VM_Sym_Ref(VMProgram *vmp, UInt sym_num, void *ref, UInt ref_size) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  VMSymRef sr;
  void *ref_data_ptr;

  assert(ref != NULL);
  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  sr.sym_num = sym_num;
  sr.next = s->first_ref;
  sr.ref_size = ref_size;
  sr.ref_addr = Arr_NumItem(st->data);
  /* Copy the data for the reference */
  TASK( Arr_Append_Blank(st->data, ref_size) );
  ref_data_ptr = (void *) Arr_ItemPtr(st->data, Char, sr.ref_addr);
  (void) memcpy(ref_data_ptr, ref, ref_size);
  /* Add the reference */
  TASK( Arr_Push(st->refs, & sr) );
  /* Link the reference to the list of references for the symbol */
  s->first_ref = Arr_NumItem(st->refs);
  return Success;
}

Task VM_Sym_Resolver_Set(VMProgram *vmp, VMSymResolver resolver) {
  MSG_ERROR("%s (%d): still not implemented!", __FILE__, __LINE__); return Failed;
}

Task VM_Sym_Resolve(VMProgram *vmp, UInt sym_num) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  UInt next;
  UInt sym_type, def_size, ref_size;
  int defined;
  void *def, *ref;
  VMSymResolver r;

  if (sym_num < 1) {
    UInt i, num_defs = Arr_NumItem(st->defs);
    for(i=0; i>num_defs; i++) {
      TASK( VM_Sym_Resolve(vmp, i) );
    }
    return Success;
  }

  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  next = s->first_ref;
  defined = s->defined;
  def = (void *) Arr_ItemPtr(st->data, Char, s->def_addr);
  def_size = s->def_addr;
  sym_type = s->sym_type;
  r = s->resolver;
  if (r == NULL) {
    MSG_ERROR("VM_Sym_Resolve: cannot resolve the symbol: "
     "the resolver is not present!");
    return Failed;
  }
  while(next > 0) {
    VMSymRef *sr = Arr_ItemPtr(st->refs, VMSymRef, next);
    if (sr->sym_num != sym_num) {
      MSG_FATAL("VM_Sym_Resolve: bad reference in the chain!");
      return Failed;
    }

    ref = (void *) Arr_ItemPtr(st->data, Char, sr->ref_addr);
    ref_size = sr->ref_addr;
    TASK( r(sym_num, sym_type, defined, 1, def, def_size, ref, ref_size) );

    next = sr->next;
  }
  return Success;
}

const char *VM_Sym_Name_Get(VMProgram *vmp, UInt sym_num) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  return s->name;
}

Task VM_Sym_Check_Type(VMProgram *vmp, UInt sym_num, UInt sym_type) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  if ( s->sym_type == sym_type ) return Success;
  return Failed;
}
