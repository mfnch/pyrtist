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

#include <stdio.h>

#include <box/logger.h>


void
BoxLogger_Log_VA(BoxLogger *logger, BoxLogPos *begin, BoxLogPos *end,
                 BoxLogLevel lev, const char *fmt, va_list ap)
{
  if (begin && end) {
    printf("Error (%s, %d-%d, %d-%d): ",
           begin->file_name, begin->line, end->line, begin->col, end->col);
    (void) vprintf(fmt, ap);
    printf(".\n");
  } else {
    printf("Error: ");
    (void) vprintf(fmt, ap);
    printf(".\n");
  }
}
