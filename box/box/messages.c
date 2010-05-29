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

/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "print.h"
#include "msgbase.h"
#include "messages.h"

MsgStack *msg_main_stack = (MsgStack *) NULL;

static Int line = -1;

static char *show_type_and_msg(UInt level, char *original_msg) {
  if (level == 0) {
    char *final_msg;
    final_msg = printdup("STAGE: %s:\n", original_msg);
    free(original_msg);
    return final_msg;

  } else {
    char *prefix="???", *final_msg;
    switch(level) {
    case MSG_LEVEL_ADVICE: prefix = "Note"; break;
    case MSG_LEVEL_WARNING: prefix = "Warning"; break;
    case MSG_LEVEL_ERROR: prefix = "Error"; break;
    case MSG_LEVEL_FATAL: prefix = "Fatal error"; break;
    }
    if (line > 0)
      final_msg = printdup("%s(line %I): %s\n", prefix, line, original_msg);
    else
      final_msg = printdup("%s: %s\n", prefix, original_msg);
    free(original_msg);
    return final_msg;
  }
}

Task Msg_Main_Init(UInt show_level) {
  Task t = Msg_Init(& msg_main_stack, 4, show_level);
  Msg_Default_Filter_Set(msg_main_stack, show_type_and_msg);
  return t;
}

void Msg_Main_Context_Begin(const char *msg) {
  Msg_Context_Begin(msg_main_stack, msg, show_type_and_msg);
}

void Msg_Line_Set(Int line_number) {
  if (line_number > 0)
    line = line_number;
  else
    line = -1;
}

static void default_fatal_handler(void) {
  /* assert(0); Useful for debugging! */
  abort();
  exit(EXIT_FAILURE);
}

static FatalHandler fatal_handler = default_fatal_handler;

/** Function used to set the fatal error message handler */
void Msg_Set_Fatal_Handler(FatalHandler fh) {
  if (fh == (FatalHandler) NULL)
    fatal_handler = default_fatal_handler;
  else
    fatal_handler = fh;
}

/** Used by MSG_FATAL to terminate the program */
void Msg_Call_Fatal_Handler(void) {
  fatal_handler();
}
