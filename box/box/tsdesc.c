/***************************************************************************
 *   Copyright (C) 2006-2011 by Matteo Franchin (fnch@libero.it)           *
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
#include "combs.h"
#include "mem.h"
#include "typesys.h"
#include "vm_private.h"
#include "vmalloc.h"
#include "tsdesc.h"
#include "vmsymstuff.h"
#include "operator.h"
#include "messages.h"

typedef struct {
  BoxVMObjDesc desc;
  BoxArr       subs;
} MyObjDescBuilder;

static void My_Build_Struc_Desc(BoxCmp *c,
                                MyObjDescBuilder *bldr, BoxType t) {
  BoxTS *ts = & c->ts;
  BoxTSStrucIt it;
  /*int i = 0;*/
  for (BoxTSStrucIt_Init(ts, & it, t); it.has_more;
       BoxTSStrucIt_Advance(& it)) {
    BoxVMAllocID alloc_id = TS_Get_AllocID(c, it.member);
    /*printf("num=%d, pos=%d, id=%d\n", i++, (int) it.position, (int) alloc_id);*/
    if (alloc_id != BOXVMALLOCID_NONE) {
      BoxVMSubObj *sub = (BoxVMSubObj *) BoxArr_Push(& bldr->subs, NULL);
      sub->alloc_id = alloc_id;
      sub->position = it.position;
    }
  }
  BoxTSStrucIt_Finish(& si);
}

static BoxVMCallNum My_Find_Proc(BoxCmp *c, BoxType child,
                                 BoxCombType comb, BoxType parent) {
  BoxVM *vm = c->vm;
  BoxTS *ts = & c->ts;

  BoxType p_old =
    BoxTS_Procedure_Search(ts, (BoxType *) NULL,
                           child, comb, parent,
                           TSSEARCHMODE_INHERITED);

  BoxXXXX *child_new = BoxType_From_Id(ts, child);
  BoxXXXX *parent_new = BoxType_From_Id(ts, parent);
  BoxTypeCmp expand_info;
  BoxCallable *cb;
  BoxVMCallNum old_callnum, new_callnum = 0;
  BoxXXXX *p = BoxType_Find_Combination(parent_new, comb, child_new,
                                        & expand_info);
  if (p && BoxType_Get_Combination_Info(p, NULL, & cb)) {
    assert(expand_info >= BOXTYPECMP_EQUAL);
    if (!BoxCallable_Get_VM_CallNum(cb, vm, & new_callnum))
      MSG_ERROR("Callable '%~s' is not registered",
                TS_Name_Get(ts, p_old));
  } else if (p_old != BOXTYPE_NONE)
    MSG_ERROR("Callable '%~s' not found", TS_Name_Get(ts, p_old));

  if (p_old == BOXTYPE_NONE)
    return BOXVMCALLNUM_NONE;

  else {
    BoxVMSymID sym_id = TS_Procedure_Get_Sym(ts, p_old);
    old_callnum = BoxVMSym_Get_Call_Num(vm, sym_id);
    assert(new_callnum == 0 || new_callnum == old_callnum);
    return old_callnum;
  }
}

#if 0
static BoxVMCallNum My_Find_Copier(BoxCmp *c, BoxType parent) {
  OprMatch match;
  Operation *opn = BoxCmp_Operator_Find_Opn(c, & c->convert, & match,
                                            parent, BOXTYPE_NONE, parent);
  if (opn == NULL)
    return BOXVMCALLNUM_NONE;

  else {
    assert(opn->asm_scheme == OPASMSCHEME_USR_UN);
    BoxVMSymID sym_id = opn->implem.sym_id;
    return BoxVMSym_Get_Call_Num(c->vm, sym_id);
  }
}
#endif

static void My_Build_Obj_Desc(BoxCmp *c, MyObjDescBuilder *bldr, BoxType t) {
  BoxType ct = TS_Get_Core_Type(& c->ts, t);
  TSKind tk = TS_Get_Kind(& c->ts, ct);

  /* Here we populate the procedures for the object */
  if (tk == TS_KIND_SUBTYPE) {
    bldr->desc.initializer = c->bltin.subtype_init;
    bldr->desc.finalizer = c->bltin.subtype_finish;
    bldr->desc.copier = BOXVMCALLNUM_NONE;
    bldr->desc.mover = BOXVMCALLNUM_NONE;
    return;

  } else {
    bldr->desc.initializer =
      My_Find_Proc(c, BOXTYPE_CREATE, BOXCOMBTYPE_AT, t);
    bldr->desc.finalizer =
      My_Find_Proc(c, BOXTYPE_DESTROY, BOXCOMBTYPE_AT, t);
    bldr->desc.copier = My_Find_Proc(c, t, BOXCOMBTYPE_COPY, t);
    bldr->desc.mover = My_Find_Proc(c, t, BOXCOMBTYPE_MOVE, t);
  }

  switch(tk) {
  case TS_KIND_STRUCTURE:
    My_Build_Struc_Desc(c, bldr, t);
    break;

  default:
    break;
  }
}

BoxVMObjDesc *TS_Get_ObjDesc(BoxCmp *c, BoxType t) {
  BoxTS *ts = & c->ts;
  if (TS_Is_Empty(ts, t) || TS_Is_Fast(ts, t))
    /* An empty (size == 0) or fast (Int, Real, ...) object does not need
     * an object descriptor!
     */
    return (BoxVMObjDesc *) NULL;

  else {
    MyObjDescBuilder bldr;
    BoxVMObjDesc *desc = NULL;

    /* The following is very important as the object will then be hashed */
    (void) memset(& bldr.desc, 0, sizeof(BoxVMObjDesc));

    /* Create the obj-desc-builder */
    bldr.desc.has.initializer = 0;
    bldr.desc.has.finalizer = 0;
    bldr.desc.has.copier = 0;
    bldr.desc.has.mover = 0;
    BoxArr_Init(& bldr.subs, sizeof(BoxVMSubObj), 16);

    /* Here we actually build the list of subobjects */
    My_Build_Obj_Desc(c, & bldr, t);

    /* The following is necessary in order for BoxVMObjDesc_Is_Empty to work
     * properly!
     */
    bldr.desc.num_subs = BoxArr_Num_Items(& bldr.subs);

    if (!BoxVMObjDesc_Is_Empty(& bldr.desc)) {
      /* Create the object description from the builder */
      size_t subs_data_size = sizeof(BoxVMSubObj)*bldr.desc.num_subs;
      desc = BoxMem_Safe_Alloc(sizeof(BoxVMObjDesc) + subs_data_size);

      (void) memcpy(desc, & bldr.desc, sizeof(BoxVMObjDesc));
      if (subs_data_size > 0)
        (void) memcpy((void *) desc + sizeof(BoxVMObjDesc),
                      BoxArr_First_Item_Ptr(& bldr.subs), subs_data_size);

      desc->size = TS_Get_Size(ts, t);
    }

    /* Destroy the builder and return */
    BoxArr_Finish(& bldr.subs);
    return desc;
  }
}

BoxVMAllocID TS_Get_AllocID(BoxCmp *c, BoxType t) {
  BoxVMObjDesc *od = TS_Get_ObjDesc(c, t);
  if (od == NULL)
    return BOXVMALLOCID_NONE;

  else {
    BoxVMAllocID id = BoxVMAllocID_From_ObjDesc(c->vm, & od);
    if (od == NULL)
      BoxVM_Set_Obj_Name(c->vm, id, TS_Name_Get(& c->ts, t));
    else
      BoxMem_Free(od); // Deallocate object
    return id;
  }
}
