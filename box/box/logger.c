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
#include <string.h>

#include <box/logger_priv.h>
#include <box/print.h>


void
BoxLogger_Init(BoxLogger *logger, BoxLoggerLogFn log_fn,
	       BoxLoggerPutsFn puts_fn, void *data)
{
  logger->log_fn = log_fn;
  logger->puts_fn = puts_fn;
  logger->data = data;
}

void
BoxLogger_Finish(BoxLogger *logger)
{
  // Nothing to do.
}

BoxLogger *
BoxLogger_Create(BoxLoggerLogFn log_fn, BoxLoggerPutsFn puts_fn, void *data)
{
  BoxLogger *logger = (BoxLogger *) Box_Mem_Alloc(sizeof(BoxLogger));
  if (logger)
    BoxLogger_Init(logger, log_fn, puts_fn, data);
  return logger;
}

void
BoxLogger_Destroy(BoxLogger *logger)
{
  BoxLogger_Finish(logger);
  Box_Mem_Free(logger);
}

char *
BoxLogger_Process_Msg(BoxLogger *logger, BoxLogPos *begin, BoxLogPos *end,
		      BoxLogLevel lev, const char *fmt, va_list ap)
{
  const char *prefix = "Fatal error";
  char *s;

  switch (lev) {
  case BOXLOGLEVEL_ADVICE: prefix = "Note"; break;
  case BOXLOGLEVEL_WARNING: prefix = "Warning"; break;
  case BOXLOGLEVEL_ERROR: prefix = "Error"; break;
  }

  s = Box_Print_VA(fmt, ap);

  if (begin && end) {
    const char *file_name = "", *sep = ", ";
    char pos_part[64];

    if (begin->file_name)
      file_name = begin->file_name;
    else if (end->file_name)
      file_name = end->file_name;
    else
      sep = "";

    if (begin->line < end->line)
      snprintf(pos_part, sizeof(pos_part), "%s%u:%u-%u:%u",
	       sep, begin->line + 1, begin->col + 1,
	       end->line + 1, end->col + 1);
    else if (begin->col < end->col)
      snprintf(pos_part, sizeof(pos_part), "%s%u:%u-%u",
	       sep, begin->line + 1, begin->col + 1, end->col + 1);
    else 
      snprintf(pos_part, sizeof(pos_part), "%s%u:%u",
	       sep, begin->line + 1, begin->col + 1);

    return Box_SPrintF("%s (%s%s): %~s", prefix, file_name, pos_part, s);
  }

  return Box_SPrintF("%s: %~s", prefix, s);
}

void
BoxLogger_Log_VA(BoxLogger *logger, BoxLogPos *begin, BoxLogPos *end,
                 BoxLogLevel lev, const char *fmt, va_list ap)
{
  char *s;
  if (logger && logger->log_fn) {
    if (logger->log_fn(logger, begin, end, lev, fmt, ap, logger->data))
      return;
  }

  s = BoxLogger_Process_Msg(logger, begin, end, lev, fmt, ap);

  if (logger && logger->puts_fn)
    logger->puts_fn(logger, s, logger->data);
  else
    puts(s);

  if (s)
    Box_Mem_Free(s);

  if (lev == BOXLOGLEVEL_FATAL)
    abort();
}

void
BoxLogger_Log(BoxLogger *logger, BoxLogPos *begin, BoxLogPos *end,
              BoxLogLevel lev, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  BoxLogger_Log_VA(logger, begin, end, lev, fmt, ap);
  va_end(ap);
}

#if 0
void
My_Wrapper_Puts_Fn(BoxLogger *logger, const char *s, void *data)
{
  BoxLogger *src = (BoxLogger *) data;
  if (src && src->puts_fn)
    src->puts_fn(src, s, src->data);
}

BoxBool
My_Wrapper_Log_Fn(BoxLogger *logger, BoxLogPos *begin, BoxLogPos *end,
		  BoxLogLevel level, const char *fmt, va_list ap, void *data)
{
  BoxLogger *src = (BoxLogger *) data;
  if (src && src->log_fn)
    return src->log_fn(src, begin, end, level, fmt, ap, src->data);
  return BOXBOOL_FALSE;
}

typedef struct {
  BoxLogger wrapper,
            logger;
} BoxLoggerWrapper;

void
BoxLogger_Init_Wrapper(BoxLoggerWrapper *dst, BoxLogger *src)
{
  BoxLogger_Init(dst, My_Log_Fn, My_Puts_Fn, src);
}
#endif
