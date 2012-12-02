/****************************************************************************
 * Copyright (C) 2008-2012 by Matteo Franchin                               *
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

/* This files implements the functionality of the Virtual Machine (VM)
 * of Box: defines functions to assemble, disassemble, execute basic
 * instructions.
 */
#define DEBUG_EXEC 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "mem.h"
#include "strutils.h"
#include "messages.h"
#include "array.h"
#include "occupation.h"
#include "vm_priv.h"
#include "vmx.h"
#include "vmsym.h"
#include "vmproc_priv.h"
#include "container.h"

/* Numero minimo di BoxVMWord che riesce a contenere tutti i tipi possibili
 * di argomenti (BoxInt, BoxReal, BoxPoint, BoxObj)
 */
#  define MAX_SIZE_IN_IWORDS \
   ((sizeof(BoxPoint) + sizeof(BoxVMWord) - 1) / sizeof(BoxVMWord))

/* Static functions defined in this file */
static void My_Set_Data_Segment_Register(BoxVM *vm);


/*****************************************************************************
 * Functions for (de)inizialization                                          *
 *****************************************************************************/

/**
 * @brief Finalize an installed type.
 *
 * This function is called when destroying the table of installed types.
 * @param type The type to destroy.
 */
static void My_Finalize_Installed_Type(void *type) {
  (void) BoxType_Unlink(*((BoxType **) type));
}

BoxTask BoxVM_Init(BoxVM *vm) {
  int i;

  vm->attr.hexcode = 0;
  vm->attr.forcelong = 0;
  vm->attr.identdata = 0;

  vm->has.globals = 0;
  vm->has.op_table = 0;

  /* Table of actions for each VM instruction */
  vm->exec_table = BoxVM_Get_Exec_Table();

  vm->vmcur = BoxMem_Alloc(sizeof(BoxVMX));
  if (vm->vmcur == NULL)
    return BOXTASK_FAILURE;

  vm->vmcur->vm = vm;

  /* Reset the global register boundaries and pointers */
  for(i = 0; i < NUM_TYPES; i++) {
    BoxVMRegs *gregs = & vm->global[i];
    gregs->ptr = NULL;
    gregs->min = 1;
    gregs->max = -1;
  }

  BoxArr_Init(& vm->types, sizeof(BoxType *), 128);
  BoxArr_Set_Finalizer(& vm->types, My_Finalize_Installed_Type);
  BoxHT_Init_Default(& vm->types_dict, 128);

  BoxArr_Init(& vm->stack, sizeof(BoxPtr), 10);
  BoxArr_Init(& vm->data_segment, sizeof(char), CMP_TYPICAL_DATA_SIZE);
  BoxArr_Init(& vm->backtrace, sizeof(BoxVMTrace), 32);

  vm->fail_msg = NULL;

  if (BoxArr_Is_Err(& vm->stack) || BoxArr_Is_Err(& vm->data_segment))
    return BOXTASK_FAILURE;

  BoxVM_Proc_Init(vm);
  BoxVMSymTable_Init(& vm->sym_table);
  return BOXTASK_OK;
}

static void My_Free_Globals(BoxVM *vmp) {
  int i;
  for(i = 0; i < NUM_TYPES; i++) {
    BoxVMRegs *gregs = & vmp->global[i];

    if (gregs->ptr != NULL) {
      if (i == BOXTYPEID_PTR) {
        BoxPtr *ptrs = gregs->ptr;
        BoxInt j;
        for (j = gregs->min; j < gregs->max; j++)
          (void) BoxPtr_Unlink(ptrs + j);
      }

      BoxMem_Free(gregs->ptr + gregs->min*size_of_type[i]);
    }
    gregs->ptr = NULL;
    gregs->min = 1;
    gregs->max = -1;
  }
  vmp->has.globals = 0;
}

void BoxVM_Finish(BoxVM *vm) {
  if (vm == NULL)
    return;

  if (vm->has.globals)
    My_Free_Globals(vm);

  BoxArr_Finish(& vm->types);
  BoxHT_Finish(& vm->types_dict);

  if (BoxArr_Num_Items(& vm->stack) != 0)
    MSG_WARNING("Run finished with non empty stack.");
  BoxArr_Finish(& vm->stack);

  BoxArr_Finish(& vm->data_segment);
  BoxArr_Finish(& vm->backtrace);

  if (vm->fail_msg != NULL)
    BoxMem_Free(vm->fail_msg);

  BoxVMSymTable_Finish(& vm->sym_table);
  BoxVM_Proc_Finish(vm);

  if (vm->has.op_table)
    BoxOpTable_Destroy(& vm->op_table);

  if (vm->vmcur != NULL)
    BoxMem_Free(vm->vmcur);
}

BoxVM *BoxVM_Create(void) {
  BoxVM *vm = BoxMem_Alloc(sizeof(BoxVM));

  if (vm == NULL)
    return NULL;

  if (BoxVM_Init(vm) != BOXTASK_OK) {
    BoxMem_Free(vm);
    return NULL;
  }

  return vm;
}

void BoxVM_Destroy(BoxVM *vm) {
  if (vm == NULL)
    return;
  BoxVM_Finish(vm);
  BoxMem_Free(vm);
}

/* Install a new type and return its type identifier for this VM. */
BoxTypeId BoxVM_Install_Type(BoxVM *vm, BoxType *t) {
  BoxHTItem *item;

  /* First, try to see whether we have already associated an ID to the type. */
  if (BoxHT_Find(& vm->types_dict, & t, sizeof(t), & item)) {
    /* Yes. We then return that ID. */
    return *((BoxTypeId *) item->object);

  } else {
    /* No. We have to generate a new ID. */
    BoxTypeId id;

    (void) BoxType_Link(t);
    (void) BoxArr_Push(& vm->types, & t);
    id = BoxArr_Get_Num_Items(& vm->types);

    /* Add the ID to the dictionary of types->ID, to avoid generating many
     * IDs for the same identical type.
     */
    (void) BoxHT_Insert_Obj(& vm->types_dict, & t, sizeof(t),
                            & id, sizeof(id));
    return id;
  }
}

/* Retrieve the type corresponding to the given type identifier. */
BoxType *
BoxVM_Get_Installed_Type(BoxVM *vm, BoxTypeId id) {
  return *((BoxType **) BoxArr_Get_Item_Ptr(& vm->types, id));
}

/* Generate a human-readable description of the table of installed types. */
char *BoxVM_Get_Installed_Types_Desc(BoxVM *vm) {
  char *s = NULL;
  size_t id, n = BoxArr_Get_Num_Items(& vm->types);

  for (id = 1; id <= n; id++) {
    BoxType *t = *((BoxType **) BoxArr_Get_Item_Ptr(& vm->types, id));
    if (s)
      s = Box_SPrintF("%~s\n%d: %~s", s, id, BoxType_Get_Repr(t));
    else
      s = Box_SPrintF("%d: %~s", id, BoxType_Get_Repr(t));
  }

  return (s) ? s : Box_SPrintF("");
}

BoxOpInfo *BoxVM_Get_Op_Info(BoxVM *vm, BoxGOp g_op) {
  assert(g_op >= 0 && g_op < BOX_NUM_GOPS);
  if (!vm->has.op_table) {
    BoxOpTable_Build(& vm->op_table);
    vm->has.op_table = 1;
  }
  return & vm->op_table.info[g_op];
}

/* Sets the number of global registers and variables for each type. */
BoxTask
BoxVM_Alloc_Global_Regs(BoxVM *vm, BoxInt num_var[], BoxInt num_reg[]) {
  int i;
  BoxPtr *reg_obj;

  assert(vm != NULL);

  if (vm->has.globals)
    My_Free_Globals(vm);

  for(i = 0; i < NUM_TYPES; i++) {
    BoxInt nv = num_var[i], nr = num_reg[i];
    BoxVMRegs *gregs = & vm->global[i];
    void *ptr;
    size_t nr_tot = nv + nr + 1;

    if (nv < 0 || nr < 0) {
      MSG_ERROR("Wrong allocation numbers for global registers.");
      My_Free_Globals(vm);
      return BOXTASK_FAILURE;
    }

    if (nr < 3) nr = 3; /* gro0, gro1, gro2 are always needed! */
    ptr = calloc(nr_tot, size_of_type[i]);
    if (ptr == NULL) {
      MSG_ERROR("Error in the allocation of the local registers.");
      My_Free_Globals(vm);
      return BOXTASK_FAILURE;
    }

    gregs->ptr = ptr + nv*size_of_type[i];
    gregs->min = -nv;
    gregs->max = nr;
    vm->has.globals = 1; /* This line must stay here, not outside the loop! */

    if (i == BOXTYPEID_PTR) {
      size_t j;
      for (j = 0; j < nr_tot; j++)
        BoxPtr_Nullify((BoxPtr *) ptr + j);
    }
  }

  reg_obj = (BoxPtr *) vm->global[TYPE_OBJ].ptr;
  vm->box_vm_current = reg_obj + 1;
  vm->box_vm_arg1    = reg_obj + 2;
  My_Set_Data_Segment_Register(vm);
  return BOXTASK_OK;
}

/* Sets the value for the global register (or variable)
 * of type type, whose number is reg. *value is the value assigned
 * to the register (variable).
 */
void BoxVM_Module_Global_Set(BoxVM *vmp, BoxInt type, BoxInt reg, void *value) {
  void *dest;
  BoxVMRegs *gregs;

  if (type < 0 || type >= NUM_TYPES) {
    MSG_FATAL("BoxVM_Module_Global_Set: Unknown register type!");
    assert(0);
  }

  gregs = & vmp->global[type];
  if (reg < gregs->min || reg > gregs->max) {
    MSG_FATAL("BoxVM_Module_Global_Set: Reference to unallocated register!");
    assert(0);
  }

  dest = gregs->ptr + reg*size_of_type[type];

  /* Unlink the reference associated to the register, before setting it. */
  if (type == BOXTYPEID_PTR)
    (void) BoxPtr_Unlink((BoxPtr *) dest);

  memcpy(dest, value, size_of_type[type]);
}

/*****************************************************************************
 * Functions to execute code                                                 *
 *****************************************************************************/

/* NOTE: we don't want this function to change gro1/gro2, as it may be
 *   indirectly called by the instruction create, which is not supposed to do
 *   so.
 */
BoxTask BoxVM_Module_Execute_With_Args(BoxVMX *vmx, BoxVMCallNum cn,
                                       BoxPtr *parent, BoxPtr *child) {
  BoxVM *vm = vmx->vm;
  BoxTask t;
  BoxPtr save_parent = *vm->box_vm_current,
         save_child = *vm->box_vm_arg1;

  if (parent) {
    *vm->box_vm_current = *parent;
    (void) BoxPtr_Link(vm->box_vm_current);
  } else
    BoxPtr_Nullify(vm->box_vm_current);

  if (child) {
    *vm->box_vm_arg1 = *child;
    (void) BoxPtr_Link(vm->box_vm_arg1);
  } else
    BoxPtr_Nullify(vm->box_vm_arg1);

  t = BoxVM_Module_Execute(vmx, cn);

  if (!BoxPtr_Is_Detached(vm->box_vm_current))
    (void) BoxPtr_Unlink(vm->box_vm_current);

  if (!BoxPtr_Is_Detached(vm->box_vm_arg1))
    (void) BoxPtr_Unlink(vm->box_vm_arg1);

  *vm->box_vm_current = save_parent;
  *vm->box_vm_arg1 = save_child;
  return t;
}

/* Execute the module number m of program vmp.
 * If initial != NULL, *initial is the initial status of the virtual machine.
 */
BoxTask BoxVM_Module_Execute(BoxVMX *vmx, BoxVMCallNum call_num) {
  BoxVM *vmp = vmx->vm;
  BoxVMX this_vmx;

  const BoxVMInstrDesc *exec_table = vmp->exec_table;
  BoxVMProcTable *pt = & vmp->proc_table;
  BoxVMProcInstalled *p;
  BoxVMWord *i_pos, *i_pos0;
  BoxValue reg0[NUM_TYPES]; /* Registri locali numero zero */
  BoxOpExecutor executor;

#if DEBUG_EXEC == 1
  BoxInt i = 0;
#endif

  /* Controlliamo che il modulo sia installato! */
  if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
    vmx->flags.error = 1;
    vmx->flags.exit = 1;
    MSG_ERROR("Call to the undefined procedure %d.", call_num);
    return BOXTASK_FAILURE;
  }

#if DEBUG_EXEC == 1
  fprintf(stderr, "call module = "SInt": %p@%p\n",
          call_num, BOX_VM_THIS_PTR(vmx, void),
          BOX_VM_ARG_PTR(vmx, void));
#endif

  p = (BoxVMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
  switch (p->type) {
  case BOXVMPROCKIND_FOREIGN:
    {
      BoxTask t = BoxCallable_CallOld(p->code.foreign, vmx);
      return t;
    }
  case BOXVMPROCKIND_C_CODE:
    return p->code.c(vmx);
  case BOXVMPROCKIND_VM_CODE:
    break;
  default:
    MSG_ERROR("Call into the broken procedure %d.", call_num);
    return BOXTASK_FAILURE;
  }

  this_vmx.vm = vmp;
  vmp->vmcur = & this_vmx;

  {
    int i;
    this_vmx.global = vmp->global;
    for (i = 0; i < NUM_TYPES; i++) {
      BoxVMRegs *lnew = & this_vmx.local[i];
      lnew->min = lnew->max = 0;
      lnew->ptr = & reg0[i];
      this_vmx.alc[i] = 0;
    }
  }

  this_vmx.p = p;
  BoxVM_Proc_Get_Ptr_And_Length(vmp, & this_vmx.i_pos, NULL, p->code.proc_id);
  i_pos0 = i_pos = this_vmx.i_pos;
  this_vmx.flags.exit = this_vmx.flags.error = 0;

  executor.vmx = & this_vmx;

  do {
    int is_long;
    BoxVMWord i_eye;

#if DEBUG_EXEC == 1
    fprintf(stderr, "module = "SInt", pos = "SInt" - reading instruction.\n",
            call_num, i*sizeof(BoxVMWord));
#endif

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    BOXVM_READ_OP_FORMAT(this_vmx.i_pos, i_eye, is_long);

    if (is_long)
      BOXVM_READ_LONGOP_HEADER(this_vmx.i_pos, i_eye, this_vmx.i_type,
                               this_vmx.i_len, this_vmx.arg_type);

    else
      BOXVM_READ_SHORTOP_HEADER(this_vmx.i_pos, i_eye, this_vmx.i_type,
                                this_vmx.i_len, this_vmx.arg_type);

    this_vmx.i_eye = i_eye;
    this_vmx.flags.is_long = is_long;

    if (this_vmx.i_type >= BOX_NUM_OPS) {
      MSG_ERROR("Unknown VM instruction!");
      vmp->vmcur = vmx;
      return BOXTASK_FAILURE;
    }

    /* Trovo il descrittore di istruzione */
    this_vmx.idesc = & exec_table[this_vmx.i_type];

    /* Localizza in memoria gli argomenti */
    if (this_vmx.idesc->numargs > 0)
      this_vmx.idesc->get_args(& executor);

    /* Esegue l'istruzione */
    if (!this_vmx.flags.error)
      this_vmx.idesc->execute(& this_vmx);

    /* Advance to the next instruction...
     * vm.i_len can be modified by 'vm.idesc->execute(vmp)' when executing
     * instructions such as 'jmp' or 'jc'
     */
    this_vmx.i_pos = (i_pos += this_vmx.i_len);
#if DEBUG_EXEC == 1
    i += this_vmx.i_len;
#endif

  } while (!this_vmx.flags.exit);

  /* Fill the backtrace */
  if (this_vmx.flags.error) {
    BoxVMTrace *trace = BoxArr_Push(& vmp->backtrace, NULL);
    trace->call_num = call_num;
    trace->vm_pos = (void *) this_vmx.i_pos - (void *) i_pos0;
  }

  /* Destroy the objects remaining in the roX registers */
  if (this_vmx.alc[BOXTYPEID_PTR] & 1) {
    BoxVMRegs *lregs = & this_vmx.local[BOXTYPEID_PTR];
    BoxPtr *ro = (BoxPtr *) lregs->ptr + lregs->min;
    int i, n = lregs->max - lregs->min + 1;
    /* ^^^ NOTE: lregs->min is negative! */

    for (i = 0; i < n; i++, ro++) {
      if (!BoxPtr_Is_Detached(ro))
        (void) BoxPtr_Unlink(ro);
    }
  }

  {
    /* Delete the registers allocated with the 'new*' instructions */
    int i;
    for (i = 0; i < NUM_TYPES; i++)
      if ((this_vmx.alc[i] & 1) != 0) {
        BoxVMRegs *lregs = & this_vmx.local[i];
        BoxMem_Free(lregs->ptr + lregs->min*size_of_type[i]);
      }
  }

  vmp->vmcur = vmx;
  return this_vmx.flags.error ? BOXTASK_FAILURE : BOXTASK_OK;
}

/*****************************************************************************
 * Functions to disassemble code                                             *
 *****************************************************************************/

int BoxVM_Set_Force_Long(BoxVM *vm, int force_long) {
  int is_long = vm->attr.forcelong;
  if ((force_long | 1) == 1) /* If force_long != 0, 1 the flag is not set */
    vm->attr.forcelong = force_long;
  return is_long;
}

void BoxVM_Set_Attr(BoxVM *vm, BoxVMAttr mask, BoxVMAttr value) {
  if ((mask & BOXVM_ATTR_ASM_LONG_FMT) != 0)
    vm->attr.forcelong = ((value & BOXVM_ATTR_ASM_LONG_FMT) != 0);
  if ((mask & BOXVM_ATTR_DASM_WITH_HEX) != 0)
    vm->attr.hexcode = ((value & BOXVM_ATTR_DASM_WITH_HEX) != 0);
  if ((mask & BOXVM_ATTR_ADD_DATA_IDENT) != 0)
    vm->attr.identdata = ((value & BOXVM_ATTR_ADD_DATA_IDENT) != 0);
}

/*****************************************************************************
 * Functions to assemble code                                                *
 *****************************************************************************/

/* Imposta le opzioni per l'assemblaggio:
 * L'opzione puo' essere settata con un valore > 0, resettata con 0
 * e lasciata inalterata con un valore < 0.
 *  1) forcelong: forza la scrittura di tutte le istruzioni in formato lungo.
 *  2) error: indica se, nel corso della scrittura del programma,
 *   si e' verificato un errore;
 *  3) inhibit: inibisce la scrittura del programma (per errori ad esempio).
 * NOTA: Le opzioni da error in poi "appartengono" al programma attualmente
 *  in scrittura.
 */
void BoxVM_ASettings(BoxVM *vmp, int forcelong, int error, int inhibit) {
  BoxVMProcTable *pt = & vmp->proc_table;
  vmp->attr.forcelong = forcelong;
  pt->target_proc->status.error = error;
  pt->target_proc->status.inhibit = inhibit;
}

/** Similar to BoxVM_Assemble, but takes a va_list argument as a replacement
 * for the extra arguments.
 */
void BoxVM_VA_Assemble(BoxVM *vmp, BoxOp instr, va_list ap) {
  const BoxVMInstrDesc *exec_table = vmp->exec_table, *idesc;
  BoxVMProcTable *pt = & vmp->proc_table;
  int i, t;
  int is_short;
  struct {
    TypeID    t;    /* Tipi degli argomenti */
    BoxOp c;    /* Categorie degli argomenti */
    void      *ptr; /* Puntatori ai valori degli argomenti */
    BoxInt    vi;   /* Destinazione dei valori...   */
    BoxReal   vr;   /* ...immediati degli argomenti */
    BoxPoint  vp;
  } arg[VM_MAX_NUMARGS];

  /* Esco subito se e' settato il flag di inibizione! */
  if (pt->target_proc->status.inhibit) return;

  if (instr < 0 || instr >= BOX_NUM_OPS) {
    MSG_ERROR("Unrecognised VM instruction while assembling (%s).",
              (instr < 0) ? "op < 0" : "op > BOX_NUM_OPS");
    return;
  }

  idesc = & exec_table[instr];

  assert(idesc->numargs <= VM_MAX_NUMARGS);

  /* Prendo argomento per argomento */
  t = 0; /* Indice di argomento */
  is_short = 1;
  for (i = 0; i < idesc->numargs; i++) {
    BoxInt vi = 0;

    /* Prendo dalla lista degli argomenti della funzione la categoria
    * dell'argomento dell'istruzione.
    */
    switch (arg[t].c = va_arg(ap, BoxContCateg)) {
    case BOXCONTCATEG_LREG:
    case BOXCONTCATEG_GREG:
    case BOXCONTCATEG_PTR:
      arg[t].t = TYPE_INT;
      arg[t].vi = vi = va_arg(ap, BoxInt);
      arg[t].ptr = (void *) (& arg[t].vi);
      break;

    case BOXCONTCATEG_IMM:
      switch (idesc->t_id) {
      case TYPE_CHAR:
        arg[t].t = TYPE_CHAR;
        arg[t].vi = va_arg(ap, BoxInt); vi = 0;
        arg[t].ptr = (void *) (& arg[t].vi);
        break;
      case TYPE_INT:
        arg[t].t = TYPE_INT;
        arg[t].vi = vi = va_arg(ap, BoxInt);
        arg[t].ptr = (void *) (& arg[t].vi);
        break;
      case TYPE_REAL:
        is_short = 0;
        arg[t].t = TYPE_REAL;
        arg[t].vr = va_arg(ap, BoxReal);
        arg[t].ptr = (void *) (& arg[t].vr);
        break;
      case TYPE_POINT:
        is_short = 0;
        arg[t].t = TYPE_POINT;
        arg[t].vp.x = va_arg(ap, BoxReal);
        arg[t].vp.y = va_arg(ap, BoxReal);
        arg[t].ptr = (void *) (& arg[t].vp);
        break;
      default:
        is_short = 0;
        break;
      }
      break;
        
    default:
      MSG_ERROR("Categoria di argomenti sconosciuta!");
      pt->target_proc->status.error = 1;
      pt->target_proc->status.inhibit = 1;
      break;
    }

    if (is_short) {
      /* Controllo che l'argomento possa essere "contenuto"
       * nel formato corto.
       */
      vi &= ~0x7fL;
      if (vi != 0 && vi != ~0x7fL)
        is_short = 0;
    }

    ++t;
  }

  assert(t == idesc->numargs);

  /* Cerco di capire se e' possibile scrivere l'istruzione in formato corto */
  if (vmp->attr.forcelong) is_short = 0;
  if (is_short == 1 && t <= 2) {
    /* L'istruzione va scritta in formato corto! */
    BoxVMWord buffer[1], *i_pos = buffer;
    BoxVMWord i_eye;
    BoxUInt atype;
    BoxArr *prog = & pt->target_proc->code;

    for (; t < 2; t++) {
      arg[t].c = 0;
      arg[t].vi = 0;
    }

    atype = (arg[1].c << 2) | arg[0].c;
    BOXVM_WRITE_SHORTOP_HEADER(i_pos, i_eye, instr, /* is_long = */ 0,
                               /* i_len = */ 1, atype);
    BOXVM_WRITE_SHORTOP_2ARGS(i_pos, i_eye, arg[0].vi, arg[1].vi);

    BoxArr_Push(prog, buffer);
    return;

  } else {
    /* L'istruzione va scritta in formato lungo! */
    BoxUInt idim, iheadpos;
    BoxVMWord iw[MAX_SIZE_IN_IWORDS];
    BoxArr *prog = & pt->target_proc->code;

    /* Lascio il posto per la "testa" dell'istruzione (non conoscendo ancora
     * la dimensione dell'istruzione, non posso scrivere adesso la testa.
     * Potro' farlo solo alla fine, dopo aver scritto tutti gli argomenti!)
     */
    iheadpos = BoxArr_Num_Items(prog) + 1;
    BoxArr_MPush(prog, NULL, idim = 2);

    for (i = 0; i < t; i++) {
      BoxUInt adim, aiwdim;

      adim = size_of_type[arg[i].t];
      aiwdim = (adim + sizeof(BoxVMWord) - 1) / sizeof(BoxVMWord);
      iw[aiwdim - 1] = 0;
      (void) memcpy( iw, arg[i].ptr, adim );

      BoxArr_MPush(prog, iw, aiwdim);
      idim += aiwdim;
    }

    {
      BoxVMWord *i_pos;
      BoxVMWord i_eye;
      BoxUInt atype;

      /* Trovo il puntatore alla testa dell'istruzione */
      i_pos = (BoxVMWord *) BoxArr_Item_Ptr(prog, iheadpos);

      for (; t < 2; t++)
        arg[t].c = 0;

      /* Scrivo la "testa" dell'istruzione */
      atype = (arg[1].c << 2) | arg[0].c;
      BOXVM_WRITE_LONGOP_HEADER(i_pos, i_eye, instr, /* is_long = */ 1,
                                /* i_len = */ idim, atype);
    }
  }
}

/** Assembla l'istruzione specificata da instr, scrivendo il codice
 * binario ad essa corrispondente nella destinazione specificata
 * dalla funzione VM_Asm_Out_Set().
 */
void BoxVM_Assemble(BoxVM *vm, BoxOp instr, ...) {
  va_list ap;
  va_start(ap, instr);
  BoxVM_VA_Assemble(vm, instr, ap);
  va_end(ap);
}

/****************************************************************************
 * Code to handle the DATA SEGMENT, the region of memory where values for   *
 * strings and other objects is put.                                        *
 ****************************************************************************/

typedef struct {
  BoxInt type;
  BoxInt size;
} DataItem;

/* This function adds a new piece of data to the data segment.
 * NOTE: It returns the address of the data item with respect to the beginning
 *  of the data segment.
 */
size_t BoxVM_Data_Add(BoxVM *vm, const void *data, size_t size, BoxInt type) {
  BoxInt addr;

  /* Now we insert the data descriptor (for debug mainly), if necessary */
  if (vm->attr.identdata) {
    DataItem di;
    di.type = type;
    di.size = size;
    BoxArr_MPush(& vm->data_segment, & di, sizeof(di));
  }

  /* And now we insert the piece of data */
  addr = BoxArr_Num_Items(& vm->data_segment);
  BoxArr_MPush(& vm->data_segment, data, size);
  return addr;
}

/* Make sure gr0 is pointing to the data segment */
static void My_Set_Data_Segment_Register(BoxVM *vm) {
  BoxPtr data_segment_ptr;
  data_segment_ptr.block = NULL; /* the VM will handle deallocation! */
  data_segment_ptr.ptr = BoxArr_First_Item_Ptr(& vm->data_segment);
  BoxVM_Module_Global_Set(vm, TYPE_OBJ, (BoxInt) 0, & data_segment_ptr);
}

void BoxVM_Data_Display(BoxVM *vm, FILE *stream) {
  char *data;
  BoxInt size, pos, ds;
  DataItem *di;

  size = BoxArr_Num_Items(& vm->data_segment);

  if (!vm->attr.identdata) {
    fprintf(stream, "*** DATA SEGMENT WITH SIZE "SUInt" ***\n", size);
    return;
  }

  data = (char *) BoxArr_First_Item_Ptr(& vm->data_segment);

  if (size < 1) {
    fprintf(stream, "*** EMPTY DATA-SEGMENT ***\n");
    return;
  }

  fprintf(stream, "*** CONTENT OF THE DATA-SEGMENT ***\n");

  pos = 0;
  while (pos + sizeof(DataItem) <= size) {
    di = (DataItem *) data;
    fprintf(stream, "  Address "SInt", size "SInt": data of type '"SInt"':\n",
            pos, di->size, di->type);

    ds = sizeof(DataItem) + di->size;
    pos += ds;
    if (di->size < 0 || pos > size) {
      fprintf(stream, "Error: bad data-block.\n");
      MSG_ERROR("Bad block size at position %d.", pos);
      return;
    }
    data += ds;
  }

  fprintf(stream, "*** END OF THE DATA-SEGMENT ***\n");
}

/* Clear the backtrace of the program. */
void BoxVM_Backtrace_Clear(BoxVM *vm) {
  BoxArr_Clear(& vm->backtrace);
}

/* Print on 'stream' a human redable representation of the backtrace
 * of the program.
 */
void BoxVM_Backtrace_Print(BoxVM *vm, FILE *stream) {
  BoxVMTrace *traces = BoxArr_First_Item_Ptr(& vm->backtrace);
  size_t n = BoxArr_Num_Items(& vm->backtrace);
  char *fail_msg;

  if (n == 0)
    fprintf(stream, "Empty traceback.\n");

  else {
    size_t i;

    fprintf(stream, "Traceback (innermost call comes last):\n");

    for (i = 0; i < n; i++) {
      BoxVMTrace *trace = & traces[n - 1 - i];
      BoxVMCallNum call_num = trace->call_num;
      BoxVMProcID proc_id = BoxVM_Proc_Get_ID(vm, call_num);
      if (proc_id != BOXVMPROCID_NONE) {
        size_t vm_pos = trace->vm_pos;
        char *desc = BoxVM_Proc_Get_Description(vm, call_num);
        BoxSrcPos *src_pos = BoxVM_Proc_Get_Source_Of(vm, proc_id, vm_pos);

        if (src_pos != NULL) {
          char *src_pos_str = BoxSrcPos_To_Str(src_pos);
          fprintf(stream, "  In %s at %s (VM address %ld).\n",
                  desc, src_pos_str, (long) vm_pos);
          BoxMem_Free(src_pos_str);

        } else
          fprintf(stream, "  In %s at %ld.\n", desc, (long) vm_pos);

        BoxMem_Free(desc);

      } else {
        fprintf(stream, "  In C code (ERROR?).\n");
      }
    }
  }

  fail_msg = BoxVMX_Get_Fail_Msg(vm->vmcur, BOXBOOL_FALSE);
  if (fail_msg)
    fprintf(stream, "Failure: %s\n", fail_msg);
}
