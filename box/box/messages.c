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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "mem.h"
#include "srcpos.h"
#include "print.h"
#include "msgbase.h"
#include "messages.h"

MsgStack *msg_main_stack = (MsgStack *) NULL;

static BoxSrc *my_src_of_err = NULL;
static const char *current_file_name = NULL;

static void My_Update_Context(BoxSrc *current) {
  if (current != NULL) {
    const char *new_file_name = current->begin.file_name;

    if (new_file_name != current_file_name) {
      if (current_file_name != NULL)
        Msg_Context_End(msg_main_stack, 1);

      if (new_file_name != NULL)
        Msg_Main_Context_Begin(Box_Print("File '%s'", new_file_name));

      current_file_name = new_file_name;
    }

  } else { /* current == NULL */
    if (current_file_name != NULL) {
      Msg_Context_End(msg_main_stack, 1);
      current_file_name = (const char *) NULL;
    }
  }
}

static char *My_Show_Msg(BoxUInt level, char *original_msg) {
  if (level == 0) {
    char *final_msg;
    final_msg = printdup("NOTE: %s:\n", original_msg);
    BoxMem_Free(original_msg);
    return final_msg;

  } else {
    char *prefix="???", *final_msg;
    switch(level) {
    case MSG_LEVEL_ADVICE: prefix = "Note"; break;
    case MSG_LEVEL_WARNING: prefix = "Warning"; break;
    case MSG_LEVEL_ERROR: prefix = "Error"; break;
    case MSG_LEVEL_FATAL: prefix = "Fatal error"; break;
    }

    if (my_src_of_err != NULL) {
      char *loc = BoxSrc_To_Str(my_src_of_err);
      final_msg = Box_SPrintF("%s(%~s): %s\n", prefix, loc, original_msg);

    } else
      final_msg = Box_SPrintF("%s: %s\n", prefix, original_msg);

    BoxMem_Free(original_msg);
    return final_msg;
  }
}

BoxTask Msg_Main_Init(BoxUInt show_level) {
  BoxTask t = Msg_Init(& msg_main_stack, 4, show_level);
  Msg_Default_Filter_Set(msg_main_stack, My_Show_Msg);
  return t;
}

void Msg_Main_Context_Begin(const char *msg) {
  Msg_Context_Begin(msg_main_stack, msg, My_Show_Msg);
}

BoxSrc *Msg_Set_Src(BoxSrc *src_of_err) {
  BoxSrc *previous = my_src_of_err;
  my_src_of_err = src_of_err;
  if (0)
    My_Update_Context(src_of_err);
  return previous;
}

static void My_Default_Fatal_Handler(void) {
  /* assert(0); Useful for debugging! */
  abort();
  exit(EXIT_FAILURE);
}

static FatalHandler fatal_handler = My_Default_Fatal_Handler;

/** Function used to set the fatal error message handler */
void Msg_Set_Fatal_Handler(FatalHandler fh) {
  if (fh == (FatalHandler) NULL)
    fatal_handler = My_Default_Fatal_Handler;
  else
    fatal_handler = fh;
}

/** Used by MSG_FATAL to terminate the program */
void Msg_Call_Fatal_Handler(void) {
  fatal_handler();
}
