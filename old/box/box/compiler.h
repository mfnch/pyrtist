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

/**
 * @file compiler.h
 * @brief The compiler of Box.
 *
 * A nice description...
 */

#ifndef _BOX_COMPILER_H
#  define _BOX_COMPILER_H

#  include <box/types.h>
#  include <box/vm_priv.h>
#  include <box/ast.h>
#  include <box/paths.h>
#  include <box/logger.h>


BOX_BEGIN_DECLS

/**
 * Create the compiler and use it to parse the given file, returning the
 * virtual machine object which was used as the target of the compilation
 * and putting in '*main' the BoxVMCallNum of the main procedure of the
 * compiled source.
 */
BOXEXPORT BoxBool
Box_Compile_To_VM_From_File(BoxVMCallNum *main, BoxVM *target_vm,
                            FILE *file, const char *file_name,
                            const char *setup_file_name,
                            BoxPaths *paths, BoxLogger *logger);

BOX_END_DECLS

#endif /* _BOX_COMPILER_H */