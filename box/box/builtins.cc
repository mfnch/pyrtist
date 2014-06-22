/****************************************************************************
 * Copyright (C) 2008-2013 by Matteo Franchin                               *
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#include "types.h"
#include "combs.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "strutils.h"
#include "vm_priv.h"
#include "vmproc.h"
#include "vmsym.h"
#include "registers.h"
#include "builtins.h"
#include "bltinstr.h"
#include "str.h"
#include "bltinio.h"

#include "compiler_priv.h"


/*******************************BOX-PROCEDURES********************************/

static BoxTask My_Subtype_Init(BoxVMX *vmx) {
  BoxSubtype *s = (BoxSubtype *) BoxVMX_Get_Parent_Target(vmx);
  BoxPtr_Nullify(& s->parent);
  BoxPtr_Nullify(& s->child);
  return BOXTASK_OK;
}

static BoxTask My_Subtype_Finish(BoxVMX *vmx) {
  BoxSubtype *s = (BoxSubtype *) BoxVMX_Get_Parent_Target(vmx);
  (void) BoxPtr_Unlink(& s->parent);
  (void) BoxPtr_Unlink(& s->child);
  return BOXTASK_OK;
}

/**********************
 * IO                 *
 **********************/

BOXEXPORT BoxException *
Box_Runtime_Pause_At_Print(BoxPtr *parent, BoxPtr *child) {
  fputs("\n", stdout);
  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_CHAR_At_Print(BoxPtr *parent, BoxPtr *child) {
  printf(SChar, *((BoxChar *) BoxPtr_Get_Target(child)));
  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_INT_At_Print(BoxPtr *parent, BoxPtr *child) {
  printf(SInt, *((BoxInt *) BoxPtr_Get_Target(child)));
  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_REAL_At_Print(BoxPtr *parent, BoxPtr *child){
  printf(SReal, *((BoxReal *) BoxPtr_Get_Target(child)));
  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_Point_At_Print(BoxPtr *parent, BoxPtr *child){
  BoxPoint *p = (BoxPoint *) BoxPtr_Get_Target(child);
  printf(SPoint, p->x, p->y);
  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_Str_At_Print(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = (BoxStr *) BoxPtr_Get_Target(child);
  if (s->ptr != NULL)
    fputs(s->ptr, stdout);
  return NULL;
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
  BoxPoint *p = (BoxPoint *) BoxVMX_Get_Child_Target(vmx);
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
  BoxPoint *p = (BoxPoint *) BoxVMX_Get_Child_Target(vmx);
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = sqrt(p->x*p->x + p->y*p->y);
  return BOXTASK_OK;
}

static BoxTask My_Math_Norm2(BoxVMX *vmx) {
  BoxPoint *p = (BoxPoint *) BoxVMX_Get_Child_Target(vmx);
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = p->x*p->x + p->y*p->y;
  return BOXTASK_OK;
}

static BoxTask My_Min_Open(BoxVMX *vmx) {
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = BOXREAL_MAX;
  return BOXTASK_OK;
}

static BoxTask My_Min_Real(BoxVMX *vmx) {
  BoxReal *cp = (BoxReal *) BoxVMX_Get_Parent_Target(vmx), c = *cp,
          x = *((BoxReal *) BoxVMX_Get_Child_Target(vmx));
  *cp = (x < c) ? x : c;
  return BOXTASK_OK;
}

static BoxTask My_Max_Open(BoxVMX *vmx) {
  *((BoxReal *) BoxVMX_Get_Parent_Target(vmx)) = BOXREAL_MIN;
  return BOXTASK_OK;
}

static BoxTask My_Max_Real(BoxVMX *vmx) {
  BoxReal *cp = (BoxReal *) BoxVMX_Get_Parent_Target(vmx), c = *cp,
          x = *((BoxReal *) BoxVMX_Get_Child_Target(vmx));
  *cp = (x > c) ? x : c;
  return BOXTASK_OK;
}

static BoxTask My_Vec_Real(BoxVMX *vmx) {
  BoxReal *angle = (BoxReal *) BoxVMX_Get_Child_Target(vmx);
  BoxPoint *p = (BoxPoint *) BoxVMX_Get_Parent_Target(vmx);
  p->x = cos(*angle);
  p->y = sin(*angle);
  return BOXTASK_OK;
}

static BoxTask My_Point_At_Ort(BoxVMX *vm) {
  BoxPoint *p_out = (BoxPoint *) BoxVMX_Get_Parent_Target(vm);
  BoxPoint *p_in = (BoxPoint *) BoxVMX_Get_Child_Target(vm);
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
  BoxVM_Set_Fail_Msg(vmx->vm, NULL);
  return BOXTASK_OK;
}

static BoxTask My_Fail(BoxVMX *vmx) {
  return BOXTASK_FAILURE;
}

static BoxTask My_Fail_Msg(BoxVMX *vmx) {
  BoxStr *s = (BoxStr *) BoxVMX_Get_Child_Target(vmx);
  char *msg = BoxStr_To_C_String(s);
  BoxVM_Set_Fail_Msg(vmx->vm, msg);
  Box_Mem_Free(msg);
  return BOXTASK_OK;
}

static BoxTask My_Num_Init(BoxVMX *vm) {
  BoxInt *length = (BoxInt *) BoxVMX_Get_Parent_Target(vm);
  *length = 0;
  return BOXTASK_OK;
}

static BoxTask My_IsValid_Init(BoxVMX *vm) {
  BoxInt *valid = (BoxInt *) BoxVMX_Get_Parent_Target(vm);
  *valid = 1;
  return BOXTASK_OK;
}

static BoxTask My_Int_At_IsValid(BoxVMX *vm) {
  BoxInt *valid = (BoxInt *) BoxVMX_Get_Parent_Target(vm);
  BoxInt *child = (BoxInt *) BoxVMX_Get_Child_Target(vm);
  *valid = (*valid && *child);
  return BOXTASK_OK;
}

static BoxTask My_Compare_Init(BoxVMX *vm) {
  BoxInt *compare = (BoxInt *) BoxVMX_Get_Parent_Target(vm);
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

#include "value.h"
#include "operator.h"
#include "namespace.h"
#include "compiler.h"

BoxVMCallNum Bltin_Proc_Add(BoxCmp *c, const char *proc_name,
                            BoxTask (*c_fn)(BoxVMX *)) {
  BoxVMCallNum call_num;

  /* We finally install the code (a C function) for the procedure */
  call_num = BoxVM_Allocate_Call_Num(c->vm);
  if (call_num == BOXVMCALLNUM_NONE)
    return BOXVMSYMID_NONE;

  if (!BoxVM_Install_Proc_CCode(c->vm, call_num, c_fn)) {
    (void) BoxVM_Deallocate_Call_Num(c->vm, call_num);
    return BOXVMSYMID_NONE;
  }

  (void) BoxVM_Set_Proc_Names(c->vm, call_num, NULL, proc_name);
  return call_num;
}

void Bltin_Comb_Def_With_Ids(BoxTypeId child, BoxCombType comb_type,
                             BoxTypeId parent, BoxTask (*c_fn)(BoxVMX *)) {
  /* We tell to the compiler that some procedures are associated to call_num */
  BoxType *child_new = Box_Get_Core_Type(child),
          *parent_new = Box_Get_Core_Type(parent);
  Bltin_Comb_Def(child_new, comb_type, parent_new, c_fn);
}

void Bltin_Comb_Def(BoxType *child, BoxCombType comb_type,
                    BoxType *parent, BoxTask (*c_fn)(BoxVMX *)) {
  /* We tell to the compiler that some procedures are associated to call_num */
  BoxCallable *callable;
  BoxType *comb;
  char *uid;

  callable = BoxCallable_Create_Undefined(parent, child);
  callable = BoxCallable_Define_From_CCallOld(callable, c_fn);
  comb = BoxType_Define_Combination(parent, comb_type, child, callable);
  assert(comb);

  uid = BoxType_Get_Repr(comb);
  BoxCallable_Set_Uid(callable, uid);
  Box_Mem_Free(uid);
}

void Bltin_Proc_Def(BoxType *parent, BoxType *child,
                    BoxTask (*c_fn)(BoxVMX *)) {
  Bltin_Comb_Def(child, BOXCOMBTYPE_AT, parent, c_fn);
}

void Bltin_Proc_Def_With_Id(BoxType *parent, BoxTypeId child_id,
                            BoxTask (*c_fn)(BoxVMX *)) {
  BoxType *child = Box_Get_Core_Type(child_id);
  assert(child);
  Bltin_Comb_Def(child, BOXCOMBTYPE_AT, parent, c_fn);
}

void Bltin_Proc_Def_With_Ids(BoxTypeId parent, BoxTypeId child,
                             BoxTask (*c_fn)(BoxVMX *)) {
  Bltin_Comb_Def_With_Ids(child, BOXCOMBTYPE_AT, parent, c_fn);
}

/* Register the core types in the current namespace, so that Box programs
 * can access and use them.
 */
static void My_Register_Core_Types(BoxCmp *c) {
  struct {
    const char *name;
    BoxTypeId  type;
  } *row, rows[] = {
    {"Char",        BOXTYPEID_CHAR},
    {"INT",         BOXTYPEID_INT},
    {"REAL",        BOXTYPEID_REAL},
    {"POINT",       BOXTYPEID_POINT},
    {"Int",         BOXTYPEID_SINT},
    {"Real",        BOXTYPEID_SREAL},
    {"Point",       BOXTYPEID_SPOINT},
    {"Num",         BOXTYPEID_NUM},
    {"Str",         BOXTYPEID_STR},
    {"Void",        BOXTYPEID_VOID},
    {"Ptr",         BOXTYPEID_PTR},
    {"CPtr",        BOXTYPEID_CPTR},
    {"If",          BOXTYPEID_IF},
    {"Else",        BOXTYPEID_ELSE},
    /*{"Elif",        BOXTYPEID_ELIF},*/
    {"For",         BOXTYPEID_FOR},
    {"Print",       BOXTYPEID_PRINT},
    {"Repr",        BOXTYPEID_REPR},
    {"Any",         BOXTYPEID_ANY},
    {"Compare",     BOXTYPEID_COMPARE},
    {"Get",         BOXTYPEID_Get},
    {"Set",         BOXTYPEID_Set},
    {"ARRAY",       BOXTYPEID_ARRAY},
    {"Array",       BOXTYPEID_Array},
    {(char *) NULL, BOXTYPEID_NONE}
  };

  for(row = & rows[0]; row->name; ++row) {
    Value v;
    Value_Init(& v, c);
    Value_Setup_As_Type(& v, Box_Get_Core_Type(row->type));
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, row->name, & v);
    Value_Finish(& v);
  }
}

/* Used to make the table of operations more compact in
 * My_Register_BinOps. This function maps a character to a BoxType
 * value. Example: My_Type_Of_Char(c, 'I') returns BOXTYPEID_INT,
 * My_Type_Of_Char(c, 'R') returns BOXTYPEID_REAL, etc.
 */
static BoxType *My_Type_Of_Char(BoxCmp *c, char t) {
  switch(t) {
  case ' ': return NULL;
  case 'C': return Box_Get_Core_Type(BOXTYPEID_CHAR);
  case 'I': return Box_Get_Core_Type(BOXTYPEID_INT);
  case 'R': return Box_Get_Core_Type(BOXTYPEID_REAL);
  case 'P': return Box_Get_Core_Type(BOXTYPEID_POINT);
  case 'i': return Box_Get_Core_Type(BOXTYPEID_SINT);
  case 'r': return Box_Get_Core_Type(BOXTYPEID_SREAL);
  case 'p': return Box_Get_Core_Type(BOXTYPEID_SPOINT);
  default:
    MSG_FATAL("My_Type_Of_Char: unexpected character.");
    return NULL;
  }
}

static OprAttr My_OprAttr_Of_Str(const char *s) {
  if (s == NULL)
    return OPR_ATTR_ALL;

  else {
    int a = 0;
    for(;*s != '\0'; s++) {
      switch(*s) {
      case 'a': a |= OPR_ATTR_ASSIGNMENT; break;
      case 'i': a |= OPR_ATTR_IGNORE_RES; break;
      case 'c': a |= OPR_ATTR_COMMUTATIVE; break;
      case 'r': a |= OPR_ATTR_UN_RIGHT; break;
      default:
        MSG_FATAL("My_OprAttr_Of_Str: error parsing string.");
        assert(0);
        return OPR_ATTR_INVALID;
      }
    }
    return (OprAttr) a;
  }
}

/* Register all the core unary operations for the Box compiler. */
static void My_Register_UnOps(BoxCmp *c) {
  struct {
    const char *types; /* Two characters describing the types of the result
                          and of the operand, following the map character->type
                          implemented by My_Type_Of_Char) */
    int        op;     /* Operator to which the operation refers */
    const char *mask,  /* Mask of attributes (a string which is converted
                          to an OprAttr by calling My_OprAttr_Of_Str) */
               *attr;  /* Attributes to set */
    int        g_op;   /* Generic opcode to use for assembling the operation */
  } *unop, unops[] = {
    { "Pp",  BOXASTUNOP_NEG,   "", NULL, BOXGOP_NEG},
    { "Rr",  BOXASTUNOP_NEG,   "", NULL, BOXGOP_NEG},
    { "Ii",  BOXASTUNOP_NEG,   "", NULL, BOXGOP_NEG},
    { "Rr", BOXASTUNOP_LINC,  "a", NULL, BOXGOP_INC},
    { "Ii", BOXASTUNOP_LINC,  "a", NULL, BOXGOP_INC},
    { "Rr", BOXASTUNOP_LDEC,  "a", NULL, BOXGOP_DEC},
    { "Ii", BOXASTUNOP_LDEC,  "a", NULL, BOXGOP_DEC},
    { "Rr", BOXASTUNOP_RINC, "ar", NULL, BOXGOP_INC},
    { "Ii", BOXASTUNOP_RINC, "ar", NULL, BOXGOP_INC},
    { "Rr", BOXASTUNOP_RDEC, "ar", NULL, BOXGOP_DEC},
    { "Ii", BOXASTUNOP_RDEC, "ar", NULL, BOXGOP_DEC},
    { "Ii", BOXASTUNOP_BNOT,   "", NULL, BOXGOP_BNOT},
    { "Ii",  BOXASTUNOP_NOT,   "", NULL, BOXGOP_LNOT},
    { NULL,               0, NULL, NULL,          0}
  };

  for(unop = & unops[0]; unop->types != NULL; ++unop) {
    Operator *opr = BoxCmp_UnOp_Get(c, (BoxASTUnOp) unop->op);
    BoxType *result = My_Type_Of_Char(c, unop->types[0]),
            *operand = My_Type_Of_Char(c, unop->types[1]);
    OprAttr mask = My_OprAttr_Of_Str(unop->mask),
            attr = My_OprAttr_Of_Str(unop->attr);
    Operation *opn = Operator_Add_Opn(opr, operand, NULL, result);
    Operation_Attr_Set(opn, mask, attr);
    opn->implem.opcode = (BoxGOp) unop->g_op;
  }
}

/* Register all the core binary operations for the Box compiler. */
static void My_Register_BinOps(BoxCmp *c) {
  struct {
    const char *types; /* Three characters describing the types of the result,
                          of the left and right operands (following the map
                          character->type implemented by My_Type_Of_Char) */
    int        op;     /* Operator to which the operation refers */
    const char *mask,  /* Mask of attributes (a string which is converted
                          to an OprAttr by calling My_OprAttr_Of_Str) */
               *attr;  /* Attributes to set */
    int        g_op;   /* Generic opcode to use for assembling the operation */

  } *binop, binops[] = {
    {"Ppp", BOXASTBINOP_ASSIGN, "ai", NULL, BOXGOP_MOV},
    {"Rrr", BOXASTBINOP_ASSIGN, "ai", NULL, BOXGOP_MOV},
    {"Iii", BOXASTBINOP_ASSIGN, "ai", NULL, BOXGOP_MOV},
    {"CCC", BOXASTBINOP_ASSIGN, "ai", NULL, BOXGOP_MOV},
    {"Ppp",    BOXASTBINOP_ADD,  "c", NULL, BOXGOP_ADD},
    {"Rrr",    BOXASTBINOP_ADD,  "c", NULL, BOXGOP_ADD},
    {"Iii",    BOXASTBINOP_ADD,  "c", NULL, BOXGOP_ADD},
    {"Ppp",    BOXASTBINOP_SUB,   "", NULL, BOXGOP_SUB},
    {"Rrr",    BOXASTBINOP_SUB,   "", NULL, BOXGOP_SUB},
    {"Iii",    BOXASTBINOP_SUB,   "", NULL, BOXGOP_SUB},
    {"Ppr",    BOXASTBINOP_MUL,   "", NULL, BOXGOP_PMULR},
    {"Prp",    BOXASTBINOP_MUL,   "", NULL, BOXGOP_PMULR},
    {"Rrr",    BOXASTBINOP_MUL,  "c", NULL, BOXGOP_MUL},
    {"Iii",    BOXASTBINOP_MUL,  "c", NULL, BOXGOP_MUL},
    {"Ppr",    BOXASTBINOP_DIV,   "", NULL, BOXGOP_PDIVR},
    {"Rrr",    BOXASTBINOP_DIV,   "", NULL, BOXGOP_DIV},
    {"Iii",    BOXASTBINOP_DIV,   "", NULL, BOXGOP_DIV},
    {"Iii",    BOXASTBINOP_REM,   "", NULL, BOXGOP_REM},
    {"Rrr",    BOXASTBINOP_POW,   "", NULL, BOXGOP_POW},
    {"Iii",    BOXASTBINOP_POW,   "", NULL, BOXGOP_POW},
    {"Iii",   BOXASTBINOP_BAND,  "c", NULL, BOXGOP_BAND},
    {"Iii",   BOXASTBINOP_BXOR,  "c", NULL, BOXGOP_BXOR},
    {"Iii",    BOXASTBINOP_BOR,  "c", NULL, BOXGOP_BOR},
    {"Iii",    BOXASTBINOP_SHL,   "", NULL, BOXGOP_SHL},
    {"Iii",    BOXASTBINOP_SHR,   "", NULL, BOXGOP_SHR},
    {"Iii",   BOXASTBINOP_LAND,  "c", NULL, BOXGOP_LAND},
    {"Iii",    BOXASTBINOP_LOR,  "c", NULL, BOXGOP_LOR},
    {"Ppp",  BOXASTBINOP_APLUS, "ai", NULL, BOXGOP_ADD},
    {"Rrr",  BOXASTBINOP_APLUS, "ai", NULL, BOXGOP_ADD},
    {"Iii",  BOXASTBINOP_APLUS, "ai", NULL, BOXGOP_ADD},
    {"Ppp", BOXASTBINOP_AMINUS, "ai", NULL, BOXGOP_SUB},
    {"Rrr", BOXASTBINOP_AMINUS, "ai", NULL, BOXGOP_SUB},
    {"Iii", BOXASTBINOP_AMINUS, "ai", NULL, BOXGOP_SUB},
    {"Rrr", BOXASTBINOP_ATIMES, "ai", NULL, BOXGOP_MUL},
    {"Iii", BOXASTBINOP_ATIMES, "ai", NULL, BOXGOP_MUL},
    {"Rrr",   BOXASTBINOP_ADIV, "ai", NULL, BOXGOP_DIV},
    {"Iii",   BOXASTBINOP_ADIV, "ai", NULL, BOXGOP_DIV},
    {"Iii",   BOXASTBINOP_AREM, "ai", NULL, BOXGOP_REM},
    {"Iii",   BOXASTBINOP_ASHL, "ai", NULL, BOXGOP_SHL},
    {"Iii",   BOXASTBINOP_ASHR, "ai", NULL, BOXGOP_SHR},
    {"Iii",  BOXASTBINOP_ABAND, "ai", NULL, BOXGOP_BAND},
    {"Iii",  BOXASTBINOP_ABXOR, "ai", NULL, BOXGOP_BXOR},
    {"Iii",   BOXASTBINOP_ABOR, "ai", NULL, BOXGOP_BOR},
    {"Ipp",     BOXASTBINOP_EQ,  "c", NULL, BOXGOP_EQ},
    {"Irr",     BOXASTBINOP_EQ,  "c", NULL, BOXGOP_EQ},
    {"Iii",     BOXASTBINOP_EQ,  "c", NULL, BOXGOP_EQ},
    {"Ipp",     BOXASTBINOP_NE,  "c", NULL, BOXGOP_NE},
    {"Irr",     BOXASTBINOP_NE,  "c", NULL, BOXGOP_NE},
    {"Iii",     BOXASTBINOP_NE,  "c", NULL, BOXGOP_NE},
    {"Irr",     BOXASTBINOP_LT,   "", NULL, BOXGOP_LT},
    {"Iii",     BOXASTBINOP_LT,   "", NULL, BOXGOP_LT},
    {"Irr",     BOXASTBINOP_LE,   "", NULL, BOXGOP_LE},
    {"Iii",     BOXASTBINOP_LE,   "", NULL, BOXGOP_LE},
    {"Irr",     BOXASTBINOP_GT,   "", NULL, BOXGOP_GT},
    {"Iii",     BOXASTBINOP_GT,   "", NULL, BOXGOP_GT},
    {"Irr",     BOXASTBINOP_GE,   "", NULL, BOXGOP_GE},
    {"Iii",     BOXASTBINOP_GE,   "", NULL, BOXGOP_GE},
    { NULL,                  0, NULL, NULL, 0}
  };

  for(binop = & binops[0]; binop->types != NULL; ++binop) {
    Operator *opr = BoxCmp_BinOp_Get(c, (BoxASTBinOp) binop->op);
    BoxType *result = My_Type_Of_Char(c, binop->types[0]),
            *left = My_Type_Of_Char(c, binop->types[1]),
            *right = My_Type_Of_Char(c, binop->types[2]);
    OprAttr mask = My_OprAttr_Of_Str(binop->mask),
            attr = My_OprAttr_Of_Str(binop->attr);
    Operation *opn = Operator_Add_Opn(opr, left, right, result);
    Operation_Attr_Set(opn, mask, attr);
    opn->implem.opcode = (BoxGOp) binop->g_op;
  }
}

/* Register all the conversion operations for the Box compiler. */
static void My_Register_Conversions(BoxCmp *c) {
  Operator *convert = & c->convert;
  BoxVMCallNum struc_to_point_call_num;

  struct {
    const char *types; /* Two characters describing the types of the source
                          and destination of the conversion, following the map
                          character->type implemented by My_Type_Of_Char) */
    const char *mask,  /* Mask of attributes (a string which is converted
                          to an OprAttr by calling My_OprAttr_Of_Str) */
               *attr;  /* Attributes to set */
    int        g_op;   /* Generic opcode to use for assembling the operation */

  } *conv, convs[] = {
    { "IR",   "", NULL, BOXGOP_REAL},
    { "CR",   "", NULL, BOXGOP_REAL},
    { "RI",   "", NULL, BOXGOP_INT},
    { "CI",   "", NULL, BOXGOP_INT},
    { NULL, NULL, NULL, 0}
  };

  for(conv = & convs[0]; conv->types != NULL; ++conv) {
    BoxType *src = My_Type_Of_Char(c, conv->types[0]),
            *dst = My_Type_Of_Char(c, conv->types[1]);
    OprAttr mask = My_OprAttr_Of_Str(conv->mask),
            attr = My_OprAttr_Of_Str(conv->attr);
    Operation *opn = Operator_Add_Opn(convert, src, NULL, dst);
    Operation_Attr_Set(opn, mask, attr);
    opn->implem.opcode = (BoxGOp) conv->g_op;
  }

  /* Conversion (Real, Real) -> Point */
  Operation *opn =
    Operator_Add_Opn(convert,
                     Box_Get_Core_Type(BOXTYPEID_REAL_COUPLE),
                     NULL, My_Type_Of_Char(c, 'P'));

  struc_to_point_call_num = Bltin_Proc_Add(c, "conv_2r_to_point", My_2R_To_P);
  Operation_Set_User_Implem(opn, struc_to_point_call_num);
}

BoxType *Bltin_Simple_Fn_Def(BoxCmp *c, const char *name,
                             BoxTypeId ret, BoxTypeId arg, BoxVMFunc fn) {
  BoxType *new_type;
  Value v;

  new_type = BoxType_Create_Ident(BoxType_Link(Box_Get_Core_Type(ret)), name);

  (void) Bltin_Proc_Def_With_Id(new_type, arg, fn);
  Value_Init(& v, c);
  Value_Setup_As_Type(& v, new_type);
  (void) BoxType_Unlink(new_type);
  Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, name, & v);
  Value_Finish(& v);
  return new_type;
}

static void My_Register_Std_IO(BoxCmp *c) {
  BoxType *t_print = Box_Get_Core_Type(BOXTYPEID_PRINT);
  BoxCombDef defs[] =
    {BOXCOMBDEF_I_AT_T(BOXTYPEID_PAUSE, t_print, Box_Runtime_Pause_At_Print),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_CHAR, t_print, Box_Runtime_CHAR_At_Print),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_INT, t_print, Box_Runtime_INT_At_Print),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_REAL, t_print, Box_Runtime_REAL_At_Print),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_SPOINT, t_print, Box_Runtime_Point_At_Print),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_STR, t_print, Box_Runtime_Str_At_Print)};
  size_t num_defs = sizeof(defs)/sizeof(BoxCombDef);
  (void) BoxCombDef_Define(defs, num_defs);
}

static void My_Register_Std_Procs(BoxCmp *c) {
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_CHAR, BOXTYPEID_CHAR, My_Char_Char);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_CHAR,  BOXTYPEID_INT, My_Char_Int);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_CHAR, BOXTYPEID_REAL, My_Char_Real);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_INT, BOXTYPEID_SINT, My_Int_Int);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_INT, BOXTYPEID_REAL, My_Int_Real);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_REAL, BOXTYPEID_SREAL, My_Real_Real);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_POINT, BOXTYPEID_SPOINT,
                                 My_Point_RealNumCouple);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_IF,    BOXTYPEID_SINT, My_If_Int);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_ELIF,    BOXTYPEID_SINT, My_If_Int);
  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_FOR,    BOXTYPEID_SINT, My_For_Int);

  c->bltin.subtype_init = Bltin_Proc_Add(c, "subtype_init", My_Subtype_Init);
  c->bltin.subtype_finish = Bltin_Proc_Add(c, "subtype_finish",
                                           My_Subtype_Finish);
}

static void My_Register_Math(BoxCmp *c) {
  BoxTypeId t_real = BOXTYPEID_SREAL,
          t_point = BOXTYPEID_SPOINT;
  struct {
    const char *name;
    BoxTypeId  parent,
               child;
    BoxVMFunc  func_begin,
               func;
  } *fn, fns[] = {
   { "Sqrt",  BOXTYPEID_REAL,  t_real, NULL,  My_Math_Sqrt},
   {  "Sin",  BOXTYPEID_REAL,  t_real, NULL,   My_Math_Sin},
   {  "Cos",  BOXTYPEID_REAL,  t_real, NULL,   My_Math_Cos},
   {  "Tan",  BOXTYPEID_REAL,  t_real, NULL,   My_Math_Tan},
   { "Asin",  BOXTYPEID_REAL,  t_real, NULL,  My_Math_Asin},
   { "Acos",  BOXTYPEID_REAL,  t_real, NULL,  My_Math_Acos},
   { "Atan",  BOXTYPEID_REAL,  t_real, NULL,  My_Math_Atan},
   {"Atan2",  BOXTYPEID_REAL, t_point, NULL,  My_Math_Atan2},
   {  "Exp",  BOXTYPEID_REAL,  t_real, NULL,   My_Math_Exp},
   {  "Log",  BOXTYPEID_REAL,  t_real, NULL,   My_Math_Log},
   {"Log10",  BOXTYPEID_REAL,  t_real, NULL, My_Math_Log10},
   { "Ceil",   BOXTYPEID_INT,  t_real, NULL,  My_Math_Ceil},
   {"Floor",   BOXTYPEID_INT,  t_real, NULL, My_Math_Floor},
   {  "Abs",  BOXTYPEID_REAL,  t_real, NULL,   My_Math_Abs},
   { "Norm",  BOXTYPEID_REAL, t_point, NULL,  My_Math_Norm},
   {"Norm2",  BOXTYPEID_REAL, t_point, NULL, My_Math_Norm2},
   {  "Vec", BOXTYPEID_POINT,  t_real, NULL,   My_Vec_Real},
   {  "Ort", BOXTYPEID_POINT, t_point, NULL, My_Point_At_Ort},
   {  "Min",  BOXTYPEID_REAL,  t_real, My_Min_Open, My_Min_Real},
   {  "Max",  BOXTYPEID_REAL,  t_real, My_Max_Open, My_Max_Real},
   {   NULL,  BOXTYPEID_NONE,  BOXTYPEID_NONE, NULL, NULL}
  };

  for(fn = fns; fn->func != NULL; fn++) {
    BoxType *func_type =
      Bltin_Simple_Fn_Def(c, fn->name, fn->parent, fn->child, fn->func);
    if (fn->func_begin != NULL)
      (void) Bltin_Proc_Def_With_Id(func_type, BOXTYPEID_BEGIN, fn->func_begin);
  }
}

static void My_Register_Sys(BoxCmp *c) {
  BoxType *fail_t = Bltin_Simple_Fn_Def(c, "Fail", BOXTYPEID_VOID,
                                        BOXTYPEID_STR, My_Fail_Msg);

  (void) Bltin_Proc_Def_With_Id(fail_t, BOXTYPEID_BEGIN, My_Fail_Clear_Msg);
  (void) Bltin_Proc_Def_With_Id(fail_t, BOXTYPEID_END, My_Fail);
  (void) Bltin_Simple_Fn_Def(c, "Exit", BOXTYPEID_VOID, BOXTYPEID_SINT,
                             My_Exit_Int);

  (void) Bltin_Proc_Def_With_Id(Box_Get_Core_Type(BOXTYPEID_NUM),
                                BOXTYPEID_BEGIN, My_Num_Init);

  BoxType *isvalid =
    Bltin_Simple_Fn_Def(c, "IsValid", BOXTYPEID_INT,
                        BOXTYPEID_BEGIN, My_IsValid_Init);
  (void) Bltin_Proc_Def_With_Id(isvalid, BOXTYPEID_INT, My_Int_At_IsValid);

  (void) Bltin_Proc_Def_With_Ids(BOXTYPEID_COMPARE, BOXTYPEID_BEGIN,
                                 My_Compare_Init);
}

/* Register bultin types, operation and functions */
void Bltin_Init(BoxCmp *c) {
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

BoxType *
Bltin_Create_Type(BoxCmp *c, const char *type_name, size_t type_size,
                  size_t alignment)
{
  Value v;
  BoxType *t;
  t = BoxType_Create_Ident(BoxType_Create_Intrinsic(type_size, alignment),
                           type_name);
  Value_Init(& v, c);
  Value_Setup_As_Type(& v, t);
  (void) BoxType_Unlink(t);
  Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, type_name, & v);
  Value_Finish(& v);
  return t;
}
