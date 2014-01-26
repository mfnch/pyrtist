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


#if 0
char *BoxSrc_To_Str(BoxLogPos *begin, BoxLogPos *end)
{
  uint32_t bl = loc->begin.line, bc = loc->begin.col,
       el = loc->end.line, ec = loc->end.col;
  const char *fn = (loc->begin.file_name ?
                    Box_SPrintF("\"%s\", ", loc->begin.file_name) :
                    Box_Mem_Strdup(""));

  if (bl == 0)
    return Box_SPrintF("%~stext ending at line %ld col %ld", fn, el, ec);

  if (el == 0)
    return Box_SPrintF("%~sfrom line %ld col %ld", fn, bl, bc);

  if (bl == el) {
    if (loc->begin.col >= loc->end.col - 1)
      return Box_SPrintF("%~sline %ld col %ld", fn, bl, bc);
    else
      return Box_SPrintF("%~sline %ld cols %ld-%ld", fn, bl, bc, ec);
  }

  return Box_SPrintF("%~sline %ld-%ld cols %ld-%ld", fn, bl, el, bc, ec);
}
#endif

void
BoxLogger_Log_VA(BoxLogger *logger, BoxLogPos *begin, BoxLogPos *end,
                 BoxLogLevel lev, const char *fmt, va_list ap)
{
  if (begin && end) {
    printf("Error (%s, %d-%d, %d-%d): ",
           begin->file_name, begin->line, end->line, begin->col, end->col);
    (void) vprintf(fmt, ap);
    printf("\n");
  } else {
    printf("Error: ");
    (void) vprintf(fmt, ap);
    printf("\n");
  }
}
