/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

#include <ltdl.h>
#define DYLIB

#include "defaults.h"
#include "types.h"
#include "strutils.h"
#include "messages.h"
#include "mem.h"
#include "vm_priv.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "vmproc.h"
#include "list.h"

#ifdef DYLIB
static void My_Close_DyLib(void *item) {
  void *dylib = *((void **) item);
  if (lt_dlclose(dylib)) {
    MSG_WARNING("BoxVMSym_Destroy: My_Close_DyLib: lt_dlclose failure");
  }
}
#endif

void BoxVMSymTable_Init(BoxVMSymTable *st) {
  BoxHT_Init_Default(& st->syms, VMSYM_SYM_HT_SIZE);
  BoxArr_Init(& st->data, sizeof(BoxChar), VMSYM_DATA_ARR_SIZE);
  BoxArr_Init(& st->defs, sizeof(BoxVMSym), VMSYM_DEF_ARR_SIZE);
  BoxArr_Init(& st->refs, sizeof(BoxVMSymRef), VMSYM_REF_ARR_SIZE);
#ifdef DYLIB
  BoxArr_Init(& st->dylibs, sizeof(void *), VMSYM_DYLIB_ARR_SIZE);
  BoxArr_Set_Finalizer(& st->dylibs, My_Close_DyLib);
#endif

  if (lt_dlinit() != 0) {
    MSG_WARNING("BoxVMSym_Init: lt_dlinit failed!");
  }
}

void BoxVMSymTable_Finish(BoxVMSymTable *st) {
  BoxHT_Finish(& st->syms);
  BoxArr_Finish(& st->data);
  BoxArr_Finish(& st->defs);
  BoxArr_Finish(& st->refs);
#ifdef DYLIB
  BoxArr_Finish(& st->dylibs);
#endif
  if (lt_dlexit() != 0) {
    MSG_WARNING("BoxVMSym_Destroy: lt_dlexit failed!");
  }
}

BoxVMSymID BoxVMSym_New(BoxVM *vm, BoxUInt sym_type, BoxUInt def_size) {
  return BoxVMSym_Create(vm, sym_type, NULL, def_size);
}

/* Create a new symbol. */
BoxVMSymID
BoxVMSym_Create(BoxVM *vm, BoxUInt sym_type,
                const void *def, size_t def_size) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSymID sym_id;
  BoxVMSym *ss = BoxArr_Push(& st->defs, & ss);

  assert(ss);
  ss->name.length = 0;
  ss->name.text = NULL;
  ss->sym_type = sym_type;
  ss->defined = 0;
  ss->def_size = def_size;
  ss->def_addr = 1 + BoxArr_Num_Items(& st->data);
  ss->first_ref = 0;

  sym_id = BoxArr_Num_Items(& st->defs);
  BoxArr_MPush(& st->data, def, def_size);
  return sym_id;
}

/** Associate a name to the symbol sym_id. */
void BoxVMSym_Set_Name(BoxVM *vm, BoxVMSymID sym_id, const char *name) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxHTItem *hi;
  BoxVMSym *s;
  /*char *n_str;*/
  size_t name_len;

  s = (BoxVMSym *) BoxArr_Item_Ptr(& st->defs, sym_id);
  if (s->name.length != 0) {
    MSG_FATAL("This symbol has already been given a name!");
    assert(0);
  }

  name_len = strlen(name) + 1; /* include also the terminating '\0' char */
  if (BoxHT_Find(& st->syms, name, name_len, & hi)) {
    MSG_FATAL("Another symbol exists having the name '%s'!", name);
    assert(0);
  }

  (void) BoxHT_Insert_Obj(& st->syms, name, name_len,
                          & sym_id, sizeof(BoxUInt));
  if (!BoxHT_Find(& st->syms, name, name_len, & hi) ) {
    MSG_FATAL("Hashtable seems not to work (from BoxVMSym_Add)");
    assert(0);
  }

  s->name.text = (char *) hi->key;
  s->name.length = hi->key_size - 1; /* Without the final '\0' */
}

const char *BoxVMSym_Get_Name(BoxVM *vm, BoxVMSymID sym_id) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSym *s = (BoxVMSym *) BoxArr_Item_Ptr(& st->defs, sym_id);
  return s->name.text;
}

/* Get the definition data for a symbol. */
void *BoxVMSym_Get_Definition(BoxVM *vm, BoxVMSymID sym_id) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSym *s = (BoxVMSym *) BoxArr_Item_Ptr(& st->defs, sym_id);
  void *def_data_ptr = BoxArr_Item_Ptr(& st->data, s->def_addr);
  return def_data_ptr;
}

/* Define a symbol which was previously created with BoxVMSym_Create(). */
BoxTask
BoxVMSym_Define(BoxVM *vm, BoxVMSymID sym_id, void *def) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSym *s = BoxArr_Get_Item_Ptr(& st->defs, sym_id);

  if (s->defined) {
    const char *sym_name = BoxVMSym_Get_Name(vm, sym_id);
    MSG_ERROR("Double definition of the symbol '%s'.", sym_name);
    return BOXTASK_FAILURE;
  }

  if (def) {
    void *def_data_ptr = BoxArr_Item_Ptr(& st->data, s->def_addr);
    (void) memcpy(def_data_ptr, def, s->def_size);
  }

  s->defined = 1;
  return BOXTASK_OK;
}

int BoxVMSym_Is_Defined(BoxVM *vm, BoxVMSymID sym_id) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSym *s = (BoxVMSym *) BoxArr_Item_Ptr(& st->defs, sym_id);
  return s->defined;
}

void BoxVMSym_Ref(BoxVM *vm, BoxVMSymID sym_id, BoxVMSymResolver r,
                  void *ref, size_t ref_size, BoxVMSymStatus resolved) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSym *s;
  BoxVMSymRef sr;
  void *ref_data_ptr;

  assert(ref_size > 0 || ref == NULL);
  s = (BoxVMSym *) BoxArr_Item_Ptr(& st->defs, sym_id);
  sr.sym_id = sym_id;
  sr.next = s->first_ref;
  sr.ref_size = ref_size;
  sr.ref_addr = 1 + BoxArr_Num_Items(& st->data);
  switch(resolved) {
  case BOXVMSYM_RESOLVED: sr.resolved = 1; break;
  case BOXVMSYM_UNRESOLVED: sr.resolved = 0; break;
  default: sr.resolved = s->defined; break;
  }
  sr.resolver = r;

  /* Copy the data for the reference */
  if (ref_size > 0) {
    BoxArr_MPush(& st->data, NULL, ref_size);
    ref_data_ptr = BoxArr_Item_Ptr(& st->data, sr.ref_addr);
    (void) memcpy(ref_data_ptr, ref, ref_size);
  }

  /* Add the reference */
  BoxArr_Push(& st->refs, & sr);
  /* Link the reference to the list of references for the symbol */
  s->first_ref = BoxArr_Num_Items(& st->refs);
}

static int
My_Check_Ref(BoxUInt item_num, void *item, void *all_resolved) {
  BoxVMSymRef *sr = (BoxVMSymRef *) item;
  *((int *) all_resolved) &= sr->resolved;
  return 1;
}

void BoxVMSym_Ref_Check(BoxVM *vm, int *all_resolved) {
  BoxVMSymTable *st = & vm->sym_table;
  *all_resolved = 1;
  (void) BoxArr_Iter(& st->refs, My_Check_Ref, all_resolved);
}

static int
My_Report_Ref(BoxUInt item_num, void *item, void *pass_data) {
  BoxVM *vm = (BoxVM *) pass_data;
  BoxVMSymRef *sr = (BoxVMSymRef *) item;
  if (!sr->resolved) {
    MSG_ERROR("Unresolved reference to the symbol (ID=%d, name='%s')",
              sr->sym_id, BoxVMSym_Get_Name(vm, sr->sym_id));
  }
  return 1;
}

void BoxVMSym_Ref_Report(BoxVM *vm) {
  BoxVMSymTable *st = & vm->sym_table;
  (void) BoxArr_Iter(& st->refs, My_Report_Ref, vm);
}

BoxTask BoxVMSym_Resolve(BoxVM *vm, BoxVMSymID sym_id) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSym *s;
  BoxUInt next;
  BoxUInt sym_type, def_size, ref_size;
  void *def, *ref;

  if (sym_id < 1) {
    BoxUInt i, num_defs = BoxArr_Num_Items(& st->defs);
    for(i=1; i<=num_defs; i++) {
      BOXTASK( BoxVMSym_Resolve(vm, i) );
    }
    return BOXTASK_OK;
  }

  s = (BoxVMSym *) BoxArr_Item_Ptr(& st->defs, sym_id);
  if (! s->defined) return BOXTASK_OK;
  next = s->first_ref;
  def = BoxArr_Item_Ptr(& st->data, s->def_addr);
  def_size = s->def_size;
  sym_type = s->sym_type;
  while (next > 0) {
    BoxVMSymRef *sr = (BoxVMSymRef *) BoxArr_Item_Ptr(& st->refs, next);
    if (sr->sym_id != sym_id) {
      MSG_FATAL("BoxVMSym_Resolve: bad reference in the chain!");
      return BOXTASK_FAILURE;
    }

    if (!sr->resolved) {
      if (sr->resolver == NULL) {
        MSG_ERROR("BoxVMSym_Resolve: cannot resolve the symbol: "
                  "the resolver is not present!");
        return BOXTASK_FAILURE;
      }

      ref_size = sr->ref_size;
      ref = (ref_size > 0) ? BoxArr_Item_Ptr(& st->data, sr->ref_addr) : 0;

      BOXTASK( sr->resolver(vm, sym_id, sym_type, 1, def, def_size,
                            ref, ref_size) );
      sr->resolved = 1;
    }

    next = sr->next;
  }
  return BOXTASK_OK;
}

void BoxVMSym_Table_Print(BoxVM *vm, FILE *out, BoxVMSymID sym_id) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSym *s;
  BoxUInt next, ref_num;
  char *sym_name;

  if (sym_id < 1) {
    BoxUInt i, num_defs = BoxArr_Num_Items(& st->defs);
    fprintf(out, "The table contains "SUInt" symbols\n", num_defs);
    for(i=1; i<=num_defs; i++) {
      BoxVMSym_Table_Print(vm, out, i);
    }
    return;
  }

  s = (BoxVMSym  *) BoxArr_Item_Ptr(& st->defs, sym_id);
  next = s->first_ref;
  sym_name = (s->name.length > 0) ? s->name.text : "";
  ref_num = 1;

  fprintf(out,
   "Symbol ID = "SUInt"; name = '%s'; type = "SUInt"; "
   "defined = %d, def_addr = "SUInt", def_size = "SUInt"\n",
   sym_id, sym_name, s->sym_type,
   s->defined, s->def_addr, s->def_size);

  while(next > 0) {
    BoxVMSymRef *sr = (BoxVMSymRef *) BoxArr_Item_Ptr(& st->refs, next);
    if (sr->sym_id != sym_id) {
      fprintf(out, "Bad reference in the chain!");
      return;
    }

    fprintf(out,
            "  Reference number = "SUInt"; ref_addr = "SUInt"; "
            "ref_size = "SUInt"; resolved = %d, resolver = %p\n",
            ref_num, sr->ref_addr,
            sr->ref_size, sr->resolved, sr->resolver);

    next = sr->next;
    ++ref_num;
  }
}

BoxTask BoxVMSym_Check_Type(BoxVM *vm, BoxVMSymID sym_id, BoxUInt sym_type) {
  BoxVMSymTable *st = & vm->sym_table;
  BoxVMSym *s;
  s = (BoxVMSym *) BoxArr_Item_Ptr(& st->defs, sym_id);
  return (s->sym_type == sym_type) ? BOXTASK_OK : BOXTASK_FAILURE;
}

typedef struct {
  BoxVM      *vm;
  void       *dylib;
  const char *lib_file;
} MyCLibRefData;

#ifdef DYLIB
static int
My_Resolve_Ref_With_CLib(BoxVMSymID sym_id, void *item, void *pass_data) {
  BoxVMSym *s = (BoxVMSym *) item;
  if (!s->defined) {
    MyCLibRefData *clrd = pass_data;
    BoxVM *vm = clrd->vm;
    if (s->sym_type == BOXVMSYMTYPE_PROC) {
      const char *proc_name;
      if (BoxVMSym_Proc_Is_Implemented(vm, sym_id, & proc_name)) {
        (void) BoxVMSym_Define_Proc(vm, sym_id, NULL);

      } else {
        const char *err_msg;
        void *sym;

        err_msg = lt_dlerror();
        sym = lt_dlsym(clrd->dylib, proc_name);
        err_msg = lt_dlerror();

        if (err_msg)
          return 1;

        if (!sym) {
          MSG_ERROR("Symbol '%s' from library '%s' is NULL",
                    proc_name, clrd->lib_file);
          return 1;
        }

        (void) BoxVMSym_Define_Proc(vm, sym_id, (BoxCCallOld) sym);
      }
    }
  }
  return 1;
}
#endif

BoxTask BoxVMSym_Resolve_CLib(BoxVM *vm, const char *lib_file) {
#ifdef DYLIB
  BoxVMSymTable *st = & vm->sym_table;
  MyCLibRefData clrd;
  clrd.vm = vm;
  clrd.lib_file = lib_file;
  clrd.dylib = lt_dlopenext(lib_file);
  if (clrd.dylib == NULL)
    return BOXTASK_FAILURE;
  BoxArr_Push(& st->dylibs, & clrd.dylib);
  (void) BoxArr_Iter(& st->defs, My_Resolve_Ref_With_CLib, & clrd);
  return BOXTASK_OK;
#else
  MSG_WARNING("Cannot load '%s': the virtual machine was compiled "
              "without support for dynamic loading of libraries.", lib_file);
  return BOXTASK_FAILURE;
#endif
}

struct clibs_data {
  BoxVM *vm;
  BoxList *lib_paths;
  char *path;
  char *lib;
};

static BoxTask
My_Iter_Over_Paths(void *string, void *pass_data) {
  struct clibs_data *cld = (struct clibs_data *) pass_data;
  char *lib_file;
  BoxTask status;
  cld->path = (char *) string;
  lib_file = Box_Mem_Strdup(Box_Print("%s/lib%s", cld->path, cld->lib));
  status = BoxVMSym_Resolve_CLib(cld->vm, lib_file);
  Box_Mem_Free(lib_file);
  if (status == BOXTASK_OK) return BOXTASK_FAILURE; /* Stop here, if we have found it! */
  return BOXTASK_OK;
}

static BoxTask
My_Iter_Over_Libs(void *string, void *pass_data) {
  struct clibs_data *cld = (struct clibs_data *) pass_data;
  cld->lib = (char *) string;
  /* IS_SUCCESS is misleading: here we use BOXTASK_OK, just to continue
   * the iteration. Therefore if we get BOXTASK_OK, it means that the library
   * has not been found
   */
  if (BoxList_Iter(cld->lib_paths, My_Iter_Over_Paths, cld) == BOXTASK_OK) {
    MSG_WARNING("'%s' <-- library has not been found or cannot be loaded!",
                cld->lib);
  }
  return BOXTASK_OK;
}

static int
My_Resolve_Own_Symbol(BoxVMSymID sym_id, void *item, void *pass) {
  BoxVM *vm = pass;
  BoxVMSym *s = (BoxVMSym *) item;
  if (!s->defined) {
    if (s->sym_type == BOXVMSYMTYPE_PROC) {
      const char *proc_name;
      if (BoxVMSym_Proc_Is_Implemented(vm, sym_id, & proc_name))
        (void) BoxVMSym_Define_Proc(vm, sym_id, NULL);

      else {
      }
    }
  }
  return 1;
}

BoxTask My_Resolve_Own_Symbols(BoxVM *vm) {
  BoxVMSymTable *st = & vm->sym_table;
  (void) BoxArr_Iter(& st->defs, My_Resolve_Own_Symbol, vm);
  return BOXTASK_OK;
}

BoxTask BoxVMSym_Resolve_CLibs(BoxVM *vm, BoxList *lib_paths, BoxList *libs) {
  struct clibs_data cld;
  (void) My_Resolve_Own_Symbols(vm);
  cld.vm = vm;
  cld.lib_paths = lib_paths;
  return BoxList_Iter(libs, My_Iter_Over_Libs, & cld);
}
