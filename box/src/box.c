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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
// #include "str.h"
#include "virtmach.h"
#include "vmproc.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "registers.h"
#include "expr.h"
#include "compiler.h"

/******************************************************************************
 *            FUNZIONI DI GESTIONE DELLE BOX (APERTURA E CHIUSURA)            *
 ******************************************************************************/

static BoxStack box_stack, *bs = & box_stack;

Task Box_Init(void) {
  TASK( Arr_New(& bs->box, sizeof(Box), BOX_ARR_SIZE) );
  bs->num_defs = 0;
  bs->cur_proc_num = -1;
  return Success;
}

void Box_Destroy(void) {
  Arr_Destroy(bs->box);
}

Int Box_Num(void) {
  return (Arr_NumItem(bs->box) - 1);
}

Int Box_Def_Num(void) {return bs->num_defs;}

static Task Box_Def_Prepare(UInt head_sym_num) {
  Int num_reg[NUM_TYPES], num_var[NUM_TYPES];
  RegLVar_Get_Nums(num_reg, num_var);
  TASK( VM_Sym_Def_Proc_Head(cmp_vm, head_sym_num, num_var, num_reg) );
  return Success;
}

Task Box_Main_Begin(void) {
  Box *b;
  /* Sets the output for the compiled code */
  UInt main_sheet;
  TASK( VM_Proc_Code_New(cmp_vm, & main_sheet) );
  TASK( VM_Proc_Target_Set(cmp_vm, main_sheet) );
  bs->cur_proc_num = main_sheet;
  TASK( Box_Instance_Begin((Expr *) NULL, 1) );
  b = Arr_LastItemPtr(bs->box, Box);
  TASK( VM_Sym_Proc_Head(cmp_vm, & b->head_sym_num) );
  return Success;
}

void Box_Main_End(void) {
  UInt level = Box_Num();
  Box *b = Arr_FirstItemPtr(bs->box, Box);
  UInt head_sym_num = b->head_sym_num;
  if (level > 0) {
    MSG_ERROR("Missing %I closing %s!", level,
              level > 1 ? "braces" : "brace");
  }
  (void) Box_Instance_End((Expr *) NULL);
  VM_Assemble(cmp_vm, ASM_RET);
  (void) Box_Def_Prepare(head_sym_num);
}

Task Box_Procedure_Search(int *found, Int procedure, BoxDepth depth,
                          Box **box, Int *prototype, UInt *sym_num,
                          int auto_define) {
  Box *b;
  Int p, dummy;

  *found = 0;
  if ( prototype == NULL ) prototype = & dummy; /* See later! */

  /* Now we use depth to identify the box, which is the parent
   * of the procedure
   */
  TASK( Box_Get(& b, depth) );
  if ( b->type == TYPE_VOID ) {
    MSG_WARNING("[...] does not admit any procedures.");
    return Failed;
  }
  *box = b;

  /* Now we search for the procedure associated with *e */
  /*p = Tym_Search_Procedure(procedure, b->is.second, b->type, prototype);*/
  TS_Procedure_Inherited_Search(cmp->ts, & p, prototype, b->type, procedure,
                                b->is.second ? 2 : 1);


  /* If a suitable procedure does not exist, we create it now,
   * and we mark it as "undefined"
   */
  if ( p == TYPE_NONE ) {
    if (! auto_define) return Success;

    MSG_ERROR("Don't know how to use '%~s' expressions inside a '%~s' box.",
              TS_Name_Get(cmp->ts, procedure), TS_Name_Get(cmp->ts, b->type));
    return Failed;
#if 0
    *prototype = TYPE_NONE;
    *asm_module = VM_Module_Next(cmp_vm);
    p = Tym_Def_Procedure(procedure, b->attr.second, b->type, *asm_module);
    if ( p == TYPE_NONE ) return Failed;
    {
      Int nm;
      TASK(VM_Module_Undefined(cmp_vm, & nm, Tym_Type_Name(p)));
      assert(nm == *asm_module);
    }
    *found = 1;
    return Success;
#endif

  } else {
    TS_Procedure_Sym_Num(cmp->ts, sym_num, p);
    *found = 1;
    return Success;
  }
}

Task Box_Procedure_Call(Expr *child, BoxDepth depth) {
  Box *b;
  UInt sym_num;
  Int prototype, t;
  int dummy = 0, *found = & dummy, auto_define = 0;
  Expr e_parent;

  /* First of all we check the attributes of the expression *child */
  if ( child->is.ignore ) return Success;
  if ( !child->is.typed ) {
    MSG_ERROR("This expression has no type!");
    return Failed;
  }

  t = Tym_Type_Resolve_Alias(child->type);
  if (t == TYPE_VOID) return Success;

  if (child->type == TYPE_IF || child->type == TYPE_FOR) {
    Cont src;
    Box *b;
    TASK( Box_Get(& b, depth) );
    Expr_Cont_Get(& src, child);
    Cont_Move(& CONT_NEW_LREG(TYPE_INT, 0), & src);
    VM_Label_Jump(cmp->vm,
                  child->type == TYPE_FOR ? b->label_begin : b->label_end,
                  1);
    return Success;
  }

  TASK( Box_Procedure_Search(found, child->type, depth, & b, & prototype,
                             & sym_num, auto_define) );

  if ( ! *found ) return Success;

  /* Now we compile the procedure */
  /* We pass the argument of the procedure if its size is > 0 */
  if (Tym_Type_Size(t) > 0) {
    if ( prototype != TYPE_NONE ) {
      /* The argument must be converted first! */
      TASK( Cmp_Expr_Expand(prototype, child) );
    }

    TASK( Cmp_Expr_To_Ptr(child, CAT_GREG, (Int) 2, 0) );
  }

  /* We pass the box which is the parent of the procedure */
  TASK( Box_Parent_Get(& e_parent, depth) );
  if ( e_parent.is.value ) {
    if (Tym_Type_Size(e_parent.resolved) > 0) {
      TASK( Cmp_Expr_Container_Change(& e_parent, CONTAINER_ARG(1)) );
    }
  }

  return VM_Sym_Call(cmp_vm, sym_num);
}

/* This function calls a procedure without value, as (;), ([) or (]).
 */
Task Box_Procedure_Call_Void(Type type, BoxDepth depth) {
  Expr e;
  Expr_New_Type(& e, type);
  TASK( Box_Procedure_Call(& e, depth) );
  (void) Cmp_Expr_Destroy_Tmp(& e);
  return Success;
}

Task Box_Def_Begin(Int proc_type) {
  UInt new_sheet;
  Box b, *b_ptr;

  /* Create a new procedure where to write the code
   * and set it as the target of code generation.
   */
  TASK( VM_Proc_Code_New(cmp_vm, & new_sheet) );
  TASK( VM_Proc_Target_Set(cmp_vm, new_sheet) );

  /* Create a new register frame for the new piece of code:
   * allocation of registers should start from scratch!
   */
  Reg_Frame_Push();

  /* Create the header of the procedure: the piece of code
   * where instructions such as "regi 1, 2" appears.
   * These instructions say to the virtual machine how many registers
   * will be used in the procedure. Currently these numbers are not known
   * therfore we use the vmsym module to register a new symbol, which
   * will used later to correct the header, once this information will
   * be known.
   */
  TASK( VM_Sym_Proc_Head(cmp_vm, & b.head_sym_num) );

  /* Now we get the parent and the child of the procedure.
   * We create two expression which will be used to refer
   * to these objects.
   */
  Expr_Parent_And_Child(& b.parent, & b.child, proc_type);

  /* We finally collect all the other data about the box and we push
   * it in the array of opened boxes.
   */
  b.is.second = 0;
  b.syms = NULL;
  b.type = proc_type;
  b.is.definition = 1;
  b.proc_num = new_sheet;
  /*Expr_New_Void(& b.parent);*/
  TASK(Arr_Push(bs->box, & b));
  bs->cur_proc_num = new_sheet;
  ++bs->num_defs;

  /* Creo le labels che puntano all'inizio ed alla fine della box */
  b_ptr = Arr_LastItemPtr(bs->box, Box);
  TASK( VM_Label_New_Here(cmp_vm, & b_ptr->label_begin) );
  TASK( VM_Label_New_Undef(cmp_vm, & b_ptr->label_end) );
  return Success;
}

Task Box_Def_End(void) {
  Box *b;
  Int proc_type;
  UInt sym_num, call_num, proc_num;

  b = Arr_LastItemPtr(bs->box, Box);
  assert(b->is.definition);

  /* Cancello le labels che puntano all'inizio ed alla fine della box */
  TASK( VM_Label_Destroy(cmp_vm, b->label_begin) );
  TASK( VM_Label_Define_Here(cmp_vm, b->label_end) );
  TASK( VM_Label_Destroy(cmp_vm, b->label_end) );

  /* Induce the VM to free all the object defined in the procedure */
  {
    Symbol *s, *next;

    for (next = b->syms; next != (Symbol *) NULL;) {
      s = next;
      next = next->brother;
      TASK( Cmp_Expr_Destroy(& (s->value), 1) );
      Sym_Symbol_Delete(s);
    }
  }

  VM_Assemble(cmp_vm, ASM_RET);
  TASK(Box_Def_Prepare(b->head_sym_num));
  proc_type = b->type;
  proc_num = b->proc_num;
  TS_Procedure_Sym_Num(cmp->ts, & sym_num, proc_type);
  /* We finally install the code for the procedure */
  TASK( VM_Proc_Install_Code(cmp_vm, & call_num, proc_num,
                             "(noname)", Tym_Type_Name(proc_type)) );
  /* And define the symbol */
  TASK( VM_Sym_Def_Call(cmp_vm, sym_num, call_num) );
  TASK( Reg_Frame_Pop() );

  TASK(Arr_Dec(bs->box));
  if (Arr_NumItem(bs->box) > 0) {
    b = Arr_LastItemPtr(bs->box, Box);
    TASK( VM_Proc_Target_Set(cmp_vm, b->proc_num) );
    bs->cur_proc_num = b->proc_num;
  }
  --bs->num_defs;
  return Success;
}

/* This function opens a new box for the expression *e.
 * If e == NULL the box is considered to be a simple untyped box.
 * If *e is a typed, but un-valued expression, then the creation of a new box
 * of type *e is started.
 * If *e has value, a modification-box for that expression is started.
 */
Task Box_Instance_Begin(Expr *e, int kind) {
  Box b, *b_ptr;

  Expr_New_Void(& b.child);
  if ( e == NULL ) {
    /* Si tratta di una box void */
    b.is.second = 0;
    b.syms = NULL;         /* Catena dei simboli figli */
    b.type = TYPE_VOID;
    Expr_New_Void(& b.parent);

  } else {
    if ( ! e->is.typed ) {
      MSG_ERROR("Cannot open the box: '%N' has no type!", & e->value.nm);
      Cmp_Expr_Destroy_Tmp(e);
      return Failed;
    }

    b.is.second = (kind == 2);
    if ( ! e->is.value ) {
      Expr_Container_New(e, e->type, CONTAINER_LREG_AUTO);
      Expr_Alloc(e);
      e->is.release = 0;
      b.is.second = 0;
    }

    /* Compilo il descrittore del nuovo esempio di sessione aperto */
    b.type = e->type;
    b.syms = NULL;        /* Catena dei simboli figli */
    b.parent = *e;         /* Valore della sessione */
  }

  /* Inserisce la nuova sessione */
  b.is.definition = 0; /* This is just an instance, not a definition */
  b.proc_num = bs->cur_proc_num; /* The procedure number where we are now */
  TASK(Arr_Push(bs->box, & b));

  if (e != (Expr *) NULL)
    TASK( Box_Procedure_Call_Void(TYPE_OPEN, BOX_DEPTH_UPPER) );

  /* Creo le labels che puntano all'inizio ed alla fine della box */
  b_ptr = Arr_LastItemPtr(bs->box, Box);
  TASK( VM_Label_New_Here(cmp_vm, & b_ptr->label_begin) );
  TASK( VM_Label_New_Undef(cmp_vm, & b_ptr->label_end) );
  return Success;
}

/** Conclude l'ultimo esempio di sessione aperto, la lista
 * delle variabili esplicite viene analizzata, per ognuna di queste
 * viene eseguita l'azione final_action, dopodiche' viene eliminata.
 */
Task Box_Instance_End(Expr *e) {
  /* Il registro occupato per la sessione ora diventa un registro normale
   * e puo' essere liberato!
   */
  if ( e != NULL ) {
    TASK( Box_Procedure_Call_Void(TYPE_CLOSE, BOX_DEPTH_UPPER) );
    e->is.release = 1;
  }

  /* Eseguo dei controlli di sicurezza */
  if ( bs->box == NULL ) {
    MSG_ERROR("Nessuna box da terminare");
    return Failed;
  }

  if ( Arr_NumItem(bs->box) < 1 ) {
    MSG_ERROR("Nessuna box da terminare");
    return Failed;
  }

  /* Cancello le variabili esplicite della sessione */
  {
    Box *box = Arr_LastItemPtr(bs->box, Box);
    Symbol *s, *next;

    /* Cancello le labels che puntano all'inizio ed alla fine della box */
    TASK( VM_Label_Destroy(cmp_vm, box->label_begin) );
    TASK( VM_Label_Define_Here(cmp_vm, box->label_end) );
    TASK( VM_Label_Destroy(cmp_vm, box->label_end) );

    for (s = box->syms; s != (Symbol *) NULL;) {
      next = s->brother;
      TASK( Cmp_Expr_Destroy(& (s->value), 1) );
      Sym_Symbol_Delete(s);
      s = next;
    }
  }

  TASK(Arr_Dec(bs->box));
  return Success;
}

/** Search among the opened boxes with depth greater than 'depth'
 * the first one with type 'type'. If 'depth == BOX_DEPTH_UPPER' search
 * starting from the last opened box (the current one). If 'depth == 1'
 * starts from the next to last and so on...
 * Returns the depth of the found box or -1 if the such a box has not been
 * found.
 */
Int Box_Search_Opened(Int type, BoxDepth depth) {
  Int max_depth, d;
  Box *box;

  assert(bs->box != NULL && depth >= 0);

  max_depth = Arr_NumItem(bs->box);
  if ( depth >= max_depth ) {
    MSG_ERROR("Indirizzo di box troppo profondo.");
    return -1;
  }

  box = Arr_LastItemPtr(bs->box, Box) - depth;
  for (d = depth; d < max_depth; d++) {
#if 0
    printf("Profondita': "SInt" -- Tipo: '%s'\n",
     d, Tym_Type_Name(box->parent.type));
#endif
    if ( box->type == type ) return d; /* BUG: bad type comparison */
    --box;
  }
  return -1;
}

/* This function returns the pointer to the structure Box
 * corresponding to the box with depth 'depth'.
 */
Task Box_Get(Box **box, BoxDepth depth) {
  Int max_depth;
  assert(bs->box != NULL);
  if (depth < BOX_DEPTH_UPPER) depth = BOX_DEPTH_UPPER;
  max_depth = Arr_NumItem(bs->box);
  if (depth >= max_depth) {
    MSG_ERROR("Invalid box depth.");
    return Failed;
  }
  *box = Arr_LastItemPtr(bs->box, Box) - depth;
  return Success;
}

Task Box_Parent_Get(Expr *e_parent, BoxDepth depth) {
  Box *b;
  TASK( Box_Get(& b, depth) );
  *e_parent = b->parent;
  e_parent->is.allocd = e_parent->is.release = 0;
  return Success;
}

Task Box_Child_Get(Expr *e_child, BoxDepth depth) {
  Box *b;
  TASK( Box_Get(& b, depth) );
  *e_child = b->child;
  return Success;
}

Int Box_Def_Depth(int n) {
  Int depth, max_depth;
  Box *box;

  assert(bs->box != NULL && n >= 0);
  max_depth = Arr_NumItems(bs->box);
  box = Arr_LastItemPtr(bs->box, Box);
  for(depth = 0; depth < max_depth; depth++) {
    if (box->is.definition) {
      if (n == 0) return depth;
      --n;
    }
    --box;
  }

  return -1;
}

Task Box_NParent_Get(Expr *parent, Int level, Int depth) {
  if (depth < 0) depth = Box_Def_Depth(0);
  switch(level) {
  case 1:
    return Box_Child_Get(parent, depth);
  case 2:
    return Box_Parent_Get(parent, depth);
  default:
    if (level > 2) {
      Expr up_parent;
      TASK( Box_NParent_Get(& up_parent, level-1, depth) );
      return Expr_Subtype_Get_Parent(parent, & up_parent);

    } else {
      MSG_FATAL("Box_NParent_Get: level < 1");
      return Failed;
    }
  }
}

/********************************[DEFINIZIONE]********************************/

/*  Questa funzione definisce un nuovo simbolo di nome *nm,
 *  attribuendolo alla box di profondita' depth.
 */
Task Sym_Explicit_New(Symbol **sym, Name *nm, BoxDepth depth) {
  Int level;
  Symbol *s;
  Box *b;

  /* Controllo che non esista un simbolo omonimo gia' definito */
  assert(depth >= 0 && depth <= Box_Num());
  s = Sym_Explicit_Find(nm, depth, EXACT_DEPTH);
  if (s != NULL) {
    MSG_ERROR("A symbol with name '%N' has already been defined", nm);
    return Failed;
  }

  /* Introduco il nuovo simbolo nella lista dei simboli correnti */
  s = Sym_Symbol_New(nm);
  if (s == NULL) return Failed;

  /* Collego il nuovo simbolo alla lista delle variabili esplicite della
   * box corrente (in modo da eliminarle alla chiusura della box).
   */
  level = Box_Num() - depth;
  b = Arr_ItemPtr(bs->box, Box, level + 1);
  s->brother = b->syms;
  b->syms = s;

  /* Inizializzo il simbolo */
  s->symattr.is_explicit = 1;
  s->child = NULL;
  s->parent.exp = level;
  s->symtype = VARIABLE;
  *sym = s;
  return Success;
}
