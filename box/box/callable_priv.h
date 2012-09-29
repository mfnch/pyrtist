/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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
 * @file callable_priv.h
 * @brief Private declarations for the callable object.
 */

#ifndef _BOX_CALLABLE_PRIV_H
#  define _BOX_CALLABLE_PRIV_H

#  include <box/types.h>
#  include <box/callable.h>


/**
 * @brief Implementation of @c BoxCallable.
 */
struct BoxCallable_struct {
  char             *uid;     /**< Unique identifier for the function. */
  BoxCallableKind  kind;     /**< Callable internal implementation. */
  BoxPtr           context;  /**< Pointer to callable private data. */
  union {
    BoxCCall1      c_call_1; /**< BoxCCall1 implementation of the callable. */
    BoxCCall2      c_call_2; /**< BoxCCall2 implementation of the callable. */
    BoxCCall3      c_call_3; /**< BoxCCall3 implementation of the callable. */
    BoxCCallOld    c_old;    /**< BoxCCallOld implementation of the
                                callable. */
    struct {
      BoxVM        *vm;      /**< Virtual Machine. */
      BoxVMCallNum call_num; /**< Call number. */
    }              vm_call;  /**< VM implementation of the callable. */
  }                implem;   /**< Implementation data. */
};

#endif /* _BOX_CALLABLE_PRIV_H */
