/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "error.h"

void BoxErr_Init(BoxErr *err) {
  err->code = BOXERR_NO_ERROR;
  err->is.tolerant = 0;
}

const char *BoxErr_Msg(BoxErrCode code) {
  switch(code) {
  case BOXERR_NO_ERROR:
    return NULL;
  case BOXERR_OUT_OF_MEMORY:
    return "Out of memory";
  case BOXERR_OUT_OF_BOUNDS:
    return "Index out of bounds";
  case BOXERR_DOUBLE_RELEASE:
    return "Double call to release on the same item";
  default:
    return "Unknown error";
  }
}

void BoxErr_Report(BoxErr *err, BoxErrCode code) {
  err->code = code;
  if (! err->is.tolerant) {
    const char *msg = BoxErr_Msg(code);
    if (msg != NULL) {
      fprintf(stderr, "Fatal error: %s\n", msg);
      /*assert(0); assert is better than exit for debugging */
      abort();
    }
  }
}

int BoxErr_Have_Err(BoxErr *err) {
  return (err->code != BOXERR_NO_ERROR);
}

int BoxErr_Set_Tolerance(BoxErr *err, int val) {
  int previous_val = err->is.tolerant;
  err->is.tolerant = val;
  return previous_val;
}

BoxErrCode BoxErr_Set_Error(BoxErr *err, BoxErrCode val) {
  BoxErrCode previous_val = err->code;
  err->code = val;
  return previous_val;
}

int BoxErr_Propagate(BoxErr *dest, BoxErr *src) {
  BoxErr_Report(dest, src->code);
  BoxErr_Init(src);
  return BoxErr_Have_Err(dest);
}

void BoxErr_Assert(BoxErr *err) {
  assert(err->code == BOXERR_NO_ERROR);
}