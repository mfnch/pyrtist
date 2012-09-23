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
#include "strutils.h"
#include "vm_private.h"
#include "vmproc.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "vmalloc.h"
#include "registers.h"
#include "builtins.h"
#include "bltinstr.h"
#include "str.h"
#include "bltinio.h"

/*******************************BOX-PROCEDURES********************************/

static BoxTask My_Subtype_Init(BoxVMX *vmx) {
  BoxSubtype *s = BoxVMX_Get_Parent_Target(vmx);
  BoxPtr_Nullify(& s->parent);
  BoxPtr_Nullify(& s->child);
  return BOXTASK_OK;
}

static BoxTask My_Subtype_Finish(BoxVMX *vmx) {
  BoxSubtype *s = BoxVMX_Get_Parent_Target(vmx);
  BoxVM_Obj_Unlink(vmx->vm, & s->parent);
  BoxVM_Obj_Unlink(vmx->vm, & s->child);
  return BOXTASK_OK;
}

/**********************
 * IO                 *
 **********************/

static BoxTask My_Print_Pause(BoxVMX *vmx) {
  printf("\n");
  return BOXTASK_OK;
}

static BoxTask My_Print_Char(BoxVMX *vmx) {
  printf(SChar, *((BoxChar *) BoxVMX_Get_Child_Target(vmx)));
  return BOXTASK_OK;
}

static BoxTask My_Print_Int(BoxVMX *vmx) {
  printf(SInt, *((BoxInt *) BoxVMX_Get_Child_Target(vmx)));
  return BOXTASK_OK;
}

static BoxTask My_Print_Real(BoxVMX *vmx) {
  printf(SReal, *((BoxReal *) BoxVMX_Get_Child_Target(vmx)));
  return BOXTASK_OK;
}

static BoxTask My_Print_Pnt(BoxVMX *vmx) {
  Point *p = BoxVMX_Get_Child_Target(vmx);
  printf(SPoint, p->x, p->y);
  return BOXTASK_OK;
}

static BoxTask My_Print_Str(BoxVMX *vmx) {
  BoxStr *s = BoxVMX_Get_Child_Target(vmx);
  if (s->ptr != NULL)
    printf("%s", s->ptr);
  return BOXTASK_OK;
}

/**********************
 * Math               *
 **********************/

#define MY_DEFINE_FN_REAL_AT_REAL(dst_fn, src_fn)               \
  static BoxTask dst_fn(BoxVMX *vmx) {                          \
    *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) =              \
      src_fn(*((BoxReal *) BoxVMX_Get_Child_Target(vmx)));      \
    return BOXTASK_OK;                                          \
  }                                                             \

MY_DEFINE_FN_REAL_AT_REAL(My_Math_Cos, cos)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Sin, sin)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Tan, tan)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Asin, asin)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Acos, acos)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Atan, atan)


static BoxTask My_Math_Atan2(BoxVMX *vmx) {
  BoxPoint *p = BoxVMX_Get_Child_Target(vmx);
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = atan2(p->y, p->x);
  return BOXTASK_OK;
}

MY_DEFINE_FN_REAL_AT_REAL(My_Math_Exp, exp)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Log, log)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Log10, log10)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Sqrt, sqrt)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Ceil, ceil)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Floor, floor)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Abs, fabs)

static BoxTask My_Math_Norm(BoxVMX *vmx) {
  BoxPoint *p = BoxVMX_Get_Child_Target(vmx);
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = sqrt(p->x*p->x + p->y*p->y);
  return BOXTASK_OK;
}

static BoxTask My_Math_Norm2(BoxVMX *vmx) {
  BoxPoint *p = BoxVMX_Get_Child_Target(vmx);
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = p->x*p->x + p->y*p->y;
  return BOXTASK_OK;
}

static BoxTask My_Min_Open(BoxVMX *vmx) {
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = BOXREAL_MAX;
  return BOXTASK_OK;
}

static BoxTask My_Min_Real(BoxVMX *vmx) {
  BoxReal *cp = BoxVMX_Get_Parent_Target(vmx), c = *cp,
          x = *((BoxReal *) BoxVMX_Get_Child_Target(vmx));
  *cp = (x < c) ? x : c;
  return BOXTASK_OK;
}

static BoxTask My_Max_Open(BoxVMX *vmx) {
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = BOXREAL_MIN;
  return BOXTASK_OK;
}

static BoxTask My_Max_Real(BoxVMX *vmx) {
  BoxReal *cp = BoxVMX_Get_Parent_Target(vmx), c = *cp,
          x = *((BoxReal *) BoxVMX_Get_Child_Target(vmx));
  *cp = (x > c) ? x : c;
  return BOXTASK_OK;
}

static BoxTask My_Vec_Real(BoxVMX *vmx) {
  BoxReal *angle = BoxVMX_Get_Child_Target(vmx);
  BoxPoint *p = BoxVMX_Get_Parent_Target(vmx);
  p->x = cos(*angle);
  p->y = sin(*angle);
  return BOXTASK_OK;
}

static BoxTask My_Point_At_Ort(BoxVMX *vm) {
  BoxPoint *p_out = BoxVMX_Get_Parent_Target(vm);
  BoxPoint *p_in = BoxVMX_Get_Child_Target(vm);
  p_out->x = -p_in->y;
  p_out->y = p_in->x;
  return BOXTASK_OK;
}

/**********************
 * Conversions        *
 **********************/

static BoxTask My_2R_To_P(BoxVMX *vmx) {
  *((BoxPoint *) BoxVMX_Get_Parent_Target(vmx))
    = *((BoxPoint *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

/**********************
 * Sys                *
 **********************/

/* This function is not politically correct!!! */
static BoxTask My_Exit_Int(BoxVMX *vmx) {
  exit(*((BoxInt *) BoxVMX_Get_Child_Target(vmx)));
}

static BoxTask My_Fail_Clear_Msg(BoxVMX *vmx) {
  BoxVMX_Set_Fail_Msg(vmx, NULL);
  return BOXTASK_OK;
}

static BoxTask My_Fail(BoxVMX *vmx) {
  return BOXTASK_FAILURE;
}

static BoxTask My_Fail_Msg(BoxVMX *vmx) {
  BoxStr *s = BoxVMX_Get_Child_Target(vmx);
  char *msg = BoxStr_To_C_String(s);
  BoxVMX_Set_Fail_Msg(vmx, msg);
  BoxMem_Free(msg);
  return BOXTASK_OK;
}

static BoxTask My_Length_Init(BoxVMX *vm) {
  BoxInt *length = BoxVMX_Get_Parent_Target(vm);
  *length = 0;
  return BOXTASK_OK;
}

static BoxTask My_Num_Init(BoxVMX *vm) {
  BoxInt *length = BoxVMX_Get_Parent_Target(vm);
  *length = 0;
  return BOXTASK_OK;
}

static BoxTask My_IsValid_Init(BoxVMX *vm) {
  BoxInt *valid = BoxVMX_Get_Parent_Target(vm);
  *valid = 1;
  return BOXTASK_OK;
}

static BoxTask My_Int_At_IsValid(BoxVMX *vm) {
  BoxInt *valid = BoxVMX_Get_Parent_Target(vm);
  BoxInt *child = BoxVMX_Get_Child_Target(vm);
  *valid = (*valid && *child);
  return BOXTASK_OK;
}

static BoxTask My_Compare_Init(BoxVMX *vm) {
  BoxInt *compare = BoxVMX_Get_Parent_Target(vm);
  *compare = 0;
  return BOXTASK_OK;
}

/*****************************************************************************
 *                       FUNCTIONS FOR CONVERSION                            *
 *****************************************************************************/

static BoxTask My_Char_Char(BoxVMX *vmx) {
  *((BoxChar *) BoxVMX_Get_Parent_Target(vmx))
    = *((BoxChar *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

static BoxTask My_Char_Int(BoxVMX *vmx) {
  *((BoxChar *) BoxVMX_Get_Parent_Target(vmx))
    = (BoxChar) *((BoxInt *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

static BoxTask My_Char_Real(BoxVMX *vmx) {
  *((BoxChar *) BoxVMX_Get_Parent_Target(vmx))
    = (BoxChar) *((BoxReal *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

static BoxTask My_Int_Int(BoxVMX *vmx) {
  *((BoxInt *) BoxVMX_Get_Parent_Target(vmx))
    = *((BoxInt *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

static BoxTask My_Int_Real(BoxVMX *vmx) {
  *((BoxInt *) BoxVMX_Get_Parent_Target(vmx))
    = (BoxInt) *((BoxReal *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

static BoxTask My_Real_Real(BoxVMX *vmx) {
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx))
    = *((BoxReal *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

static BoxTask My_Point_RealNumCouple(BoxVMX *vmx) {
  *((BoxPoint *) BoxVMX_Get_Parent_Target(vmx))
    = *((BoxPoint *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

static BoxTask My_If_Int(BoxVMX *vmx) {
  *((BoxInt *) BoxVMX_Get_Parent_Target(vmx))
    = !*((BoxInt *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

static BoxTask My_For_Int(BoxVMX *vmx) {
  *((BoxInt *) BoxVMX_Get_Parent_Target(vmx))
    = *((BoxInt *) BoxVMX_Get_Child_Target(vmx));
  return BOXTASK_OK;
}

/****************************************************************************/
/* NEW COMPILER */

#include "typesys.h"
#include "value.h"
#include "operator.h"
#include "namespace.h"
#include "compiler.h"

BoxVMSymID Bltin_Proc_Add(BoxCmp *c, const char *proc_name,
                          BoxTask (*c_fn)(BoxVMX *)) {
  BoxVMSymID sym_num;
  BoxVMCallNum call_num;

  /* We finally install the code (a C function) for the procedure */
  call_num = BoxVM_Proc_Install_CCode(c->vm, BOXVMPROCID_NONE, c_fn,
                                      (char *) NULL, proc_name);

  /* We create the symbol associated with this name and define it */
  sym_num = BoxVMSym_New_Call(c->vm, call_num);
  BoxVMSym_Def_Call(c->vm, sym_num);
  return sym_num;
}

BoxVMSymID Bltin_Comb_Def(BoxCmp *c, BoxTypeId child, BoxCombType comb,
                          BoxTypeId parent, BoxTask (*c_fn)(BoxVMX *)) {
  BoxVMSymID sym_num;
  BoxVMCallNum call_num;
  BoxTypeId new_proc;
  char *proc_name = NULL;

  /* We reserve the call number and associate a symbol to it */
  call_num = BoxVM_Allocate_CallNum(c->vm);
  sym_num = BoxVMSym_New_Call(c->vm, call_num);

  /* We tell to the compiler that some procedures are associated to sym_num */
  BoxType *child_new = BoxType_From_Id(& c->ts, child),
          *parent_new = BoxType_From_Id(& c->ts, parent);
  BoxCallable *callable = BoxCallable_Create_Undefined(parent_new, child_new);
  callable = BoxCallable_Define_From_VM(callable, NULL, c->vm, call_num);
  new_proc = BoxTS_Procedure_Define(& c->ts, child, comb, parent, sym_num,
                                    callable);
  proc_name = TS_Name_Get(& c->ts, new_proc);

  /* We finally install the code (a C function) for the procedure */
  (void) BoxVM_Proc_Install_CCode(c->vm, call_num, c_fn,
                                  (char *) NULL, proc_name);
  BoxMem_Free(proc_name);

  /* Mark the symbol as defined */
  BoxVMSym_Def_Call(c->vm, sym_num);
  return sym_num;
}

BoxVMSymID Bltin_Proc_Def(BoxCmp *c, BoxTypeId parent, BoxTypeId child,
                          BoxTask (*c_fn)(BoxVMX *)) {
  return Bltin_Comb_Def(c, child, BOXCOMBTYPE_AT, parent, c_fn);
}

/* Define some core types such as Int, Real and Point (for example, define
 * Int as (Char->Int) and Real as (Char->Int->Real)).
 */
static void My_Define_Core_Types(BltinStuff *b, TS *ts) {
  b->any = BoxTS_New_Any(ts);

  /* Define Int */
  b->species_int = BoxTS_Begin_Species(ts);
  BoxTS_Add_Species_Member(ts, b->species_int, BOXTYPE_CHAR);
  BoxTS_Add_Species_Member(ts, b->species_int, BOXTYPE_INT);
  b->species_int = BoxTS_New_Alias_With_Name(ts, b->species_int, "Int");

  /* Define Real */
  b->species_real = BoxTS_Begin_Species(ts);
  BoxTS_Add_Species_Member(ts, b->species_real, BOXTYPE_CHAR);
  BoxTS_Add_Species_Member(ts, b->species_real, BOXTYPE_INT);
  BoxTS_Add_Species_Member(ts, b->species_real, BOXTYPE_REAL);
  b->species_real = BoxTS_New_Alias_With_Name(ts, b->species_real, "Real");

  /* Define (Real, Real) */
  b->struc_real_real = BoxTS_Begin_Struct(ts);
  BoxTS_Add_Struct_Member(ts, b->struc_real_real, b->species_real, NULL);
  BoxTS_Add_Struct_Member(ts, b->struc_real_real, b->species_real, NULL);

  /* Define Point as ((Real, Real) -> POINT) */
  b->species_point = BoxTS_Begin_Species(ts);
  BoxTS_Add_Species_Member(ts, b->species_point, b->struc_real_real);
  BoxTS_Add_Species_Member(ts, b->species_point, BOXTYPE_POINT);
  b->species_point = BoxTS_New_Alias_With_Name(ts, b->species_point, "Point");

  /* Define If, Else, Elif and For */
  b->alias_if =
    BoxTS_New_Alias_With_Name(ts, BoxTS_New_Raised(ts, BOXTYPE_INT), "If");
  b->alias_else =
    BoxTS_New_Alias_With_Name(ts, BoxTS_New_Raised(ts, BOXTYPE_VOID), "Else");
  b->alias_elif =
    BoxTS_New_Alias_With_Name(ts, BoxTS_New_Raised(ts, BOXTYPE_INT), "Elif");
  b->alias_for =
    BoxTS_New_Alias_With_Name(ts, BoxTS_New_Raised(ts, BOXTYPE_INT), "For");

  /* Define Str */
  b->string = BoxTS_New_Intrinsic_With_Name(ts, sizeof(BoxStr),
                                            __alignof__(BoxStr), "Str");

  b->print = BoxTS_New_Alias_With_Name(ts, BOXTYPE_VOID, "Print");
  b->repr = BoxTS_New_Alias_With_Name(ts, b->string, "Repr");
}

/* Register the core types in the current namespace, so that Box programs
 * can access and use them.
 */
static void My_Register_Core_Types(BoxCmp *c) {
  struct {
    const char *name;
    BoxTypeId    type;

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
    {"Ptr",         BOXTYPE_PTR},
    {"CPtr",        BOXTYPE_CPTR},
    {"If",          c->bltin.alias_if},
    {"Else",        c->bltin.alias_else},
    {"Elif",        c->bltin.alias_elif},
    {"For",         c->bltin.alias_for},
    {"Print",       c->bltin.print},
    {"Repr",        c->bltin.repr},
    {"ANY",         c->bltin.any},
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
static BoxTypeId My_Type_Of_Char(BoxCmp *c, char t) {
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
    BoxTypeId result = My_Type_Of_Char(c, unop->types[0]),
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
    {"Ppr",    ASTBINOP_MUL,   "", NULL, BOXGOP_PMULR},
    {"Prp",    ASTBINOP_MUL,   "", NULL, BOXGOP_PMULR},
    {"Rrr",    ASTBINOP_MUL,  "c", NULL, BOXGOP_MUL},
    {"Iii",    ASTBINOP_MUL,  "c", NULL, BOXGOP_MUL},
    {"Ppr",    ASTBINOP_DIV,   "", NULL, BOXGOP_PDIVR},
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
    BoxTypeId result = My_Type_Of_Char(c, binop->types[0]),
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
    BoxTypeId src = My_Type_Of_Char(c, conv->types[0]),
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

BoxTypeId Bltin_Simple_Fn_Def(BoxCmp *c, const char *name,
                              BoxTypeId ret, BoxTypeId arg, BoxVMFunc fn) {
  BoxTypeId new_type;
  Value *v;

  new_type = BoxTS_New_Alias_With_Name(& c->ts, ret, name);

  (void) Bltin_Proc_Def(c, new_type, arg, fn);
  v = Value_New(c->cur_proc);
  Value_Setup_As_Type(v, new_type);
  Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, name, v);
  Value_Unlink(v);
  return new_type;
}

static void My_Register_Std_IO(BoxCmp *c) {
  BoxTypeId t_print = c->bltin.print;
  (void) Bltin_Proc_Def(c, t_print, BOXTYPE_PAUSE, My_Print_Pause);
  (void) Bltin_Proc_Def(c, t_print,  BOXTYPE_CHAR, My_Print_Char);
  (void) Bltin_Proc_Def(c, t_print,   BOXTYPE_INT, My_Print_Int);
  (void) Bltin_Proc_Def(c, t_print,  BOXTYPE_REAL, My_Print_Real);
  (void) Bltin_Proc_Def(c, t_print, c->bltin.species_point, My_Print_Pnt);
  (void) Bltin_Proc_Def(c, t_print, c->bltin.string, My_Print_Str);
}

static void My_Register_Std_Procs(BoxCmp *c) {
  BoxTypeId t_int   = c->bltin.species_int,
          t_real  = c->bltin.species_real,
          t_point = c->bltin.species_point,
          t_if    = c->bltin.alias_if,
          t_elif  = c->bltin.alias_elif,
          t_for   = c->bltin.alias_for;
  (void) Bltin_Proc_Def(c,  BOXTYPE_CHAR, BOXTYPE_CHAR, My_Char_Char);
  (void) Bltin_Proc_Def(c,  BOXTYPE_CHAR,  BOXTYPE_INT, My_Char_Int);
  (void) Bltin_Proc_Def(c,  BOXTYPE_CHAR, BOXTYPE_REAL, My_Char_Real);
  (void) Bltin_Proc_Def(c,   BOXTYPE_INT,        t_int, My_Int_Int);
  (void) Bltin_Proc_Def(c,   BOXTYPE_INT, BOXTYPE_REAL, My_Int_Real);
  (void) Bltin_Proc_Def(c,  BOXTYPE_REAL,       t_real, My_Real_Real);
  (void) Bltin_Proc_Def(c, BOXTYPE_POINT,      t_point,
                        My_Point_RealNumCouple);
  (void) Bltin_Proc_Def(c,          t_if,        t_int, My_If_Int);
  (void) Bltin_Proc_Def(c,        t_elif,        t_int, My_If_Int);
  (void) Bltin_Proc_Def(c,         t_for,        t_int, My_For_Int);

  c->bltin.subtype_init = Bltin_Proc_Add(c, "subtype_init", My_Subtype_Init);
  c->bltin.subtype_finish = Bltin_Proc_Add(c, "subtype_finish",
                                           My_Subtype_Finish);
}

static void My_Register_Math(BoxCmp *c) {
  BoxTypeId t_real = c->bltin.species_real,
          t_point = c->bltin.species_point;
  struct {
    const char *name;
    BoxTypeId    parent,
               child;
    BoxVMFunc  func_begin,
               func;
  } *fn, fns[] = {
   { "Sqrt",  BOXTYPE_REAL,  t_real, NULL,  My_Math_Sqrt},
   {  "Sin",  BOXTYPE_REAL,  t_real, NULL,   My_Math_Sin},
   {  "Cos",  BOXTYPE_REAL,  t_real, NULL,   My_Math_Cos},
   {  "Tan",  BOXTYPE_REAL,  t_real, NULL,   My_Math_Tan},
   { "Asin",  BOXTYPE_REAL,  t_real, NULL,  My_Math_Asin},
   { "Acos",  BOXTYPE_REAL,  t_real, NULL,  My_Math_Acos},
   { "Atan",  BOXTYPE_REAL,  t_real, NULL,  My_Math_Atan},
   {"Atan2",  BOXTYPE_REAL, t_point, NULL,  My_Math_Atan2},
   {  "Exp",  BOXTYPE_REAL,  t_real, NULL,   My_Math_Exp},
   {  "Log",  BOXTYPE_REAL,  t_real, NULL,   My_Math_Log},
   {"Log10",  BOXTYPE_REAL,  t_real, NULL, My_Math_Log10},
   { "Ceil",   BOXTYPE_INT,  t_real, NULL,  My_Math_Ceil},
   {"Floor",   BOXTYPE_INT,  t_real, NULL, My_Math_Floor},
   {  "Abs",  BOXTYPE_REAL,  t_real, NULL,   My_Math_Abs},
   { "Norm",  BOXTYPE_REAL, t_point, NULL,  My_Math_Norm},
   {"Norm2",  BOXTYPE_REAL, t_point, NULL, My_Math_Norm2},
   {  "Vec", BOXTYPE_POINT,  t_real, NULL,   My_Vec_Real},
   {  "Ort", BOXTYPE_POINT, t_point, NULL, My_Point_At_Ort},
   {  "Min",  BOXTYPE_REAL,  t_real, My_Min_Open, My_Min_Real},
   {  "Max",  BOXTYPE_REAL,  t_real, My_Max_Open, My_Max_Real},
   {   NULL,  BOXTYPE_NONE,  BOXTYPE_NONE, NULL, NULL}
  };

  for(fn = fns; fn->func != NULL; fn++) {
    BoxTypeId func_type =
      Bltin_Simple_Fn_Def(c, fn->name, fn->parent, fn->child, fn->func);
    if (fn->func_begin != NULL)
      (void) Bltin_Proc_Def(c, func_type, BOXTYPE_BEGIN, fn->func_begin);
  }
}

static void My_Register_Sys(BoxCmp *c) {
  BoxTypeId fail_t = Bltin_Simple_Fn_Def(c, "Fail", BOXTYPE_VOID,
                                       c->bltin.string, My_Fail_Msg);

  (void) Bltin_Proc_Def(c, fail_t, BOXTYPE_BEGIN, My_Fail_Clear_Msg);
  (void) Bltin_Proc_Def(c, fail_t, BOXTYPE_END, My_Fail);
  (void) Bltin_Simple_Fn_Def(c, "Exit", BOXTYPE_VOID, c->bltin.species_int,
                             My_Exit_Int);
  c->bltin.length =
    Bltin_Simple_Fn_Def(c, "Length", BOXTYPE_INT,
                        BOXTYPE_BEGIN, My_Length_Init);

  c->bltin.num =
    Bltin_Simple_Fn_Def(c, "Num", BOXTYPE_INT,
                        BOXTYPE_BEGIN, My_Num_Init);

  c->bltin.valid =
    Bltin_Simple_Fn_Def(c, "IsValid", BOXTYPE_INT,
                        BOXTYPE_BEGIN, My_IsValid_Init);
  (void) Bltin_Proc_Def(c, c->bltin.valid, BOXTYPE_INT, My_Int_At_IsValid);

  c->bltin.compare =
    Bltin_Simple_Fn_Def(c, "Compare", BOXTYPE_INT,
                        BOXTYPE_BEGIN, My_Compare_Init);
}

/* Register bultin types, operation and functions */
void Bltin_Init(BoxCmp *c) {
  My_Define_Core_Types(& c->bltin, & c->ts);
  My_Register_Core_Types(c);
  My_Register_UnOps(c);
  My_Register_BinOps(c);
  My_Register_Conversions(c);
  My_Register_Std_IO(c);
  My_Register_Std_Procs(c);
  My_Register_Math(c);
  My_Register_Sys(c);
  Bltin_Str_Register_Procs(c);
  Bltin_IO_Register(c);
}

void Bltin_Finish(BoxCmp *c) {
  Bltin_IO_Unregister(c);
}

/*****************************************************************************
 * Generic procedures for builtin stuff defined inside other files.
 */

BoxTypeId Bltin_New_Type(BoxCmp *c, const char *type_name,
                         size_t type_size, size_t alignment) {
  TS *ts = & c->ts;
  Value *v = Value_New(c->cur_proc);
  BoxTypeId t =
    BoxTS_New_Intrinsic_With_Name(ts, type_size, alignment, type_name);
  Value_Setup_As_Type(v, t);
  Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, type_name, v);
  Value_Unlink(v);
  return t;
}
