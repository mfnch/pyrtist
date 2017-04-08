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

/**
 * @file msgbase.h
 * @brief Centralized generic message handler.
 *
 * This file provides the declarations of all the procedures which
 * are required to setup and use the centralized message handler.
 * You can open and close message contexts and send messages
 * specifying a message level (which can be used to distinguish
 * between errors, warnings, etc.).
 * You can decide what is the minimum level a message should have
 * to be shown. You can install a message filter. For example
 * if you are writing a compiler and you want display the line
 * numbers where error occurrs, then you can therefore add a filter
 * which prints ("Error at line %d: %s\n", num_line, orig_msg)
 * for every original message orig_msg.
 * You can then send messages without specifying the line number:
 * the filter will do that for you.
 * Contexts are shown only if they contain messages.
 * But this behaviour can be tuned.
 */

#ifndef _MSGBASE_H
#  define _MSGBASE_H

#include <stdio.h>

#include <box/types.h>
#include <box/array.h>


/** Receives a string together with its deallocation responsibility,
 * returns a string together with its deallocation responsibility.
 * Therefore the following examples are perfectly right:
 *   const char *my_filter(BoxUInt, const char *original_msg) {
 *     return original_msg;
 *   }
 *   const char *my_filter(BoxUInt, const char *original_msg) {
 *     free(original_msg);
 *     return strdup("I hide the message! Ha ha!\n");
 *   }
 * NOTE: level is 0 for contexts, is between 1 and num_levels
 *  for messages.
 */
typedef char *(*MsgFilter)(BoxUInt level, char *original_msg);

typedef struct {
  BoxUInt level;
  MsgFilter filter;
  char *msg;
} Msg;

typedef struct {
  BoxUInt num_levels;
  BoxUInt show_level;
  BoxUInt last_shown;
  BoxUInt *level;
  MsgFilter filter;
  MsgFilter default_filter;
  BoxArr msgs;
  int flush;
  FILE *out;
} MsgStack;

BOX_BEGIN_DECLS

/** Initialization of the message module */
BoxTask Msg_Init(MsgStack **ms_ptr, BoxUInt num_levels, BoxUInt show_level);

/** Finalization of the message module */
void Msg_Destroy(MsgStack *ms);

/** Set the default filter to use */
void Msg_Default_Filter_Set(MsgStack *ms, MsgFilter mf);

/** Set the minimum level a message should have to be considered */
void Msg_Show_Level_Set(MsgStack *ms, BoxUInt show_level);

/** Get the value of the specified message counter */
BoxUInt Msg_Counter_Get(MsgStack *ms, BoxUInt level);

/** Get the number of messages whose level is greater or equal than
 * the spefified one.
 */
BoxUInt Msg_Counter_Sum_Get(MsgStack *ms, BoxUInt level);

/** Clear one message counter */
void Msg_Counter_Clear(MsgStack *ms, BoxUInt level);

/** Clear all message counters */
void Msg_Counter_Clear_All(MsgStack *ms);

/** Show the messages which have still not been shown */
void Msg_Show(MsgStack *ms);

void Msg_Context_Begin(MsgStack *ms, const char *msg, MsgFilter mf);

void Msg_Context_End(MsgStack *ms, BoxUInt repeat);

void Msg_Add(MsgStack *ms, BoxUInt level, const char *msg);

BOX_END_DECLS

#endif