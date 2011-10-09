/****************************************************************************
 * Copyright (C) 2011 by Matteo Franchin                                    *
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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <box/types.h>
#include <box/mem.h>
#include <box/str.h>

#include "graphic.h"
#include "cmd.h"

BoxGObjKind BoxGCmdArgKind_To_Obj_Kind(BoxGCmdArgKind t) {
  switch (t) {
  case BOXGCMDARGKIND_INT: return BOXGOBJKIND_INT;
  case BOXGCMDARGKIND_REAL: return BOXGOBJKIND_REAL;
  case BOXGCMDARGKIND_STR: return BOXGOBJKIND_STR;
  case BOXGCMDARGKIND_POINT:
  case BOXGCMDARGKIND_VECTOR:
  case BOXGCMDARGKIND_REALP:
    return BOXGOBJKIND_POINT;
  case BOXGCMDARGKIND_WIDTH: return BOXGOBJKIND_REAL;
  default: return BOXGOBJKIND_EMPTY;
  }
}

BoxGCmdSig BoxGCmdSig_Get(BoxGCmd cmd_id) {
  static struct cmd_signature_s {
    BoxGCmd    cmd_id;    /**< Command ID (one of BOXGCMD_*). */
    BoxGCmdSig signature; /**< Command signature. */

  } cmd_signatures[] = {
    {BOXGCMD_SAVE, BOXGCMDSIG_NONE},
    {BOXGCMD_RESTORE, BOXGCMDSIG_NONE},
    {BOXGCMD_SET_ANTIALIAS, BOXGCMDSIG_I},
    {BOXGCMD_MOVE_TO, BOXGCMDSIG_P},
    {BOXGCMD_LINE_TO, BOXGCMDSIG_P},
    {BOXGCMD_CURVE_TO, BOXGCMDSIG_PPP},
    {BOXGCMD_CLOSE_PATH, BOXGCMDSIG_NONE},
    {BOXGCMD_NEW_PATH, BOXGCMDSIG_NONE},
    {BOXGCMD_NEW_SUB_PATH, BOXGCMDSIG_NONE},
    {BOXGCMD_STROKE, BOXGCMDSIG_NONE},
    {BOXGCMD_STROKE_PRESERVE, BOXGCMDSIG_NONE},
    {BOXGCMD_FILL, BOXGCMDSIG_NONE},
    {BOXGCMD_FILL_PRESERVE, BOXGCMDSIG_NONE},
    {BOXGCMD_CLIP, BOXGCMDSIG_NONE},
    {BOXGCMD_CLIP_PRESERVE, BOXGCMDSIG_NONE},
    {BOXGCMD_RESET_CLIP, BOXGCMDSIG_NONE},
    {BOXGCMD_PUSH_GROUP, BOXGCMDSIG_NONE},
    {BOXGCMD_POP_GROUP_TO_SOURCE, BOXGCMDSIG_NONE},
    {BOXGCMD_SET_OPERATOR, BOXGCMDSIG_I},
    {BOXGCMD_PAINT, BOXGCMDSIG_NONE},
    {BOXGCMD_PAINT_WITH_ALPHA, BOXGCMDSIG_R},
    {BOXGCMD_COPY_PAGE, BOXGCMDSIG_NONE},
    {BOXGCMD_SHOW_PAGE, BOXGCMDSIG_NONE},
    {BOXGCMD_SET_LINE_WIDTH, BOXGCMDSIG_W},
    {BOXGCMD_SET_LINE_CAP, BOXGCMDSIG_I},
    {BOXGCMD_SET_LINE_JOIN, BOXGCMDSIG_I},
    {BOXGCMD_SET_MITER_LIMIT, BOXGCMDSIG_R},
    {BOXGCMD_SET_DASH, BOXGCMDSIG_W_},
    {BOXGCMD_SET_FILL_RULE, BOXGCMDSIG_I},
    {BOXGCMD_SET_SOURCE_RGBA, BOXGCMDSIG_RRRR},
    {BOXGCMD_TEXT_PATH, BOXGCMDSIG_S},
    {BOXGCMD_TRANSLATE, BOXGCMDSIG_V},
    {BOXGCMD_SCALE, BOXGCMDSIG_V},
    {BOXGCMD_ROTATE, BOXGCMDSIG_R},
    {BOXGCMD_PATTERN_CREATE_RGB, BOXGCMDSIG_RRR},
    {BOXGCMD_PATTERN_CREATE_RGBA, BOXGCMDSIG_RRRR},
    {BOXGCMD_PATTERN_CREATE_LINEAR, BOXGCMDSIG_PP},
    {BOXGCMD_PATTERN_CREATE_RADIAL, BOXGCMDSIG_PPPPRR},
    {BOXGCMD_PATTERN_ADD_COLOR_STOP_RGB, BOXGCMDSIG_RRRR},
    {BOXGCMD_PATTERN_ADD_COLOR_STOP_RGBA, BOXGCMDSIG_RRRRR},
    {BOXGCMD_PATTERN_SET_EXTEND, BOXGCMDSIG_I},
    {BOXGCMD_PATTERN_SET_FILTER, BOXGCMDSIG_I},
    {BOXGCMD_PATTERN_DESTROY, BOXGCMDSIG_NONE},
    {BOXGCMD_SET_SOURCE, BOXGCMDSIG_NONE},
    {BOXGCMD_EXT_JOINARC_TO, BOXGCMDSIG_PPP},
    {BOXGCMD_EXT_ARC_TO, BOXGCMDSIG_PPPRR},
    {BOXGCMD_EXT_SET_FONT, BOXGCMDSIG_S},
    {BOXGCMD_EXT_TEXT_PATH, BOXGCMDSIG_PPPpS},
    {BOXGCMD_EXT_TRANSFORM, BOXGCMDSIG_PPP},
    {BOXGCMD_EXT_FILL_STROKE, BOXGCMDSIG_NONE}
  };

  assert(cmd_id >= 0 && cmd_id < BOXG_NUM_CMDS);
  assert(cmd_signatures[cmd_id].cmd_id == cmd_id);
  return cmd_signatures[cmd_id].signature;
}

int BoxGCmdSig_Is_Variadic(BoxGCmdSig sig) {
  switch (sig) {
  case BOXGCMDSIG_W_:
    return 1;
  default:
    return 0;
  }
}

int BoxGCmdSig_Get_Arg_Kinds(BoxGCmdSig sig, BoxGCmdArgKind *kinds) {
  switch (sig) {
  case BOXGCMDSIG_NONE: return 0;
  case BOXGCMDSIG_I: kinds[0] = BOXGCMDARGKIND_INT; return 1;
  case BOXGCMDSIG_W: kinds[0] = BOXGCMDARGKIND_WIDTH; return 1;
  case BOXGCMDSIG_R: kinds[0] = BOXGCMDARGKIND_REAL; return 1;
  case BOXGCMDSIG_P: kinds[0] = BOXGCMDARGKIND_POINT; return 1;
  case BOXGCMDSIG_V: kinds[0] = BOXGCMDARGKIND_VECTOR; return 1;
  case BOXGCMDSIG_S: kinds[0] = BOXGCMDARGKIND_STR; return 1;
  case BOXGCMDSIG_PP: kinds[0] = kinds[1] = BOXGCMDARGKIND_POINT; return 2;
  case BOXGCMDSIG_RRR:
    kinds[0] = kinds[1] = kinds[2] = BOXGCMDARGKIND_REAL;
    return 3;
  case BOXGCMDSIG_PPP:
    kinds[0] = kinds[1] = kinds[2] = BOXGCMDARGKIND_POINT;
    return 3;
  case BOXGCMDSIG_RRRR:
    kinds[0] = kinds[1] = kinds[2] = kinds[3] = BOXGCMDARGKIND_REAL;
    return 4;
  case BOXGCMDSIG_RRRRR:
    kinds[0] = kinds[1] = kinds[2] = kinds[3] = kinds[4] = BOXGCMDARGKIND_REAL;
    return 5;
  case BOXGCMDSIG_PPPRR:
    kinds[0] = kinds[1] = kinds[2] = BOXGCMDARGKIND_POINT;
    kinds[3] = kinds[4] = BOXGCMDARGKIND_REAL;
    return 5;
  case BOXGCMDSIG_PPPpS:
    kinds[0] = kinds[1] = kinds[2] = BOXGCMDARGKIND_POINT;
    kinds[3] = BOXGCMDARGKIND_REALP;
    kinds[4] = BOXGCMDARGKIND_STR;
    return 5;
  case BOXGCMDSIG_PPPPRR:
    kinds[0] = kinds[1] = kinds[2] = kinds[3] = BOXGCMDARGKIND_POINT;
    kinds[4] = kinds[5] = BOXGCMDARGKIND_REAL;
    return 6;
  case BOXGCMDSIG_W_:
    kinds[0] = BOXGCMDARGKIND_WIDTH;
    return 1;
  }

  abort();
  return -1;
}

int BoxGCmdSig_Get_Num_Args(BoxGCmdSig sig) {
  BoxGCmdArgKind kinds[BOXG_MAX_NUM_CMD_ARGS];
  int n = BoxGCmdSig_Get_Arg_Kinds(sig, kinds);
  assert(n <= BOXG_MAX_NUM_CMD_ARGS);
  return n;
}

/* MyIterStuff provides a convenient way to handle variadic commands.  If a
 * command require many arguments, then MyIterStuff_Alloc_Args automatically
 * allocates extra spaces for them.
 */
typedef struct {
  /* Stuff to pass to the user-provided iterator function */
  BoxGCmdIter    iter;
  void           *pass;

  /* Array of arguments kinds, pointers and values. */
  size_t         size;
  BoxGCmdArgKind *kinds, kinds_small[BOXG_MAX_NUM_CMD_ARGS];
  void           **ptrs, *ptrs_small[BOXG_MAX_NUM_CMD_ARGS];
  BoxGCmdArg     *values, values_small[BOXG_MAX_NUM_CMD_ARGS];

} MyIterStuff;

static void MyIterStuff_Init(MyIterStuff *stuff) {
  stuff->size = 0;
  stuff->kinds = stuff->kinds_small;
  stuff->ptrs = stuff->ptrs_small;
  stuff->values = stuff->values_small;
}

static void MyIterStuff_Finish(MyIterStuff *stuff) {
  if (stuff->size > 0) {
    BoxMem_Free(stuff->kinds);
    BoxMem_Free(stuff->ptrs);
    BoxMem_Free(stuff->values);
  }
}

static void MyIterStuff_Alloc_Args(MyIterStuff *stuff, size_t num_args) {
  if (num_args > BOXG_MAX_NUM_CMD_ARGS) {
    if (stuff->size < num_args) {
      if (stuff->size > 0)
        MyIterStuff_Finish(stuff);

      stuff->kinds  = BoxMem_Alloc(num_args*sizeof(BoxGCmdArgKind));
      stuff->ptrs   = BoxMem_Alloc(num_args*sizeof(void *));
      stuff->values = BoxMem_Alloc(num_args*sizeof(BoxGCmdArg));
      stuff->size = num_args;
    }
  }
}
                               
static BoxGErr My_Iter_One_Cmd(MyIterStuff *stuff, BoxGObj *args_obj) {
  size_t args_obj_len = BoxGObj_Get_Length(args_obj);
  BoxGCmdIter iter = stuff->iter;
  void *pass = stuff->pass;

  if (args_obj_len >= 1) {
    BoxInt *first_item =
      (BoxInt *) BoxGObj_To(BoxGObj_Get(args_obj, 0), BOXGOBJKIND_INT);
    if (first_item != NULL) {
      BoxGCmd cmd = *first_item;
      BoxGCmdSig sig = BoxGCmdSig_Get(cmd);
      int num_args_wanted = BoxGCmdSig_Get_Arg_Kinds(sig, stuff->kinds),
          num_args = args_obj_len - 1;

      assert(num_args_wanted <= BOXG_MAX_NUM_CMD_ARGS);

      if (num_args >= num_args_wanted) {
        size_t i;

        /* Check for variadic commands */
        if (num_args > num_args_wanted) {
          if (BoxGCmdSig_Is_Variadic(sig)) {
            assert(num_args_wanted >= 1);
            /* ^^^ variadic commands must have at least one argument, as the
             *     variadic arguments inherit the kind of the last non-variadic
             *     argument.
             */

            MyIterStuff_Alloc_Args(stuff, num_args);
            (void) BoxGCmdSig_Get_Arg_Kinds(sig, stuff->kinds);

          } else
            return BOXGERR_CMD_UNEXPECTED_ARGS;
        }

        int required_kind = -1;
        for (i = 0; i < num_args; i++) {
          BoxGObj *arg_obj = BoxGObj_Get(args_obj, i + 1);
          assert(arg_obj != NULL);

          /* Variadic arguments have the same kind of the last required arg */
          if (i < num_args_wanted)
            required_kind = stuff->kinds[i];
          else
            stuff->kinds[i] = required_kind;

          void *val =
            BoxGObj_To(arg_obj, BoxGCmdArgKind_To_Obj_Kind(required_kind));

          if (val != NULL)
            stuff->ptrs[i] = val;   
          else
            return BOXGERR_CMD_BAD_ARGS;
        }

        if (iter(cmd, sig, num_args, stuff->kinds,
                 stuff->ptrs, stuff->values, pass) == BOXTASK_OK)
          return BOXGERR_NO_ERR;
        else
          return BOXGERR_CMD_EXEC;

      } else
        return BOXGERR_CMD_MISSING_ARGS;

    } else
      return BOXGERR_CMD_BAD_INDEX;

  } else
    return BOXGERR_CMD_EMPTY;
}


BoxTask BoxGCmdIter_Iter(BoxGCmdIter iter, BoxGObj *obj, void *pass) {
  size_t i, n = BoxGObj_Get_Length(obj);
  MyIterStuff stuff;
  BoxGErr err = BOXGERR_NO_ERR;

  MyIterStuff_Init(& stuff);
  stuff.iter = iter;
  stuff.pass = pass;

  for (i = 0; i < n && err == BOXGERR_NO_ERR; i++) {
    BoxInt sub_type = BoxGObj_Get_Type(obj, i);
    if (sub_type == BOXGOBJKIND_COMPOSITE) {
      BoxGObj *obj_sub = BoxGObj_Get(obj, i);
      err = My_Iter_One_Cmd(& stuff, obj_sub);

    } else
      err = BOXGERR_CMD_BAD;
  }

  MyIterStuff_Finish(& stuff);

  if (err != BOXGERR_NO_ERR) {
    printf("Error in command Obj: %s.\n", BoxGErr_To_Str(err));
    return BOXTASK_FAILURE;

  } else
    return BOXTASK_OK;
}

/** Type of the 'pass' argument used by BoxGCmdIter_Filter_Append. */
typedef struct {
  BoxGCmdIter filter;
  void        *pass;
  BoxGObj     *dst;

} MyFilterData;

static BoxTask My_Filter_Append_Iter(BoxGCmd cmd, BoxGCmdSig sig, int num_args,
                                     BoxGCmdArgKind *kinds, void **args,
                                     BoxGCmdArg *aux, void *pass) {
  MyFilterData *data = (MyFilterData *) pass;
  BoxTask t = data->filter(cmd, sig, num_args, kinds, args, aux, data->pass);

  if (t == BOXTASK_OK) {
    BoxGObj *cmd_obj = BoxGObj_Append_Composite(data->dst, num_args + 1);
    BoxInt cmd_int = (BoxInt) cmd;
    size_t i;

    BoxGObj_Append_C_Value(cmd_obj, BOXGOBJKIND_INT, & cmd_int);
    for (i = 0; i < num_args; i++) {
      BoxGObjKind arg_kind =  BoxGCmdArgKind_To_Obj_Kind(kinds[i]);
      void *arg_data = args[i];
      BoxGObj_Append_C_Value(cmd_obj, arg_kind, arg_data);
    }

    return BOXTASK_OK;

  } else
    return BOXTASK_FAILURE;
}

BoxTask BoxGCmdIter_Filter_Append(BoxGCmdIter filter,
                                  BoxGObj *dst, BoxGObj *src, void *pass) {
  MyFilterData data;
  data.filter = filter;
  data.pass = pass;
  data.dst = dst;
  return BoxGCmdIter_Iter(My_Filter_Append_Iter, src, & data);
}


#if 0

#include <box/virtmach.h>

BoxTask My_Dummy_Iter(BoxGCmd cmd, BoxGCmdSig sig, int num_args,
                      BoxGCmdArgKind *kinds, void **args, void *pass) {

  printf("Command: %d; num_args=%d\n", cmd, num_args);
  return BOXTASK_OK;
}

BoxTask GLib_Obj_At_CmdIter(BoxVM *vm) {
  //void *obj_data = BOX_VM_THIS_PTR(vm, void);
  BoxGObjPtr gobj = BOX_VM_ARG(vm, BoxGObjPtr);

  BoxGCmdIter_Iter(My_Dummy_Iter, gobj, NULL);
  return BOXTASK_OK;
}
#endif
