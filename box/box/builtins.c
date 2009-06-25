/****************************************************************************
 * Copyright (C) 2008, 2009 by Matteo Franchin                              *
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

#define USE_PRIVILEGED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "str.h"
#include "virtmach.h"
#include "vmproc.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "registers.h"
#include "builtins.h"
#include "bltinstr.h"
#include "bltinio.h"

#if 0
static Task Print_String(VMProgram *vmp);
static Task Print_NewLine(VMProgram *vmp);
static Task Exit_Int(VMProgram *vmp);
/* Functions for conversions */
static Task Conv_2RealNum_to_Point(VMProgram *vmp);
static Task Char_Char(VMProgram *vmp);
static Task Char_Int(VMProgram *vmp);
static Task Char_Real(VMProgram *vmp);
static Task Int_IntNum(VMProgram *vmp);
static Task Int_RealNum(VMProgram *vmp);
static Task Real_RealNum(VMProgram *vmp);
static Task Point_RealNumCouple(VMProgram *vmp);
static Task If_IntNum(VMProgram *vmp);
static Task For_IntNum(VMProgram *vmp);
/* Mathematical functions */
static Task Min_Open(VMProgram *vmp);
static Task Min_RealNum(VMProgram *vmp);
static Task Max_Open(VMProgram *vmp);
static Task Max_RealNum(VMProgram *vmp);
static Task Vec_RealNum(VMProgram *vmp);

/******************************************************************************
 * Le procedure che seguono servono a inizializzare/resettare il compilatore. *
 ******************************************************************************/

/* Con queste macro abbrevio la definizione delle operazioni */
#define NEW_OPERATOR(o, str) \
  status |= ( (cmp_opr.o = Cmp_Operator_New(str)) == NULL );

#define ADD_OPERATION(o, a1, a2, rs, asm, attr_c, attr_i, attr_a) \
  opn = Cmp_Operation_Add(cmp_opr.o, a1, a2, rs); \
  status |= ( opn == NULL ); \
  opn->is.commutative = attr_c; \
  opn->is.intrinsic = attr_i; \
  opn->is.assignment = attr_a; \
  opn->is.link = 0; \
  opn->asm_code = asm;

/* Defines some types and some basic boxes.
 */
Task Blt_Define_All(void) {
  TASK( Blt_Define_Basics() );
  TASK( Blt_Define_Math() );
  TASK( Blt_Define_Print() );
  TASK( Blt_Define_Sys() );
  return Success;
}

static Task Blt_Define_Types(void) {
  int i;

  struct {
    Name name;
    Int size;
    Type type;

  } intrinsic[] = {
    {NAME("Void"),     0,             TYPE_VOID},
    {NAME("([)"),      0,             TYPE_OPEN},
    {NAME("(])"),      0,             TYPE_CLOSE},
    {NAME("(;)"),      0,             TYPE_PAUSE},
    {NAME("(\\)"),     0,             TYPE_DESTROY},
    {NAME("(?)"),      0,             TYPE_ITER},
    {NAME("Ptr"),      sizeof(Ptr),   TYPE_PTR},
    {NAME(""),        -1,             TYPE_NONE}
  };

  struct {
    Name name;
    Type target;
    Type type;

  } alias[] = {
    {NAME("If"),   TYPE_INT,  TYPE_IF},
    {NAME("For"),  TYPE_INT,  TYPE_FOR},
    {NAME("CHAR"), TYPE_CHAR, TYPE_NONE},
    {NAME(""),     TYPE_NONE, TYPE_NONE}
  };

  struct {
    Name name;
    Type *target;

  } alias2[] = {
    {NAME("Int"),   & type_IntSpecies},
    {NAME("Real"),  & type_RealSpecies},
    {NAME("Point"), & type_PointSpecies},
    {NAME(""),     (Type *) NULL}
  };

  /* Define the intrinsic types */
  for(i = 0; intrinsic[i].size >= 0; i++) {
    Type t;
    TASK( Tym_Def_Intrinsic(& t, & intrinsic[i].name, intrinsic[i].size) );
    assert(t == intrinsic[i].type || intrinsic[i].type == TYPE_NONE);
  }

  /* Define the alias types */
  for(i = 0; alias[i].target != TYPE_NONE; i++) {
    Type t;
    TASK( Tym_Def_Explicit_Alias(& t, & alias[i].name, alias[i].target) );
    assert(t == alias[i].type || alias[i].type == TYPE_NONE);
  }

  /* Define Int = (CHAR -> INT) */
  type_IntSpecies = TYPE_NONE;
  TASK( Tym_Def_Specie(& type_IntSpecies, TYPE_CHAR) );
  TASK( Tym_Def_Specie(& type_IntSpecies, TYPE_INT) );

  /* Define Real = (CHAR -> INT -> REAL) */
  type_RealSpecies = TYPE_NONE;
  TASK( Tym_Def_Specie(& type_RealSpecies, TYPE_CHAR) );
  TASK( Tym_Def_Specie(& type_RealSpecies, TYPE_INT) );
  TASK( Tym_Def_Specie(& type_RealSpecies, TYPE_REAL) );

  /* Define Point = ((Real, Real) -> POINT)*/
  type_RealCouple = TYPE_NONE;
  TASK( Tym_Def_Structure(& type_RealCouple, type_RealSpecies) );
  TASK( Tym_Def_Structure(& type_RealCouple, type_RealSpecies) );
  type_PointSpecies = TYPE_NONE;
  TASK( Tym_Def_Specie(& type_PointSpecies, type_RealCouple) );
  TASK( Tym_Def_Specie(& type_PointSpecies, TYPE_POINT) );

  /* Ora definisco il tipo stringa */
  type_CharArray = Tym_Def_Array_Of(-1, TYPE_CHAR);
  if ( type_CharArray == TYPE_NONE ) return Failed;

  /* Define the alias types */
  for(i = 0; alias2[i].target != (Type *) NULL; i++) {
    Type t;
    TASK( Tym_Def_Explicit_Alias(& t, & alias2[i].name, *alias2[i].target) );
  }

  return Success;
}

/* Initialize the compiler:
 *  1) Creates the fundamental operators and associated operations
 *     (those between intrinsic types)
 *  2) Sets the output for compiled code.
 */
Task Builtins_Init(void) {

  /* § OPERATORE DI PRODOTTO */
  /* §§ PRODOTTO FRA REALI E INTERI */
  ADD_OPERATION(times, type_Point, type_RealNum, TYPE_POINT, ASM_PMULR_P, 1, 1, 0);

  /* § OPERATORE DI DIVISIONE */
  /* §§ DIVISIONE PUNTO / REALE */
  ADD_OPERATION(div, type_Point, type_RealNum, TYPE_POINT, ASM_PDIVR_P, 0, 1, 0);

  return Success;
}

static Task Blt_Define_Basics(void) {
  /* Ora faccio l'overloading dell'operatore di conversione
   * e definisco le conversioni automatiche che il compilatore
   * dovra' fare.
   */
  TASK( Cmp_Builtin_Conv_Def("conv_2Real_to_Point", type_RealCouple,
                             TYPE_POINT, Conv_2RealNum_to_Point) );

  /* Define the conversions (example: Real@Int such as: a = Int[ 1.2 ]) */
  TASK(Cmp_Builtin_Proc_Def(TYPE_CHAR, BOX_CREATION, TYPE_CHAR, Char_Char));
  TASK(Cmp_Builtin_Proc_Def(TYPE_INT,  BOX_CREATION, TYPE_CHAR, Char_Int ));
  TASK(Cmp_Builtin_Proc_Def(TYPE_REAL, BOX_CREATION, TYPE_CHAR, Char_Real));
  TASK(Cmp_Builtin_Proc_Def(type_IntNum, BOX_CREATION, TYPE_INT, Int_IntNum ));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum, BOX_CREATION, TYPE_INT, Int_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum, BOX_CREATION, TYPE_REAL, Real_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_Point, BOX_CREATION, TYPE_POINT, Point_RealNumCouple ));
  TASK(Cmp_Builtin_Proc_Def(type_IntNum, BOX_CREATION, TYPE_IF, If_IntNum ));
  TASK(Cmp_Builtin_Proc_Def(type_IntNum, BOX_CREATION, TYPE_FOR, For_IntNum ));
  return Success;
}

#define DEFINE_TYPE(name, type) \
  Tym_Def_Explicit_Alias(& type_##name, & NAME(#name), type)

static Task Blt_Define_Math(void) {
  Int type_Min, type_Max, type_Vec;

  TASK( DEFINE_TYPE(Min,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Max,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Vec,   TYPE_POINT) );

  TASK(Cmp_Builtin_Proc_Def(TYPE_OPEN,   BOX_CREATION,type_Min,  Min_Open));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Min,  Min_RealNum));
  TASK(Cmp_Builtin_Proc_Def(TYPE_OPEN,   BOX_CREATION,type_Max,  Max_Open));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Max,  Max_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Vec,  Vec_RealNum));
  return Success;
}

static Task Blt_Define_Print(void) {
  TASK(Tym_Def_Explicit_Alias(& type_Print, & NAME("Print"), TYPE_VOID));
  TASK(Cmp_Builtin_Proc_Def(TYPE_CHAR,  BOX_CREATION,type_Print, My_Print_Char));
  TASK(Cmp_Builtin_Proc_Def(TYPE_INTG,  BOX_CREATION,type_Print, My_Print_Int));
  TASK(Cmp_Builtin_Proc_Def(TYPE_REAL,  BOX_CREATION,type_Print, My_Print_Real));
  TASK(Cmp_Builtin_Proc_Def(type_Point, BOX_CREATION,type_Print, My_Print_Pnt));
  TASK(Cmp_Builtin_Proc_Def(type_CharArray,BOX_CREATION,type_Print, Print_String));
  TASK(Cmp_Builtin_Proc_Def(TYPE_PAUSE, BOX_CREATION,type_Print,Print_NewLine));
  /*Tym_Print_Procedure(stdout, type_new);*/
  return Success;
}

static Task Blt_Define_Sys(void) {
  Int type_Exit;
  TASK( Tym_Def_Explicit_Alias(& type_Exit, & NAME("Exit"), TYPE_VOID) );
  TASK( Cmp_Builtin_Proc_Def(TYPE_INT, BOX_CREATION, type_Exit, Exit_Int) );
  return Success;
}
#endif

/*******************************BOX-PROCEDURES********************************/

/**********************
 * IO                 *
 **********************/

static Task My_Print_Char(BoxVM *vm) {
  printf(SChar, BOX_VM_ARG(vm, Char));
  return Success;
}

static Task My_Print_Int(BoxVM *vm) {
  printf(SInt, BOX_VM_ARG(vm, Int));
  return Success;
}

static Task My_Print_Real(BoxVM *vm) {
  printf(SReal, BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Print_Pnt(BoxVM *vm) {
  Point *p = BOX_VM_ARGPTR1(vm, Point);
  printf(SPoint, p->x, p->y);
  return Success;
}

static Task My_Print_Str(BoxVM *vm) {
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  if (s != NULL)
    printf("%s", s->ptr);
  return Success;
}


/**********************
 * Math               *
 **********************/

static Task My_Math_Sin(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = sin(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Cos(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = cos(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Tan(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = tan(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Asin(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = asin(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Acos(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = acos(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Atan(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = atan(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Exp(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = exp(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Log(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = log(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Log10(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = log10(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Sqrt(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = sqrt(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Ceil(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Int) = ceil(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Floor(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Int) = floor(BOX_VM_ARG(vm, Real));
  return Success;
}

static Task My_Math_Abs(BoxVM *vm) {
  BOX_VM_CURRENT(vm, Real) = fabs(BOX_VM_ARG(vm, Real));
  return Success;
}

/**********************
 * Conversions        *
 **********************/

static Task My_2R_To_P(BoxVM *vm) {
  BOX_VM_THIS(vm, Point) = *(BOX_VM_ARG_PTR(vm, Point));
  return Success;
}

#if 0
static Task Print_NewLine(VMProgram *vmp) {
  printf("\n"); return Success;
}

/*****************************************************************************
 *                           SYSTEM PROCEDURES                               *
 *****************************************************************************/

/* This function is not politically correct!!! */
static Task Exit_Int(VMProgram *vmp) {
  exit(BOX_VM_ARG1(vmp, Int));
}

/*****************************************************************************
 *                       FUNCTIONS FOR CONVERSION                            *
 *****************************************************************************/

static Task Char_Char(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Char) = BOX_VM_ARG1(vmp, Char); return Success;}
static Task Char_Int(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Char) = (Char) BOX_VM_ARG1(vmp, Int); return Success;}
static Task Char_Real(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Char) = (Char) BOX_VM_ARG1(vmp, Real); return Success;}
static Task Int_IntNum(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Int) = BOX_VM_ARG1(vmp, Int); return Success;}
static Task Int_RealNum(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Int) = (Int) BOX_VM_ARG1(vmp, Real); return Success;}
static Task Real_RealNum(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Real) = BOX_VM_ARG1(vmp, Real); return Success;}
static Task Point_RealNumCouple(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Point) = BOX_VM_ARG1(vmp, Point); return Success;}
static Task If_IntNum(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Int) = !BOX_VM_ARG1(vmp, Int); return Success;}
static Task For_IntNum(VMProgram *vmp)
  {BOX_VM_CURRENT(vmp, Int) = BOX_VM_ARG1(vmp, Int); return Success;}

/*****************************************************************************
 *                        MATHEMATICAL FUNCTIONS                             *
 *****************************************************************************/
static Task Min_Open(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = REAL_MAX;
  return Success;
}
static Task Min_RealNum(VMProgram *vmp) {
  Real *cp = BOX_VM_THIS_PTR(vmp, Real), c = *cp,
       x = BOX_VM_ARG1(vmp, Real);
  *cp = (x < c) ? x : c;
  return Success;
}
static Task Max_Open(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = REAL_MIN;
  return Success;
}
static Task Max_RealNum(VMProgram *vmp) {
  Real *cp = BOX_VM_THIS_PTR(vmp, Real), c = *cp,
       x = BOX_VM_ARG1(vmp, Real);
  *cp = (x > c) ? x : c;
  return Success;
}
static Task Vec_RealNum(VMProgram *vmp) {
  Real angle = BOX_VM_ARG1(vmp, Real);
  Point *p = BOX_VM_THIS_PTR(vmp, Point);
  p->x = cos(angle);
  p->y = sin(angle);
  return Success;
}
#endif

/****************************************************************************/
/* NEW COMPILER */

#include "typesys.h"
#include "value.h"
#include "operator.h"
#include "namespace.h"
#include "compiler.h"

BoxVMSymID Bltin_Proc_Add(BoxCmp *c, const char *proc_name,
                          Task (*c_fn)(BoxVM *)) {
  BoxVMSymID sym_num;
  BoxVMCallNum call_num;

  /* We create the symbol associated with this name */
  sym_num = BoxVMSym_New_Call(c->vm);

  /* We finally install the code (a C function) for the procedure */
  VM_Proc_Install_CCode(c->vm, & call_num, c_fn,
                        "(noname)", proc_name);

  /* And define the symbol */
  ASSERT_TASK(BoxVMSym_Def_Call(c->vm, sym_num, call_num));
  return sym_num;
}

BoxVMSymID Bltin_Proc_Def(BoxCmp *c, BoxType parent, BoxType child,
                          Task (*c_fn)(BoxVM *)) {
  BoxVMSymID sym_num;
  BoxVMCallNum call_num;
  BoxType new_proc;
  char *proc_name = NULL;

  /* We create the symbol associated with this name */
  sym_num = BoxVMSym_New_Call(c->vm);

  /* We tell to the compiler that some procedures are associated with it */
  TS_Procedure_New(& c->ts, & new_proc, parent, child, /*kind*/ 1);
  TS_Procedure_Register(& c->ts, new_proc, sym_num);
  proc_name = TS_Name_Get(& c->ts, new_proc);

  /* We finally install the code (a C function) for the procedure */
  VM_Proc_Install_CCode(c->vm, & call_num, c_fn,
                        "(noname)", proc_name);
  BoxMem_Free(proc_name);

  /* And define the symbol */
  ASSERT_TASK(BoxVMSym_Def_Call(c->vm, sym_num, call_num));
  return sym_num;
}

/* Define some core types such as Int, Real and Point (for example, define
 * Int as (Char->Int) and Real as (Char->Int->Real)).
 */
static void My_Define_Core_Types(BltinStuff *b, TS *ts) {
  /* Define Int */
  TS_Species_Begin(ts, & b->species_int);
  TS_Species_Add(ts, b->species_int, BOXTYPE_CHAR);
  TS_Species_Add(ts, b->species_int, BOXTYPE_INT);
  TS_Name_Set(ts, b->species_int, "Int");

  /* Define Real */
  TS_Species_Begin(ts, & b->species_real);
  TS_Species_Add(ts, b->species_real, BOXTYPE_CHAR);
  TS_Species_Add(ts, b->species_real, BOXTYPE_INT);
  TS_Species_Add(ts, b->species_real, BOXTYPE_REAL);
  TS_Name_Set(ts, b->species_real, "Real");

  /* Define (Real, Real) */
  TS_Structure_Begin(ts, & b->struc_real_real);
  TS_Structure_Add(ts, b->struc_real_real, b->species_real, NULL);
  TS_Structure_Add(ts, b->struc_real_real, b->species_real, NULL);

  /* Define Point as ((Real, Real) -> POINT) */
  TS_Species_Begin(ts, & b->species_point);
  TS_Species_Add(ts, b->species_point, b->struc_real_real);
  TS_Species_Add(ts, b->species_point, BOXTYPE_POINT);
  TS_Name_Set(ts, b->species_point, "Point");

  /* Define Str */
  TS_Intrinsic_New(ts, & b->string, sizeof(BoxStr));
  TS_Name_Set(ts, b->string, "Str");

  TS_Alias_New(ts, & b->print, BOXTYPE_VOID);
  TS_Name_Set(ts, b->print, "Print");
}

/* Register the core types in the current namespace, so that Box programs
 * can access and use them.
 */
static void My_Register_Core_Types(BoxCmp *c) {
  struct {
    const char *name;
    Type       type;

  } *type_to_register, types_to_register[] = {
    {"Char",        BOXTYPE_CHAR},
    {"INT",         BOXTYPE_INT},
    {"REAL",        BOXTYPE_REAL},
    {"POINT",       BOXTYPE_POINT},
    {"Int",         c->bltin.species_int},
    {"Real",        c->bltin.species_real},
    {"Point",       c->bltin.species_point},
    {"Str",         c->bltin.string},
    {"Void",        BOXTYPE_VOID},
    {"Print",       c->bltin.print},
    {(char *) NULL, BOXTYPE_NONE}
  };

  for(type_to_register = & types_to_register[0];
      type_to_register->name != NULL;
      ++type_to_register) {
    Value *v = Value_New(c->cur_proc);
    Value_Setup_As_Type(v, type_to_register->type);
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT,
                        type_to_register->name, v);
    Value_Unlink(v);
  }
}

/* Used to make the table of operations more compact in
 * My_Register_BinOps. This function maps a character to a BoxType
 * value. Example: My_Type_Of_Char(c, 'I') returns BOXTYPE_INT,
 * My_Type_Of_Char(c, 'R') returns BOXTYPE_REAL, etc.
 */
static BoxType My_Type_Of_Char(BoxCmp *c, char t) {
  switch(t) {
  case ' ': return BOXTYPE_NONE;
  case 'C': return BOXTYPE_CHAR;
  case 'I': return BOXTYPE_INT;
  case 'R': return BOXTYPE_REAL;
  case 'P': return BOXTYPE_POINT;
  case 'i': return c->bltin.species_int;
  case 'r': return c->bltin.species_real;
  case 'p': return c->bltin.species_point;
  default:
    MSG_FATAL("My_Type_Of_Char: unexpected character.");
    assert(0);
    return BOXTYPE_NONE;
  }
}

static OprAttr My_OprAttr_Of_Str(const char *s) {
  if (s == NULL)
    return OPR_ATTR_ALL;

  else {
    OprAttr a = 0;
    for(;*s != '\0'; s++) {
      switch(*s) {
      case 'a': a |= OPR_ATTR_ASSIGNMENT; break;
      case 'i': a |= OPR_ATTR_IGNORE_RES; break;
      case 'c': a |= OPR_ATTR_COMMUTATIVE; break;
      case 'r': a |= OPR_ATTR_UN_RIGHT; break;
      default:
        MSG_FATAL("My_OprAttr_Of_Str: error parsing string.");
        assert(0);
        return BOXTYPE_NONE;
      }
    }
    return a;
  }
}

/* Register all the core unary operations for the Box compiler. */
static void My_Register_UnOps(BoxCmp *c) {
  struct {
    const char *types; /* Two characters describing the types of the result
                          and of the operand, following the map character->type
                          implemented by My_Type_Of_Char) */
    ASTUnOp    op;     /* Operator to which the operation refers */
    const char *mask,  /* Mask of attributes (a string which is converted
                          to an OprAttr by calling My_OprAttr_Of_Str) */
               *attr;  /* Attributes to set */
    BoxGOp     g_op;   /* Generic opcode to use for assembling the operation */

  } *unop, unops[] = {
    { "Pp",  ASTUNOP_NEG,   "", NULL, BOXGOP_NEG},
    { "Rr",  ASTUNOP_NEG,   "", NULL, BOXGOP_NEG},
    { "Ii",  ASTUNOP_NEG,   "", NULL, BOXGOP_NEG},
    { "Rr", ASTUNOP_LINC,  "a", NULL, BOXGOP_INC},
    { "Ii", ASTUNOP_LINC,  "a", NULL, BOXGOP_INC},
    { "Rr", ASTUNOP_LDEC,  "a", NULL, BOXGOP_DEC},
    { "Ii", ASTUNOP_LDEC,  "a", NULL, BOXGOP_DEC},
    { "Rr", ASTUNOP_RINC, "ar", NULL, BOXGOP_INC},
    { "Ii", ASTUNOP_RINC, "ar", NULL, BOXGOP_INC},
    { "Rr", ASTUNOP_RDEC, "ar", NULL, BOXGOP_DEC},
    { "Ii", ASTUNOP_RDEC, "ar", NULL, BOXGOP_DEC},
    { "Ii", ASTUNOP_BNOT,   "", NULL, BOXGOP_BNOT},
    { "Ii", ASTUNOP_NOT,    "", NULL, BOXGOP_LNOT},


    { NULL,            0, NULL, NULL,          0}
  };

  for(unop = & unops[0]; unop->types != NULL; ++unop) {
    Operator *opr = BoxCmp_UnOp_Get(c, unop->op);
    BoxType result = My_Type_Of_Char(c, unop->types[0]),
            operand = My_Type_Of_Char(c, unop->types[1]);
    OprAttr mask = My_OprAttr_Of_Str(unop->mask),
            attr = My_OprAttr_Of_Str(unop->attr);
    Operation *opn = Operator_Add_Opn(opr, operand, BOXTYPE_NONE, result);
    Operation_Attr_Set(opn, mask, attr);
    opn->implem.opcode = unop->g_op;
  }
}

/* Register all the core binary operations for the Box compiler. */
static void My_Register_BinOps(BoxCmp *c) {
  struct {
    const char *types; /* Three characters describing the types of the result,
                          of the left and right operands (following the map
                          character->type implemented by My_Type_Of_Char) */
    ASTBinOp   op;     /* Operator to which the operation refers */
    const char *mask,  /* Mask of attributes (a string which is converted
                          to an OprAttr by calling My_OprAttr_Of_Str) */
               *attr;  /* Attributes to set */
    BoxGOp     g_op;   /* Generic opcode to use for assembling the operation */

  } *binop, binops[] = {
    {"Ppp", ASTBINOP_ASSIGN, "ai", NULL, BOXGOP_MOV},
    {"Rrr", ASTBINOP_ASSIGN, "ai", NULL, BOXGOP_MOV},
    {"Iii", ASTBINOP_ASSIGN, "ai", NULL, BOXGOP_MOV},
    {"CCC", ASTBINOP_ASSIGN, "ai", NULL, BOXGOP_MOV},
    {"Ppp",    ASTBINOP_ADD,  "c", NULL, BOXGOP_ADD},
    {"Rrr",    ASTBINOP_ADD,  "c", NULL, BOXGOP_ADD},
    {"Iii",    ASTBINOP_ADD,  "c", NULL, BOXGOP_ADD},
    {"Ppp",    ASTBINOP_SUB,   "", NULL, BOXGOP_SUB},
    {"Rrr",    ASTBINOP_SUB,   "", NULL, BOXGOP_SUB},
    {"Iii",    ASTBINOP_SUB,   "", NULL, BOXGOP_SUB},
    {"Rrr",    ASTBINOP_MUL,  "c", NULL, BOXGOP_MUL},
    {"Iii",    ASTBINOP_MUL,  "c", NULL, BOXGOP_MUL},
    {"Rrr",    ASTBINOP_DIV,   "", NULL, BOXGOP_DIV},
    {"Iii",    ASTBINOP_DIV,   "", NULL, BOXGOP_DIV},
    {"Iii",    ASTBINOP_REM,   "", NULL, BOXGOP_REM},
    {"Rrr",    ASTBINOP_POW,   "", NULL, BOXGOP_POW},
    {"Iii",    ASTBINOP_POW,   "", NULL, BOXGOP_POW},
    {"Iii",   ASTBINOP_BAND,  "c", NULL, BOXGOP_BAND},
    {"Iii",   ASTBINOP_BXOR,  "c", NULL, BOXGOP_BXOR},
    {"Iii",    ASTBINOP_BOR,  "c", NULL, BOXGOP_BOR},
    {"Iii",    ASTBINOP_SHL,   "", NULL, BOXGOP_SHL},
    {"Iii",    ASTBINOP_SHR,   "", NULL, BOXGOP_SHR},
    {"Iii",   ASTBINOP_LAND,  "c", NULL, BOXGOP_LAND},
    {"Iii",    ASTBINOP_LOR,  "c", NULL, BOXGOP_LOR},
    {"Ppp",  ASTBINOP_APLUS, "ai", NULL, BOXGOP_ADD},
    {"Rrr",  ASTBINOP_APLUS, "ai", NULL, BOXGOP_ADD},
    {"Iii",  ASTBINOP_APLUS, "ai", NULL, BOXGOP_ADD},
    {"Ppp", ASTBINOP_AMINUS, "ai", NULL, BOXGOP_SUB},
    {"Rrr", ASTBINOP_AMINUS, "ai", NULL, BOXGOP_SUB},
    {"Iii", ASTBINOP_AMINUS, "ai", NULL, BOXGOP_SUB},
    {"Rrr", ASTBINOP_ATIMES, "ai", NULL, BOXGOP_MUL},
    {"Iii", ASTBINOP_ATIMES, "ai", NULL, BOXGOP_MUL},
    {"Rrr",   ASTBINOP_ADIV, "ai", NULL, BOXGOP_DIV},
    {"Iii",   ASTBINOP_ADIV, "ai", NULL, BOXGOP_DIV},
    {"Iii",   ASTBINOP_AREM, "ai", NULL, BOXGOP_REM},
    {"Iii",   ASTBINOP_ASHL, "ai", NULL, BOXGOP_SHL},
    {"Iii",   ASTBINOP_ASHR, "ai", NULL, BOXGOP_SHR},
    {"Iii",  ASTBINOP_ABAND, "ai", NULL, BOXGOP_BAND},
    {"Iii",  ASTBINOP_ABXOR, "ai", NULL, BOXGOP_BXOR},
    {"Iii",   ASTBINOP_ABOR, "ai", NULL, BOXGOP_BOR},
    {"Ipp",     ASTBINOP_EQ,  "c", NULL, BOXGOP_EQ},
    {"Irr",     ASTBINOP_EQ,  "c", NULL, BOXGOP_EQ},
    {"Iii",     ASTBINOP_EQ,  "c", NULL, BOXGOP_EQ},
    {"Ipp",     ASTBINOP_NE,  "c", NULL, BOXGOP_NE},
    {"Irr",     ASTBINOP_NE,  "c", NULL, BOXGOP_NE},
    {"Iii",     ASTBINOP_NE,  "c", NULL, BOXGOP_NE},
    {"Irr",     ASTBINOP_LT,   "", NULL, BOXGOP_LT},
    {"Iii",     ASTBINOP_LT,   "", NULL, BOXGOP_LT},
    {"Irr",     ASTBINOP_LE,   "", NULL, BOXGOP_LE},
    {"Iii",     ASTBINOP_LE,   "", NULL, BOXGOP_LE},
    {"Irr",     ASTBINOP_GT,   "", NULL, BOXGOP_GT},
    {"Iii",     ASTBINOP_GT,   "", NULL, BOXGOP_GT},
    {"Irr",     ASTBINOP_GE,   "", NULL, BOXGOP_GE},
    {"Iii",     ASTBINOP_GE,   "", NULL, BOXGOP_GE},
    { NULL,               0, NULL, NULL, 0}
  };

  for(binop = & binops[0]; binop->types != NULL; ++binop) {
    Operator *opr = BoxCmp_BinOp_Get(c, binop->op);
    BoxType result = My_Type_Of_Char(c, binop->types[0]),
            left = My_Type_Of_Char(c, binop->types[1]),
            right = My_Type_Of_Char(c, binop->types[2]);
    OprAttr mask = My_OprAttr_Of_Str(binop->mask),
            attr = My_OprAttr_Of_Str(binop->attr);
    Operation *opn = Operator_Add_Opn(opr, left, right, result);
    Operation_Attr_Set(opn, mask, attr);
    opn->implem.opcode = binop->g_op;
  }
}

/* Register all the conversion operations for the Box compiler. */
static void My_Register_Conversions(BoxCmp *c) {
  Operator *convert = & c->convert;
  BoxVMSymID struc_to_point_sym_id;

  struct {
    const char *types; /* Two characters describing the types of the source
                          and destination of the conversion, following the map
                          character->type implemented by My_Type_Of_Char) */
    const char *mask,  /* Mask of attributes (a string which is converted
                          to an OprAttr by calling My_OprAttr_Of_Str) */
               *attr;  /* Attributes to set */
    BoxGOp     g_op;   /* Generic opcode to use for assembling the operation */

  } *conv, convs[] = {
    { "IR",   "", NULL, BOXGOP_REAL},
    { "CR",   "", NULL, BOXGOP_REAL},
    { "RI",   "", NULL, BOXGOP_INT},
    { "CI",   "", NULL, BOXGOP_INT},
    { NULL, NULL, NULL, 0}
  };

  for(conv = & convs[0]; conv->types != NULL; ++conv) {
    BoxType src = My_Type_Of_Char(c, conv->types[0]),
            dst = My_Type_Of_Char(c, conv->types[1]);
    OprAttr mask = My_OprAttr_Of_Str(conv->mask),
            attr = My_OprAttr_Of_Str(conv->attr);
    Operation *opn = Operator_Add_Opn(convert, src, BOXTYPE_NONE, dst);
    Operation_Attr_Set(opn, mask, attr);
    opn->implem.opcode = conv->g_op;
  }

  /* Conversion (Real, Real) -> Point */
  Operation *opn = Operator_Add_Opn(convert, c->bltin.struc_real_real,
                                    BOXTYPE_NONE, BOXTYPE_POINT);
  struc_to_point_sym_id = Bltin_Proc_Add(c, "conv_2r_to_point", My_2R_To_P);
  Operation_Set_User_Implem(opn, struc_to_point_sym_id);
}

void Bltin_Simple_Fn_Def(BoxCmp *c, const char *name,
                         BoxType ret, BoxType arg, BoxVMFunc fn) {
  BoxType new_type;
  Value *v;

  TS_Alias_New(& c->ts, & new_type, ret);
  TS_Name_Set(& c->ts, new_type, name);
  (void) Bltin_Proc_Def(c, new_type, arg, fn);
  v = Value_New(c->cur_proc);
  Value_Setup_As_Type(v, new_type);
  Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, name, v);
  Value_Unlink(v);
}

static void My_Register_Std_IO(BoxCmp *c) {
  BoxType t_print = c->bltin.print;
  (void) Bltin_Proc_Def(c, t_print,  BOXTYPE_CHAR, My_Print_Char);
  (void) Bltin_Proc_Def(c, t_print,   BOXTYPE_INT, My_Print_Int);
  (void) Bltin_Proc_Def(c, t_print,  BOXTYPE_REAL, My_Print_Real);
  (void) Bltin_Proc_Def(c, t_print, c->bltin.species_point, My_Print_Pnt);
  (void) Bltin_Proc_Def(c, t_print, c->bltin.string, My_Print_Str);

#if 0
  TASK(Cmp_Builtin_Proc_Def(TYPE_PAUSE, BOX_CREATION,type_Print,Print_NewLine));
  /*Tym_Print_Procedure(stdout, type_new);*/
#endif
}

static void My_Register_Math(BoxCmp *c) {
  struct {
    const char *name;
    BoxType    parent,
               child;
    BoxVMFunc  func;
  } *fn, fns[] = {
   { "Sqrt", BOXTYPE_REAL, c->bltin.species_real, My_Math_Sqrt},
   {  "Sin", BOXTYPE_REAL, c->bltin.species_real, My_Math_Sin},
   {  "Cos", BOXTYPE_REAL, c->bltin.species_real, My_Math_Cos},
   {  "Tan", BOXTYPE_REAL, c->bltin.species_real, My_Math_Tan},
   { "Asin", BOXTYPE_REAL, c->bltin.species_real, My_Math_Asin},
   { "Acos", BOXTYPE_REAL, c->bltin.species_real, My_Math_Acos},
   { "Atan", BOXTYPE_REAL, c->bltin.species_real, My_Math_Atan},
   {  "Exp", BOXTYPE_REAL, c->bltin.species_real, My_Math_Exp},
   {  "Log", BOXTYPE_REAL, c->bltin.species_real, My_Math_Log},
   {"Log10", BOXTYPE_REAL, c->bltin.species_real, My_Math_Log10},
   { "Ceil",  BOXTYPE_INT, c->bltin.species_real, My_Math_Ceil},
   {"Floor",  BOXTYPE_INT, c->bltin.species_real, My_Math_Floor},
   {  "Abs", BOXTYPE_REAL, c->bltin.species_real, My_Math_Abs},
   {   NULL, BOXTYPE_NONE,          BOXTYPE_NONE, NULL}
  };

  for(fn = fns; fn->func != NULL; fn++) {
    Bltin_Simple_Fn_Def(c, fn->name, fn->parent, fn->child, fn->func);
  }
}

/* Register bultin types, operation and functions */
void Bltin_Init(BoxCmp *c) {
  My_Define_Core_Types(& c->bltin, & c->ts);
  My_Register_Core_Types(c);
  My_Register_UnOps(c);
  My_Register_BinOps(c);
  My_Register_Conversions(c);
  My_Register_Std_IO(c);
  My_Register_Math(c);
  Bltin_Str_Register_Procs(c);
}

void Bltin_Finish(BoxCmp *c) {
}

