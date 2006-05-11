/* vmexec.c - Autore: Franchin Matteo - giugno 2004
 *
 * Questo file contiene la funzioni che eseguono le istruzioni della macchina
 * virtuale (spesso denotata con VM).
 * Questo file e' incluso direttamente dal file "virtmach.c"
 */
#include <math.h>

static void VM__Exec_Ret(void)
{
	vmcur->flags.exit = 1;
	return;
}

static void VM__Exec_Call_I(void) {
  if IS_SUCCESSFUL( VM_Module_Execute(*((Intg *) vmcur->arg1)) ) return;
  vmcur->flags.error = vmcur->flags.exit = 1;
}

static void VM__Exec_Line_I(void)
{
	vmcur->line = *((Intg *) vmcur->arg1);
	return;
}

#define VM__NEW(name, TYPE_ID, Type)                                   \
static void VM__Exec_ ## name ## _II(void) {                           \
  register Type *ptr;                                                  \
  register Intg numvar = *((Intg *) vmcur->arg1),                      \
    numreg = *((Intg *) vmcur->arg2), numtot = numvar + numreg + 1;    \
  if ( (vmcur->alc[TYPE_ID] & 1) != 0 ) goto err;                      \
  vmcur->alc[TYPE_ID] |= 1;                                            \
  vmcur->lmin[TYPE_ID] = -numvar; vmcur->lmax[TYPE_ID] = numreg;       \
  ptr = (Type *) calloc(numtot, sizeof(Type));                         \
  vmcur->local[TYPE_ID] = ptr + numvar;                                \
  if ( (ptr != NULL) && (numvar >= 0) && (numtot > numvar) ) return;  \
  MSG_FATAL(#name ": Impossibile allocare lo spazio per i registri."); \
  vmcur->flags.error = vmcur->flags.exit = 1; return;                  \
err:  MSG_FATAL(#name ": Registri gia' allocati!");                    \
  vmcur->flags.error = vmcur->flags.exit = 1; return;                  \
}

VM__NEW(NewC, TYPE_CHAR,  Char)
VM__NEW(NewI, TYPE_INTG,  Intg)
VM__NEW(NewR, TYPE_REAL,  Real)
VM__NEW(NewP, TYPE_POINT, Point)
VM__NEW(NewO, TYPE_OBJ,   Obj)

static void VM__Exec_Mov_CC(void)
	{*((Char *) vmcur->arg1) = *((Char *) vmcur->arg2);}
static void VM__Exec_Mov_II(void)
	{*((Intg *) vmcur->arg1) = *((Intg *) vmcur->arg2);}
static void VM__Exec_Mov_RR(void)
	{*((Real *) vmcur->arg1) = *((Real *) vmcur->arg2);}
static void VM__Exec_Mov_PP(void)
	{*((Point *) vmcur->arg1) = *((Point *) vmcur->arg2);}
static void VM__Exec_Mov_OO(void)
	{*((Obj *) vmcur->arg1) = *((Obj *) vmcur->arg2);}

static void VM__Exec_BNot_I(void)
	{*((Intg *) vmcur->arg1) = ~ *((Intg *) vmcur->arg1);}
static void VM__Exec_BAnd_II(void)
	{*((Intg *) vmcur->arg1) &= *((Intg *) vmcur->arg2);}
static void VM__Exec_BXor_II(void)
	{*((Intg *) vmcur->arg1) ^= *((Intg *) vmcur->arg2);}
static void VM__Exec_BOr_II(void)
	{*((Intg *) vmcur->arg1) |= *((Intg *) vmcur->arg2);}

static void VM__Exec_Shl_II(void)
 { *((Intg *) vmcur->arg1) <<= *((Intg *) vmcur->arg2); }
static void VM__Exec_Shr_II(void)
 { *((Intg *) vmcur->arg1) >>= *((Intg *) vmcur->arg2); }

static void VM__Exec_Inc_I(void) { ++ *((Intg *) vmcur->arg1); }
static void VM__Exec_Inc_R(void) { ++ *((Real *) vmcur->arg1); }
static void VM__Exec_Dec_I(void) { -- *((Intg *) vmcur->arg1); }
static void VM__Exec_Dec_R(void) { -- *((Real *) vmcur->arg1); }

static void VM__Exec_Pow_II(void) { /* bad implementation!!! */
  Intg i, r = 1, b = *((Intg *) vmcur->arg1), p = *((Intg *) vmcur->arg2);
  for (i = 0; i < p; i++) r *= b;
  *((Intg *) vmcur->arg1) = r;
}
static void VM__Exec_Pow_RR(void) {
  *((Real *) vmcur->arg1) =
   pow(*((Real *) vmcur->arg1), *((Real *) vmcur->arg2));
}

static void VM__Exec_Add_II(void)
  {*((Intg *) vmcur->arg1) += *((Intg *) vmcur->arg2); return;}
static void VM__Exec_Add_RR(void)
  {*((Real *) vmcur->arg1) += *((Real *) vmcur->arg2); return;}
static void VM__Exec_Add_PP(void) {
  ((Point *) vmcur->arg1)->x += ((Point *) vmcur->arg2)->x;
  ((Point *) vmcur->arg1)->y += ((Point *) vmcur->arg2)->y;
}

static void VM__Exec_Sub_II(void)
  {*((Intg *) vmcur->arg1) -= *((Intg *) vmcur->arg2); return;}
static void VM__Exec_Sub_RR(void)
  {*((Real *) vmcur->arg1) -= *((Real *) vmcur->arg2); return;}
static void VM__Exec_Sub_PP(void) {
  ((Point *) vmcur->arg1)->x -= ((Point *) vmcur->arg2)->x;
  ((Point *) vmcur->arg1)->y -= ((Point *) vmcur->arg2)->y;
}

static void VM__Exec_Mul_II(void)
  {*((Intg *) vmcur->arg1) *= *((Intg *) vmcur->arg2);}
static void VM__Exec_Mul_RR(void)
  {*((Real *) vmcur->arg1) *= *((Real *) vmcur->arg2);}

static void VM__Exec_Div_II(void)
  {*((Intg *) vmcur->arg1) /= *((Intg *) vmcur->arg2);}
static void VM__Exec_Div_RR(void)
  {*((Real *) vmcur->arg1) /= *((Real *) vmcur->arg2);}
static void VM__Exec_Rem_II(void)
  {*((Intg *) vmcur->arg1) %= *((Intg *) vmcur->arg2);}

static void VM__Exec_Neg_I(void)
  {*((Intg *) vmcur->arg1) = -*((Intg *) vmcur->arg1); return;}
static void VM__Exec_Neg_R(void)
  {*((Real *) vmcur->arg1) = -*((Real *) vmcur->arg1); return;}
static void VM__Exec_Neg_P(void) {
  ((Point *) vmcur->arg1)->x = -((Point *) vmcur->arg1)->x;
  ((Point *) vmcur->arg1)->y = -((Point *) vmcur->arg1)->y;
}

static void VM__Exec_PMulR_PR(void) {
  Real r = *((Real *) vmcur->local[TYPE_REAL]);
  ((Point *) vmcur->arg1)->x *= r;
  ((Point *) vmcur->arg1)->y *= r;
}
static void VM__Exec_PDivR_PR(void) {
  Real r = *((Real *) vmcur->local[TYPE_REAL]);
  ((Point *) vmcur->arg1)->x /= r;
  ((Point *) vmcur->arg1)->y /= r;
}

static void VM__Exec_Real_C(void) {
  *((Real *) vmcur->local[TYPE_REAL]) = (Real) *((Char *) vmcur->arg1);
}
static void VM__Exec_Real_I(void) {
  *((Real *) vmcur->local[TYPE_REAL]) = (Real) *((Intg *) vmcur->arg1);
}
static void VM__Exec_Intg_R(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) = (Intg) *((Real *) vmcur->arg1);
}
static void VM__Exec_Point_II(void) {
  ((Point *) vmcur->local[TYPE_POINT])->x = (Real) *((Intg *) vmcur->arg1);
  ((Point *) vmcur->local[TYPE_POINT])->y = (Real) *((Intg *) vmcur->arg2);
}
static void VM__Exec_Point_RR(void) {
  ((Point *) vmcur->local[TYPE_POINT])->x = *((Real *) vmcur->arg1);
  ((Point *) vmcur->local[TYPE_POINT])->y = *((Real *) vmcur->arg2);
}
static void VM__Exec_ProjX_P(void) {
  *((Real *) vmcur->local[TYPE_REAL]) = ((Point *) vmcur->arg1)->x;
}
static void VM__Exec_ProjY_P(void) {
  *((Real *) vmcur->local[TYPE_REAL]) = ((Point *) vmcur->arg1)->y;
}
static void VM__Exec_PPtrX_P(void) {
  *((Obj *) vmcur->local[TYPE_OBJ]) = & (((Point *) vmcur->arg1)->x);
}
static void VM__Exec_PPtrY_P(void) {
  *((Obj *) vmcur->local[TYPE_OBJ]) = & (((Point *) vmcur->arg1)->y);
}

static void VM__Exec_Eq_II(void) {
  *((Intg *) vmcur->arg1) =
   *((Intg *) vmcur->arg1) == *((Intg *) vmcur->arg2);
}
static void VM__Exec_Eq_RR(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) =
   *((Real *) vmcur->arg1) == *((Real *) vmcur->arg2);
}
static void VM__Exec_Eq_PP(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) =
     ( ((Point *) vmcur->arg1)->x == ((Point *) vmcur->arg2)->x )
  && ( ((Point *) vmcur->arg1)->y == ((Point *) vmcur->arg2)->y );
}
static void VM__Exec_Ne_II(void) {
  *((Intg *) vmcur->arg1) =
   *((Intg *) vmcur->arg1) != *((Intg *) vmcur->arg2);
}
static void VM__Exec_Ne_RR(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) =
   *((Real *) vmcur->arg1) != *((Real *) vmcur->arg2);
}
static void VM__Exec_Ne_PP(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) =
     ( ((Point *) vmcur->arg1)->x != ((Point *) vmcur->arg2)->x )
  || ( ((Point *) vmcur->arg1)->y != ((Point *) vmcur->arg2)->y );
}

static void VM__Exec_Lt_II(void) {
  *((Intg *) vmcur->arg1) =
   *((Intg *) vmcur->arg1) < *((Intg *) vmcur->arg2);
}
static void VM__Exec_Lt_RR(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) =
   *((Real *) vmcur->arg1) < *((Real *) vmcur->arg2);
}
static void VM__Exec_Le_II(void) {
  *((Intg *) vmcur->arg1) =
   *((Intg *) vmcur->arg1) <= *((Intg *) vmcur->arg2);
}
static void VM__Exec_Le_RR(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) =
   *((Real *) vmcur->arg1) <= *((Real *) vmcur->arg2);
}
static void VM__Exec_Gt_II(void) {
  *((Intg *) vmcur->arg1) =
   *((Intg *) vmcur->arg1) > *((Intg *) vmcur->arg2);
}
static void VM__Exec_Gt_RR(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) =
   *((Real *) vmcur->arg1) > *((Real *) vmcur->arg2);
}
static void VM__Exec_Ge_II(void) {
  *((Intg *) vmcur->arg1) =
   *((Intg *) vmcur->arg1) >= *((Intg *) vmcur->arg2);
}
static void VM__Exec_Ge_RR(void) {
  *((Intg *) vmcur->local[TYPE_INTG]) =
   *((Real *) vmcur->arg1) >= *((Real *) vmcur->arg2);
}

static void VM__Exec_LNot_I(void) {
  *((Intg *) vmcur->arg1) = ! *((Intg *) vmcur->arg1);
}
static void VM__Exec_LAnd_II(void) {
  *((Intg *) vmcur->arg1) =
   *((Intg *) vmcur->arg1) && *((Intg *) vmcur->arg2);
}
static void VM__Exec_LOr_II(void) {
  *((Intg *) vmcur->arg1) =
   *((Intg *) vmcur->arg1) || *((Intg *) vmcur->arg2);
}

static void VM__Exec_Malloc_I(void) {
  *((Obj *) vmcur->local[TYPE_OBJ]) = (Obj) malloc( *((Intg *) vmcur->arg1) );
  /* MANCA LA VERIFICA DI MEMORIA ESAURITA */
}
static void VM__Exec_MFree_O(void) { free(*((Obj *) vmcur->arg1)); }
static void VM__Exec_MCopy_OO(void) {
  (void) memcpy(
   *((Obj *) vmcur->arg1),             /* destination */
   *((Obj *) vmcur->arg2),             /* source */
   *((Intg *) vmcur->local[TYPE_INTG]) /* size */
  );
}

static void VM__Exec_Lea(void) {
  *((Obj *) vmcur->local[TYPE_OBJ]) = (Obj) vmcur->arg1;
}
static void VM__Exec_Lea_OO(void) {
  *((Obj *) vmcur->arg1) = (Obj) vmcur->arg2;
}
