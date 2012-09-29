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
 * @file vmproc_priv.h
 * @brief The private header for the VM procedure module.
 */

#ifndef _BOX_VMPROC_PRIV_H
#  define _BOX_VMPROC_PRIV_H

#  include <stdlib.h>
#  include <stdio.h>

#  include <box/occupation.h>
#  include <box/vm.h>
#  include <box/srcpos.h>
#  include <box/callable.h>
#  include <box/vmproc.h>

/**
 * @brief Implementation of #BoxVMProcTable;
 */
struct BoxVMProcTable_struct {
  BoxUInt
    target_proc_num,       /**< Number of the target procedure */
    tmp_proc;              /**< Procedure used as temporary buffer */
  BoxVMProc *target_proc;  /**< The target procedure */
  BoxArr installed;        /**< Array of the installed procedures */
  BoxOcc uninstalled;      /**< Array of the uninstalled procedures */
};

/**
 * @brief Implementation of #BoxVMProcInstalled.
 */
struct BoxVMProcInstalled_struct {
  BoxVMProcKind type;     /**< Kind of procedure */
  char          *name;    /**< Symbol-name of the procedure */
  char          *desc;    /**< Description of the procedure */
  union {
    BoxCallable *foreign; /**< Foreign callable. */
    BoxTask (*c)(void *); /**< Pointer to the C function (can't use
                               BoxVMCCode!) */
    BoxVMCallNum proc_id; /**< Number of the procedure which contains
                               the code */
  } code;
};

#endif /* _BOX_VMPROC_PRIV_H */
