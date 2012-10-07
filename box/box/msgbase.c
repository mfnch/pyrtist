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

/* messages.c - january 2007
 *
 * This file defines the message handler for the Box compiler.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "types.h"
#include "mem.h"
#include "defaults.h"
#include "array.h"
#include "msgbase.h"
#include "print.h"

/** Initialization of the message module */
BoxTask Msg_Init(MsgStack **ms_ptr, BoxUInt num_levels, BoxUInt show_level) {
  MsgStack *ms;
  BoxUInt i;
  *ms_ptr = ms = (MsgStack *) malloc(sizeof(MsgStack));
  if (ms == (MsgStack *) NULL) return BOXTASK_FAILURE;
  ms->show_level = show_level;
  ms->last_shown = 0;
  ms->filter = ms->default_filter = (MsgFilter) NULL;
  ms->flush = 0;
  ms->out = stderr;
  BoxArr_Init(& ms->msgs, sizeof(Msg), MSG_TYPICAL_NUM_MSGS);

  num_levels = num_levels > 0 ? num_levels : 1;
  ms->level = (BoxUInt *) malloc(num_levels*sizeof(BoxUInt));
  if (ms->level == (BoxUInt *) NULL) return BOXTASK_FAILURE;
  ms->num_levels = num_levels;
  for(i=0; i<num_levels; i++) ms->level[i] = 0;
  return BOXTASK_OK;
}

#define EXIT_IF_NOT_INIT(ms) if (ms == (MsgStack *) NULL) return;

/** Finalization of the message module */
void Msg_Destroy(MsgStack *ms) {
  BoxUInt i, n;
  EXIT_IF_NOT_INIT(ms);
  n = BoxArr_Num_Items(& ms->msgs);
  for(i=1; i<=n; i++) {
    Msg *msg = (Msg *) BoxArr_Item_Ptr(& ms->msgs, i);
    free(msg->msg);
  }
  BoxArr_Finish(& ms->msgs);
  free(ms->level);
  free(ms);
  Print_Finalize();
}

void Msg_Default_Filter_Set(MsgStack *ms, MsgFilter mf) {
  EXIT_IF_NOT_INIT(ms);
  ms->filter = ms->default_filter = mf;
}

void Msg_Show_Level_Set(MsgStack *ms, BoxUInt show_level) {
  EXIT_IF_NOT_INIT(ms);
  ms->show_level = show_level;
}

/** Get the value of the specified message counter */
BoxUInt Msg_Counter_Get(MsgStack *ms, BoxUInt level) {
  if (ms == (MsgStack *) NULL) return 0;
  if (level >= 1 && level <= ms->num_levels) return ms->level[level-1];
  return 0;
}

/** Get the number of messages whose level is greater or equal than
 * the spefified one.
 */
BoxUInt Msg_Counter_Sum_Get(MsgStack *ms, BoxUInt level) {
  BoxUInt i, sum=0;
  if (ms == (MsgStack *) NULL) return 0;
  if (level > ms->num_levels) return 0;
  for(i = level < 1 ? 0 : level-1; i<ms->num_levels; i++)
    sum += ms->level[i];
  return sum;
}

/** Clear one message counter */
void Msg_Counter_Clear(MsgStack *ms, BoxUInt level) {
  EXIT_IF_NOT_INIT(ms);
  if (level >= 1 && level <= ms->num_levels) ms->level[level] = 0;
}

/** Clear all message counters */
void Msg_Counter_Clear_All(MsgStack *ms) {
  BoxUInt i;
  EXIT_IF_NOT_INIT(ms);
  for(i=0; i<ms->num_levels; i++) ms->level[i] = 0;
}

static void Msg_Clean(MsgStack *ms) {
  BoxUInt i, num_msgs;
  EXIT_IF_NOT_INIT(ms);
  num_msgs = BoxArr_Num_Items(& ms->msgs);
  for(i=num_msgs; i>0; i--) {
    Msg *m = (Msg *) BoxArr_Item_Ptr(& ms->msgs, i);
    if (m->level == 0 || m->msg != (char *) NULL) {
      if (i < num_msgs) break;
      return;
    }
  }
  BoxArr_MPop(& ms->msgs, NULL, num_msgs - i);
}

/** Show the messages which have still not been shown */
void Msg_Show(MsgStack *ms) {
  BoxUInt i, num_msgs;
  EXIT_IF_NOT_INIT(ms);
  num_msgs = BoxArr_Num_Items(& ms->msgs);
  for(i=ms->last_shown+1; i<=num_msgs; i++) {
    Msg *m = BoxArr_Item_Ptr(& ms->msgs, i);
    if (m->filter != (MsgFilter) NULL) m->msg = m->filter(m->level, m->msg);
    fputs(m->msg, ms->out);
    if (ms->flush) fflush(ms->out);
    free(m->msg);
    m->msg = (char *) NULL;
  }
  Msg_Clean(ms);
  ms->last_shown = BoxArr_Num_Items(& ms->msgs);
}

void Msg_Context_Begin(MsgStack *ms, const char *msg, MsgFilter mf) {
  Msg m;
  EXIT_IF_NOT_INIT(ms);
  m.level = 0;
  m.msg = BoxMem_Strdup(msg);
  m.filter = ms->filter = mf;
  BoxArr_Push(& ms->msgs, & m);
}

void Msg_Context_End(MsgStack *ms, BoxUInt repeat) {
  BoxUInt i;
  Msg *m;
  EXIT_IF_NOT_INIT(ms);
  for(i=0; i<repeat;) {
    if (BoxArr_Num_Items(& ms->msgs) < 1) return;
    m = (Msg *) BoxArr_Last_Item_Ptr(& ms->msgs);
    if (m->level == 0) ++i;
    free(m->msg);
    BoxArr_Pop(& ms->msgs, NULL);
  }
  i = BoxArr_Num_Items(& ms->msgs);
  if (i < ms->last_shown) ms->last_shown = i;
  if (i > 0) {
    m = (Msg *) BoxArr_Last_Item_Ptr(& ms->msgs);
    ms->filter = m->filter;
  } else {
    ms->filter = ms->default_filter;
  }
}

void Msg_Add(MsgStack *ms, BoxUInt level, const char *msg) {
  Msg m;
  EXIT_IF_NOT_INIT(ms);
  if (level < 1 || level > ms->num_levels) return;
  ++ms->level[level-1];
  if (level < ms->show_level) return;
  m.level = level;
  m.msg = BoxMem_Strdup(msg);
  m.filter = ms->filter;
  BoxArr_Push(& ms->msgs, & m);
  Msg_Show(ms);
}


#if 0
#include "print.h"

char *my_filter(BoxUInt level, char *original_msg) {
  char *prefix="Fatal", *final_msg;
  if (level == 0) {
    final_msg = BoxMem_Strdup(print("%s:\n", original_msg));
    free(original_msg);
    return final_msg;
  }
  switch(level) {
  case 1: prefix = "Note"; break;
  case 2: prefix = "Warning"; break;
  case 3: prefix = "Error"; break;
  }
  final_msg = BoxMem_Strdup(print("%s: %s\n", prefix, original_msg));
  free(original_msg);
  return final_msg;
}

int main(void) {
  MsgStack *ms;
  (void) _Msg_Init(& ms, 4, 2);
  Msg_Context_Begin(ms, "First context", my_filter);
  Msg_Add(ms, 1, "My worst fatal error message!");
  Msg_Add(ms, 1, "My worst error message!");
  Msg_Add(ms, 1, "My worst warning message!");
  Msg_Add(ms, 1, "My worst hello message!");
  Msg_Context_End(ms, 1);
  Msg_Context_Begin(ms, "Second context", my_filter);
  Msg_Add(ms, 4, "My preferite fatal error message!");
  Msg_Add(ms, 3, "My preferite error message!");
  Msg_Add(ms, 2, "My preferite warning message!");
  Msg_Add(ms, 1, "My preferite hello message!");
  Msg_Context_End(ms, 1);
  Msg_Destroy(ms);
  return 0;
}
#endif
