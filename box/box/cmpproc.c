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

void CmpProc_VA_Assemble(CmpProc *p, AsmCode instr, va_list ap) {
  BoxVMProcID proc_id, previous_target;
  proc_id = CmpProc_Get_ProcID(p);
  previous_target = BoxVM_Proc_Target_Set(& p->cmp->vm, proc_id);
  VM_VA_Assemble(& p->cmp->vm, instr, ap);
  (void) BoxVM_Proc_Target_Set(& p->cmp->vm, previous_target);
}

void CmpProc_Assemble(CmpProc *p, AsmCode instr, ...) {
  va_list ap;
  va_start(ap, instr);
  CmpProc_VA_Assemble(p, instr, ap);
  va_end(ap);
}
