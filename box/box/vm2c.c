/****************************************************************************
 * Copyright (C) 2013 by Matteo Franchin                                    *
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

/**
 * @file vm2c.h
 * @brief Convert Box VM code to C.
 */

#include <assert.h>

#include <box/vm2c.h>

#include <box/vm_priv.h>
#include <box/vmdasm_priv.h>

/**
 * @brief 
 */
typedef struct {
  FILE *out;
} MyTranslatorData;

/**
 * @brief 
 */
#define MY_MAX_ARG_LENGTH (64)


/**
 * @brief 
 */
static void 
My_Arg_To_Str(char *out, size_t out_size,
              int arg_format, BoxTypeId args_type, BoxInt arg_value) {
  BoxInt arg_abs_value = abs(arg_value);
  char reg_char = "vr"[arg_value >= 0],
       type_char = "cirpo"[args_type];
  switch(arg_format) {
  case BOXCONTCATEG_GREG:
    sprintf(out, "g%c%c" SInt, reg_char, type_char, arg_abs_value);
    break;
  case BOXCONTCATEG_LREG:
    sprintf(out, "%c%c" SInt, reg_char, type_char, arg_abs_value);
    break;
  case BOXCONTCATEG_PTR:
    {
      char sgn_char = "+-"[arg_value < 0];
      const char *types[] = {"BoxChar", "BoxInt", "BoxReal", "BoxPoint",
                           "BoxPtr"};
      sprintf(out, "(*(%s *) (ro0.ptr %c " SInt "))",
              types[args_type], sgn_char, arg_abs_value);
      break;
    }
  case BOXCONTCATEG_IMM:
    if (args_type == BOXTYPEID_CHAR)
      arg_value = (BoxInt) ((BoxChar) arg_value);
    sprintf(out, SInt, arg_value);
    break;
  default:
    abort();
  }
}

/**
 * @brief Translate one single instruction to C code.
 */
static BoxTask
My_Translate_Op(BoxVMDasm *dasm, void *pass) {
  MyTranslatorData *data = (MyTranslatorData *) pass;
  FILE *out = data->out;
  BoxOp *op = & dasm->op;
  char arg_buf[BOX_OP_MAX_NUM_ARGS + 1][MY_MAX_ARG_LENGTH];
  int num_args, num_written_bufs;

  /* Get the number of arguments. */
  num_args = (dasm->op_desc) ? dasm->op_desc->num_args : 0;

  /* Now we convert the arguments to string. */
  num_written_bufs = 0;
  if (num_args > 0) {
    BoxTypeId t = dasm->op_desc->t_id;
    int i;

    assert(num_args <= BOX_OP_MAX_NUM_ARGS);
    
    for (i = 0; i < num_args; i++)
      My_Arg_To_Str(arg_buf[i], MY_MAX_ARG_LENGTH,
                    (op->args_forms >> (2*i)) & 0x3, t, op->args[i]);

    //if (op->has_data)
    //  My_Data_To_Str(arg_buf[i++], arg_buf_size, t, op->data);

    num_written_bufs = i;
  }

#if 0
  {
    int i;
    char *sep = " ";

    for (i = 0; i < num_written_bufs; i++, sep = ", ")
      fprintf(out, "%s%s", sep, arg_buf[i]);
  }
#endif

  switch (op->id) {
  case BOXOP_NEWI_II:
    //fprintf();
    break;

  case BOXOP_MOV_CC:
  case BOXOP_MOV_II:
  case BOXOP_MOV_RR:
  case BOXOP_MOV_PP:
    fprintf(out, "  %s = %s;\n", arg_buf[0], arg_buf[1]);
    break;

  case BOXOP_RET:
    fprintf(out, "  return;\n");
    break;

  case BOXOP_ADD_II:
  case BOXOP_ADD_RR:
    fprintf(out, "  %s += %s;\n", arg_buf[0], arg_buf[1]);
    break;
  case BOXOP_ADD_PP:
    fprintf(out, "  %s.x += %s.x;\n", arg_buf[0], arg_buf[1]);
    fprintf(out, "  %s.y += %s.y;\n", arg_buf[0], arg_buf[1]);
    break;
  case BOXOP_SUB_II:
  case BOXOP_SUB_RR:
    fprintf(out, "  %s -= %s;\n", arg_buf[0], arg_buf[1]);
    break;
  case BOXOP_SUB_PP:
    fprintf(out, "  %s.x -= %s.x;\n", arg_buf[0], arg_buf[1]);
    fprintf(out, "  %s.y -= %s.y;\n", arg_buf[0], arg_buf[1]);
    break;
  case BOXOP_MUL_II:
  case BOXOP_MUL_RR:
    fprintf(out, "  %s *= %s;\n", arg_buf[0], arg_buf[1]);
    break;
  case BOXOP_DIV_II:
  case BOXOP_DIV_RR:
    fprintf(out, "  %s /= %s;\n", arg_buf[0], arg_buf[1]);
    break;
#if 0
  case BOXOP_REM_II:
    fprintf(out, "  %s /= %s;\n", arg_buf[0], arg_buf[1]);
    break;
#endif
  case BOXOP_NEG_I:
  case BOXOP_NEG_R:
    fprintf(out, "  %s = -%s;\n", arg_buf[0]);
    break;
  case BOXOP_NEG_P:
    fprintf(out, "  %s.x = -%s.x;\n", arg_buf[0], arg_buf[0]);
    fprintf(out, "  %s.y = -%s.y;\n", arg_buf[0], arg_buf[0]);
    break;
  case BOXOP_PMULR_P:
    fprintf(out, "  %s.x *= rr0;\n", arg_buf[0]);
    fprintf(out, "  %s.y *= rr0;\n", arg_buf[0]);
    break;
  case BOXOP_PDIVR_P:
    fprintf(out, "  %s.x /= rr0;\n", arg_buf[0]);
    fprintf(out, "  %s.y /= rr0;\n", arg_buf[0]);
    break;

  default:
    fprintf(out, "  /* ERROR: op-id=%d (%s) */\n",
            (int) op->id, (dasm->op_desc) ? dasm->op_desc->name : "unknown");
    break;
  }

  return BOXTASK_OK;
}

/* Convert the VM object to C source code. */
BoxBool
BoxVM_Export_To_C_Code(BoxVM *vm, FILE *out) {
  MyTranslatorData data;
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMCallNum call_num, n = BoxArr_Get_Num_Items(& pt->installed);
  const char *fn_sep = "";

  /* Prepare the translator datastructures. */
  data.out = out;

#if 0

  BoxUInt n, proc_id;
  char *s;

  BoxVM_Data_Display(vmp, out);
  fprintf(out, "\n");

  fprintf(out, "*** OBJECT DESCRIPTOR TABLE ***\n");
  s = BoxVM_Get_Installed_Types_Desc(vmp);
  fprintf(out, "%s\n", s);
  Box_Mem_Free(s);

  fprintf(out, "*** END OF OBJECT DESCRIPTOR TABLE ***\n\n");

#endif

  for (call_num = 1; call_num <= n; call_num++) {
    BoxVMProcInstalled *p = BoxArr_Item_Ptr(& pt->installed, call_num);

    fputs(fn_sep, out);
    fn_sep = "\n";

    if (p) {
      fprintf(out, "/* Name: %s, Description: %s */\n",
              (p->desc) ? p->desc : "(none)", (p->name) ? p->name : "(none)");

      if (p->type == BOXVMPROCKIND_VM_CODE) {
        BoxVMProcID proc_id = p->code.proc_id;
        BoxVMWord *ptr;
        BoxUInt length;

        /* Get the pointer to the procedure code and the length. */
        BoxVM_Proc_Get_Ptr_And_Length(vm, & ptr, & length, proc_id);

        fprintf(out, "static void fn%d(void) {\n", (int) call_num);

        if (BoxVM_Disassemble_Block(vm, ptr, length, My_Translate_Op, & data)
            != BOXTASK_OK)
          return BOXBOOL_FALSE;
        fprintf(out, "}\n");
      }
    }

  }

  return BOXBOOL_TRUE;
}
