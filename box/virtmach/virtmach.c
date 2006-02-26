/* virtmach.c - Autore: Franchin Matteo - giugno 2004
 *
 * Questo file contiene la macchina virtuale e le funzioni per scrivere
 * (assemblare) e visualizzare (in formato testo) pezzi di programma.
 */

/*#define NDEBUG*/
/*#define DEBUG_EXEC*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "virtmach.h"
#include "str.h"

/* Print the header of every instruction while disassembling */
/* #define DEBUG_VM_D_EVERY_ONE */

/*****************************************************************************
 *                                MACRO GENERICHE                            *
 *****************************************************************************/

/* Per settare le opzioni di assemblaggio, disassemblaggio, etc. */
#define VM_SET(s, o) if ( o >= 0 ) (s).o = (o == 0) ? 0 : 1;

/*****************************************************************************
 * The following macros contain the code used to write and read the binary   *
 * representations of the instructions, so they contain the code which       *
 * depends also on the particular machine we are on (little/big endian?):    *
 *                                                                           *
 * #define ASM_GET_FORMAT(i_pos, i_eye, is_long)                             *
 * #define ASM_SHORT_GET_HEADER(i_pos, i_eye, i_type, i_len, arg_type)       *
 * #define ASM_SHORT_GET_1ARG(i_pos, arg)                                    *
 * #define ASM_SHORT_GET_2ARGS(i_pos, arg1, arg2)                            *
 * #define ASM_SHORT_PUT_HEADER(i_pos, i_eye, i_type,is_long,i_len,arg_type) *
 * #define ASM_SHORT_PUT_1ARG(i_pos, arg)                                    *
 * #define ASM_SHORT_PUT_2ARGS(i_pos, arg1, arg2)                            *
 * #define ASM_LONG_GET_HEADER(i_pos, i_eye, i_type, i_len, arg_type)        *
 * #define ASM_LONG_GET_1ARG(i_pos, arg)                                     *
 * #define ASM_LONG_GET_2ARGS(i_pos, arg1, arg2)                             *
 * #define ASM_LONG_PUT_HEADER(i_pos, i_eye, i_type, is_long,i_len,arg_type) *
 * #define ASM_LONG_PUT_1ARG(i_pos, arg)                                     *
 * #define ASM_LONG_PUT_2ARGS(i_pos, arg1, arg2)                             *
 *                                                                           *
 *****************************************************************************/

typedef unsigned char Byte;
typedef unsigned long ByteX4;
typedef signed char SByte;

/* Numero massimo degli argomenti di un'istruzione */
#define MAX_NUMARGS 2

/* Read the first 4 bytes (ByteX4), extract the format bit and put the "rest"
 * in the i_eye (which should be defined as 'register ByteX4 i_eye;')
 * is_long tells if the instruction is encoded with the packed format (4 bytes)
 * or with the long format (> 4 bytes). To read / write an instruction
 * represented with the packed format one should use the macros
 * ASM_SHORT_GET_* / ASM_SHORT_PUT_*. To read / write an instruction written
 * with the long format one should use instead the macros ASM_LONG_GET_* /
 * ASM_LONG_PUT_*
 */
#define ASM_GET_FORMAT(i_pos, i_eye, is_long) \
{ i_eye = *(i_pos++); is_long = (i_eye & 0x1); i_eye >>= 1; }

/* SHORT INSTRUCTION: we assemble the istruction header in the following way:
 * (note: 1 is represented with bit 0 = 1 and all other bits = 0)
 *  bit 0: true if the instruction is long
 *  bit 1-4: type of arguments
 *  bit 5-7: length of instruction
 *  bit 8-15: type of instruction
 *  (bit 16-23: left empty for argument 1)
 *  (bit 24-31: left empty for argument 2)
 */
#define ASM_SHORT_PUT_HEADER(i_pos, i_eye, i_type, is_long, i_len, arg_type) \
{ i_eye = (((i_type) & 0xff)<<3 | ((i_len) & 0x7))<<4 | ((arg_type) & 0xf); \
  i_eye = i_eye<<1 | ((is_long) & 0x1); }

#define ASM_SHORT_PUT_1ARG(i_pos, i_eye, arg) \
{ *(i_pos++) = ((arg) & 0xff) << 16 | i_eye; }

#define ASM_SHORT_PUT_2ARGS(i_pos, i_eye, arg1, arg2) \
{ *(i_pos++) = i_eye = (((arg2) & 0xff)<<8 | ((arg1) & 0xff))<<16 | i_eye; }

#define ASM_SHORT_GET_HEADER(i_pos, i_eye, i_type, i_len, arg_type) \
{ arg_type = i_eye & 0xf; i_len = (i_eye >>= 4) & 0x7; \
  i_type = (i_eye >>= 3) & 0xff; }

#define ASM_SHORT_GET_1ARG(i_pos, i_eye, arg) \
{ arg = (signed char) ((i_eye >>= 8) & 0xff); }

#define ASM_SHORT_GET_2ARGS(i_pos, i_eye, arg1, arg2) \
{ arg1 = (signed char) ((i_eye >>= 8) & 0xff); \
  arg2 = (signed char) ((i_eye >>= 8) & 0xff); }

/* LONG INSTRUCTION: we assemble the istruction header in the following way:
 *  FIRST FOUR BYTES:
 *    bit 0: true if the instruction is long
 *    bit 1-4: type of arguments
 *    bit 5-31: length of instruction
 *  SECOND FOUR BYTES:
 *    bit 0-31: type of instruction
 *  (THIRD FOUR BYTES: argument 1)
 *  (FOURTH FOUR BYTES: argument 2)
 */
#define ASM_LONG_PUT_HEADER(i_pos, i_eye, i_type, is_long, i_len, arg_type) \
{ i_eye = ((i_len) & 0x07ff)<<4 | ((arg_type) & 0xf); \
  i_eye = i_eye<<1 | ((is_long) & 0x1); \
  *(i_pos++) = i_eye; *(i_pos++) = i_type; }

#define ASM_LONG_GET_HEADER(i_pos, i_eye, i_type, i_len, arg_type) \
{ arg_type = i_eye & 0xf; i_len = (i_eye >>= 4); i_type = *(i_pos++); }

#define ASM_LONG_GET_1ARG(i_pos, i_eye, arg) \
{ arg = i_eye = *(i_pos++); }

#define ASM_LONG_GET_2ARGS(i_pos, i_eye, arg1, arg2) \
{ arg1 = *(i_pos++); arg2 = i_eye = *(i_pos++); }

/*****************************************************************************
 *                           DEFINIZIONE DEI TIPI                            *
 *****************************************************************************/

UInt size_of_type[NUM_TYPES] = {
  sizeof(Char),
  sizeof(Intg),
  sizeof(Real),
  sizeof(Point),
  sizeof(Obj)
};

/* Elenco dei moduli installati */
Array *vm_modules_list = NULL;

typedef struct {
  ModuleType type;  /* Tipo di modulo */
  const char *name; /* Nome del modulo */
  ModulePtr ptr;    /* Puntatore al modulo */
  Intg length;      /* Dimensione del modulo (in numero di ByteX4) */
} Module;

typedef struct {
  const char *name;             /* Nome dell'istruzione */
  UInt numargs;                 /* Numero di argomenti dell'struzione */
  TypeID t_id;                  /* Numero che identifica il tipo degli argomenti (interi, reali, ...) */
  void (*get_args)(void);       /* Codice per trattare gli argomenti dell'istruz. */
  void (*execute)(void);        /* Codice che esegue l'istruzione */
  void (*disasm)(char **iarg);  /* Codice per disassemblare gli argomenti dell'istruzione */
} InstrDesc;

typedef struct {
  /* Flags della VM */
  struct {
    unsigned int error    : 1; /* L'istruzione ha provocato un errore! */
    unsigned int exit     : 1; /* Bisogna uscire dall'esecuzione! */
    unsigned int is_long  : 1; /* L'istruzione e' in formato lungo? */
  } flags;

  Intg line;          /* Numero di linea (fissato dall'istruzione line) */
  Module *m;          /* Modulo correntemente in esecuzione */

  /* Variabili che riguardano l'istruzione in esecuzione: */
  ByteX4 *i_pos;      /* Puntatore all'istruzione */
  ByteX4 i_eye;       /* Occhio di lettura (gli ultimi 4 byte letti) */
  UInt i_type, i_len; /* Tipo e dimensione dell'istruzione */
  UInt arg_type;      /* Tipo degli argomenti dell'istruzione */
  InstrDesc *idesc;   /* Descrittore dell'istruzione corrente */
  void *arg1, *arg2;  /* Puntatori agli argomenti del'istruzione */

  /* Array di puntatori alle zone registri globali e locali */
  void *global[NUM_TYPES];
  void *local[NUM_TYPES];
  /* Numero di registro globale minimo e massimo */
  Intg gmin[NUM_TYPES], gmax[NUM_TYPES];
  /* Numero di registro locale minimo e massimo */
  Intg lmin[NUM_TYPES], lmax[NUM_TYPES];
  /* Stato di allocazione dei registri per il modulo in esecuzione */
  Intg alc[NUM_TYPES];
} VMStatus;

int vm_globals = 0;
void *vm_global[NUM_TYPES] = {};
Intg vm_gmin[NUM_TYPES] = {}, vm_gmax[NUM_TYPES] = {};
static Generic reg0[NUM_TYPES]; /* Registri locali numero zero */
static VMStatus *vmcur = NULL;  /* Stato corrente della macchina virtuale */
Obj *box_vm_current, *box_vm_arg1, *box_vm_arg2;

/* Il seguente file contiene la definizione delle funzioni del tipo
 * "InstrDesc.execute", quelle che vengono chiamate per eseguire le istruzioni
 * della VM.
 */
#include "vmexec.c"


/*******************************************************************************
 *  Le seguenti funzioni servono a ricavare gli indirizzi in memoria dei       *
 *  registri globali (indicati con 'G'), dei registri locali ('L'),            *
 *  dei puntatori ('P') o degli immediati interi ('I') che compaiono come      *
 *  argomenti delle istruzioni. Ad esempio: l'istruzione "mov ri2, i[ro0+4]"   *
 *  dice alla macchina virtuale di mettere nel registro intero locale numero 2 *
 *  (ri2) il valore intero presente alla locazione (ro0 + 4). A tale scopo     *
 *  la VM calcola gli indirizzi in memoria dei 2 argomenti (usando VM__Get_L   *
 *  per il primo argomento, ri2, e usando VM_Get_P per il secondo, i[ro0+4]).  *
 *  Una volta calcolati gli indirizzi la VM chiamera' VM__Exec_Mov_II per      *
 *  eseguire materialmente l'operazione.                                       *
 *******************************************************************************/

/* DESCRIZIONE: Restituisce il puntatore al registro globale n-esimo.
 */
static void *VM__Get_G(Intg n)
{
  register TypeID t = vmcur->idesc->t_id;

  assert( (t >= 0) && (t < NUM_TYPES) );

#ifdef VM_SAFE_EXEC1
  if ( (n < vmcur->gmin[t]) || (n > vmcur->gmax[t]) ) {
    MSG_ERROR("Riferimento a registro globale non allocato!");
    vmcur->flags.error = vmcur->flags.exit = 1;
    return NULL;
  }
#endif
  return (
   (void *) (vmcur->global[t]) + (Intg) (n * size_of_type[t])
  );
}

/* DESCRIZIONE: Restituisce il puntatore al registro locale n-esimo.
 */
static void *VM__Get_L(Intg n)
{
  register TypeID t = vmcur->idesc->t_id;

  assert( (t >= 0) && (t < NUM_TYPES) );

#ifdef VM_SAFE_EXEC1
  if ( (n < vmcur->lmin[t]) || (n > vmcur->lmax[t]) ) {
    MSG_ERROR("Riferimento a registro locale non allocato!");
    vmcur->flags.error = vmcur->flags.exit = 1;
    return NULL;
  }
#endif
  return (
   (void *) (vmcur->local[t]) + (Intg) (n * size_of_type[t])
  );
}

/* DESCRIZIONE: Restituisce il puntatore alla locazione di memoria (ro0 + n).
 */
static void *VM__Get_P(Intg n)
{
  return *((void **) vmcur->local[TYPE_OBJ]) + n;
}

/* DESCRIZIONE: Restituisce il puntatore ad un intero/reale di valore n.
 */
static void *VM__Get_I(Intg n)
{
  register TypeID t = vmcur->idesc->t_id;
  static Intg i = 0;
  static union { Char c; Intg i; Real r; } v[2], *value;

  assert( (t >= TYPE_CHAR) && (t <= TYPE_REAL) );

  value = & v[i]; i ^= 1;
  switch (t) {
   case TYPE_CHAR:
      value->c = (Char) n; return (void *) (& value->c);
   case TYPE_INTG:
      value->i = (Intg) n; return (void *) (& value->i);
   case TYPE_REAL:
      value->r = (Real) n; return (void *) (& value->r);
   default: /* This shouldn't happen! */
      return (void *) (& value->i); break;
  }
}

/* Array di puntatori alle 4 funzioni sopra: */
static void *(*vm_gets[4])() = { VM__Get_G, VM__Get_L, VM__Get_P, VM__Get_I };

/* DESCRIZIONE: Questa funzione trova e imposta gli indirizzi corrispondenti
 *  ai 2 argomenti dell'istruzione. In modo da poter procedere all'esecuzione.
 * NOTA: Questa funzione gestisce solo le istruzioni di tipo GLP-GLPI,
 *  cioe' le istruzioni in cui: il primo argomento e' 'G'lobale, 'L'ocale,
 *  'P'untatore, mentre il secondo puo' essere dei tipi appena enumerati oppure
 *  puo' essere un 'I'mmediato intero.
 */
static void VM__GLP_GLPI(void)
{
  signed long narg1, narg2;
  register UInt atype = vmcur->arg_type;

  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_2ARGS( vmcur->i_pos, vmcur->i_eye, narg1, narg2);
  } else {
    ASM_SHORT_GET_2ARGS( vmcur->i_pos, vmcur->i_eye, narg1, narg2);
  }

  vmcur->arg1 = vm_gets[atype & 0x3](narg1);
  vmcur->arg2 = vm_gets[(atype >> 2) & 0x3](narg2);
#ifdef DEBUG_EXEC
  printf("Cathegories: arg1 = %lu - arg2 = %lu\n",
   atype & 0x3, (atype >> 2) & 0x3);
#endif
}

/* DESCRIZIONE: Questa funzione e' analoga alla precedente, ma gestisce
 *  istruzioni come: "mov ri1, 123456", "mov rf2, 3.14", "mov rp5, (1, 2)", etc.
 */
static void VM__GLP_Imm(void)
{
  signed long narg1;
  register UInt atype = vmcur->arg_type;

  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  } else {
    ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  }

  vmcur->arg1 = vm_gets[atype & 0x3](narg1);
  vmcur->arg2 = vmcur->i_pos;
}

/* DESCRIZIONE: Questa funzione e' analoga alle precedenti, ma gestisce
 *  istruzioni con un solo argomento di tipo GLPI (Globale oppure Locale
 *  o Puntatore o Immediato intero).
 */
static void VM__GLPI(void)
{
  signed long narg1;
  register UInt atype = vmcur->arg_type;

  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  } else {
    ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  }

  vmcur->arg1 = vm_gets[atype & 0x3](narg1);
}

/* DESCRIZIONE: Questa funzione e' analoga alle precedenti, ma gestisce
 *  istruzioni con un solo argomento di tipo immediato (memorizzato subito
 *  di seguito all'istruzione).
 */
static void VM__Imm(void)
{
  vmcur->arg1 = (void *) vmcur->i_pos;
}

/*******************************************************************************
 *  Le seguenti funzioni servono a disassemblare gli argomenti delle           *
 *  istruzioni e vengono usate pertanto solo dalla funzione VM_Disassemble.    *
 *******************************************************************************
 */

/* Array in cui verranno disassemblati gli argomenti (scritti con sprintf) */
static char iarg_str[MAX_NUMARGS][64];

/* DESCRIZIONE: Questa funzione serve a disassemblare gli argomenti di
 *  un'istruzione di tipo GLPI-GLPI.
 *  iarg e' una tabella di puntatori alle stringhe che corrisponderanno
 *  agli argomenti disassemblati.
 */
static void VM__D_GLPI_GLPI(char **iarg)
{
  UInt n, na = vmcur->idesc->numargs;
  UInt iaform[2] = {vmcur->arg_type & 3, (vmcur->arg_type >> 2) & 3};
  Intg iaint[2];

  assert(na <= 2);

  /* Recupero i numeri (interi) di registro/puntatore/etc. */
  switch (na) {
   case 1: {
    void *arg2;

    if ( vmcur->flags.is_long ) {
      ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, iaint[0]);
      arg2 = vmcur->i_pos;
    } else {
      ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, iaint[0]);
      arg2 = vmcur->i_pos;
    }
    break; }
   case 2:
    if ( vmcur->flags.is_long ) {
      ASM_LONG_GET_2ARGS( vmcur->i_pos, vmcur->i_eye, iaint[0], iaint[1]);
    } else {
      ASM_SHORT_GET_2ARGS( vmcur->i_pos, vmcur->i_eye, iaint[0], iaint[1]);
    }
    break;
  }

  for (n = 0; n < na; n++) {
    UInt iaf = iaform[n];
    UInt iat = vmcur->idesc->t_id;

    assert(iaf < 4);

    {
      Intg iai = iaint[n], uiai = iai;
      char rc, tc;
      const char typechars[NUM_TYPES] = "cirpo";

      tc = typechars[iat];
      if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
      switch(iaf) {
       case CAT_GREG:
        sprintf(iarg_str[n], "g%c%c" SIntg, rc, tc, uiai);
        break;
       case CAT_LREG:
        sprintf(iarg_str[n], "%c%c" SIntg, rc, tc, uiai);
        break;
       case CAT_PTR:
        if ( iai < 0 )
          sprintf(iarg_str[n], "%c[ro0 - " SIntg "]", tc, uiai);
        else if ( iai == 0 )
          sprintf(iarg_str[n], "%c[ro0]", tc);
        else
          sprintf(iarg_str[n], "%c[ro0 + " SIntg "]", tc, uiai);
        break;
       case CAT_IMM:
        if ( iat == TYPE_CHAR ) iai = (Intg) ((Char) iai);
        sprintf(iarg_str[n], SIntg, iai);
        break;
      }
    }

    *(iarg++) = iarg_str[n];
  }
  return;
}

/* DESCRIZIONE: Analoga alla precedente, ma per istruzioni CALL.
 */
static void VM__D_CALL(char **iarg) {
  register UInt na = vmcur->idesc->numargs;

  assert(na <= 2);

  if ( (na == 1) && ((vmcur->arg_type & 3) == CAT_IMM) ) {
    UInt iat = vmcur->idesc->t_id;
    Intg m_num;
    void *arg2;

    if ( vmcur->flags.is_long ) {
      ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, m_num);
      arg2 = vmcur->i_pos;
    } else {
      ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, m_num);
      arg2 = vmcur->i_pos;
    }

    if ( iat == TYPE_CHAR ) m_num = (Intg) ((Char) m_num);
    {

      if ( (m_num < 1) || (m_num > Arr_NumItem(vm_modules_list)) ) {
        sprintf(iarg_str[0], SIntg, m_num);
        return;

      } else {
        char *m_name;
        Module *m = Arr_ItemPtr(vm_modules_list, Module, m_num);
        m_name = Str_Cut(m->name, 40, 85);
        sprintf(iarg_str[0], SIntg"('%.40s')", m_num, m_name);
        free(m_name);
      }

    }
    return;

  } else {
    VM__D_GLPI_GLPI(iarg);
  }
}

/* DESCRIZIONE: Analoga alla precedente, ma per istruzioni del tipo GLPI-Imm.
 */
static void VM__D_GLPI_Imm(char **iarg)
{
  UInt iaf = vmcur->arg_type & 3, iat = vmcur->idesc->t_id;
  Intg iai;
  ByteX4 *arg2;

  assert(vmcur->idesc->numargs == 2);
  assert(iat < 4);

  /* Recupero il numero (intero) di registro/puntatore/etc. */
  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, iai);
    arg2 = vmcur->i_pos;
  } else {
    ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, iai);
    arg2 = vmcur->i_pos;
  }

  /* Primo argomento */
  {
    Intg uiai = iai;
    char rc, tc;
    const char typechars[NUM_TYPES] = "cirpo";

    tc = typechars[iat];
    if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
    switch(iaf) {
     case CAT_GREG:
      sprintf(iarg_str[1], "g%c%c" SIntg, rc, tc, uiai);
      break;
     case CAT_LREG:
      sprintf(iarg_str[1], "%c%c" SIntg, rc, tc, uiai);
      break;
     case CAT_PTR:
      if ( iai < 0 )
        sprintf(iarg_str[1], "%c[ro0 - " SIntg "]", tc, uiai);
      else if ( iai == 0 )
        sprintf(iarg_str[1], "%c[ro0]", tc);
      else
        sprintf(iarg_str[1], "%c[ro0 + " SIntg "]", tc, uiai);
      break;
     case CAT_IMM:
      sprintf(iarg_str[1], SIntg, iai);
      break;
    }
  }

  /* Secondo argomento */
  switch (iat) {
   case TYPE_CHAR:
    sprintf( iarg_str[2], SChar, *((Char *) arg2) );
    break;
   case TYPE_INTG:
    sprintf( iarg_str[2], SIntg, *((Intg *) arg2) );
    break;
   case TYPE_REAL:
    sprintf( iarg_str[2], SReal, *((Real *) arg2) );
    break;
   case TYPE_POINT:
    sprintf( iarg_str[2], SPoint,
     ((Point *) arg2)->x, ((Point *) arg2)->y );
    break;
  }

  *(iarg++) = iarg_str[1];
  *iarg = iarg_str[2];
  return;
}

/*******************************************************************************
 * La seguente tabella descrive le istruzioni e specifica quali funzioni       *
 * debbano essere chiamate per localizzare la posizione in memoria degli       *
 * argomenti e quali per eseguire l'azione associata all'istruzione stessa.    *
 *******************************************************************************
 */
InstrDesc vm_instr_desc_table[] = {
  { },
  { "line", 1, TYPE_INTG, VM__Imm,      VM__Exec_Line_I, VM__D_GLPI_GLPI }, /* line imm_i         */
  { "call", 1, TYPE_INTG, VM__GLPI,     VM__Exec_Call_I, VM__D_CALL      }, /* call reg_i         */
  { "call", 1, TYPE_INTG, VM__Imm,      VM__Exec_Call_I, VM__D_CALL      }, /* call imm_i         */
  { "newc", 2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_NewC_II,VM__D_GLPI_GLPI }, /* newc imm_i, imm_i  */
  { "newi", 2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_NewI_II,VM__D_GLPI_GLPI }, /* newi imm_i, imm_i  */
  { "newr", 2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_NewR_II,VM__D_GLPI_GLPI }, /* newr imm_i, imm_i  */
  { "newp", 2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_NewP_II,VM__D_GLPI_GLPI }, /* newp imm_i, imm_i  */
  { "newo", 2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_NewO_II,VM__D_GLPI_GLPI }, /* newo imm_i, imm_i  */
  { "mov",  2, TYPE_CHAR, VM__GLP_Imm,  VM__Exec_Mov_CC, VM__D_GLPI_Imm  }, /* mov reg_c, imm_c   */
  { "mov",  2, TYPE_INTG, VM__GLP_Imm,  VM__Exec_Mov_II, VM__D_GLPI_Imm  }, /* mov reg_i, imm_i   */
  { "mov",  2, TYPE_REAL, VM__GLP_Imm,  VM__Exec_Mov_RR, VM__D_GLPI_Imm  }, /* mov reg_r, imm_r   */
  { "mov",  2, TYPE_POINT,VM__GLP_Imm,  VM__Exec_Mov_PP, VM__D_GLPI_Imm  }, /* mov reg_p, imm_p   */
  { "mov",  2, TYPE_CHAR, VM__GLP_GLPI, VM__Exec_Mov_CC, VM__D_GLPI_GLPI }, /* mov reg_c, reg_c   */
  { "mov",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_Mov_II, VM__D_GLPI_GLPI }, /* mov reg_i, reg_i   */
  { "mov",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Mov_RR, VM__D_GLPI_GLPI }, /* mov reg_r, reg_r   */
  { "mov",  2, TYPE_POINT,VM__GLP_GLPI, VM__Exec_Mov_PP, VM__D_GLPI_GLPI }, /* mov reg_p, reg_p   */
  { "mov",  2, TYPE_OBJ,  VM__GLP_GLPI, VM__Exec_Mov_OO, VM__D_GLPI_GLPI }, /* mov reg_o, reg_o   */
  { "bnot", 1, TYPE_INTG, VM__GLPI,     VM__Exec_BNot_I, VM__D_GLPI_GLPI }, /* bnot reg_i         */
  { "band", 2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_BAnd_II,VM__D_GLPI_GLPI }, /* band reg_i, reg_i  */
  { "bxor", 2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_BXor_II,VM__D_GLPI_GLPI }, /* bxor reg_i, reg_i  */
  { "bor",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_BOr_II, VM__D_GLPI_GLPI }, /* bor reg_i, reg_i   */
  { "shl",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_Shl_II, VM__D_GLPI_GLPI }, /* shl reg_i, reg_i   */
  { "shr",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_Shr_II, VM__D_GLPI_GLPI }, /* shr reg_i, reg_i   */
  { "inc",  1, TYPE_INTG, VM__GLPI,     VM__Exec_Inc_I,  VM__D_GLPI_GLPI }, /* inc reg_i          */
  { "inc",  1, TYPE_REAL, VM__GLPI,     VM__Exec_Inc_R,  VM__D_GLPI_GLPI }, /* inc reg_r          */
  { "dec",  1, TYPE_INTG, VM__GLPI,     VM__Exec_Dec_I,  VM__D_GLPI_GLPI }, /* dec reg_i          */
  { "dec",  1, TYPE_REAL, VM__GLPI,     VM__Exec_Dec_R,  VM__D_GLPI_GLPI }, /* dec reg_r          */
  { "add",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_Add_II, VM__D_GLPI_GLPI }, /* add reg_i, reg_i   */
  { "add",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Add_RR, VM__D_GLPI_GLPI }, /* add reg_r, reg_r   */
  { "add",  2, TYPE_POINT,VM__GLP_GLPI, VM__Exec_Add_PP, VM__D_GLPI_GLPI }, /* add reg_p, reg_p   */
  { "sub",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_Sub_II, VM__D_GLPI_GLPI }, /* sub reg_i, reg_i   */
  { "sub",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Sub_RR, VM__D_GLPI_GLPI }, /* sub reg_r, reg_r   */
  { "sub",  2, TYPE_POINT,VM__GLP_GLPI, VM__Exec_Sub_PP, VM__D_GLPI_GLPI }, /* sub reg_p, reg_p   */
  { "mul",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_Mul_II, VM__D_GLPI_GLPI }, /* mul reg_i, reg_i   */
  { "mul",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Mul_RR, VM__D_GLPI_GLPI }, /* mul reg_r, reg_r   */
  { "div",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_Div_II, VM__D_GLPI_GLPI }, /* div reg_i, reg_i   */
  { "div",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Div_RR, VM__D_GLPI_GLPI }, /* div reg_r, reg_r   */
  { "rem",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_Rem_II, VM__D_GLPI_GLPI }, /* rem reg_i, reg_i   */
  { "neg",  1, TYPE_INTG, VM__GLPI,     VM__Exec_Neg_I,  VM__D_GLPI_GLPI }, /* neg reg_i          */
  { "neg",  1, TYPE_REAL, VM__GLPI,     VM__Exec_Neg_R,  VM__D_GLPI_GLPI }, /* neg reg_r          */
  { "neg",  1, TYPE_POINT,VM__GLPI,     VM__Exec_Neg_P,  VM__D_GLPI_GLPI }, /* neg reg_p          */
  { "pmulr",1, TYPE_POINT,VM__GLPI,   VM__Exec_PMulR_PR, VM__D_GLPI_GLPI }, /* pmulr reg_p        */
  { "pdivr",1, TYPE_POINT,VM__GLPI,   VM__Exec_PDivR_PR, VM__D_GLPI_GLPI }, /* pdivr reg_p        */
  { "eq?",  2, TYPE_INTG, VM__GLP_GLPI,  VM__Exec_Eq_II, VM__D_GLPI_GLPI }, /* eq? reg_i, reg_i   */
  { "eq?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Eq_RR, VM__D_GLPI_GLPI }, /* eq? reg_r, reg_r   */
  { "eq?",  2, TYPE_POINT,VM__GLP_GLPI,  VM__Exec_Eq_PP, VM__D_GLPI_GLPI }, /* eq? reg_p, reg_p   */
  { "ne?",  2, TYPE_INTG, VM__GLP_GLPI,  VM__Exec_Ne_II, VM__D_GLPI_GLPI }, /* ne? reg_i, reg_i   */
  { "ne?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Ne_RR, VM__D_GLPI_GLPI }, /* ne? reg_r, reg_r   */
  { "ne?",  2, TYPE_POINT,VM__GLP_GLPI,  VM__Exec_Ne_PP, VM__D_GLPI_GLPI }, /* ne? reg_p, reg_p   */
  { "lt?",  2, TYPE_INTG, VM__GLP_GLPI,  VM__Exec_Lt_II, VM__D_GLPI_GLPI }, /* lt? reg_i, reg_i   */
  { "lt?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Lt_RR, VM__D_GLPI_GLPI }, /* lt? reg_r, reg_r   */
  { "le?",  2, TYPE_INTG, VM__GLP_GLPI,  VM__Exec_Le_II, VM__D_GLPI_GLPI }, /* le? reg_i, reg_i   */
  { "le?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Le_RR, VM__D_GLPI_GLPI }, /* le? reg_r, reg_r   */
  { "gt?",  2, TYPE_INTG, VM__GLP_GLPI,  VM__Exec_Gt_II, VM__D_GLPI_GLPI }, /* gt? reg_i, reg_i   */
  { "gt?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Gt_RR, VM__D_GLPI_GLPI }, /* gt? reg_r, reg_r   */
  { "ge?",  2, TYPE_INTG, VM__GLP_GLPI,  VM__Exec_Ge_II, VM__D_GLPI_GLPI }, /* ge? reg_i, reg_i   */
  { "ge?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Ge_RR, VM__D_GLPI_GLPI }, /* ge? reg_r, reg_r   */
  { "lnot", 1, TYPE_INTG, VM__GLP_GLPI, VM__Exec_LNot_I, VM__D_GLPI_GLPI }, /* lnot reg_i         */
  { "land", 2, TYPE_INTG, VM__GLP_GLPI,VM__Exec_LAnd_II, VM__D_GLPI_GLPI }, /* land reg_i, reg_i  */
  { "lor",  2, TYPE_INTG, VM__GLP_GLPI, VM__Exec_LOr_II, VM__D_GLPI_GLPI }, /* lor  reg_i, reg_i  */
  { "real", 1, TYPE_CHAR, VM__GLPI,     VM__Exec_Real_C, VM__D_GLPI_GLPI }, /* real reg_c         */
  { "real", 1, TYPE_INTG, VM__GLPI,     VM__Exec_Real_I, VM__D_GLPI_GLPI }, /* real reg_i         */
  { "intg", 1, TYPE_REAL, VM__GLPI,     VM__Exec_Intg_R, VM__D_GLPI_GLPI }, /* intg reg_r         */
  { "point",2, TYPE_INTG, VM__GLP_GLPI,VM__Exec_Point_II,VM__D_GLPI_GLPI }, /* point reg_i, reg_i */
  { "point",2, TYPE_REAL, VM__GLP_GLPI,VM__Exec_Point_RR,VM__D_GLPI_GLPI }, /* point reg_r, reg_r */
  { "projx",1, TYPE_POINT,VM__GLPI,    VM__Exec_ProjX_P, VM__D_GLPI_GLPI }, /* projx reg_p        */
  { "projy",1, TYPE_POINT,VM__GLPI,    VM__Exec_ProjY_P, VM__D_GLPI_GLPI }, /* projy reg_p        */
  { "pptrx",1, TYPE_POINT,VM__GLPI,    VM__Exec_PPtrX_P, VM__D_GLPI_GLPI }, /* pptrx reg_p        */
  { "pptry",1, TYPE_POINT,VM__GLPI,    VM__Exec_PPtrY_P, VM__D_GLPI_GLPI }, /* pptry reg_p        */
  { "ret",  0, TYPE_NONE, NULL,         VM__Exec_Ret,    VM__D_GLPI_GLPI }, /* ret                */
  {"malloc",1, TYPE_INTG, VM__GLPI,   VM__Exec_Malloc_I, VM__D_GLPI_GLPI }, /* malloc reg_i       */
  { "mfree",1, TYPE_OBJ, VM__GLPI,    VM__Exec_MFree_O,  VM__D_GLPI_GLPI }, /* mfree reg_o        */
  { "mcopy",2, TYPE_OBJ, VM__GLP_GLPI,VM__Exec_MCopy_OO, VM__D_GLPI_GLPI }, /* mcopy reg_o, reg_o */
  {  "lea", 1,TYPE_CHAR, VM__GLP_GLPI,  VM__Exec_Lea,    VM__D_GLPI_GLPI }, /* lea c[ro0+...]     */
  {  "lea", 1,TYPE_INTG, VM__GLP_GLPI,  VM__Exec_Lea,    VM__D_GLPI_GLPI }, /* lea i[ro0+...]     */
  {  "lea", 1,TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Lea,    VM__D_GLPI_GLPI }, /* lea r[ro0+...]     */
  {  "lea", 1,TYPE_POINT,VM__GLP_GLPI,  VM__Exec_Lea,    VM__D_GLPI_GLPI }, /* lea p[ro0+...]     */
  {  "lea", 2, TYPE_OBJ, VM__GLP_GLPI,  VM__Exec_Lea_OO, VM__D_GLPI_GLPI }  /* lea reg_o, o[ro0+...] */
};


/*******************************************************************************
 *           FUNZIONI PRINCIPALI per installare ed eseguire i moduli           *
 *******************************************************************************
 */

/* DESCRIZIONE: Installa un nuovo modulo di programma con nome name.
 *  Un modulo e' semplicemente un pezzo di codice che puo' essere eseguito.
 *  E' possibile installare 2 tipi di moduli:
 *   1) (caso t = MODULE_IS_VM_CODE) il modulo e' costituito da
       codice eseguibile dalla VM
 *     (p.vm_code e' il puntatore alla prima istruzione nel codice);
 *   2) (caso t = MODULE_IS_C_FUNC) il modulo e' semplicemente una funzione
 *     scritta in C (p.c_func e' il puntatore alla funzione)
 *  Restituisce il numero assegnato al modulo (> 0), oppure 0 se qualcosa
 *  e' andato storto. Tale numero, se usato da un istruzione call, provoca
 *  l'esecuzione del modulo come procedura.
 */
Intg VM_Module_Install(ModuleType t, const char *name, ModulePtr p)
{
  /* Creo la lista dei moduli se non esiste */
  if ( vm_modules_list == NULL ) {
    vm_modules_list = Array_New( sizeof(Module), VM_TYPICAL_NUM_MODULES );
    if ( vm_modules_list == NULL ) return 0;
  }

  {
    Module new;

    new.type = t;
    new.name = strdup(name);
    new.ptr = p;
    if IS_FAILED( Arr_Push( vm_modules_list, & new ) ) return 0;
  }

  return Arr_NumItem( vm_modules_list );
}

/* DESCRIPTION: This function defines an undefined module (previously created
 *  with VM_Module_Undefined).
 */
Task VM_Module_Define(Intg module_num, ModuleType t, ModulePtr p) {
  Module *m;
  if ( vm_modules_list == NULL ) {
    MSG_ERROR("La lista dei moduli e' vuota!");
    return Failed;
  }
  if ( (module_num < 1) || (module_num > Arr_NumItem(vm_modules_list)) ) {
    MSG_ERROR("Impossibile definire un modulo non esistente.");
    return Failed;
  }
  m = ((Module *) vm_modules_list->ptr) + (module_num - 1);
  if ( m->type != MODULE_UNDEFINED ) {
    MSG_ERROR("Questo modulo non puo' essere definito!");
    return Failed;
  }
  m->type = t;
  m->ptr = p;
  return Success;
}

/* DESCRIPTION: This function creates a new undefined module with name name.
 */
Intg VM_Module_Undefined(const char *name) {
  return VM_Module_Install(MODULE_UNDEFINED, name, (ModulePtr) {{0, NULL}});
}

/* DESCRIPTION: This function returns the module-number which will be
 *  associated with the next module that will be installed.
 */
Intg VM_Module_Next(void) {
  if ( vm_modules_list == NULL ) return 1;
  return Arr_NumItem(vm_modules_list) + 1;
}

/* DESCRIPTION: This function checks the status of definitions of all the
 *  created modules. If one of the modules is undefined (and report_errs == 1)
 *  it prints an error message for every undefined module.
 */
Task VM_Module_Check(int report_errs) {
  Module *m;
  Intg mn;
  int status = Success;

  if ( vm_modules_list == NULL ) return Success;
  m = Arr_FirstItemPtr(vm_modules_list, Module);
  for (mn = Arr_NumItem(vm_modules_list); mn > 0; mn--) {
    if ( m->type == MODULE_UNDEFINED ) {
      status = Failed;
      if ( report_errs ) {
        MSG_ERROR("'%s' <-- Modulo non definito!", m->name);
      }
    }
    ++m;
  }
  return status;
}

/* DESCRIPTION: Sets the number of global registers and variables
 *  for each type.
 */
Task VM_Module_Globals(Intg *num_var, Intg *num_reg) {
  register int i;

  if ( vm_globals != 0 ) {
    for(i = 0; i < NUM_TYPES; i++) {
      free( vm_global[i] );
      vm_global[i] = NULL;
      vm_gmin[i] = 1;
      vm_gmax[i] = -1;
    }
  }

  for(i = 0; i < NUM_TYPES; i++) {
    register Intg nv = num_var[i], nr = num_reg[i];
    if ( (nv < 0) || (nr < 0) ) {
      MSG_ERROR("Errore nella definizione dei numeri di registri globali.");
      vm_globals = -1;
      return Failed;
    }
    if ( (vm_global[i] = calloc(nv+nr+1, size_of_type[i])) == NULL ) {
      MSG_ERROR("Errore nella allocazione dei registri globali.");
      vm_globals = -1;
      return Failed;
    }
    vm_gmin[i] = -nv;
    vm_gmax[i] = nr;
  }
  vm_globals = 1;
  {
    register Obj *reg_obj;
    reg_obj = ((Obj *) vm_global[TYPE_OBJ]) + vm_gmin[TYPE_OBJ];
    box_vm_current = reg_obj + 1;
    box_vm_arg1    = reg_obj + 2;
    box_vm_arg2    = reg_obj + 3;
  }
  return Success;
}

/* DESCRIPTION: Sets the value for the global register (or variable)
 *  of type type, whose number is reg. *value is the value assigned to the
 *  register (variable).
 */
Task VM_Module_Global_Set(Intg type, Intg reg, void *value) {
  if ( (type < 0) || (type >= NUM_TYPES ) ) {
    MSG_ERROR("VM_Module_Global_Set: Non esistono registri di questo tipo!");
    return Failed;
  }

  if ( (reg < vm_gmin[type]) || (reg > vm_gmax[type]) ) {
    MSG_ERROR("VM_Module_Global_Set: Riferimento a registro non allocato!");
    return Failed;
  }

  {
    register int s = size_of_type[type];
    register void *dest;
    dest = vm_global[type] + (reg - vm_gmin[type]) * s;
    memcpy(dest, value, s);
  }
  return Success;
}

/* DESCRIPTION: Execute the module number m.
 */
Task VM_Module_Execute(Intg mnum) {
  Module *m;
  register ByteX4 *i_pos;
  VMStatus vm, *vmold = vmcur;
#ifdef DEBUG_EXEC
  Intg i = 0;
#endif

  MSG_LOCATION("VM_Module_Execute");

  /* Controlliamo che il modulo sia installato! */
  if ( (mnum < 1) || (mnum > Arr_NumItem(vm_modules_list)) ) {
    MSG_ERROR("Modulo non installato!");
    return Failed;
  }

  m = Arr_ItemPtr( vm_modules_list, Module, mnum );
  switch (m->type) {
   case MODULE_IS_C_FUNC: return m->ptr.c_func();
   case MODULE_IS_VM_CODE: break;
   default:
    MSG_ERROR("Tentativo di esecuzione di modulo non definito o difettoso.");
    return Failed;
  }

  if ( vmcur == NULL ) {
    register Intg i;
    for(i = 0; i < NUM_TYPES; i++) {
      vm.lmin[i] = 0; vm.lmax[i] = 0; vm.local[i] = & reg0[i];
      if ( vm_global[i] != NULL ) {
        vm.gmin[i] = vm_gmin[i]; vm.gmax[i] = vm_gmax[i];
        vm.global[i] = vm_global[i] + size_of_type[i]*(-vm_gmin[i]);
      } else {
        vm.gmin[i] = 1; vm.gmax[i] = -1; vm.global[i] = NULL;
      }
    }
    vmcur = & vm;
  } else {
    /* Imposto lo stato della VM su una copia dello stato corrente */
    vm = *vmcur;
    vmcur = & vm;
  }

  vm.m = m;
  vm.i_pos = i_pos = (ByteX4 *) m->ptr.vm.code;
  vm.flags.exit = vm.flags.error = 0;
  {register int i; for(i = 0; i < NUM_TYPES; i++) vm.alc[i] = 0;}

  do {
    register int is_long;
    register ByteX4 i_eye;

#ifdef DEBUG_EXEC
    printf("module = "SIntg", pos = "SIntg" - reading instruction.\n",
     mnum, i*sizeof(ByteX4));
#endif

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    ASM_GET_FORMAT(vm.i_pos, i_eye, is_long);
    vm.flags.is_long = is_long;
    if ( is_long ) {
      ASM_LONG_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 1;

    } else {
      ASM_SHORT_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 0;
    }

    if ( vm.i_type >= ASM_ILLEGAL ) {
      MSG_ERROR("Istruzione non riconosciuta!");
      vmcur = vmold; return Failed;
    }

    /* Trovo il descrittore di istruzione */
    vm.idesc = & vm_instr_desc_table[vm.i_type];

    /* Localizza in memoria gli argomenti */
    if ( vm.idesc->numargs > 0 ) vm.idesc->get_args();
    /* Esegue l'istruzione */
    if ( ! vm.flags.error ) vm.idesc->execute();

    /* Passo alla prossima istruzione */
    vm.i_pos = (i_pos += vm.i_len);
#ifdef DEBUG_EXEC
    i += vm.i_len;
#endif

  } while ( ! vm.flags.exit );

  /* Delete the registers allocated with the new* instructions */
  {
    register int i;
    for(i = 0; i < NUM_TYPES; i++)
      if ( (vm.alc[i] & 1) != 0 )
        free(vm.local[i] + vm.lmin[i]*size_of_type[i]);
  }

  if ( ! vm.flags.error ) return Success;
  return Failed;
}

/*******************************************************************************
 *   FUNZIONI PRINCIPALI per disassemblare pezzi di memoria o interi moduli    *
 *******************************************************************************
 */
static struct {
  unsigned int hexcode : 1;
} vm_dflags = { 0 };

/* DESCRIZIONE: Imposta le opzioni per il disassemblaggio:
 *  L'opzione puo' essere settata con un valore > 0, resettata con 0
 *  e lasciata inalterata con un valore < 0.
 *  1) hexcode: scrive nel listato anche i codici esadecimali delle
 *   istruzioni.
 */
void VM_DSettings(int hexcode)
{
  VM_SET(vm_dflags, hexcode);
}

/* DESCRIPTION: This function prints the assembly source code of the module
 *  whose number is module_num.
 */
Task VM_Module_Disassemble(FILE *stream, Intg module_num) {
  Module *m;
  char *mod_type;
  int print_code;

  if ( vm_modules_list == NULL ) {
    MSG_ERROR("Nessun modulo installato!");
    return Failed;
  }
  if ( (module_num < 1) || (module_num > Arr_NumItem(vm_modules_list)) ) {
    MSG_ERROR("Il modulo da disassemblare non esiste!");
    return Failed;
  }

  m = ((Module *) vm_modules_list->ptr) + (module_num - 1);
  fprintf(stream, "\n----------------------------------------\n");
  fprintf(stream, "Module number: "SIntg"\n", module_num);
  if ( m->name != NULL )
    fprintf(stream, "Module name:   '%s'\n", m->name);
  else
    fprintf(stream, "Module name:   (undefined)\n");

  print_code = 0;
  switch(m->type) {
   case MODULE_IS_VM_CODE: mod_type = "BOX-VM code"; print_code = 0; break;
   case MODULE_IS_C_FUNC:  mod_type = "external C-function"; break;
   case MODULE_UNDEFINED:  mod_type = "undefined"; break;
   default:  mod_type = "this should never appear!"; break;
  }
  fprintf(stream, "Module type:   %s\n", mod_type);

  /*if ( print_code )
    return VM_Disassemble(stream, m->ptr.vm.code, m->ptr.vm.dim);*/
  return Success;
}

/* DESCRIPTION: This function prints the assembly source code of the module
 *  whose number is module_num.
 */
Task VM_Module_Disassemble_All(FILE *stream) {
  Intg n, module_num;

  if ( vm_modules_list == NULL ) return Success;

  module_num = Arr_NumItem(vm_modules_list);
  for(n = 1; n <= module_num; n++) {
    TASK( VM_Module_Disassemble(stream, n) );
  }

  return Success;
}

/* DESCRIZIONE: Traduce il codice binario della VM, in formato testo.
 *  prog e' il puntatore all'inizio del codice, dim e' la dimensione del codice
 *  da tradurre (espresso in "numero di ByteX4").
 */
Task VM_Disassemble(FILE *output, void *prog, UInt dim)
{
  register ByteX4 *i_pos = (ByteX4 *) prog;
  VMStatus vm, *vmold = vmcur;
  UInt pos, nargs;
  const char *iname;
  char *iarg[MAX_NUMARGS];

  MSG_LOCATION("VM_Disassemble");

  vmcur = & vm;
  vm.flags.exit = vm.flags.error = 0;
  for ( pos = 0; pos < dim; ) {
    register ByteX4 i_eye;
    register int is_long;

    vm.i_pos = i_pos;

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    ASM_GET_FORMAT(vm.i_pos, i_eye, is_long);
    vm.flags.is_long = is_long;
    if ( is_long ) {
      ASM_LONG_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 1;

    } else {
      ASM_SHORT_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 0;
    }
#ifdef DEBUG_VM_D_EVERY_ONE
    printf("Instruction at position "SUInt": "
     "{is_long = %d, length = "SUInt", type = "SUInt
     ", arg_type = "SUInt")\n",
     pos, vm.flags.is_long, vm.i_len, vm.i_type, vm.arg_type);
#endif

    if ( (vm.i_type < 1) || (vm.i_type >= ASM_ILLEGAL) ) {
      iname = "???";
      vm.i_len = 1;
      nargs = 0;

    } else {
      /* Trovo il descrittore di istruzione */
      vm.idesc = & vm_instr_desc_table[vm.i_type];
      iname = vm.idesc->name;

      /* Localizza in memoria gli argomenti */
      nargs = vm.idesc->numargs;

      vm.idesc->disasm(iarg);
      if ( vm.flags.exit ) goto vm_disassemble_err;
    }

    if ( vm.flags.error ) {
       fprintf(output, SUInt "\t%8.8lx\tError!",
        (UInt) (pos * sizeof(ByteX4)), *i_pos);

    } else {
      int i;
      ByteX4 *i_pos2 = i_pos;

      /* Stampo l'istruzione e i suoi argomenti */
      fprintf( output, SUInt "\t", (UInt) (pos * sizeof(ByteX4)) );
      if ( vm_dflags.hexcode )
        fprintf(output, "%8.8lx\t", *(i_pos2++));
      fprintf(output, "%s", iname);

      if ( nargs > 0 ) {
        UInt n;

        assert(nargs <= MAX_NUMARGS);

        fprintf(output, " %s", iarg[0]);
        for ( n = 1; n < nargs; n++ )
          fprintf(output, ", %s", iarg[n]);
      }
      fprintf(output, "\n");

      /* Stampo i restanti codici dell'istruzione in esadecimale */
      if ( vm_dflags.hexcode ) {
        for( i = 1; i < vm.i_len; i++ )
          fprintf(output, "\t%8.8lx\n", *(i_pos2++));
      }
    }

    /* Passo alla prossima istruzione */
    if ( vm.i_len < 1 ) goto vm_disassemble_err;

    vm.i_pos = (i_pos += vm.i_len);
    pos += vm.i_len;
  }

  vmcur = vmold;
  return Success;

vm_disassemble_err:
  vmcur = vmold;
  return Failed;
}

/*******************************************************************************
 *         FUNZIONI PRINCIPALI per assemblare le istruzioni per la VM          *
 *******************************************************************************
 */

static struct {
  unsigned int forcelong : 1;
} vm_aflags = { 0 };

static AsmOut *vm_cur_output = NULL;
static AsmOut *tmp_code = NULL;


/* DESCRIZIONE: Imposta le opzioni per l'assemblaggio:
 *  L'opzione puo' essere settata con un valore > 0, resettata con 0
 *  e lasciata inalterata con un valore < 0.
 *  1) forcelong: forza la scrittura di tutte le istruzioni in formato lungo.
 *  2) error: indica se, nel corso della scrittura del programma,
 *   si e' verificato un errore;
 *  3) inhibit: inibisce la scrittura del programma (per errori ad esempio).
 * NOTA: Le opzioni da error in poi "appartengono" al programma attualmente
 *  in scrittura.
 */
void VM_ASettings(int forcelong, int error, int inhibit)
{
  /* VM_SET e' definita nell'header virtmach.h */
  VM_SET(vm_aflags, forcelong);

  VM_SET(vm_cur_output->status, error);
  VM_SET(vm_cur_output->status, inhibit);

  return;
}

/* DESCRIZIONE: Crea un nuovo "foglio bianco" dove poter scrivere le istruzioni
 *  assemblate da VM_Assemble. Ad esempio, dopo l'esecuzione delle istruzioni:
 *   out = VM_Asm_Out_New(-1);
 *   VM_Asm_Out_Set(out);
 *  L'output della funzione VM_Assemble finira' in out.
 */
AsmOut *VM_Asm_Out_New(Intg dim)
{
  AsmOut *out;
  Array *prog;

  if ( dim < 1 ) dim = VM_TYPICAL_MOD_DIM;
  prog = Array_New( sizeof(ByteX4), dim );
  if ( prog == NULL ) return NULL;

  out = (AsmOut *) malloc( sizeof(AsmOut) );
  if ( out == NULL ) {
    MSG_ERROR("Memoria esaurita!");
    return NULL;
  }

  out->status.error = 0;
  out->status.inhibit = 0;
  out->program = prog;
  return out;
}

/* DESCRIZIONE: Seleziona l'output della funzione VM_Assemble().
 * NOTA: Se out = NULL, restituisce Failed.
 */
Task VM_Asm_Out_Set(AsmOut *out)
{
  if ( out == NULL ) return Failed;
  vm_cur_output = out;
  return Success;
}

/* DESCRIPTION: This function execute the final steps to prepare the program
 *  to be installed as a module and to be executed.
 *  num_reg and num_var are the pointers to arrays of NUM_TYPES elements
 *  containing the numbers of register and variables used by the program
 *  for every type.
 *  module is the module-number of an undefined module which will be used
 *  to install the program.
 */
Task VM_Asm_Prepare(Intg *num_var, Intg *num_reg) {
  AsmOut *save_cur = vm_cur_output;
  Array *prog, *prog_beginning;

  /* Clean or set-up the array for the temporary code */
  if ( tmp_code == NULL ) {
    tmp_code = VM_Asm_Out_New( VM_TYPICAL_TMP_SIZE );
    if ( tmp_code == NULL ) return Failed;
  } else {
    if IS_FAILED( Arr_Clear(tmp_code->program) ) return Failed;
  }

  VM_Assemble(ASM_RET);

  vm_cur_output = tmp_code;
  {
    register Intg i;
    Intg instruction[NUM_TYPES] = {
      ASM_NEWC_II, ASM_NEWI_II, ASM_NEWR_II, ASM_NEWP_II, ASM_NEWO_II};

      for(i = 0; i < NUM_TYPES; i++) {
        register Intg nv = num_var[i], nr = num_reg[i];
        if ( (nv < 0) || (nr < 0) ) {
          MSG_ERROR("Errore nella chiamata di VM_Asm_Prepare."); goto err;
        }
        if ( (nv > 0) || (nr > 0) )
          VM_Assemble(instruction[i], CAT_IMM, nv, CAT_IMM, nr);
      }
  }
  vm_cur_output = save_cur;
  prog_beginning = tmp_code->program;
  prog = vm_cur_output->program;

  if IS_FAILED(
   Arr_Insert(prog, 1, Arr_NumItem(prog_beginning), Arr_Ptr(prog_beginning))
   ) return Failed;
  return Success;

err:
  vm_cur_output = save_cur;
  return Failed;
}

/* DESCRIPTION: This function install a program (written using
 *  the object AsmOut) into the module with number module.
 * NOTE: program will be destroyed.
 */
Task VM_Asm_Install(Intg module, AsmOut *program) {
  Array *p;
  ModulePtr ptr;
  p = program->program;
  ptr.vm.dim = Arr_NumItem(p);
  ptr.vm.code = Arr_Data_Only(p);
  free(program);
  if IS_FAILED( VM_Module_Define(module, MODULE_IS_VM_CODE, ptr) )
    return Failed;
  return Success;
}

/* DESCRIZIONE: Assembla l'istruzione specificata da instr, scrivendo il codice
 *  binario ad essa corrispondente nella destinazione specificata dalla funzione
 *  VM_Asm_Out_Set().
 */
void VM_Assemble(AsmCode instr, ...)
{
  va_list ap;
  int i, t;
  InstrDesc *idesc;
  int is_short;
  struct {
    TypeID t;  /* Tipi degli argomenti */
    AsmArg c;  /* Categorie degli argomenti */
    void *ptr; /* Puntatori ai valori degli argomenti */
    Intg  vi;   /* Destinazione dei valori...   */
    Real  vr;   /* ...immediati degli argomenti */
    Point vp;
  } arg[MAX_NUMARGS];

  MSG_LOCATION("VM_Assemble");

  /* Esco subito se e' settato il flag di inibizione! */
  if ( vm_cur_output->status.inhibit ) return;

  if ( (instr < 1) || (instr >= ASM_ILLEGAL) ) {
    MSG_ERROR("Istruzione non riconosciuta!");
    return;
  }

  idesc = & vm_instr_desc_table[instr];

  assert( idesc->numargs <= MAX_NUMARGS );

  va_start(ap, instr);

  /* Prendo argomento per argomento */
  t = 0; /* Indice di argomento */
  is_short = 1;
  for ( i = 0; i < idesc->numargs; i++ ) {
    Intg vi;

    /* Prendo dalla lista degli argomenti della funzione la categoria
     * dell'argomento dell'istruzione.
     */
    switch ( arg[t].c = va_arg(ap, AsmArg) ) {
     case CAT_LREG:
     case CAT_GREG:
     case CAT_PTR:
      arg[t].t = TYPE_INTG;
      arg[t].vi = vi = va_arg(ap, Intg);
      arg[t].ptr = (void *) (& arg[t].vi);
      break;

     case CAT_IMM:
      switch (idesc->t_id) {
       case TYPE_CHAR:
        arg[t].t = TYPE_CHAR;
        arg[t].vi = va_arg(ap, Intg); vi = 0;
        arg[t].ptr = (void *) (& arg[t].vi);
        break;
       case TYPE_INTG:
        arg[t].t = TYPE_INTG;
        arg[t].vi = vi = va_arg(ap, Intg);
        arg[t].ptr = (void *) (& arg[t].vi);
        break;
       case TYPE_REAL:
        is_short = 0;
        arg[t].t = TYPE_REAL;
        arg[t].vr = va_arg(ap, Real);
        arg[t].ptr = (void *) (& arg[t].vr);
        break;
       case TYPE_POINT:
        is_short = 0;
        arg[t].t = TYPE_POINT;
        arg[t].vp.x = va_arg(ap, Real);
        arg[t].vp.y = va_arg(ap, Real);
        arg[t].ptr = (void *) (& arg[t].vp);
        break;
       default:
        is_short = 0;
        break;
      }
      break;

     default:
      MSG_ERROR("Categoria di argomenti sconosciuta!");
      vm_cur_output->status.error = 1;
      vm_cur_output->status.inhibit = 1;
      break;
    }

    if (is_short) {
      /* Controllo che l'argomento possa essere "contenuto"
       * nel formato corto.
       */
      vi &= ~0x7fL;
      if ( (vi != 0) && (vi != ~0x7fL) )
        is_short = 0;
    }

    ++t;
  }

  va_end(ap);

  assert(t == idesc->numargs);

  /* Cerco di capire se e' possibile scrivere l'istruzione in formato corto */
  if ( vm_aflags.forcelong ) is_short = 0;
  if ( (is_short == 1) && (t <= 2) ) {
    /* L'istruzione va scritta in formato corto! */
    ByteX4 buffer[1], *i_pos = buffer;
    register ByteX4 i_eye;
    UInt atype;
    Array *prog = vm_cur_output->program;

    for ( ; t < 2; t++ ) {
      arg[t].c = 0;
      arg[t].vi = 0;
    }

    atype = (arg[1].c << 2) | arg[0].c;
    ASM_SHORT_PUT_HEADER(i_pos, i_eye, instr, /* is_long = */ 0,
                         /* i_len = */ 1, atype);
    ASM_SHORT_PUT_2ARGS(i_pos, i_eye, arg[0].vi, arg[1].vi);

    if IS_FAILED( Arr_Push(prog, buffer) ) {
      vm_cur_output->status.error = 1;
      vm_cur_output->status.inhibit = 1;
      return;
    }
    return;

  } else {
    /* L'istruzione va scritta in formato lungo! */
    UInt idim, iheadpos;
    ByteX4 iw[MAX_SIZE_IN_IWORDS];
    Array *prog = vm_cur_output->program;

    /* Lascio il posto per la "testa" dell'istruzione (non conoscendo ancora
     * la dimensione dell'istruzione, non posso scrivere adesso la testa.
     * Potro' farlo solo alla fine, dopo aver scritto tutti gli argomenti!)
     */
    iheadpos = Arr_NumItem(prog) + 1;
    Arr_MInc(prog, idim = 2);

    for ( i = 0; i < t; i++ ) {
      UInt adim, aiwdim;

      adim = size_of_type[arg[i].t];
      aiwdim = (adim + sizeof(ByteX4) - 1) / sizeof(ByteX4);
      iw[aiwdim - 1] = 0;
      (void) memcpy( iw, arg[i].ptr, adim );

      if IS_FAILED( Arr_MPush(prog, iw, aiwdim) ) {
        vm_cur_output->status.error = 1;
        vm_cur_output->status.inhibit = 1;
        return;
      }

      idim += aiwdim;
    }

    {
      ByteX4 *i_pos;
      register ByteX4 i_eye;
      UInt atype;

      /* Trovo il puntatore alla testa dell'istruzione */
      i_pos = Arr_ItemPtr(prog, ByteX4, iheadpos);

      for ( ; t < 2; t++ )
        arg[t].c = 0;

      /* Scrivo la "testa" dell'istruzione */
      atype = (arg[1].c << 2) | arg[0].c;
      ASM_LONG_PUT_HEADER(i_pos, i_eye, instr, /* is_long = */ 1,
                          /* i_len = */ idim, atype);
    }
  }

  return;
}
