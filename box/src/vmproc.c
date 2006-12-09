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

#include "virtmach.h"
#include "vmproc.h"

typedef Task (*VMCCode)(VMProgram *);
typedef unsigned int VMCode;

static Task Procedure_Destroy(void *s) {
  Array *code = ((VMProc *) s)->code;
  if ( code != (Array *) NULL ) Arr_Destroy( code );
  return Success;
}

Task VM_Proc_Init(VMProgram *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  TASK( Clc_New(& pt->uninstalled, sizeof(VMProc), VMPROC_INST_CLC_SIZE) );
  Clc_Destructor(pt->uninstalled, Procedure_Destroy);
  return Success;
}

void VM_Proc_Destroy(VMProgram *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  Clc_Destroy(pt->uninstalled);
}

#if 0
Task VM_Proc_Install_CCode(VMProgram *vmp, int *proc_num, VMCCode proc_code);

Task VM_Proc_Install_Code(VMProgram *vmp, int *proc_num, VMCode proc_code);

/******************************************************************************
 * Functions to handle sheets: a sheet is a place where you can put temporary *
 * code, which can then be istalled as a new module or handled in other ways. *
 ******************************************************************************/


/* This function creates a new sheet. A sheet is a place where to put
 * the assembly code (bytecode) produced by VM_Assemble and is identified
 * by an integer assigned by this function. This integer is passed back
 * through *sheet_id.
 */
Task VM_Sheet_New(VMProgram *vmp, int *sheet_id) {
  VMSheet sheet;
  Array *program;

  if ( vmp->sheets == (Collection *) NULL ) {
     TASK( Clc_New(& vmp->sheets, sizeof(VMSheet), VM_TYPICAL_NUM_SHEETS) );
     Clc_Destructor(vmp->sheets, VM__Sheet_Destroy);
  }

  TASK( Arr_New(& program, sizeof(VMByteX4), VM_TYPICAL_MOD_DIM) );

  sheet.program = program;
  sheet.status.error = 0;
  sheet.status.inhibit = 0;
  return Clc_Occupy(vmp->sheets, & sheet, sheet_id);
}

/* Destroys the sheet 'sheet_id' created with 'VM_Sheet_New'.
 */
Task VM_Sheet_Destroy(VMProgram *vmp, int sheet_id) {
  return Clc_Release(vmp->sheets, sheet_id);
}

/* Get the VMSheet structure which describes sheet number 'sheet_id'.
 */
Task VM_Sheet(VMProgram *vmp, VMSheet **s, int sheet_id) {
  assert( vmp->sheets != (Collection *) NULL);
  return Clc_Object_Ptr(vmp->sheets, (void **) s, sheet_id);
}

/* Get the ID of the active sheet (the current target of VM_Assemble).
 */
int VM_Sheet_Get_Current(VMProgram *vmp) {
  return vmp->current_sheet_id;
}

/* Set ''sheet_id to be the active sheet.
 */
Task VM_Sheet_Set_Current(VMProgram *vmp, int sheet_id) {
  VMSheet *sheet;
  assert( vmp->sheets != (Collection *) NULL);
  TASK( Clc_Object_Ptr(vmp->sheets, (void **) & sheet, sheet_id) );
  vmp->current_sheet_id = sheet_id;
  vmp->current_sheet = sheet;
  return Success;
}

/* Remove all the code assembled inside the sheet 'sheet_id'.
 * WARNING: Labels and their references are not removed!
 */
Task VM_Sheet_Clear(VMProgram *vmp, int sheet_id) {
  VMSheet *sheet;
  assert( vmp->sheets != (Collection *) NULL);
  TASK( Clc_Object_Ptr(vmp->sheets, (void **) & sheet, sheet_id) );
  TASK( Arr_Clear(sheet->program) );
  return Success;
}

/* Install the sheet 'sheet_id' and assign to it the module number 'module'.
 * After this function has been executed, the VM will recognize the instruction
 * 'call module' as a call to the code containing in the sheet 'sheet_id'.
 */
Task VM_Sheet_Install(VMProgram *vmp, Intg module, int sheet_id) {
  VMSheet *sheet;
  VMModulePtr module_ptr;
  assert( vmp->sheets != (Collection *) NULL);
  TASK( Clc_Object_Ptr(vmp->sheets, (void **) & sheet, sheet_id) );
  module_ptr.vm.dim = Arr_NumItem(sheet->program);
  TASK( Arr_Data_Only(sheet->program, & module_ptr.vm.code) );
  return VM_Module_Define(vmp, module, MODULE_IS_VM_CODE, module_ptr);
}

/* Print as plain text the code contained inside the sheet 'sheet_id'
 * inside out.
 */
Task VM_Sheet_Disassemble(VMProgram *vmp, int sheet_id, FILE *out) {
  VMSheet *sheet;
  assert( vmp->sheets != (Collection *) NULL);
  TASK( Clc_Object_Ptr(vmp->sheets, (void **) & sheet, sheet_id) );
  {
    Array *prg = sheet->program;
    int prg_len = Arr_NumItem(prg);
    void *prg_ptr = Arr_Ptr(prg);
    return VM_Disassemble(vmp, out, prg_ptr, prg_len);
  }
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
Task VM_Module_Install(VMProgram *vmp, Intg *new_module,
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

  *new_module = Arr_NumItem(vmp->vm_modules_list);
  {
    VMSym s;
    Name n = {strlen(name), name};
    VM_Sym_Procedure(& s, & n, *new_module);
    TASK( VM_Sym_Add(vmp, & s) );
  }
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

  if ( (reg < vmp->vm_gmin[type]) || (reg > vmp->vm_gmax[type]) ) {
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
#endif
