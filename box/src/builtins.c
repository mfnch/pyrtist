/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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
#include "compiler.h"
#include "builtins.h"

/* Important builtin types */
Type type_Point, type_RealNum, type_IntgNum, type_CharNum, type_String,
     type_File;

static Task Blt_Define_Basics(void);
static Task Blt_Define_Math(void);
static Task Blt_Define_Print(void);
static Task Blt_Define_Sys(void);

/* box-procedures */
static Task Print_Char(VMProgram *vmp);
static Task Print_Int(VMProgram *vmp);
static Task Print_Real(VMProgram *vmp);
static Task Print_Pnt(VMProgram *vmp);
static Task Print_String(VMProgram *vmp);
static Task Print_NewLine(VMProgram *vmp);
static Task Exit_Int(VMProgram *vmp);
static Task C_File_Open(VMProgram *vmp);
static Task C_File_String(VMProgram *vmp);
static Task C_File_Close(VMProgram *vmp);
static Task M_File_String(VMProgram *vmp);
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
static Task Sin_RealNum(VMProgram *vmp);
static Task Cos_RealNum(VMProgram *vmp);
static Task Tan_RealNum(VMProgram *vmp);
static Task Asin_RealNum(VMProgram *vmp);
static Task Acos_RealNum(VMProgram *vmp);
static Task Atan_RealNum(VMProgram *vmp);
static Task Exp_RealNum(VMProgram *vmp);
static Task Log_RealNum(VMProgram *vmp);
static Task Log10_RealNum(VMProgram *vmp);
static Task Sqrt_RealNum(VMProgram *vmp);
static Task Ceil_RealNum(VMProgram *vmp);
static Task Floor_RealNum(VMProgram *vmp);
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

/* Initialize the compiler:
 *  1) Creates the fundamental operators and associated operations
 *     (those between intrinsic types)
 *  2) Sets the output for compiled code.
 */
Task Builtins_Define() {
  Operation *opn;
  int status;

  /* Definisco i tipi intrinseci */
  {
    Intg t;
    TASK( Tym_Def_Intrinsic(& t, & NAME("Char"), sizeof(Char)) );
    assert(t == TYPE_CHAR);
    TASK( Tym_Def_Intrinsic(& t, & NAME("Int"), sizeof(Int)  ) );
    assert(t == TYPE_INTG);
    TASK( Tym_Def_Intrinsic(& t, & NAME("Real"), sizeof(Real)  ) );
    assert(t == TYPE_REAL);
    TASK( Tym_Def_Intrinsic(& t, & NAME("Point"), sizeof(Point) ) );
    assert(t == TYPE_POINT);
    TASK( Tym_Def_Intrinsic(& t, & NAME("(Object)"), 0 ) ); /* NOTE: ??? */
    assert(t == TYPE_OBJ);
    TASK( Tym_Def_Intrinsic(& t, & NAME("Void"), 0 ) );
    assert(t == TYPE_VOID);
    TASK( Tym_Def_Intrinsic(& t, & NAME("([)"), 0 ) );
    assert(t == TYPE_OPEN);
    TASK( Tym_Def_Intrinsic(& t, & NAME("(])"), 0 ) );
    assert(t == TYPE_CLOSE);
    TASK( Tym_Def_Intrinsic(& t, & NAME("(;)"), 0 ) );
    assert(t == TYPE_PAUSE);
    TASK( Tym_Def_Intrinsic(& t, & NAME("(\\)"), 0 ) );
    assert(t == TYPE_DESTROY);
    TASK( Tym_Def_Intrinsic(& t, & NAME("Ptr"), sizeof(Ptr) ) );

    /* Define conditional types */
    TASK( Tym_Def_Explicit_Alias(& t, & NAME("If"), TYPE_INT) );
    assert(t == TYPE_IF);
    TASK( Tym_Def_Explicit_Alias(& t, & NAME("For"), TYPE_INT) );
    assert(t == TYPE_FOR);
  }

  /* Inizializzo il segmento-dati */
  TASK( Cmp_Data_Init() );

  /* Inizializzo il segmento che contiene gli oggetti con valore immediato */
  TASK( Cmp_Imm_Init() );

  /* Creo gli operatori */
  status = 0; /* Se qualcosa va male trovero' status = 1, alla fine! */

  /* Operatore usato per la conversione fra tipi diversi */
  NEW_OPERATOR(converter, "of conversion");
  /* Operatori di assegnazione */
  NEW_OPERATOR(assign,"=");
  NEW_OPERATOR(aplus, "+=");
  NEW_OPERATOR(aminus,"-=");
  NEW_OPERATOR(atimes,"*=");
  NEW_OPERATOR(adiv,  "/=");
  NEW_OPERATOR(arem,  "%=");
  NEW_OPERATOR(aband, "&=");
  NEW_OPERATOR(abxor, "^=");
  NEW_OPERATOR(abor,  "|=");
  NEW_OPERATOR(ashl,  "<<=");
  NEW_OPERATOR(ashr,  ">>=");
  /* Operatori di incremento/decremento */
  NEW_OPERATOR(inc, "++");
  NEW_OPERATOR(dec, "--");
  /* Operatori aritmetici convenzionali */
  NEW_OPERATOR(pow, "**");
  NEW_OPERATOR(plus, "+");
  NEW_OPERATOR(minus,"-");
  NEW_OPERATOR(times,"*");
  NEW_OPERATOR(div,  "/");
  NEW_OPERATOR(rem,  "%");
  /* Operatori bit-bit */
  NEW_OPERATOR(bor,  "|");
  NEW_OPERATOR(bxor, "^");
  NEW_OPERATOR(band, "&");
  NEW_OPERATOR(bnot, "~");
  /* Operatori di shift */
  NEW_OPERATOR(shl, "<<");
  NEW_OPERATOR(shr, ">>");
  /* Operatori di confronto */
  NEW_OPERATOR(eq, "==");
  NEW_OPERATOR(ne, "!=");
  NEW_OPERATOR(gt, ">");
  NEW_OPERATOR(ge, ">=");
  NEW_OPERATOR(lt, "<");
  NEW_OPERATOR(le, "<=");
  /* Operatori logici */
  NEW_OPERATOR(lnot, "!");
  NEW_OPERATOR(land, "&&");
  NEW_OPERATOR(lor,  "||");

  /* Esco con errore se qualcuna delle precedenti creazioni e' fallita! */
  if ( status == 1 ) return Failed;

  TASK( Blt_Define_All() );

  status = 0; /* Se qualcosa va male trovero' status = 1, alla fine! */
  /* § OPERATORE DI ASSEGNAZIONE */
  /* §§ ASSEGNAZIONE DI PUNTI */
  ADD_OPERATION(assign,TYPE_POINT, type_Point, TYPE_POINT, ASM_MOV_PP, 0, 1, 1);
  /* §§ ASSEGNAZIONE DI REALI */
  ADD_OPERATION(assign, TYPE_REAL,type_RealNum,TYPE_REAL, ASM_MOV_RR, 0, 1, 1);
  /* §§ ASSEGNAZIONE DI INTERI */
  ADD_OPERATION(assign, TYPE_INTG,type_IntgNum,TYPE_INTG, ASM_MOV_II, 0, 1, 1);
  /* §§ ASSEGNAZIONE DI CARATTERI */
  ADD_OPERATION(assign, TYPE_CHAR, TYPE_CHAR,  TYPE_CHAR, ASM_MOV_CC, 0, 1, 1);

  /* § OPPOSTO BIT-PER-BIT DI UN INTERO */
  ADD_OPERATION(bnot, TYPE_INTG, TYPE_NONE,  TYPE_INTG, ASM_BNOT_I, 0, 1, 0);
  /* § OPERATORE DI AND BIT-PER-BIT FRA INTERI */
  ADD_OPERATION(band, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BAND_II, 1, 1, 0);
  /* § OPERATORE DI XOR BIT-PER-BIT FRA INTERI */
  ADD_OPERATION(bxor, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BXOR_II, 1, 1, 0);
  /* § OPERATORE DI OR BIT-PER-BIT FRA INTERI */
  ADD_OPERATION(bor, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BOR_II, 1, 1, 0);
  /* § OPERATORE DI SHIFT-LEFT */
  ADD_OPERATION(shl, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SHL_II, 0, 1, 0);
  /* § OPERATORE DI SHIFT-RIGHT */
  ADD_OPERATION(shr, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SHR_II, 0, 1, 0);
  /* § OPERATORE DI INCREMENTO */
  /* §§ INCREMENTO(SINISTRO) DI UN INTERO */
  ADD_OPERATION(inc, TYPE_INTG, TYPE_NONE,  TYPE_INTG,  ASM_INC_I, 0, 1, 1);
  /* §§ INCREMENTO(DESTRO) DI UN INTERO */
  ADD_OPERATION(inc, TYPE_NONE, TYPE_INTG,  TYPE_INTG,  ASM_INC_I, 0, 1, 1);
  /* §§ INCREMENTO(SINISTRO) DI UN REALE */
  ADD_OPERATION(inc, TYPE_REAL, TYPE_NONE,  TYPE_REAL,  ASM_INC_R, 0, 1, 1);
  /* §§ INCREMENTO(DESTRO) DI UN REALE */
  ADD_OPERATION(inc, TYPE_NONE, TYPE_REAL,  TYPE_REAL,  ASM_INC_R, 0, 1, 1);
  /* § OPERATORE DI DECREMENTO */
  /* §§ DECREMENTO(SINISTRO) DI UN INTERO */
  ADD_OPERATION(dec, TYPE_INTG, TYPE_NONE,  TYPE_INTG,  ASM_DEC_I, 0, 1, 1);
  /* §§ DECREMENTO(DESTRO) DI UN INTERO */
  ADD_OPERATION(dec, TYPE_NONE, TYPE_INTG,  TYPE_INTG,  ASM_DEC_I, 0, 1, 1);
  /* §§ DECREMENTO(SINISTRO) DI UN REALE */
  ADD_OPERATION(dec, TYPE_REAL, TYPE_NONE,  TYPE_REAL,  ASM_DEC_R, 0, 1, 1);
  /* §§ DECREMENTO(DESTRO) DI UN REALE */
  ADD_OPERATION(dec, TYPE_NONE, TYPE_REAL,  TYPE_REAL,  ASM_DEC_R, 0, 1, 1);

  /* § OPERATORE DI POTENZA */
  /* §§ POTENZA DI UN REALE */
  ADD_OPERATION(pow, type_RealNum, type_RealNum, type_RealNum, ASM_POW_RR, 0, 1, 0);
  /* §§ POTENZA DI UN INTERO */
  ADD_OPERATION(pow, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_POW_II, 0, 1, 0);

  /* § OPERATORE DI SOMMA */
  /* §§ SOMMA FRA INTERI */
  //ADD_OPERATION(plus,   TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_ADD_II, 1, 1, 0);

  /* §§ SOMMA FRA REALI E INTERI */
  ADD_OPERATION(plus, type_RealNum, type_RealNum, type_RealNum, ASM_ADD_RR, 1, 1, 0);
  ADD_OPERATION(plus, type_IntgNum, type_IntgNum, type_IntgNum, ASM_ADD_II, 1, 1, 0);
  /* §§ SOMMA FRA PUNTI */
  ADD_OPERATION(plus,  type_Point,type_Point, TYPE_POINT,  ASM_ADD_PP, 1, 1, 0);

  /* § OPERATORE DI DIFFERENZA */
  /* §§ OPPOSTO DI UN INTERO */
  ADD_OPERATION(minus,  TYPE_INTG, TYPE_NONE,  TYPE_INTG,   ASM_NEG_I, 0, 1, 0);
  /* §§ OPPOSTO DI UN REALE */
  ADD_OPERATION(minus,  TYPE_REAL, TYPE_NONE,  TYPE_REAL,   ASM_NEG_R, 0, 1, 0);
  /* §§ OPPOSTO DI UN PUNTO */
  ADD_OPERATION(minus, type_Point, TYPE_NONE, TYPE_POINT,   ASM_NEG_P, 0, 1, 0);

  /* §§ DIFFERENZA FRA REALI E INTERI */
  ADD_OPERATION(minus, type_RealNum, type_RealNum, type_RealNum, ASM_SUB_RR, 0, 1, 0);
  ADD_OPERATION(minus, type_IntgNum, type_IntgNum, type_IntgNum, ASM_SUB_II, 0, 1, 0);
  /* §§ DIFFERENZA FRA PUNTI */
  ADD_OPERATION(minus, type_Point,type_Point, TYPE_POINT,  ASM_SUB_PP, 0, 1, 0);

  /* § OPERATORE DI PRODOTTO */
  /* §§ PRODOTTO FRA REALI E INTERI */
  ADD_OPERATION(times, type_RealNum, type_RealNum, type_RealNum, ASM_MUL_RR, 1, 1, 0);
  ADD_OPERATION(times, type_IntgNum, type_IntgNum, type_IntgNum, ASM_MUL_II, 1, 1, 0);
  /* §§ PRODOTTO REALE * PUNTO O VICEVERSA */
  ADD_OPERATION(times, type_Point, type_RealNum, TYPE_POINT, ASM_PMULR_P, 1, 1, 0);

  /* § OPERATORE DI DIVISIONE */
  /* §§ DIVISIONE FRA REALI E INTERI */
  ADD_OPERATION(div, type_RealNum, type_RealNum, type_RealNum, ASM_DIV_RR, 0, 1, 0);
  ADD_OPERATION(div, type_IntgNum, type_IntgNum, type_IntgNum, ASM_DIV_II, 0, 1, 0);
  /* §§ DIVISIONE PUNTO / REALE */
  ADD_OPERATION(div, type_Point, type_RealNum, TYPE_POINT, ASM_PDIVR_P, 0, 1, 0);

  /* § RESTO DELLA DIVISIONE FRA INTERI */
  ADD_OPERATION(rem,  TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_REM_II,  0, 1, 0);

  /* § OPERATORE DI UGUAGLIANZA */
  /* §§ UGUAGLIANZA FRA INTERI */
  ADD_OPERATION(eq,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_EQ_II,   1, 1, 0);
  /* §§ UGUAGLIANZA FRA REALI */
  ADD_OPERATION(eq,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_EQ_RR,   1, 1, 0);
  /* §§ UGUAGLIANZA FRA PUNTI */
  ADD_OPERATION(eq,  type_Point,type_Point, TYPE_INTG, ASM_EQ_PP,   1, 1, 0);
  /* § OPERATORE DI DISUGUAGLIANZA */
  /* §§ DISUGUAGLIANZA FRA INTERI */
  ADD_OPERATION(ne,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_NE_II,   1, 1, 0);
  /* §§ DISUGUAGLIANZA FRA REALI */
  ADD_OPERATION(ne,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_NE_RR,   1, 1, 0);
  /* §§ DISUGUAGLIANZA FRA PUNTI */
  ADD_OPERATION(ne,  type_Point,type_Point, TYPE_INTG, ASM_NE_PP,   1, 1, 0);
  /* § OPERATORE DI MINORE */
  /* §§ MINORE FRA INTERI */
  ADD_OPERATION(lt,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_LT_II,   0, 1, 0);
  /* §§ MINORE FRA REALI */
  ADD_OPERATION(lt,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_LT_RR,   0, 1, 0);
  /* § OPERATORE DI MINORE-UGUALE */
  /* §§ MINORE-UGUALE FRA INTERI */
  ADD_OPERATION(le,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_LE_II,   0, 1, 0);
  /* §§ MINORE-UGUALE FRA REALI */
  ADD_OPERATION(le,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_LE_RR,   0, 1, 0);
  /* § OPERATORE DI MAGGIORE */
  /* §§ MAGGIORE FRA INTERI */
  ADD_OPERATION(gt,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_GT_II,   0, 1, 0);
  /* §§ MAGGIORE FRA REALI */
  ADD_OPERATION(gt,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_GT_RR,   0, 1, 0);
  /* § OPERATORE DI MAGGIORE-UGUALE */
  /* §§ MAGGIORE-UGUALE FRA INTERI */
  ADD_OPERATION(ge,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_GE_II,   0, 1, 0);
  /* §§ MAGGIORE-UGUALE FRA REALI */
  ADD_OPERATION(ge,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_GE_RR,   0, 1, 0);
  /* § OPERATORE DI NOT LOGICO DI UN INTERO */
  ADD_OPERATION(lnot, TYPE_INTG, TYPE_NONE, TYPE_INTG, ASM_LNOT_I,  0, 1, 0);
  /* § OPERATORE DI AND LOGICO FRA INTERI */
  ADD_OPERATION(land, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_LAND_II, 1, 1, 0);
  /* § OPERATORE DI OR LOGICO FRA INTERI */
  ADD_OPERATION(lor,  TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_LOR_II,  1, 1, 0);

  /*******OPERATORI DI ASSEGNAZIONE DEL TIPO x= DOVE x E' UN OPERATORE*******/
  /* § OPERATORE &= FRA INTERI */
  ADD_OPERATION(aband, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BAND_II, 0, 1, 1);
  /* § OPERATORE ^= FRA INTERI */
  ADD_OPERATION(abxor, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BXOR_II, 0, 1, 1);
  /* § OPERATORE |= FRA INTERI */
  ADD_OPERATION(abor, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BOR_II, 0, 1, 1);
  /* § OPERATORE <<= FRA INTERI */
  ADD_OPERATION(ashl, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SHL_II, 0, 1, 1);
  /* § OPERATORE >>= FRA INTERI */
  ADD_OPERATION(ashr, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SHR_II, 0, 1, 1);
  /* § OPERATORE += */
  /* §§ += FRA INTERI */
  ADD_OPERATION(aplus,  TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_ADD_II, 0, 1, 1);
  /* §§ += FRA REALI */
  ADD_OPERATION(aplus,  TYPE_REAL, TYPE_REAL,  TYPE_REAL,  ASM_ADD_RR, 0, 1, 1);
  /* §§ += FRA PUNTI */
  ADD_OPERATION(aplus, TYPE_POINT,type_Point, TYPE_POINT,  ASM_ADD_PP, 0, 1, 1);
  /* § OPERATORE -= */
  /* §§ -= FRA INTERI */
  ADD_OPERATION(aminus, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SUB_II, 0, 1, 1);
  /* §§ -= FRA REALI */
  ADD_OPERATION(aminus, TYPE_REAL, TYPE_REAL,  TYPE_REAL,  ASM_SUB_RR, 0, 1, 1);
  /* §§ -= FRA PUNTI */
  ADD_OPERATION(aminus,TYPE_POINT,type_Point, TYPE_POINT,  ASM_SUB_PP, 0, 1, 1);
  /* § OPERATORE *= */
  /* §§ *= FRA INTERI */
  ADD_OPERATION(atimes, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_MUL_II, 0, 1, 1);
  /* §§ *= FRA REALI */
  ADD_OPERATION(atimes, TYPE_REAL, TYPE_REAL,  TYPE_REAL,  ASM_MUL_RR, 0, 1, 1);
  /* § OPERATORE /= */
  /* §§ /= FRA INTERI */
  ADD_OPERATION(adiv,   TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_DIV_II, 0, 1, 1);
  /* §§ /= FRA REALI */
  ADD_OPERATION(adiv,   TYPE_REAL, TYPE_REAL,  TYPE_REAL,  ASM_DIV_RR, 0, 1, 1);
  /* § OPERATORE %= FRA INTERI */
  ADD_OPERATION(arem,   TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_REM_II, 0, 1, 1);

  /* OPERATORI CHE DEFINISCONO */
  cmp_opr.assign->can_define = 1;

  opn = Cmp_Operation_Add(cmp_opr.converter, TYPE_INTG, TYPE_NONE, TYPE_REAL);
  status |= ( opn == NULL );
  opn->is.commutative = 0;
  opn->is.intrinsic = 1;
  opn->is.assignment = 0;
  opn->is.link = 0;
  opn->asm_code = ASM_REAL_I;

  opn = Cmp_Operation_Add(cmp_opr.converter, TYPE_CHAR, TYPE_NONE, TYPE_REAL);
  status |= ( opn == NULL );
  opn->is.commutative = 0;
  opn->is.intrinsic = 1;
  opn->is.assignment = 0;
  opn->is.link = 0;
  opn->asm_code = ASM_REAL_C;

  opn = Cmp_Operation_Add(cmp_opr.converter, TYPE_REAL, TYPE_NONE, TYPE_INTG);
  status |= ( opn == NULL );
  opn->is.commutative = 0;
  opn->is.intrinsic = 1;
  opn->is.assignment = 0;
  opn->is.link = 0;
  opn->asm_code = ASM_INTG_R;

  /* Se il caricamento degli operatori non e' perfettamente riuscito esco! */
  if ( status == 1 ) return Failed;

  return Success;
}

static Task Blt_Define_Basics(void) {
  Intg type_2RealNum;

  /* Ora definisco il tipo stringa */
  type_String = Tym_Def_Array_Of(-1, TYPE_CHAR);
  if ( type_String == TYPE_NONE ) return Failed;

  /* Definisco type_IntgNum --> (Int < Char) */
  type_IntgNum = TYPE_NONE;
  TASK( Tym_Def_Specie(& type_IntgNum, TYPE_CHAR) );
  TASK( Tym_Def_Specie(& type_IntgNum, TYPE_INTG) );

  /* Definisco type_RealNum --> (Real < Int < Char) */
  type_RealNum = TYPE_NONE;
  TASK( Tym_Def_Specie(& type_RealNum, TYPE_CHAR) );
  TASK( Tym_Def_Specie(& type_RealNum, TYPE_INTG) );
  TASK( Tym_Def_Specie(& type_RealNum, TYPE_REAL) );

  type_2RealNum = TYPE_NONE;
  TASK( Tym_Def_Structure(& type_2RealNum, type_RealNum) );
  TASK( Tym_Def_Structure(& type_2RealNum, type_RealNum) );
  type_Point = TYPE_NONE;
  TASK( Tym_Def_Specie(& type_Point, type_2RealNum) );
  TASK( Tym_Def_Specie(& type_Point, TYPE_POINT) );

  /* Ora faccio l'overloading dell'operatore di conversione
   * e definisco le conversioni automatiche che il compilatore
   * dovra' fare.
   */
  {
    UInt sym_num, call_num;
    Operation *opn;

    TASK( VM_Sym_New_Call(cmp_vm, & sym_num) );
    TASK( VM_Proc_Install_CCode(cmp_vm, & call_num, Conv_2RealNum_to_Point,
     "(noname)", "conv_2Real_to_Point") );
    TASK( VM_Sym_Def_Call(cmp_vm, sym_num, call_num) );

    opn = Cmp_Operation_Add(cmp_opr.converter, type_2RealNum, TYPE_NONE, TYPE_POINT);
    if ( opn == NULL ) return Failed;
    opn->is.commutative = 0;
    opn->is.intrinsic = 0;
    opn->is.assignment = 0;
    opn->module = sym_num;
  }

  /* Define the conversions (example: Real@Int such as: a = Int[ 1.2 ]) */
  TASK(Cmp_Builtin_Proc_Def(TYPE_CHAR, BOX_CREATION, TYPE_CHAR, Char_Char));
  TASK(Cmp_Builtin_Proc_Def(TYPE_INTG, BOX_CREATION, TYPE_CHAR, Char_Int ));
  TASK(Cmp_Builtin_Proc_Def(TYPE_REAL, BOX_CREATION, TYPE_CHAR, Char_Real));
  TASK(Cmp_Builtin_Proc_Def(type_IntgNum, BOX_CREATION, TYPE_INTG, Int_IntNum ));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum, BOX_CREATION, TYPE_INTG, Int_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum, BOX_CREATION, TYPE_REAL, Real_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_Point, BOX_CREATION, TYPE_POINT, Point_RealNumCouple ));
  TASK(Cmp_Builtin_Proc_Def(type_IntgNum, BOX_CREATION, TYPE_IF, If_IntNum ));
  TASK(Cmp_Builtin_Proc_Def(type_IntgNum, BOX_CREATION, TYPE_FOR, For_IntNum ));
  return Success;
}

#define DEFINE_TYPE(name, type) \
  Tym_Def_Explicit_Alias(& type_##name, & NAME(#name), type)

static Task Blt_Define_Math(void) {
  Intg type_Sin, type_Cos, type_Tan, type_Asin, type_Acos, type_Atan,
   type_Exp, type_Log, type_Log10, type_Sqrt, type_Ceil, type_Floor,
   type_Min, type_Max, type_Vec;

  TASK( DEFINE_TYPE(Sin,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Cos,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Tan,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Asin,  TYPE_REAL) );
  TASK( DEFINE_TYPE(Acos,  TYPE_REAL) );
  TASK( DEFINE_TYPE(Atan,  TYPE_REAL) );
  TASK( DEFINE_TYPE(Exp,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Log,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Log10, TYPE_REAL) );
  TASK( DEFINE_TYPE(Sqrt,  TYPE_REAL) );
  TASK( DEFINE_TYPE(Ceil,  TYPE_INT)  );
  TASK( DEFINE_TYPE(Floor, TYPE_INT)  );
  TASK( DEFINE_TYPE(Min,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Max,   TYPE_REAL) );
  TASK( DEFINE_TYPE(Vec,   TYPE_POINT) );

  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Cos,  Cos_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Sin,  Sin_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Tan,  Tan_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Asin, Asin_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Acos, Acos_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Atan, Atan_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Exp,  Exp_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Log,  Log_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Log10,Log10_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Sqrt, Sqrt_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Ceil, Ceil_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Floor,Floor_RealNum));
  TASK(Cmp_Builtin_Proc_Def(TYPE_OPEN,   BOX_CREATION,type_Min,  Min_Open));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Min,  Min_RealNum));
  TASK(Cmp_Builtin_Proc_Def(TYPE_OPEN,   BOX_CREATION,type_Max,  Max_Open));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Max,  Max_RealNum));
  TASK(Cmp_Builtin_Proc_Def(type_RealNum,BOX_CREATION,type_Vec,  Vec_RealNum));
  return Success;
}

static Task Blt_Define_Print(void) {
  Intg type_Print;
  TASK(Tym_Def_Explicit_Alias(& type_Print, & NAME("Print"), TYPE_VOID));
  TASK(Cmp_Builtin_Proc_Def(TYPE_CHAR,  BOX_CREATION,type_Print, Print_Char));
  TASK(Cmp_Builtin_Proc_Def(TYPE_INTG,  BOX_CREATION,type_Print, Print_Int));
  TASK(Cmp_Builtin_Proc_Def(TYPE_REAL,  BOX_CREATION,type_Print, Print_Real));
  TASK(Cmp_Builtin_Proc_Def(type_Point, BOX_CREATION,type_Print, Print_Pnt));
  TASK(Cmp_Builtin_Proc_Def(type_String,BOX_CREATION,type_Print, Print_String));
  TASK(Cmp_Builtin_Proc_Def(TYPE_PAUSE, BOX_CREATION,type_Print,Print_NewLine));
  /*Tym_Print_Procedure(stdout, type_new);*/
  return Success;
}

typedef struct {
  unsigned int opened : 1;
  char *name;
  FILE *file;
} File;

static Task Blt_Define_Sys(void) {
  Intg type_Exit;
  TASK( Tym_Def_Explicit_Alias(& type_Exit, & NAME("Exit"), TYPE_VOID) );
  TASK( Cmp_Builtin_Proc_Def(TYPE_INT, BOX_CREATION, type_Exit, Exit_Int) );

  TASK( Tym_Def_Intrinsic(& type_File, & NAME("File"), sizeof(File)) );

  TASK(Cmp_Builtin_Proc_Def(TYPE_OPEN,  BOX_CREATION, type_File, C_File_Open));
  TASK(Cmp_Builtin_Proc_Def(type_String,BOX_CREATION, type_File, C_File_String));
  TASK(Cmp_Builtin_Proc_Def(TYPE_CLOSE, BOX_CREATION, type_File, C_File_Close));
  TASK(Cmp_Builtin_Proc_Def(type_String,BOX_MODIFICATION,type_File,M_File_String));
  return Success;
}

/*******************************BOX-PROCEDURES********************************/
static Task Print_Char(VMProgram *vmp) {
  printf(SChar, BOX_VM_ARG1(vmp, Char));
  return Success;
}
static Task Print_Int(VMProgram *vmp) {
  printf(SIntg, BOX_VM_ARG1(vmp, Intg));
  return Success;
}
static Task Print_Real(VMProgram *vmp) {
  printf(SReal, BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Print_Pnt(VMProgram *vmp) {
  Point *p = BOX_VM_ARGPTR1(vmp, Point);
  printf(SPoint, p->x, p->y);
  return Success;
}
static Task Print_String(VMProgram *vmp) {
  printf("%s", BOX_VM_ARGPTR1(vmp, char));
  return Success;
}
static Task Print_NewLine(VMProgram *vmp) {
  printf("\n"); return Success;
}

/*****************************************************************************
 *                           SYSTEM PROCEDURES                               *
 *****************************************************************************/

/* This function is not politically correct!!! */
static Task Exit_Int(VMProgram *vmp) {
  exit(BOX_VM_ARG1(vmp, Intg));
}

static Task C_File_Open(VMProgram *vmp) {
  File *f = BOX_VM_CURRENTPTR(vmp, File);
  f->opened = 0;
  f->name = "";
  return Success;
}
static Task C_File_String(VMProgram *vmp) {
  File *f = BOX_VM_CURRENTPTR(vmp, File);
  f->name = BOX_VM_ARGPTR1(vmp, char);
  return Success;
}
static Task C_File_Close(VMProgram *vmp) {
  File *f = BOX_VM_CURRENTPTR(vmp, File);
  f->file = fopen(f->name, "w");
  if ( f->file == NULL ) {
    fprintf(stderr, "Error opening the file. Exiting!\n");
    return Failed;
  }
  f->opened = 1;
  return Success;
}
static Task M_File_String(VMProgram *vmp) {
  File *f = BOX_VM_CURRENTPTR(vmp, File);
  if ( ! f->opened ) {
    fprintf(stderr, "Error: writing to a not opened file. Exiting!\n");
    return Failed;
  }
  fprintf(f->file, "%s", BOX_VM_ARGPTR1(vmp, char));
  return Success;
}

/*****************************************************************************
 *                       FUNCTIONS FOR CONVERSION                            *
 *****************************************************************************/
static Task Conv_2RealNum_to_Point(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Point) = *(BOX_VM_ARGPTR1(vmp, Point));
  return Success;
}

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
static Task Sin_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = sin(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Cos_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = cos(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Tan_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = tan(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Asin_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = asin(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Acos_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = acos(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Atan_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = atan(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Exp_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = exp(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Log_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = log(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Log10_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = log10(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Sqrt_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = sqrt(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Ceil_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Intg) = ceil(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Floor_RealNum(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Intg) = floor(BOX_VM_ARG1(vmp, Real));
  return Success;
}
static Task Min_Open(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = REAL_MAX;
  return Success;
}
static Task Min_RealNum(VMProgram *vmp) {
  Real *cp = BOX_VM_CURRENTPTR(vmp, Real), c = *cp,
       x = BOX_VM_ARG1(vmp, Real);
  *cp = (x < c) ? x : c;
  return Success;
}
static Task Max_Open(VMProgram *vmp) {
  BOX_VM_CURRENT(vmp, Real) = REAL_MIN;
  return Success;
}
static Task Max_RealNum(VMProgram *vmp) {
  Real *cp = BOX_VM_CURRENTPTR(vmp, Real), c = *cp,
       x = BOX_VM_ARG1(vmp, Real);
  *cp = (x > c) ? x : c;
  return Success;
}
static Task Vec_RealNum(VMProgram *vmp) {
  Real angle = BOX_VM_ARG1(vmp, Real);
  Point *p = BOX_VM_CURRENTPTR(vmp, Point);
  p->x = cos(angle);
  p->y = sin(angle);
  return Success;
}
