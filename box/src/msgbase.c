/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
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
#include "defaults.h"
#include "array.h"
#include "msgbase.h"

/* Inizializza il sistema di gestione dei messaggi.
 * ignore_level(0-17) specifica il livello di gravita' minimo richiesto
 * affinche' un messaggio sia considerato (in tutti gli altri casi verra'
 * ignorato, cioe' non verra' mostrato, anche se incrementera'
 * il corrispondente contatore dei messaggi, a cui si accede con Msg_Num()).
 * act_level(0-17) specifica il livello di gravita' minimo affinche' venga chiamata
 * la funzione di gestione degli errori (che viene impostata proprio
 * con Msg_Init, vedi fn, piu' avanti).
 * ctxt_always indica se il contesto deve essere mostrato sempre, anche se non
 * ci sono messaggi da visualizzare (ctxt_always = 1 --> visualizza sempre)
 * fn e' la funzione che viene chiamata se il livello di gravita' del messaggio
 * supera act_level.
 */

/** Initialization of the message module */
Task Msg_Init(MsgStack **ms_ptr, UInt num_levels, UInt show_level) {
  MsgStack *ms;
  UInt i;
  *ms_ptr = ms = (MsgStack *) malloc(sizeof(MsgStack));
  if (ms == (MsgStack *) NULL) return Failed;
  ms->show_level = show_level;
  ms->last_shown = 0;
  ms->filter = ms->default_filter = (MsgFilter) NULL;
  ms->flush = 0;
  ms->out = stderr;
  TASK( Arr_New(& ms->msgs, sizeof(Msg), MSG_TYPICAL_NUM_MSGS) );

  num_levels = num_levels > 0 ? num_levels : 1;
  ms->level = (UInt *) malloc(num_levels*sizeof(UInt));
  if (ms->level == (UInt *) NULL) return Failed;
  ms->num_levels = num_levels;
  for(i=0; i<num_levels; i++) ms->level[i] = 0;
  return Success;
}

#define EXIT_IF_NOT_INIT(ms) if (ms == (MsgStack *) NULL) return;

/** Finalization of the message module */
void Msg_Destroy(MsgStack *ms) {
  UInt i, n;
  EXIT_IF_NOT_INIT(ms);
  n = Arr_NumItem(ms->msgs);
  for(i=0; i<n; i++) free(Arr_ItemPtr(ms->msgs, Msg, i)->msg);
  Arr_Destroy(ms->msgs);
  free(ms->level);
  free(ms);
}

void Msg_Default_Filter_Set(MsgStack *ms, MsgFilter mf) {
  EXIT_IF_NOT_INIT(ms);
  ms->filter = ms->default_filter = mf;
}

void Msg_Show_Level_Set(MsgStack *ms, UInt show_level) {
  EXIT_IF_NOT_INIT(ms);
  ms->show_level = show_level;
}

/** Get the value of the specified message counter */
UInt Msg_Counter_Get(MsgStack *ms, UInt level) {
  if (ms == (MsgStack *) NULL) return 0;
  if (level >= 1 && level <= ms->num_levels) return ms->level[level-1];
  return 0;
}

/** Get the number of messages whose level is greater or equal than
 * the spefified one.
 */
UInt Msg_Counter_Sum_Get(MsgStack *ms, UInt level) {
  UInt i, sum=0;
  if (ms == (MsgStack *) NULL) return 0;
  if (level > ms->num_levels) return 0;
  for(i = level < 1 ? 0 : level-1; i<ms->num_levels; i++)
    sum += ms->level[i];
  return sum;
}

/** Clear one message counter */
void Msg_Counter_Clear(MsgStack *ms, UInt level) {
  EXIT_IF_NOT_INIT(ms);
  if (level >= 1 && level <= ms->num_levels) ms->level[level] = 0;
}

/** Clear all message counters */
void Msg_Counter_Clear_All(MsgStack *ms) {
  UInt i;
  EXIT_IF_NOT_INIT(ms);
  for(i=0; i<ms->num_levels; i++) ms->level[i] = 0;
}

/** Show the messages which have still not been shown */
void Msg_Show(MsgStack *ms) {
  UInt i, num_msgs;
  EXIT_IF_NOT_INIT(ms);
  num_msgs = Arr_NumItem(ms->msgs);
  for(i=ms->last_shown+1; i<=num_msgs; i++) {
    Msg *m = Arr_ItemPtr(ms->msgs, Msg, i);
    if (m->filter != (MsgFilter) NULL) m->msg = m->filter(m->level, m->msg);
    fputs(m->msg, ms->out);
    if (ms->flush) fflush(ms->out);
    free(m->msg);
    m->msg = (char *) NULL;
  }
  ms->last_shown = num_msgs;
}

void Msg_Context_Begin(MsgStack *ms, const char *msg, MsgFilter mf) {
  Msg m;
  EXIT_IF_NOT_INIT(ms);
  m.level = 0;
  m.msg = strdup(msg);
  m.filter = ms->filter = mf;
  (void) Arr_Push(ms->msgs, & m);
}

void Msg_Context_End(MsgStack *ms, UInt repeat) {
  UInt i;
  Msg *m;
  EXIT_IF_NOT_INIT(ms);
  for(i=0; i<repeat;) {
    if (Arr_NumItem(ms->msgs) < 1) return;
    m = Arr_LastItemPtr(ms->msgs, Msg);
    if (m->level == 0) ++i;
    free(m->msg);
    (void) Arr_Dec(ms->msgs);
  }
  i = Arr_NumItem(ms->msgs);
  if (i < ms->last_shown) ms->last_shown = i;
  if (i > 0) {
    m = Arr_LastItemPtr(ms->msgs, Msg);
    ms->filter = m->filter;
  } else {
    ms->filter = ms->default_filter;
  }
}

void Msg_Add(MsgStack *ms, UInt level, const char *msg) {
  Msg m;
  EXIT_IF_NOT_INIT(ms);
  if (level < 1 || level > ms->num_levels) return;
  ++ms->level[level-1];
  if (level < ms->show_level) return;
  m.level = level;
  m.msg = strdup(msg);
  m.filter = ms->filter;
  (void) Arr_Push(ms->msgs, & m);
  Msg_Show(ms);
}


#if 0
#include "print.h"

char *my_filter(UInt level, char *original_msg) {
  char *prefix="Fatal", *final_msg;
  if (level == 0) {
    final_msg = strdup(print("%s:\n", original_msg));
    free(original_msg);
    return final_msg;
  }
  switch(level) {
  case 1: prefix = "Note"; break;
  case 2: prefix = "Warning"; break;
  case 3: prefix = "Error"; break;
  }
  final_msg = strdup(print("%s: %s\n", prefix, original_msg));
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
