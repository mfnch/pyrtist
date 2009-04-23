/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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

#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "mem.h"
#include "messages.h"
#include "virtmach.h"
#include "vmsym.h"
#include "vmproc.h"
#include "vmsymstuff.h"
#include "typesys.h"

#include "compiler.h"
#include "cmpproc.h"

void CmpProc_Init(CmpProc *p, BoxCmp *c) {
  p->cmp = c;
  p->have.sym = 0;
  p->have.proc_id = 0;
  p->have.proc_name = 0;
  p->have.call_num = 0;
}

void CmpProc_Finish(CmpProc *p) {
  if (p->have.proc_name)
    BoxMem_Free(p->proc_name);
}

CmpProc *CmpProc_New(BoxCmp *c) {
  CmpProc *p = BoxMem_Alloc(sizeof(CmpProc));
  if (p == NULL) return NULL;
  CmpProc_Init(p, c);
  return p;
}

void CmpProc_Destroy(CmpProc *p) {
  CmpProc_Finish(p);
  BoxMem_Free(p);
}

BoxVMSymID CmpProc_Get_Sym(CmpProc *p) {
  if (p->have.sym)
    return p->sym;

  else {
    p->have.sym = 1;
    return VM_Sym_New_Call(cmp->vm);
  }
}

BoxVMProcID CmpProc_Get_ProcID(CmpProc *p) {
  if (p->have.proc_id)
    return p->proc_id;

  else {
    p->have.proc_id = 1;
    ASSERT_TASK( VM_Proc_Code_New(& p->cmp->vm, & p->proc_id) );
    return p->proc_id;
  }
}

char *CmpProc_Get_Proc_Desc(CmpProc *p) {
  /* The description is just a help string to make the ASM more readable
   * (may disappear in the future). For now it is:
   *  - the type of the procedure, if this is known;
   *  - the name of the procedure, if this is known;
   *  - "|unknown|"
   */
  if (p->have.type)
    return TS_Name_Get(& p->cmp->ts, p->type);

  else
    return BoxMem_Strdup((p->have.proc_name) ? p->proc_name : "|unknown|");
}

BoxVMCallNum CmpProc_Get_Call_Num(CmpProc *p) {
  if (p->have.call_num)
    return p->sym;

  else {
    BoxVMProcID pn = CmpProc_Get_ProcID(p);
    char *proc_desc = CmpProc_Get_Proc_Desc(p),
         *proc_name = (p->have.proc_name) ? p->proc_name : "(noname)";
    VM_Proc_Install_Code(& p->cmp->vm, & p->call_num, pn,
                         proc_name, proc_desc);
    BoxMem_Free(proc_desc);
    p->have.call_num = 1;
    return p->call_num;
  }
}

void CmpProc_Raw_VA_Assemble(CmpProc *p, BoxOpcode op, va_list ap) {
  BoxVMProcID proc_id, previous_target;
  proc_id = CmpProc_Get_ProcID(p);
  previous_target = BoxVM_Proc_Target_Set(& p->cmp->vm, proc_id);
  VM_VA_Assemble(& p->cmp->vm, op, ap);
  (void) BoxVM_Proc_Target_Set(& p->cmp->vm, previous_target);
}

void CmpProc_Raw_Assemble(CmpProc *p, BoxOpcode op, ...) {
  va_list ap;
  va_start(ap, op);
  CmpProc_Raw_VA_Assemble(p, op, ap);
  va_end(ap);
}

static void My_Prepare_Ptr_Access(CmpProc *p, const BoxCont *c) {
  Int ptr_reg = c->value.ptr.reg;
  if (c->categ == CAT_PTR && ptr_reg != 0) {
    Int addr_categ = c->value.ptr.is_greg ? CAT_GREG : CAT_LREG;
    CmpProc_Raw_Assemble(p, ASM_MOV_OO, CAT_LREG, (Int) 0,
                         addr_categ, ptr_reg);
  }
}

void CmpProc_VA_Assemble(CmpProc *p, BoxGOp g_op, int num_args, va_list ap) {
  const BoxCont *args[BOXOP_MAX_NUM_ARGS];
  BoxOpInfo *oi;
  int i;

  if (num_args > BOXOP_MAX_NUM_ARGS) {
    MSG_FATAL("CmpProc_Assemble: the given number of arguments is too high.");
    assert(0);
  }

  for(i = 0; i < num_args; i++)
    args[i] = va_arg(ap, BoxCont *);

  /* Search for operations whose argument number and type match */
  oi = BoxVM_Get_Op_Info(& p->cmp->vm, g_op);
  for(; oi != NULL; oi = oi->next) {
    if (oi->num_args == num_args) {
      int i;
      for(i = 0; i < num_args: i++) {
        BoxOpReg *reg = & oi->regs[i];
        if (1) break;
      }

      if (i >= num_args) {


      }
    }
  }

  if (oi == NULL) {
    MSG_FATAL("CmpProc_Assemble: cannot find a matching operation.");
    assert(0);
  }

#if 0
  const BoxCont *arg1=NULL, *arg2=NULL;
  int num_args = BoxOp_Get_Num_Args(op); /* Returns -1 if op is invalid */
  BoxType arg_type = BoxOp_Get_Arg_Type(op);

  assert(num_args >= 0 && num_args <= 2); /* This is an assumption
                                             this code makes */

  /* gets the two containers */
  if (num_args >= 1) {
    arg1 = va_arg(ap, BoxCont *);
    assert(arg1->type == arg_type);
  }

  if (num_args >= 2) {
    arg2 = va_arg(ap, BoxCont *);
    assert(arg2->type == arg_type);
  }

  if (num_args == 1) {
    My_Prepare_Ptr_Access(p, arg1);
    switch(arg1->categ) {
    case BOXCONTCATEG_GREG:
    case BOXCONTCATEG_LREG:
      CmpProc_Raw_Assemble(p, op, arg1->categ, arg1->value.reg);
      return;
    case BOXCONTCATEG_PTR:
      CmpProc_Raw_Assemble(p, op, arg1->categ, arg1->value.ptr.offset);
      return;
    case BOXCONTCATEG_IMM:
      assert(0);
    }

  } else if (num_args == 2) {
    assert(0);
  }
#endif
}

#if 0
for X = c, i, r, p
(example mov i[ro5+16], i[ro6+8])

mov ro0, ro6
mov rX0, X[ro0 + 8]
mov ro0, ro5
op X[ro0+16], rX0

for X = o
(example mov o[ro5+16], o[ro6+32])

mov ro0, ro6
mov ro1, o[ro0+32]
#endif

void CmpProc_Assemble(CmpProc *p, BoxGOp g_op, int num_args, ...) {
  va_list ap;
  va_start(ap, num_args);
  CmpProc_VA_Assemble(p, g_op, num_args, ap);
  va_end(ap);
}
