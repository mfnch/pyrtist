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
 * @file messages.h
 * @brief The message handler for the box copiler (wraps up msgbase)
 *
 * This file uses the msgbase module to provide a message handler
 * with all the features required for the Box compiler.
 * This module actually wraps up the msgbase one.
 */

#ifndef _MESSAGES_H
#  define _MESSAGES_H

#  include <box/types.h>
#  include <box/print.h>
#  include <box/msgbase.h>
#  include <box/srcpos.h>

extern MsgStack *msg_main_stack;

/** Initialize the main message handler */
BoxTask Msg_Main_Init(BoxUInt show_level);

#  define MSG_LEVEL_ADVICE 1
#  define MSG_LEVEL_WARNING 2
#  define MSG_LEVEL_ERROR 3
#  define MSG_LEVEL_FATAL 4
#  define MSG_LEVEL_MAX 4

/** INTERNAL (used throughout macros): Begin a context installing
 * an appropriate filter.
 */
void Msg_Main_Context_Begin(const char *msg);

/** Set the text in the source which may be the source for the next errors.
 * If the pointer is NULL the source will be considered as "unset".
 */
BoxSrc *Msg_Set_Src(BoxSrc *src_of_err);

#define MSG_UNDEF_LINE -1

/** Type of the function called when a fatal error message is reported */
typedef void (*FatalHandler)(void);

/** Function used to set the fatal error message handler */
void Msg_Set_Fatal_Handler(FatalHandler fh);

/** Used by MSG_FATAL to terminate the program */
void Msg_Call_Fatal_Handler(void);

/** Finalize the main message handler */
#  define Msg_Main_Destroy() Msg_Destroy(msg_main_stack)

/** Decide what is the minimum level a message should have to be processed
 * by the message handler
 */
#  define Msg_Main_Show_Level_Set(show_level) \
     Msg_Show_Level_Set(msg_main_stack, show_level)

/** Reset all the counters containing the number of received messages.
 */
#  define Msg_Main_Counter_Clear_All() \
     Msg_Counter_Clear_All(msg_main_stack)

/** Get the number of received messages for the specified level */
#  define Msg_Main_Counter_Get(level) \
     Msg_Main_Counter_Get(msg_main_stack, level)

/** Get the number of messages whose level is greater than the specified one
 */
#  define Msg_Main_Counter_Get_Sum(level) \
     Msg_Main_Counter_Get_Sum(msg_main_stack, level)

/****************************************************************************
 * MACROS TO OBTAIN THE NUMBER OF RECEIVED MESSAGES
 */

/** This macro returns the number of messages of kind 'advice' */
#  define MSG_NUM_ADVICES (Msg_Counter_Get(msg_main_stack, MSG_LEVEL_ADVICE))

/** This macro returns the number of messages of kind 'warning' */
#  define MSG_NUM_WARNINGS (Msg_Counter_Get(msg_main_stack,MSG_LEVEL_WARNING))

/** This macro returns the number of messages of kind 'error' */
#  define MSG_NUM_ERRORS (Msg_Counter_Get(msg_main_stack, MSG_LEVEL_ERROR))

/** This macro returns the number of messages of kind 'fatal' */
#  define MSG_NUM_FATALS (Msg_Counter_Get(msg_main_stack, MSG_LEVEL_FATAL))

/****************************************************************************
 * MACROS TO OBTAIN THE NUMBER OF RECEIVED MESSAGES WITH LEVEL GREATER
 * THAN THE ONE SPECIFIED
 */

/** This macro returns the number of messages whose level is
 * equal or greater than 'advice': all the messages!
 */
#  define MSG_GT_ADVICES \
     (Msg_Counter_Sum_Get(msg_main_stack, MSG_LEVEL_ADVICE))

/** This macro returns the number of messages whose level is
 * equal or greater than 'warning'.
 */
#  define MSG_GT_WARNINGS \
     (Msg_Counter_Sum_Get(msg_main_stack, MSG_LEVEL_WARNING))

/** This macro returns the number of messages whose level is
 * equal or greater than 'error'.
 */
#  define MSG_GT_ERRORS \
     (Msg_Counter_Sum_Get(msg_main_stack, MSG_LEVEL_ERROR))

/** This macro returns the number of messages whose level is
 * equal or greater than 'fatal'.
 */
#  define MSG_GT_FATALS \
     (Msg_Counter_Sum_Get(msg_main_stack, MSG_LEVEL_FATAL))

/****************************************************************************
 * MACROS USED TO BEGIN AND END CONTEXTS
 */
#  define MSG_CONTEXT_BEGIN(...) \
  Msg_Main_Context_Begin(Box_Print(__VA_ARGS__))

#  define MSG_CONTEXT_END() Msg_Context_End(msg_main_stack, 1)

/****************************************************************************
 * MACROS USED TO SIGNAL MESSAGES
 */

/** Macro to signal an information */
#  define MSG_ADVICE(...) \
     Msg_Add(msg_main_stack, MSG_LEVEL_ADVICE, Box_Print(__VA_ARGS__))

/** Macro used to signal a warning message */
#  define MSG_WARNING(...) \
     Msg_Add(msg_main_stack, MSG_LEVEL_WARNING, Box_Print(__VA_ARGS__))

/** Macro used to signal an error message */
#  define MSG_ERROR(...) \
     Msg_Add(msg_main_stack, MSG_LEVEL_ERROR, Box_Print(__VA_ARGS__))

/** Macro used to signal a fatal error message */
#  define MSG_FATAL(...) \
     do { \
       Msg_Add(msg_main_stack, MSG_LEVEL_FATAL, Box_Print(__VA_ARGS__)); \
       Msg_Call_Fatal_Handler(); \
     } while(1)

#endif
