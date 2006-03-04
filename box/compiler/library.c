#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#include "types.h"
#include "defaults.h"
#include "array.h"
#include "virtmach.h"
#include "registers.h"
#include "compiler.h"

/* Important builtin types */
Intg type_Point, type_RealNum, type_IntgNum, type_CharNum;
Intg type_String;

static Task Tmp(void);
static Task Lib_Define_Basics(void);
static Task Lib_Define_Math(void);
static Task Lib_Define_Print(void);

/* box-procedures */
static Task Print_Char(void);
static Task Print_Int(void);
static Task Print_Real(void);
static Task Print_Pnt(void);
static Task Print_String(void);
static Task Print_NewLine(void);
/* Functions for conversions */
static Task conv_2RealNum_to_Point(void);
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

Task Lib_Define_All(void) {
  TASK( Tmp() );
  TASK( Lib_Define_Basics() );
  TASK( Lib_Define_Math() );
  TASK( Lib_Define_Print() );
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

static Task Lib_Define_Basics(void) {
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

    p.c_func = conv_2RealNum_to_Point;
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

static Task Lib_Define_Math(void) {
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

static Task Lib_Define_Print(void) {
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
static Task conv_2RealNum_to_Point(void) {
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
