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
 */
typedef BoxBool (*BoxLoggerCallback)(BoxLogger *logger, BoxLogPos *begin,
                                     BoxLogPos *end, BoxLogLevel level,
                                     const char *fmt, va_list ap,
                                     void *data);

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
