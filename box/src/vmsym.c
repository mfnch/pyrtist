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
#include "vmproc.h"

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

Task VM_Sym_New(VMProgram *vmp, UInt *sym_num, UInt sym_type, UInt def_size) {
  VMSymTable *st = & vmp->sym_table;
  HashItem *hi;
  VMSym ss;

#ifdef DEBUG
  printf("VM_Sym_New: new symbol '%s'\n", Name_To_Str(n));
#endif
  ss.name.length = 0;
  ss.name.text = (char *) NULL;
  ss.sym_type = sym_type;
  ss.defined = 0;
  ss.def_size = def_size;
  ss.def_addr = Arr_NumItem(st->data);
  ss.first_ref = 0;
  ss.resolver = (VMSymResolver) NULL;
  TASK( Arr_Push(st->defs, & ss) );
  *sym_num = Arr_NumItem(st->defs);
  TASK( Arr_Append_Blank(st->data, def_size) );
  return Success;
}

Task VM_Sym_Name_Set(VMProgram *vmp, UInt sym_num, Name *n) {
  VMSymTable *st = & vmp->sym_table;
  HashItem *hi;
  VMSym *s;

  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  if (s->name.length != 0) {
    MSG_ERROR("This symbol has already been given a name!");
    return Failed;
  }

  if ( HT_Find(st->syms, n->text, n->length, & hi) ) {
    MSG_ERROR("Another symbol exists having the name '%s'!", Name_To_Str(n));
    return Failed;
  }

  (void) HT_Insert_Obj(st->syms, n->text, n->length, sym_num, sizeof(UInt));
  if ( ! HT_Find(st->syms, n->text, n->length, & hi) ) {
    MSG_ERROR("Hashtable seems not to work (from VM_Sym_Add)");
    return Failed;
  }

  s->name.text = hi->key;
  s->name.length = hi->key_size;
  return Success;
}

const char *VM_Sym_Name_Get(VMProgram *vmp, UInt sym_num) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  return s->name;
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
  sr.resolved = 0;
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

Task VM_Sym_Resolver_Set(VMProgram *vmp, UInt sym_num, VMSymResolver r) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;

  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  s->resolver = r;
  return Success;
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
    for(i=1; i<=num_defs; i++) {
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
    ref_size = sr->ref_size;
    if (!sr->resolved) {
      TASK( r(vmp, sym_num, sym_type, defined, def, def_size, ref, ref_size) );
      sr->resolved = 1;
    }

    next = sr->next;
  }
  return Success;
}

void VM_Sym_Table_Print(VMProgram *vmp, FILE *out, UInt sym_num) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  UInt next, ref_num;

  if (sym_num < 1) {
    UInt i, num_defs = Arr_NumItem(st->defs);
    fprintf(out, "The table contains "SUInt" symbols\n", num_defs);
    for(i=1; i<=num_defs; i++) {
      VM_Sym_Table_Print(vmp, out, i);
    }
    return;
  }

  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  next = s->first_ref;
  ref_num = 1;

  fprintf(out,
   "Symbol ID = "SUInt"; name = '%s'; type = "SUInt"; resolver = %p; "
   "defined = %d, def_addr = "SUInt", def_size = "SUInt"\n",
   sym_num, VM_Sym_Name_Get(vmp, sym_num), s->sym_type, s->resolver,
   s->defined, s->def_addr, s->def_size);

  while(next > 0) {
    VMSymRef *sr = Arr_ItemPtr(st->refs, VMSymRef, next);
    if (sr->sym_num != sym_num) {
      fprintf(out, "Bad reference in the chain!");
      return;
    }

    fprintf(out,
     "  Reference number = "SUInt"; ref_addr = "SUInt"; "
     "ref_size = "SUInt"; resolved = %d\n",
     ref_num, sr->ref_addr,
     sr->ref_size, sr->resolved);

    next = sr->next;
    ++ref_num;
  }
}

Task VM_Sym_Check_Type(VMProgram *vmp, UInt sym_num, UInt sym_type) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  if ( s->sym_type == sym_type ) return Success;
  return Failed;
}

/****************************************************************************/

static Task code_generator(VMProgram *vmp, UInt sym_num, UInt sym_type,
 int defined, void *def, UInt def_size, void *ref, UInt ref_size) {
  VMSymCodeRef *ref_head = (VMSymCodeRef *) ref;
  void *ref_tail = ref + sizeof(VMSymCodeRef);
  UInt ref_tail_size = ref_size - sizeof(VMSymCodeRef);
  VMProcTable *pt = & vmp->proc_table;
  VMProc *tmp_proc;
  UInt saved_proc_num;

  saved_proc_num = VM_Proc_Target_Get(vmp);
  TASK( VM_Proc_Empty(vmp, pt->tmp_proc) );
  TASK( VM_Proc_Target_Set(vmp, pt->tmp_proc) );
  tmp_proc = pt->target_proc;
  /* Call the procedure here! */
  TASK( ref_head->code_gen(vmp, sym_num, sym_type, defined,
    def, def_size, ref_tail, ref_tail_size) );
  TASK( VM_Proc_Target_Set(vmp, ref_head->proc_num) );
  /* Replace the referencing code with the generated code */
  {
    void *src = Arr_FirstItemPtr(tmp_proc->code, void);
    int src_size = Arr_NumItem(tmp_proc->code);
    Array *dest =  pt->target_proc->code; /* Destination sheet */
    int dest_pos = ref_head->pos + 1; /* NEED TO ADD 1 */
    if (src_size != ref_head->size) {
      MSG_ERROR("vmsym.c, code_generator: The code for the resolved "
        "reference does not match the space which was reserved for it!");
      return Failed;
    }
    TASK(Arr_Overwrite(dest, dest_pos, src, src_size));
  }
  TASK( VM_Proc_Target_Set(vmp, saved_proc_num) );
  return Success;
}

Task VM_Sym_Code_Ref(VMProgram *vmp, UInt sym_num, VMSymCodeGen code_gen) {
  VMSymTable *st = & vmp->sym_table;
  VMSym *s;
  void *def;
  VMProcTable *pt = & vmp->proc_table;
  VMSymCodeRef ref_data;

  s = Arr_ItemPtr(st->defs, VMSym, sym_num);
  def = (void *) Arr_ItemPtr(st->data, Char, s->def_addr);
  ref_data.code_gen = code_gen;
  ref_data.proc_num = pt->target_proc_num;
  ref_data.pos = Arr_NumItem(pt->target_proc->code);
  TASK( code_gen(vmp, sym_num, s->sym_type, s->defined, def, s->def_size, NULL, 0) );
  if (pt->target_proc_num != ref_data.proc_num) {
    MSG_ERROR("VM_Sym_Code_Ref: the function 'code_gen' must not change "
     "the current target for compilation!");
  }
  ref_data.size = Arr_NumItem(pt->target_proc->code) - ref_data.pos;
  TASK( VM_Sym_Ref(vmp, sym_num, & ref_data, sizeof(VMSymCodeRef)) );
  TASK( VM_Sym_Resolver_Set(vmp, sym_num, code_generator) );
  return Success;
}

#if 0
/* Usage for the function 'VM_Sym_Code_Ref' */

#  define CALL_TYPE 1

/* This is the function which assembles the code for the function call */
Task Assemble_Call(VMProgram *vmp, UInt sym_num, UInt sym_type,
 int defined, void *def, UInt def_size, void *ref, UInt ref_size) {
  UInt call_num = 0;
  assert(sym_type == CALL_TYPE);
  if (defined && def != NULL) {
    assert(def_size=sizeof(UInt));
    call_num = *((UInt *) def);
  }
  VM_Assemble(vmp, ASM_CALL_I, CAT_IMM, call_num);
  return Success;
}

int main(void) {
  UInt sym_num;
  VMProgram *vmp;
  /* Initialization of the VM goes here
   ...
   ...
   */

  /* Here we define that a symbol with name "my_proc" and type call_type=1 */
  (void) VM_Sym_New(vmp, & sym_num, NAME("my_proc"), CALL_TYPE, sizeof(UInt));
  /* Here we make a reference to it: at this point the assembled code for
   * "call 0" will be added to the current target procedure.
   * 0 is wrong, but we don't know what is the right number, because
   * the symbol is still not defined.
   */
  (void) VM_Sym_Code_Ref(vmp, sym_num, Assemble_Call);
  /* Here we define the symbol "my_proc" to be (UInt) 123.
   * Now we know that all the "call" instruction referencing
   * the symbol "my_proc" should be assembled with "call 123"
   */
  (void) VM_Sym_Def(vmp, sym_num, & ((UInt) 123));
  /* We can now resolve all the past references which were made when
   * we were not aware of the real value of "my_proc".
   * The code "call 0" will be replaced with "call 123"
   */
  (void) VM_Sym_Resolve_All(vmp);
  return 0;
}
#endif
