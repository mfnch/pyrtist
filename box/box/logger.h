/****************************************************************************
 * Copyright (C) 2014 by Matteo Franchin                                    *
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
 * @file logger.h
 * @brief Logging functionality for the Box compiler.
 */
#ifndef _BOX_LOGGER_H
#  define _BOX_LOGGER_H

#  include <stdarg.h>
#  include <stdint.h>

#  include <box/core.h>


/**
 * @brief The position inside a text file (file name, line and column).
 */
typedef struct {
  const char *file_name;
  uint32_t    line,
              col;
} BoxLogPos;

/**
 * @brief Logger object.
 */
typedef struct BoxLogger_struct BoxLogger;

/**
 * @brief Callback function to intercept messages sent to a logger.
 *
 * The user can provide its own version of this function to intercept log
 * messages before they are processed and sent to the destination. The
 * function can return true to indicate that the message does not need to be
 * further processed.
 */
typedef BoxBool (*BoxLoggerLogFn)(BoxLogger *logger, BoxLogPos *begin,
				  BoxLogPos *end, BoxLogLevel level,
				  const char *fmt, va_list ap,
				  void *data);

/**
 * @brief Callback function which receives the final string.
 *
 * The user can provide its own version of this function to redirect log
 * messages to another destination.
 */
typedef void (*BoxLoggerPutsFn)(BoxLogger *logger, const char *s, void *data);

/**
 * @brief Create a #BoxLogger object.
 *
 * @param log_fn Log message interceptor function (@c NULL for none).
 * @param puts_fn Output function (@c NULL to use the default, puts()).
 * @param data Pointer to be passed to @p log_fn and @p puts_fn.
 * @return The newly created logger or @c NULL in case of failure. 
 */
BOXEXPORT BoxLogger *
BoxLogger_Create(BoxLoggerLogFn log_fn, BoxLoggerPutsFn puts_fn, void *data);

/**
 * @brief Destroy a #BoxLogger created with BoxLogger_Create().
 */
BOXEXPORT void
BoxLogger_Destroy(BoxLogger *logger);

/**
 * @brief Var-Arg version of BoxLogger_Log().
 */
BOXEXPORT void
BoxLogger_Log_VA(BoxLogger *logger, BoxLogPos *begin, BoxLogPos *end,
                 BoxLogLevel lev, const char *fmt, va_list ap);

/**
 * @brief Generic logging function for the Box compiler.
 *
 * The one below is the most primitive error reporting function in the
 * compiler. The idea is that the user can intercept this function to redirect
 * the output of the compiler logging system.
 */
BOXEXPORT void
BoxLogger_Log(BoxLogger *logger, BoxLogPos *begin, BoxLogPos *end,
              BoxLogLevel lev, const char *fmt, ...);

#endif /* _BOX_LOGGER_H */
