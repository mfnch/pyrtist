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

/* This files implements the functionality of the Virtual Machine (VM)
 * of Box: defines functions to assemble, disassemble, execute basic
 * instructions.
 */

/* Read the first 4 bytes (VMByteX4), extract the format bit and put the "rest"
 * in the i_eye (which should be defined as 'register VMByteX4 i_eye;')
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

/* We need to pass the current VM to all the functions. This requires to define
 * every function in the following way:
 *   RetType function(VMProgram *vmp, ...);
 * This is what we do for most of the functions defined here.
 * However for some internal functions we use a different strategy: we pass
 * the current VM status using the static pointer defined in the following
 * statement.
 */
static VMStatus *vmcur;

/* This array lets us to obtain the size of a type by type index.
 * (Useful in what follows)
 */
UInt size_of_type[NUM_TYPES] = {
  sizeof(Char),
  sizeof(Intg),
  sizeof(Real),
  sizeof(Point),
  sizeof(Obj)
};

/* Restituisce il puntatore al registro globale n-esimo. */
static void *VM__Get_G(Intg n) {
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

/* Restituisce il puntatore al registro locale n-esimo. */
static void *VM__Get_L(Intg n) {
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

/* Restituisce il puntatore alla locazione di memoria (ro0 + n). */
static void *VM__Get_P(Intg n) {
  return *((void **) vmcur->local[TYPE_OBJ]) + n;
}

/* Restituisce il puntatore ad un intero/reale di valore n. */
static void *VM__Get_I(Intg n) {
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

/*******************************************************************************
 * Functions used to execute the instructions                                  *
 *******************************************************************************/

/* Questa funzione trova e imposta gli indirizzi corrispondenti
 *  ai 2 argomenti dell'istruzione. In modo da poter procedere all'esecuzione.
 * NOTA: Questa funzione gestisce solo le istruzioni di tipo GLP-GLPI,
 *  cioe' le istruzioni in cui: il primo argomento e' 'G'lobale, 'L'ocale,
 *  'P'untatore, mentre il secondo puo' essere dei tipi appena enumerati oppure
 *  puo' essere un 'I'mmediato intero.
 */
static void VM__GLP_GLPI(void) {
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

/* Questa funzione e' analoga alla precedente, ma gestisce
 *  istruzioni come: "mov ri1, 123456", "mov rf2, 3.14", "mov rp5, (1, 2)", etc.
 */
static void VM__GLP_Imm(void) {
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

/* Questa funzione e' analoga alle precedenti, ma gestisce
 *  istruzioni con un solo argomento di tipo GLPI (Globale oppure Locale
 *  o Puntatore o Immediato intero).
 */
static void VM__GLPI(void) {
  signed long narg1;
  register UInt atype = vmcur->arg_type;

  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  } else {
    ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  }

  vmcur->arg1 = vm_gets[atype & 0x3](narg1);
}

/* Questa funzione e' analoga alle precedenti, ma gestisce
 *  istruzioni con un solo argomento di tipo immediato (memorizzato subito
 *  di seguito all'istruzione).
 */
static void VM__Imm(void) {
  vmcur->arg1 = (void *) vmcur->i_pos;
}

/*******************************************************************************
 * Functions used to disassemble the instructions (see VM_Disassemble)         *
 *******************************************************************************/
/* Array in cui verranno disassemblati gli argomenti (scritti con sprintf) */
static char iarg_str[VM_MAX_NUMARGS][64];

/* Questa funzione serve a disassemblare gli argomenti di
 *  un'istruzione di tipo GLPI-GLPI.
 *  iarg e' una tabella di puntatori alle stringhe che corrisponderanno
 *  agli argomenti disassemblati.
 */
static void VM__D_GLPI_GLPI(char **iarg) {
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

/* Analoga alla precedente, ma per istruzioni CALL. */
static void VM__D_CALL(char **iarg) {
  register UInt na = vmcur->idesc->numargs;

  assert(na <= 2);

  *iarg = iarg_str[0];
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
        VMModule *m = Arr_ItemPtr(vm_modules_list, VMModule, m_num);
        m_name = Str_Cut(m->name, 40, 85);
        sprintf(iarg_str[0], SIntg"('%.40s')", m_num, m_name);
        free(m_name);
        return;
      }

    }

  } else {
    VM__D_GLPI_GLPI(iarg);
  }
}

/* Analoga alla precedente, ma per istruzioni del tipo GLPI-Imm. */
static void VM__D_GLPI_Imm(char **iarg) {
  UInt iaf = vmcur->arg_type & 3, iat = vmcur->idesc->t_id;
  Intg iai;
  VMByteX4 *arg2;

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
 * Functions for (de)inizialization                                            *
 *******************************************************************************/
Task VMProg_Init(VMProgram *new_vmp) {
  new_vmp = (VMProgram *) malloc(sizeof(VMProgram));
  if (new_vmp == NULL) return Failed;
  new_vmp->vm_modules_list = (Array *) NULL;
  new_vmp->vm_globals = 0;
  return Success;
}

void VMProg_Destroy(VMProgram *vmp) {
  assert(vmp != (VMProgram *) NULL);
  if (vmp->vm_globals != 0) {
    int i;
    for(i = 0; i < NUM_TYPES; i++) free( vm_global[i] );
  }
  Arr_Destroy(vmp->vm_modules_list);
  free(vmp);
  return Success;
}

/*******************************************************************************
 * Functions to handle sheets: a sheet is a place where you can put temporary  *
 * code, which can then be istalled as a new module or handled in other ways.  *
 *******************************************************************************/
Task VMProg_New_Sheet(VMProgram *vmp, UInt *new_sheet) {
}

Task VMProg_Install_Sheet(VMProgram *vmp, UInt sheet, UInt *assigned_module) {

}

/* Installa un nuovo modulo di programma con nome name.
 * Un modulo e' semplicemente un pezzo di codice che puo' essere eseguito.
 * E' possibile installare 2 tipi di moduli:
 *  1) (caso t = MODULE_IS_VM_CODE) il modulo e' costituito da
      codice eseguibile dalla VM
 *    (p.vm_code e' il puntatore alla prima istruzione nel codice);
 *  2) (caso t = MODULE_IS_C_FUNC) il modulo e' semplicemente una funzione
 *    scritta in C (p.c_func e' il puntatore alla funzione)
 * Restituisce il numero assegnato al modulo (> 0), oppure 0 se qualcosa
 * e' andato storto. Tale numero, se usato da un istruzione call, provoca
 * l'esecuzione del modulo come procedura.
 */
Task VM_Module_Install(VMProgram *vmp, Intg *new_module
 VMModuleType t, const char *name, VMModulePtr p) {
  /* Creo la lista dei moduli se non esiste */
  if ( vmp->vm_modules_list == NULL ) {
    vmp->vm_modules_list = Array_New(sizeof(VMModule), VM_TYPICAL_NUM_MODULES);
    if ( vmp->vm_modules_list == NULL ) return Failed;
  }

  {
    VMModule new;
    new.type = t;
    new.name = strdup(name);
    new.ptr = p;
    TASK( Arr_Push( vmp->vm_modules_list, & new ) );
  }

  *new_module = Arr_NumItem(vm_modules_list);
  return Success;
}

/* This function defines an undefined module (previously created
 * with VM_Module_Undefined).
 */
Task VM_Module_Define(VMProgram *vmp, Intg module_num,
 VMModuleType t, VMModulePtr p) {
  VMModule *m;
  if ( vmp->vm_modules_list == NULL ) {
    MSG_ERROR("La lista dei moduli e' vuota!");
    return Failed;
  }
  if ((module_num < 1) || (module_num > Arr_NumItem(vmp->vm_modules_list))) {
    MSG_ERROR("Impossibile definire un modulo non esistente.");
    return Failed;
  }
  m = ((VMModule *) vmp->vm_modules_list->ptr) + (module_num - 1);
  if ( m->type != MODULE_UNDEFINED ) {
    MSG_ERROR("Questo modulo non puo' essere definito!");
    return Failed;
  }
  m->type = t;
  m->ptr = p;
  return Success;
}

/* This function creates a new undefined module with name name. */
Task VM_Module_Undefined(VMProgram *vmp, Intg *new_module, const char *name) {
  return VM_Module_Install(vmp, new_module,
   MODULE_UNDEFINED, name, (VMModulePtr) {{0, NULL}});
}

/* This function returns the module-number which will be
 * associated with the next module that will be installed.
 */
Intg VM_Module_Next(VMProgram *vmp) {
  if ( vmp->vm_modules_list == NULL ) return 1;
  return Arr_NumItem(vmp->vm_modules_list) + 1;
}

/* This function checks the status of definitions of all the
 * created modules. If one of the modules is undefined (and report_errs == 1)
 * it prints an error message for every undefined module.
 */
Task VM_Module_Check(VMProgram *vmp, int report_errs) {
  VMModule *m;
  Intg mn;
  int status = Success;

  if ( vmp->vm_modules_list == NULL ) return Success;
  m = Arr_FirstItemPtr(vmp->vm_modules_list, VMModule);
  for (mn = Arr_NumItem(vmp->vm_modules_list); mn > 0; mn--) {
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

/* Sets the number of global registers and variables for each type. */
Task VM_Module_Globals(VMProgram *vmp, Intg num_var[], Intg num_reg[]) {
  int i;

  assert(vmp != (VMProgram *) NULL);

  if ( vmp->vm_globals != 0 ) {
    int i;
    for(i = 0; i < NUM_TYPES; i++) {
      free( vmp->vm_global[i] );
      vmp->vm_global[i] = NULL;
      vmp->vm_gmin[i] = 1;
      vmp->vm_gmax[i] = -1;
    }
  }

  for(i = 0; i < NUM_TYPES; i++) {
    Intg nv = num_var[i], nr = num_reg[i];
    if (nv < 0 || nr < 0) {
      MSG_ERROR("Errore nella definizione dei numeri di registri globali.");
      return Failed;
    }
  }

  for(i = 0; i < NUM_TYPES; i++) {
    Intg nv = num_var[i], nr = num_reg[i];
    if ( (vmp->vm_global[i] = calloc(nv+nr+1, size_of_type[i])) == NULL ) {
      MSG_ERROR("Errore nella allocazione dei registri globali.");
      vmp->vm_globals = -1;
      return Failed;
    }
    vmp->vm_gmin[i] = -nv;
    vmp->vm_gmax[i] = nr;
  }

  vmp->vm_globals = 1;
  {
    register Obj *reg_obj;
    reg_obj = ((Obj *) vmp->vm_global[TYPE_OBJ]) + vmp->vm_gmin[TYPE_OBJ];
    vmp->box_vm_current = reg_obj + 1;
    vmp->box_vm_arg1    = reg_obj + 2;
    vmp->box_vm_arg2    = reg_obj + 3;
  }
  return Success;
}

/* Sets the value for the global register (or variable)
 * of type type, whose number is reg. *value is the value assigned
 * to the register (variable).
 */
Task VM_Module_Global_Set(VMProgram *vmp, Intg type, Intg reg, void *value) {
  if ( (type < 0) || (type >= NUM_TYPES ) ) {
    MSG_ERROR("VM_Module_Global_Set: Non esistono registri di questo tipo!");
    return Failed;
  }

  if ( (vmp->reg < vmp->vm_gmin[type]) || (reg > vmp->vm_gmax[type]) ) {
    MSG_ERROR("VM_Module_Global_Set: Riferimento a registro non allocato!");
    return Failed;
  }

  {
    int s = size_of_type[type];
    void *dest;
    dest = vmp->vm_global[type] + (reg - vmp->vm_gmin[type]) * s;
    memcpy(dest, value, s);
  }
  return Success;
}

/*******************************************************************************
 * Functions to execute code                                                   *
 *******************************************************************************/
/* Execute the module number m. */
Task VM_Module_Execute(VMProgram *vmp, Intg mnum) {
  VMModule *m;
  register VMByteX4 *i_pos;
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

  m = Arr_ItemPtr( vm_modules_list, VMModule, mnum );
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
  vm.i_pos = i_pos = (VMByteX4 *) m->ptr.vm.code;
  vm.flags.exit = vm.flags.error = 0;
  {register int i; for(i = 0; i < NUM_TYPES; i++) vm.alc[i] = 0;}

  do {
    register int is_long;
    register VMByteX4 i_eye;

#ifdef DEBUG_EXEC
    printf("module = "SIntg", pos = "SIntg" - reading instruction.\n",
           mnum, i*sizeof(VMByteX4));
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
 * Functions to assemble code                                                  *
 *******************************************************************************/

Task VMProg_Execute_Module(VMProgram *vmp, UInt module, int *status);
Task VMProg_Assemble(VMProgram *vmp, AsmCode instr, ...);
Task VMProg_Dasm(VMProgram *vmp, UInt module, FILE *out);
Task VMProg_Dasm_Data(VMProgram *vmp, FILE *out);
Task VMProg_Dasm_All(VMProgram *vmp, FILE *out);
Task VMProg_Dasm_Opts(VMProgram *vmp, ???);
Task VMProg_Save(VMProgram *vmp, FILE *out);
Task VMProg_Load(VMProgram *vmp, FILE *in);

Task VM_Module_Globals(Intg *num_var, Intg *num_reg);
Task VM_Module_Global_Set(Intg type, Intg reg, void *value);
  Task VM_Module_Execute(Intg mnum);
  void VM_DSettings(int hexcode);
  Task VM_Module_Disassemble(FILE *stream, Intg module_num);
  Task VM_Module_Disassemble_All(FILE *stream);
  Task VM_Disassemble(FILE *output, void *prog, UInt dim);
AsmOut *VM_Asm_Out_New(Intg dim);
Task VM_Asm_Out_Set(AsmOut *out);
Task VM_Asm_Prepare(Intg *num_var, Intg *num_reg);
Task VM_Asm_Install(Intg module, AsmOut *program);
  void VM_Assemble(AsmCode instr, ...);
int VM_Save(FILE *out, Intg module_num);
