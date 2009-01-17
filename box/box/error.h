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

/** @file error.h
 * @brief Common error handling for the Box core library.
 *
 * Handling errors in C is quite painful and often forces to fill the code
 * with error checking conditional statements, such as:
 *
 *   err = my_fun(...);
 *   if (err) ...
 *
 * There are then two kind of situations: the one where you do not expect
 * an error at all (such as when you call malloc to allocate space for
 * an object: if you cannot malloc a small object, then you may not want
 * to continue executing the program at all!) and the one where you may really
 * have an error (such as when reading a file from disk: the file may not
 * exist and you may want to handle carefully the error).
 * We then need a good strategy to handle both situations. Here we provide
 * a framework for error handling for the Box core library. We proceed as
 * follows: we embed the "error state" inside the objects. The error state
 * contains information about the last error occurred when operating
 * on the corresponding object. It also contains a flag determining how
 * to behave when errors occur for its object. The object may be error
 * tolerant, meaning that errors can happen and can be detected. Or it may be
 * non tolerant, meaning that any error occurring on the object will lead
 * to exiting the program abnormally. We provide generic macros, so that
 * one does not need to rewrite the same error handling functions again
 * and again. We do this by sticking to few conventions: a member with type
 * BoxErr and name err has to be defined for objects wanting to use
 * the generic error handling mechanism here described.
 */

#ifndef _BOX_ERROR_H
#  define _BOX_ERROR_H

#  include <stdlib.h>

#  include "error.h"

/** Integer corresponding to a different array in the Box library */
typedef enum {
  BOXERR_NO_ERROR=0,
  BOXERR_OUT_OF_MEMORY,
  BOXERR_OUT_OF_BOUNDS,
  BOXERR_DOUBLE_RELEASE
} BoxErrCode;

/** This object should be embedded in object wanting to stick to a common
 * uniform error handling mechanism.
 */
typedef struct {
  struct {
    unsigned int
             tolerant; /**< 1 to trap erros, 0 to exit on errors */
  } is;
  BoxErrCode code;     /**< Error code */
} BoxErr;

/** Initialize the error state. */
void BoxErr_Init(BoxErr *err);

/** Return a string representation of the given error code or NULL
 * if the code corresponds to BOXERR_NO_ERROR.
 */
const char *BoxErr_Msg(BoxErrCode code);

/** Used to report an error. */
void BoxErr_Report(BoxErr *err, BoxErrCode code);

/** Return 1 in case of error, 0 otherwise */
int BoxErr_Have_Err(BoxErr *err);

/** Set the error tolerance and return the previous value */
int BoxErr_Set_Tolerance(BoxErr *err, int val);

/** Set the error code and return the previous value */
BoxErrCode BoxErr_Set_Error(BoxErr *err, BoxErrCode val);

/** Equivalent to BoxErr_Set_Error(err, BOXERR_NO_ERROR) */
#  define BoxErr_Clear_Error(err) BoxErr_Set_Error((err), BOXERR_NO_ERROR)

/** Propagate the error from 'src' to 'dest', reset the error status in 'src'
 * and return 1 if there actually was an error in 'src'.
 */
int BoxErr_Propagate(BoxErr *dest, BoxErr *src);

/** Used to state that the programmer does not expect an error at a certain
 * point in the execution.
 */
void BoxErr_Assert(BoxErr *err);

#endif /* _BOX_ERROR_H */
