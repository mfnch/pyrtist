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
#include "mem.h"
#include "typesys.h"
#include "virtmach.h"
#include "vmalloc.h"
#include "tsdesc.h"
#include "vmsymstuff.h"

typedef struct {
  BoxVMObjDesc desc;
  BoxArr       subs;
} MyObjDescBuilder;

static void My_Build_Struc_Desc(BoxTS *ts, BoxVM *vm,
                                MyObjDescBuilder *bldr, BoxType t) {
  BoxTSStrucIt it;
  /*int i = 0;*/
  for (BoxTSStrucIt_Init(ts, & it, t); it.has_more;
       BoxTSStrucIt_Advance(& it)) {
    BoxVMAllocID alloc_id = TS_Get_AllocID(ts, vm, it.member);
    /*printf("num=%d, pos=%d, id=%d\n", i++, (int) it.position, (int) alloc_id);*/
    if (alloc_id != BOXVMALLOCID_NONE) {
      BoxVMSubObj *sub = (BoxVMSubObj *) BoxArr_Push(& bldr->subs, NULL);
      sub->alloc_id = alloc_id;
      sub->position = it.position;
    }
  }
  BoxTSStrucIt_Finish(& si);
}

static void My_Build_Obj_Desc(BoxTS *ts, BoxVM *vm,
                              MyObjDescBuilder *bldr, BoxType t) {
  BoxType ct = TS_Get_Core_Type(ts, t);
  switch(TS_Get_Kind(ts, ct)) {
  case TS_KIND_STRUCTURE:
    My_Build_Struc_Desc(ts, vm, bldr, t);
    break;

  default:
    break;
  }
}

static BoxVMCallNum My_Find_Proc(TS *ts, BoxVM *vm,
                                 BoxType child, BoxType parent) {
  BoxType p =
    TS_Procedure_Search(ts, (BoxType *) NULL, child, parent,
                        TSSEARCHMODE_INHERITED);
  if (p == BOXTYPE_NONE)
    return BOXVMCALLNUM_NONE;

  else {
    BoxVMSymID sym_id = TS_Procedure_Get_Sym(ts, p);
    return BoxVMSym_Get_Call_Num(vm, sym_id);
  }
}

BoxVMObjDesc *TS_Get_ObjDesc(BoxTS *ts, BoxVM *vm, BoxType t) {
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

    /* Here we populate the procedures for the object */
    bldr.desc.initializer = My_Find_Proc(ts, vm, BOXTYPE_CREATE, t);
    bldr.desc.finalizer = My_Find_Proc(ts, vm, BOXTYPE_DESTROY, t);
    bldr.desc.copier = BOXVMCALLNUM_NONE;
    bldr.desc.mover = BOXVMCALLNUM_NONE;

    /* Here we actually build the list of subobjects */
    My_Build_Obj_Desc(ts, vm, & bldr, t);

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

BoxVMAllocID TS_Get_AllocID(BoxTS *ts, BoxVM *vm, BoxType t) {
  BoxVMObjDesc *od = TS_Get_ObjDesc(ts, vm, t);
  if (od == NULL)
    return BOXVMALLOCID_NONE;

  else {
    BoxVMAllocID id = BoxVMAllocID_From_ObjDesc(vm, & od);
    if (od == NULL)
      BoxVM_Set_Obj_Name(vm, id, TS_Name_Get(ts, t));
    else
      BoxMem_Free(od); // Deallocate object
    return id;
  }
}
