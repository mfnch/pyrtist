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
#include "virtmach.h"
#include "vmproc.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "registers.h"
#include "expr.h"
#include "compiler.h"
#include "autogen.h"

/******************************************************************************
 *            FUNZIONI DI GESTIONE DELLE BOX (APERTURA E CHIUSURA)            *
 ******************************************************************************/

static BoxStack box_stack, *bs = & box_stack;

void Box_Init(void) {
  BoxArr_Init(& bs->box, sizeof(Box), BOX_ARR_SIZE);
  bs->num_defs = 0;
  bs->cur_sheet = -1;
}

void Box_Destroy(void) {
  BoxArr_Finish(& bs->box);
}

Int Box_Num(void) {
  return BoxArr_Num_Items(& bs->box) - 1;
}

Int Box_Def_Num(void) {return bs->num_defs;}

static Task Box_Def_Prepare(UInt head_sym_num) {
  Int num_reg[NUM_TYPES], num_var[NUM_TYPES];
  RegLVar_Get_Nums(& cmp->ra, num_reg, num_var);
  TASK( VM_Sym_Def_Proc_Head(cmp_vm, head_sym_num, num_var, num_reg) );
  return Success;
}

Task Box_Main_Begin(void) {
  Box *b;
  /* Sets the output for the compiled code */
  UInt main_sheet;
  TASK( VM_Proc_Code_New(cmp_vm, & main_sheet) );
  BoxVM_Proc_Target_Set(cmp_vm, main_sheet);
  bs->cur_sheet = main_sheet;
  TASK( Box_Instance_Begin((Expr *) NULL, 1) );
  b = (Box *) BoxArr_Last_Item_Ptr(& bs->box);
  TASK( VM_Sym_Proc_Head(cmp_vm, & b->head_sym_num) );
  return Success;
}

void Box_Main_End(void) {
  UInt level = Box_Num();
  Box *b = (Box *) BoxArr_First_Item_Ptr(& bs->box);
  UInt head_sym_num = b->head_sym_num;
  if (level > 0) {
    MSG_ERROR("Missing %I closing %s!", level,
              level > 1 ? "braces" : "brace");
  }
  (void) Box_Instance_End((Expr *) NULL);
  VM_Assemble(cmp_vm, ASM_RET);
  (void) Box_Def_Prepare(head_sym_num);
}

#if 0
# OBSOLETE
int Box_Procedure_Search(Box *b, Int procedure, Type *prototype,
                         UInt *sym_num, BoxMsg verbosity) {
  Int p;
  Type dummy_prototype;

  if (prototype == NULL) prototype = & dummy_prototype;

  if (b->type == TYPE_VOID) {
    if (verbosity == BOX_MSG_VERBOSE)
      MSG_WARNING("[...] does not admit any procedures.");
    return 0;
  }

  /* Now we search for the procedure associated with *e */
  TS_Procedure_Inherited_Search(cmp->ts, & p, prototype, b->type, procedure,
                                b->is.second ? 2 : 1);

  /* If a suitable procedure does not exist, we create it now,
   * and we mark it as "undefined"
   */
  if (p == TYPE_NONE) {
    if (verbosity == BOX_MSG_VERBOSE)
      MSG_WARNING("Don't know how to use '%~s' expressions inside a '%~s' "
                  "box.", TS_Name_Get(cmp->ts, procedure),
                  TS_Name_Get(cmp->ts, b->type));
    return 0;

  } else {
    TS_Procedure_Sym_Num_Get(cmp->ts, sym_num, p);
    return 1;
  }
}
#endif

Task Box_Procedure_Quick_Call(Expr *child, BoxDepth depth, BoxMsg verbosity) {
  Box *b = Box_Get(depth);
  UInt sym_num;
  Type prototype, t, p;
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
    UInt jump_label;
    Expr_Cont_Get(& src, child);
    Cont_Move(& CONT_NEW_LREG(TYPE_INT, 0), & src);
    jump_label = child->type == TYPE_FOR ? b->label_begin : b->label_end;
    VM_Sym_Jc(cmp->vm, jump_label);
    return Success;
  }

  if (b->type == TYPE_VOID) {
    if (verbosity == BOX_MSG_VERBOSE)
      MSG_WARNING("[...] does not admit any procedures.");
    return Success;
  }

  /* Just to allow the automatic creation of destructors and co. */
  Auto_Acknowledge_Call(b->type, child->type, b->is.second ? 2 : 1);

  /* Now we search for the procedure associated with *child */
  TS_Procedure_Inherited_Search(cmp->ts, & p, & prototype,
                                b->type, child->type, b->is.second ? 2 : 1);

  if (p == TYPE_NONE) {
    if (verbosity == BOX_MSG_SILENT)
      return Success;
    MSG_WARNING("Don't know how to use '%~s' expressions inside a '%~s' box.",
                TS_Name_Get(cmp->ts, child->type),
                TS_Name_Get(cmp->ts, b->type));
    return Success;
  }

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

  TS_Procedure_Sym_Num_Get(cmp->ts, & sym_num, p);
  return VM_Sym_Call(cmp_vm, sym_num);
}

Task Box_Procedure_Call(Expr *parent, Expr *child, int kind,
                        BoxMsg verbosity) {
  UInt sym_num;
  Type prototype, p;

  /* First of all we check the attributes of the expression *child */
  assert(!child->is.ignore && child->is.typed);

  if (parent->type == TYPE_VOID) {
    if (verbosity == BOX_MSG_VERBOSE)
      MSG_WARNING("[...] does not admit any procedures.");
    return Success;
  }

  /* Just to allow the automatic creation of destructors and co. */
  Auto_Acknowledge_Call(parent->type, child->type, kind);

  /* Now we search for the procedure associated with *child */
  TS_Procedure_Inherited_Search(cmp->ts, & p, & prototype,
                                parent->type, child->type, kind);

  if (p == TYPE_NONE) {
    if (verbosity == BOX_MSG_SILENT)
      return Success;
    MSG_WARNING("Cannot find procedure '%~s' of '%~s'.",
                TS_Name_Get(cmp->ts, child->type),
                TS_Name_Get(cmp->ts, parent->type));
    return Success;
  }

  /* Now we compile the procedure */
  /* We pass the argument of the procedure if its size is > 0 */
  if (Tym_Type_Size(child->type) > 0) {
    if ( prototype != TYPE_NONE ) {
      /* The argument must be converted first! */
      TASK( Cmp_Expr_Expand(prototype, child) );
    }

    TASK( Cmp_Expr_To_Ptr(child, CAT_GREG, (Int) 2, 0) );
  }

  /* We pass the box which is the parent of the procedure */
  if ( parent->is.value ) {
    if (Tym_Type_Size(parent->resolved) > 0) {
      TASK( Cmp_Expr_Container_Change(parent, CONTAINER_ARG(1)) );
    }
  }

  TS_Procedure_Sym_Num_Get(cmp->ts, & sym_num, p);
  return VM_Sym_Call(cmp_vm, sym_num);
}

Task Box_Procedure_Call_Void(Expr *parent, Type child, int kind,
                             BoxMsg verbosity) {
  Expr child_e;
  Expr_New_Type(& child_e, child);
  TASK( Box_Procedure_Call(parent, & child_e, kind, verbosity) );
  (void) Cmp_Expr_Destroy_Tmp(& child_e);
  return Success;
}



/* This function calls a procedure without value, as (;), ([) or (]).
 */
Task Box_Procedure_Quick_Call_Void(Type type, BoxDepth depth,
                                   BoxMsg verbosity) {
  Expr e;
  Expr_New_Type(& e, type);
  TASK( Box_Procedure_Quick_Call(& e, depth, verbosity) );
  (void) Cmp_Expr_Destroy_Tmp(& e);
  return Success;
}

Task Box_Procedure_Begin(Type parent, Type child, Type procedure) {
  UInt new_sheet;
  Box b, *b_ptr;

  /* Create a new procedure where to write the code
   * and set it as the target of code generation.
   */
  TASK( VM_Proc_Code_New(cmp_vm, & new_sheet) );
  BoxVM_Proc_Target_Set(cmp_vm, new_sheet);

  /* Create a new register frame for the new piece of code:
   * allocation of registers should start from scratch!
   */
  Reg_Frame_Push(& cmp->ra);

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
   * We create two expressions which will be used to refer
   * to these objects.
   */
  Expr_Parent_And_Child(& b.parent, & b.child, parent, child);

  /* We finally collect all the other data about the box and we push
   * it in the array of opened boxes.
   */
  b.is.second = 0;
  b.syms = NULL;
  b.type = procedure;
  b.is.definition = 1;
  b.sheet = new_sheet;
  BoxArr_Push(& bs->box, & b);
  bs->cur_sheet = new_sheet;
  ++bs->num_defs;

  /* Used to create jump labels for If and For */
  b_ptr = (Box *) BoxArr_Last_Item_Ptr(& bs->box);
  TASK( VM_Sym_New_Label_Here(cmp_vm, & b_ptr->label_begin) );
  b_ptr->label_end = VM_Sym_New_Label(cmp_vm);
  return Success;
}

Task Box_Procedure_End(UInt *call_num) {
  Box *b;
  UInt my_call_num;

  b = (Box *) BoxArr_Last_Item_Ptr(& bs->box);
  assert(b->is.definition);

  /* Cancello le labels che puntano all'inizio ed alla fine della box */
  TASK( VM_Sym_Destroy_Label(cmp_vm, b->label_begin) );
  TASK( VM_Sym_Def_Label_Here(cmp_vm, b->label_end) );
  TASK( VM_Sym_Destroy_Label(cmp_vm, b->label_end) );

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

  /* Adjust register allocation instructions at the beginning
   * of the procedure
   */
  TASK(Box_Def_Prepare(b->head_sym_num));

  /* We finally install the code for the procedure */
  VM_Proc_Install_Code(cmp_vm, & my_call_num, b->sheet,
                       "(noname)", Tym_Type_Name(b->type));
  /* And define the symbol */
  Reg_Frame_Pop(& cmp->ra);

  /* Restore to the state before the call to Box_Procedure_Begin */
  BoxArr_Pop(& bs->box, NULL);
  if (BoxArr_Num_Items(& bs->box) > 0) {
    b = (Box *) BoxArr_Last_Item_Ptr(& bs->box);
    BoxVM_Proc_Target_Set(cmp_vm, b->sheet);
    bs->cur_sheet = b->sheet;
  }
  --bs->num_defs;
  if (call_num != NULL) *call_num = my_call_num;
  return Success;
}


Task Box_Def_Begin(Type parent, Type child, int kind) {
  UInt sym_num;
  Type procedure;

  sym_num = VM_Sym_New_Call(cmp_vm);
  procedure = TS_Procedure_Def(child, kind, parent, sym_num);
  if (procedure == TYPE_NONE) return Failed;
  return Box_Procedure_Begin(parent, child, procedure);
}

Task Box_Def_End(void) {
  Box *b = Box_Get(0);
  UInt sym_num, call_num;

  /* We first register the procedure for the VM: the VM knows about it */
  TASK( Box_Procedure_End(& call_num) );

  /* We then register the procedure for the compiler: Box programs will
   * be able to link agains it.
   */
  TS_Procedure_Sym_Num_Get(cmp->ts, & sym_num, b->type);
  TASK( VM_Sym_Def_Call(cmp_vm, sym_num, call_num) );
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
  b.sheet = bs->cur_sheet; /* The procedure sheet number where we are now */
  BoxArr_Push(& bs->box, & b);

  if (e != NULL)
    (void) Box_Procedure_Quick_Call_Void(TYPE_OPEN, BOX_DEPTH_UPPER,
                                         BOX_MSG_SILENT);

  /* Creo le labels che puntano all'inizio ed alla fine della box */
  b_ptr = (Box *) BoxArr_Last_Item_Ptr(& bs->box);
  TASK( VM_Sym_New_Label_Here(cmp_vm, & b_ptr->label_begin) );
  b_ptr->label_end = VM_Sym_New_Label(cmp_vm);
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
  if (e != NULL) {
    (void) Box_Procedure_Quick_Call_Void(TYPE_CLOSE, BOX_DEPTH_UPPER,
                                         BOX_MSG_SILENT);
    e->is.release = 1;
  }

  /* Eseguo dei controlli di sicurezza */
  if (BoxArr_Num_Items(& bs->box) < 1) {
    MSG_ERROR("Cannot close the box. All boxes have already been closed.");
    return Failed;
  }

  /* Cancello le variabili esplicite della sessione */
  {
    Box *box = (Box *) BoxArr_Last_Item_Ptr(& bs->box);
    Symbol *s, *next;

    /* Cancello le labels che puntano all'inizio ed alla fine della box */
    TASK( VM_Sym_Destroy_Label(cmp_vm, box->label_begin) );
    TASK( VM_Sym_Def_Label_Here(cmp_vm, box->label_end) );
    TASK( VM_Sym_Destroy_Label(cmp_vm, box->label_end) );

    for (s = box->syms; s != (Symbol *) NULL;) {
      next = s->brother;
      TASK( Cmp_Expr_Destroy(& (s->value), 1) );
      Sym_Symbol_Delete(s);
      s = next;
    }
  }

  BoxArr_Pop(& bs->box, NULL);
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

  assert(depth >= 0);

  max_depth = BoxArr_Num_Items(& bs->box);
  if (depth >= max_depth) {
    MSG_ERROR("Indirizzo di box troppo profondo.");
    return -1;
  }

  box = (Box *) BoxArr_Last_Item_Ptr(& bs->box) - depth;
  for (d = depth; d < max_depth; d++) {
#if 0
    printf("Profondita': "SInt" -- Tipo: '%s'\n",
           d, Tym_Type_Name(box->parent.type));
#endif
    if (box->type == type) return d; /* BUG: bad type comparison */
    --box;
  }
  return -1;
}

/* This function returns the pointer to the structure Box
 * corresponding to the box with depth 'depth'.
 */
Box *Box_Get(BoxDepth depth) {
  Int max_depth;
  if (depth < BOX_DEPTH_UPPER) depth = BOX_DEPTH_UPPER;
  max_depth = BoxArr_Num_Items(& bs->box);
  assert(depth < max_depth);
  return (Box *) BoxArr_Last_Item_Ptr(& bs->box) - depth;
}

Task Box_Parent_Get(Expr *e_parent, BoxDepth depth) {
  Box *b = Box_Get(depth);
  *e_parent = b->parent;
  e_parent->is.allocd = e_parent->is.release = 0;
  return Success;
}

Task Box_Child_Get(Expr *e_child, BoxDepth depth) {
  Box *b = Box_Get(depth);
  *e_child = b->child;
  return Success;
}

Int Box_Def_Depth(int n) {
  Int depth, max_depth;
  Box *box;

  assert(n >= 0);
  max_depth = BoxArr_Num_Items(& bs->box);
  box = (Box *) BoxArr_Last_Item_Ptr(& bs->box);
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
  b = (Box *) BoxArr_Item_Ptr(& bs->box, level + 1);
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
