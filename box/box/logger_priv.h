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
 * @file logger_priv.h
 * @brief Private definitions for logger.h.
 */
#ifndef _BOX_LOGGER_PRIV_H
#  define _BOX_LOGGER_PRIV_H

#  include <box/logger.h>


struct BoxLogger_struct {
  BoxLoggerLogFn  log_fn;
  BoxLoggerPutsFn puts_fn;
  void            *data;
};

/**
 * @brief Create a #BoxLogger object.
 * @see BoxLogger_Create()
 * @see BoxLogger_Finish()
 */
BOXEXPORT void
BoxLogger_Init(BoxLogger *logger, BoxLoggerLogFn log_fn,
	       BoxLoggerPutsFn puts_fn, void *data);

/**
 * @brief Destroy a logger created with BoxLogger_Init().
 */
BOXEXPORT void
BoxLogger_Finish(BoxLogger *logger);

#endif /* _BOX_LOGGER_PRIV_H */
