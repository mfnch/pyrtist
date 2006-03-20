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
#include "registers.h"
#include "compiler.h"
#include "builtins.h"

/* Important builtin types */
Intg type_Point, type_RealNum, type_IntgNum, type_CharNum, type_String;

static Task Tmp(void);
static Task Blt_Define_Basics(void);
static Task Blt_Define_Math(void);
static Task Blt_Define_Print(void);

/* box-procedures */
static Task Print_Char(void);
static Task Print_Int(void);
static Task Print_Real(void);
static Task Print_Pnt(void);
static Task Print_String(void);
static Task Print_NewLine(void);
/* Functions for conversions */
static Task Conv_2RealNum_to_Point(void);
/* Mathematical functions */
static Task Sin_RealNum(void);
static Task Cos_RealNum(void);
static Task Tan_RealNum(void);
static Task Asin_RealNum(void);
static Task Acos_RealNum(void);
static Task Atan_RealNum(void);
static Task Exp_RealNum(void);
static Task Log_RealNum(void);
static Task Log10_RealNum(void);
static Task Sqrt_RealNum(void);
static Task Ceil_RealNum(void);
static Task Floor_RealNum(void);

/******************************************************************************
 * Le procedure che seguono servono a inizializzare/resettare il compilatore. *
 ******************************************************************************/

/* Destinazione attuale del codice compilato dal compilatore */
AsmOut *Cmp_Curr_Output;

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
  TASK( Tmp() );
  TASK( Blt_Define_Basics() );
  TASK( Blt_Define_Math() );
  TASK( Blt_Define_Print() );
  return Success;
}

/* Initialize the compiler:
 *  1) Creates the fundamental operators and associated operations
 *     (those between intrinsic types)
 *  2) Sets the output for compiled code.
 */
Task Builtins_Define()
{
  Operation *opn;
  int status;
  static UInt typl_nreg[NUM_TYPES] = REG_OCC_TYP_SIZE;
  static UInt typl_nvar[NUM_TYPES] = VAR_OCC_TYP_SIZE;
  Name intrinsic_type[6] = {
    {4, "CHAR"},
    {3, "INT"},
    {4, "REAL"},
    {5, "POINT"},
    {6, "OBJECT"},
    {4, "VOID"}
  };

  MSG_LOCATION("Cmp_Define_Builtins");

  /* Inizializzo le liste di occupazione di registri e variabili */
  TASK( Reg_Init(typl_nreg) );
  TASK( Var_Init(typl_nvar) );

  /* Setto l'output della compilazione */
  Cmp_Curr_Output = VM_Asm_Out_New(-1);
  TASK( VM_Asm_Out_Set(Cmp_Curr_Output) );
  TASK( Cmp_Box_Instance_Begin( NULL ) );

  /* Definisco i tipi intrinseci */
  if (
     ( Tym_Intrinsic_New( intrinsic_type+0, sizeof(Char)  ) != TYPE_CHAR  )
  || ( Tym_Intrinsic_New( intrinsic_type+1, sizeof(Intg)  ) != TYPE_INTG  )
  || ( Tym_Intrinsic_New( intrinsic_type+2, sizeof(Real)  ) != TYPE_REAL  )
  || ( Tym_Intrinsic_New( intrinsic_type+3, sizeof(Point) ) != TYPE_POINT )
  || ( Tym_Intrinsic_New( intrinsic_type+4, 0 ) != TYPE_OBJ )
  || ( Tym_Intrinsic_New( intrinsic_type+5, 0 ) != TYPE_VOID )
  ) return Failed;

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

  /* T E M P O R A N E O ! ! ! */
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

  /* Se il caricamento degli operatori non e' perfettamente riuscito esco! */
  if ( status == 1 ) return Failed;

  return Success;
}

static Task Tmp(void) {
  Intg type_New, type_New2;

  type_New = Tym_Box_Abstract_New(& ((Name) {7, "Punto2d"}));
  if (type_New < 0) return Failed;

  if IS_FAILED(
    Tym_Box_Abstract_Member_New(type_New, & ((Name) {6, "comp_x"}), TYPE_REAL)
  ) return Failed;

  if IS_FAILED(
    Tym_Box_Abstract_Member_New(type_New, & ((Name) {6, "comp_y"}), TYPE_REAL)
  ) return Failed;

  type_New2 = Tym_Box_Abstract_New(& ((Name) {7, "Punto3d"}));
  if (type_New2 < 0) return Failed;

  if IS_FAILED(
    Tym_Box_Abstract_Member_New(type_New2, & ((Name) {2, "xy"}), type_New)
  ) return Failed;

  if IS_FAILED(
    Tym_Box_Abstract_Member_New(type_New2, & ((Name) {1, "z"}), TYPE_REAL)
  ) return Failed;

  /*Tym_Box_Abstract_Print(stdout, type_new);
  Tym_Box_Abstract_Print(stdout, type_new2);*/
  return Success;
}

static Task Blt_Define_Basics(void) {
  Intg type_2RealNum;

  /* Ora definisco il tipo stringa */
  type_String = Tym_Build_Array_Of(-1, TYPE_CHAR);
  if ( type_String == TYPE_NONE ) return Failed;

  /* Definisco type_IntgNum --> (Int < Char) */
  type_IntgNum = TYPE_NONE;
  TASK( Tym_Build_Specie(& type_IntgNum, TYPE_INTG) );
  TASK( Tym_Build_Specie(& type_IntgNum, TYPE_CHAR) );
  /* Definisco type_RealNum --> (Real < Int < Char) */
  type_RealNum = TYPE_NONE;
  TASK( Tym_Build_Specie(& type_RealNum, TYPE_REAL) );
  TASK( Tym_Build_Specie(& type_RealNum, TYPE_INTG) );
  TASK( Tym_Build_Specie(& type_RealNum, TYPE_CHAR) );

  type_2RealNum = TYPE_NONE;
  TASK( Tym_Build_Structure(& type_2RealNum, type_RealNum) );
  TASK( Tym_Build_Structure(& type_2RealNum, type_RealNum) );
  type_Point = TYPE_NONE;
  TASK( Tym_Build_Specie(& type_Point, TYPE_POINT) );
  TASK( Tym_Build_Specie(& type_Point, type_2RealNum) );

  /* Ora faccio l'overloading dell'operatore di conversione
   * e definisco le conversioni automatiche che il compilatore
   * dovra' fare.
   */
  {
    Intg m;
    ModulePtr p;
    Operation *opn;

    p.c_func = Conv_2RealNum_to_Point;
    m = VM_Module_Install(MODULE_IS_C_FUNC, "conv_2Real_to_Point", p);
    if ( m < 1 ) return Failed;
    opn = Cmp_Operation_Add(cmp_opr.converter, type_2RealNum, TYPE_NONE, TYPE_POINT);
    if ( opn == NULL ) return Failed;
    opn->is.commutative = 0;
    opn->is.intrinsic = 0;
    opn->is.assignment = 0;
    opn->module = m;
  }

  return Success;
}

#define DEFINE_TYPE(name, length, type)     \
  type_##name = Tym_Box_Abstract_New_Alias( \
   & ((Name) {length, #name}), type);       \
  if ( type_##name < 0 ) return Failed;

static Task Blt_Define_Math(void) {
  Intg type_Sin, type_Cos, type_Tan, type_Asin, type_Acos, type_Atan,
   type_Exp, type_Log, type_Log10, type_Sqrt, type_Ceil, type_Floor;

  DEFINE_TYPE(Sin,   3, TYPE_REAL)
  DEFINE_TYPE(Cos,   3, TYPE_REAL)
  DEFINE_TYPE(Tan,   3, TYPE_REAL)
  DEFINE_TYPE(Asin,  4, TYPE_REAL)
  DEFINE_TYPE(Acos,  4, TYPE_REAL)
  DEFINE_TYPE(Atan,  4, TYPE_REAL)
  DEFINE_TYPE(Exp,   3, TYPE_REAL)
  DEFINE_TYPE(Log,   3, TYPE_REAL)
  DEFINE_TYPE(Log10, 5, TYPE_REAL)
  DEFINE_TYPE(Sqrt,  4, TYPE_REAL)
  DEFINE_TYPE(Ceil,  4, TYPE_INTG)
  DEFINE_TYPE(Floor, 5, TYPE_INTG)

  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Cos,   Cos_RealNum)   );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Sin,   Sin_RealNum)   );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Tan,   Tan_RealNum)   );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Asin,  Asin_RealNum)  );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Acos,  Acos_RealNum)  );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Atan,  Atan_RealNum)  );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Exp,   Exp_RealNum)   );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Log,   Log_RealNum)   );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Log10, Log10_RealNum) );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Sqrt,  Sqrt_RealNum)  );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Ceil,  Ceil_RealNum)  );
  TASK( Cmp_Def_C_Procedure(type_RealNum, type_Floor, Floor_RealNum) );
  return Success;
}

static Task Blt_Define_Print(void) {
  Intg type_Print;
  type_Print = Tym_Box_Abstract_New_Alias(& ((Name) {5, "Print"}), TYPE_VOID);
  if (type_Print < 0) return Failed;
  TASK( Cmp_Def_C_Procedure(TYPE_CHAR,   type_Print, Print_Char  ) );
  TASK( Cmp_Def_C_Procedure(TYPE_INTG,   type_Print, Print_Int   ) );
  TASK( Cmp_Def_C_Procedure(TYPE_REAL,   type_Print, Print_Real  ) );
  TASK( Cmp_Def_C_Procedure(type_Point,  type_Print, Print_Pnt ) );
  TASK( Cmp_Def_C_Procedure(type_String, type_Print, Print_String) );
  TASK( Cmp_Def_C_Procedure(PROC_PAUSE,  type_Print, Print_NewLine) );
  /*Tym_Print_Procedure(stdout, type_new);*/
  return Success;
}

/*******************************BOX-PROCEDURES********************************/
static Task Print_Char(void) {printf(SChar, BOX_VM_ARG1(Char)); return Success;}
static Task Print_Int(void)  {printf(SIntg, BOX_VM_ARG1(Intg)); return Success;}
static Task Print_Real(void) {printf(SReal, BOX_VM_ARG1(Real)); return Success;}
static Task Print_Pnt(void) {
  Point *p = BOX_VM_ARGPTR1(Point);
  printf(SPoint, p->x, p->y);
  return Success;
}
static Task Print_String(void) {printf("%s", BOX_VM_ARGPTR1(char)); return Success;}
static Task Print_NewLine(void) {printf("\n"); return Success;}

/*****************************************************************************
 *                       FUNCTIONS FOR CONVERSION                            *
 *****************************************************************************/
static Task Conv_2RealNum_to_Point(void) {
  BOX_VM_CURRENT(Point) = *(BOX_VM_ARGPTR1(Point));
  return Success;
}

/*****************************************************************************
 *                        MATHEMATICAL FUNCTIONS                             *
 *****************************************************************************/
static Task Sin_RealNum(void) {
  BOX_VM_CURRENT(Real) = sin(BOX_VM_ARG1(Real));
  return Success;
}
static Task Cos_RealNum(void) {
  BOX_VM_CURRENT(Real) = cos(BOX_VM_ARG1(Real));
  return Success;
}
static Task Tan_RealNum(void) {
  BOX_VM_CURRENT(Real) = tan(BOX_VM_ARG1(Real));
  return Success;
}
static Task Asin_RealNum(void) {
  BOX_VM_CURRENT(Real) = asin(BOX_VM_ARG1(Real));
  return Success;
}
static Task Acos_RealNum(void) {
  BOX_VM_CURRENT(Real) = acos(BOX_VM_ARG1(Real));
  return Success;
}
static Task Atan_RealNum(void) {
  BOX_VM_CURRENT(Real) = atan(BOX_VM_ARG1(Real));
  return Success;
}
static Task Exp_RealNum(void) {
  BOX_VM_CURRENT(Real) = exp(BOX_VM_ARG1(Real));
  return Success;
}
static Task Log_RealNum(void) {
  BOX_VM_CURRENT(Real) = log(BOX_VM_ARG1(Real));
  return Success;
}
static Task Log10_RealNum(void) {
  BOX_VM_CURRENT(Real) = log10(BOX_VM_ARG1(Real));
  return Success;
}
static Task Sqrt_RealNum(void) {
  BOX_VM_CURRENT(Real) = sqrt(BOX_VM_ARG1(Real));
  return Success;
}
static Task Ceil_RealNum(void) {
  BOX_VM_CURRENT(Intg) = ceil(BOX_VM_ARG1(Real));
  return Success;
}
static Task Floor_RealNum(void) {
  BOX_VM_CURRENT(Intg) = floor(BOX_VM_ARG1(Real));
  return Success;
}
