/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin (fnch@libero.it)                *
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

#include <assert.h>
#include <stdlib.h>

#include "types.h"
#include "messages.h"
#include "typesys.h"
#include "virtmach.h"
#include "cmpproc.h"
#include "compiler.h"
#include "value.h"
#include "autogen.h"
#include "vmsymstuff.h"

/* Autogeneration for structures */
static int My_Autogen_Struc_Proc(BoxCmp *c, BoxType t_child, BoxType t_parent) {
  size_t initial_proc_size;
  ValueStrucIter vsi;
  Value v_struc;

  Value_Init(& v_struc, c->cur_proc);
  Value_Setup_As_Parent(& v_struc, t_parent);
  initial_proc_size = CmpProc_Get_Code_Size(c->cur_proc);
  for(ValueStrucIter_Init(& vsi, & v_struc, c->cur_proc);
      vsi.has_next; ValueStrucIter_Do_Next(& vsi)) {
    Value v_child;
    Value_Init(& v_child, c->cur_proc);
    Value_Setup_As_Type(& v_child, t_child);
    (void) Value_Emit_Call_Or_Blacklist(& vsi.v_member, & v_child);
  }
  ValueStrucIter_Finish(& vsi);

  return (CmpProc_Get_Code_Size(c->cur_proc) > initial_proc_size);
}

int Auto_Generate_Code(BoxCmp *c, BoxType t_child, BoxType t_parent) {
  TS *ts = & c->ts;
  BoxType ct_parent = TS_Get_Core_Type(ts, t_parent);

  assert(TS_Is_Special(t_child) != BOXTYPE_NONE);

  switch(TS_Get_Kind(ts, ct_parent)) {
  case TS_KIND_STRUCTURE:
    return My_Autogen_Struc_Proc(c, t_child, t_parent);
    break;

  default:
    return 0;
    break;
  }
}

/* Here we assume that the procedure child@parent is not registered! */
BoxType Auto_Generate_Procedure(BoxCmp *c, BoxType t_child,
				BoxType t_parent) {
  TS *ts = & c->ts;
  BoxType t_proc = BOXTYPE_NONE;
  int autogenerated = 0;

  if (TS_Is_Special(t_child) == BOXTYPE_NONE)
    return t_proc;

  else {
    CmpProc *save_cur_proc = c->cur_proc;
    c->cur_proc = CmpProc_New(c, CMPPROCSTYLE_SUB);

    autogenerated = Auto_Generate_Code(c, t_child, t_parent);
 
    /* If the procedure is not empty, we register it */
    if (autogenerated) {
      /* Create the procedure type and obtain its name */
      t_proc = TS_Procedure_New(ts, t_parent, t_child, 3);
      char *proc_name = TS_Name_Get(& c->ts, t_proc);
      CmpProc_Set_Name(c->cur_proc, proc_name);
      BoxMem_Free(proc_name);

      /* Define the symbol associated to the procedure */
      BoxVMSymID sym_id = CmpProc_Get_Sym(c->cur_proc);
      BoxVMCallNum call_num = CmpProc_Get_Call_Num(c->cur_proc);
      ASSERT_TASK( BoxVMSym_Def_Call(c->vm, sym_id, call_num) );

      /* Register the procedure in the type system */
      TS_Procedure_Register(ts, t_proc, sym_id);

    } else {
      /* Here we should destroy the code, since the procedure is not
       * going to be installed)
       *
       * BUT WE ARE NOT DOING THIS, YET!
       */
      CmpProc_End(c->cur_proc);
      /* ^^^ this is a temporary fix, to avoid problems with sym resolution
       *     (the procedure header needs to be defined...)
       * XXX We should later do something about that and avoid all this...
       */
    }

    CmpProc_Destroy(c->cur_proc);
    c->cur_proc = save_cur_proc;
    return t_proc;
  }
}
