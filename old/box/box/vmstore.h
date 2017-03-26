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
 * @file vmstore.h
 * @brief Module which allows saving/loading VMs.
 */

#ifndef _BOX_VMSTORE_H
#  define _BOX_VMSTORE_H

#  include <box/types.h>
#  include <box/vm.h>
#  include <box/stream.h>

/**
 * Save the VM to file.
 * @param vm the VM to save.
 * @param stream the destination stream.
 * @return whether the operation succedeed.
 */
BOXEXPORT BoxTask BoxVM_Save_To_Stream(BoxVM *vm, BoxStream *stream);

/**
 * Load a VM from a stream.
 * @param vm an initialized, but empty VM.
 * @param stream the stream from which the VM should be read.
 * @return whether the operation succedeed.
 */
BOXEXPORT BoxTask BoxVM_Load_From_Stream(BoxVM *vm, BoxStream *stream);

#endif /* _BOX_VMSTORE_H */
