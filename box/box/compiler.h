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


/**
 * @brief The Box Compiler.
 */
typedef struct BoxCmp_struct BoxCmp;


BOXEXPORT BoxCmp *
BoxCmp_Create(BoxVM *target_vm);

BOXEXPORT void
BoxCmp_Destroy(BoxCmp *c);

/**
 * Steal the VM which is being used as the target for the compilation.
 */
BOXEXPORT BoxVM *
BoxCmp_Steal_VM(BoxCmp *c);

/**
 * Create the compiler and use it to parse the given file, returning the
 * virtual machine object which was used as the target of the compilation
 * and putting in '*main' the BoxVMCallNum of the main procedure of the
 * compiled source.
 */
BOXEXPORT BoxVM *
Box_Compile_To_VM_From_File(BoxVMCallNum *main, BoxVM *target_vm,
                            FILE *file, const char *file_name,
                            const char *setup_file_name,
                            BoxPaths *paths);

/**
 * @brief Compile from the give abstract syntax tree.
 *
 * @param c Compiler to use.
 * @param program Abstract syntax tree of the program to compile.
 */
BOXEXPORT void
BoxCmp_Compile(BoxCmp *c, BoxAST *ast);

#endif /* _BOX_COMPILER_H */
