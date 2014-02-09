/****************************************************************************
 * Copyright (C) 2008-2012 by Matteo Franchin                               *
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
 * @file vmsym.h
 * @brief The implementation of the reference/definition list for the box VM.
 *
 * This file implements the mechanism to define and reference procedures,
 * labels and other symbols needed by a Box bytecode compiled program.
 * This mechanism is similar to the linking of the object files produced
 * by the compilation of several C-source files. In this sense whe could
 * say that this file implements the Box linker.
 */

#ifndef _BOX_VMSYM_H
#  define _BOX_VMSYM_H

#  include <box/types.h>
#  include <box/array.h>
#  include <box/hashtable.h>
#  include <box/list.h>
#  include <box/vm.h>

/**
 * @brief An integer number used to identify a symbol.
 */
typedef BoxUInt BoxVMSymID;

/** Invalid symbol ID */
#define BOXVMSYMID_NONE ((BoxVMSymID) 0)

/**
 * @brief The table of references and definitions for the Box VM.
 *
 * This structure is embedded in the main VM structure VMProgram.
 */
typedef struct {
  BoxHT  syms;
  BoxArr data,
         defs,
         refs,
         dylibs;
} BoxVMSymTable;

/** This is the prototype for a function which generates a piece of code
 * which makes a reference to a symbol.
 * Functions of this type are called when a reference is made to a symbol
 * ('resolving=0') or when the reference is being resolved ('resolving=1').
 * If the symbol has already been defined, then 'defined=1', otherwise
 * 'defined=0'. 'def' and 'def_size'are respectively the pointer
 * and the size of the definition data (which are allocated when the symbol
 * is created with BoxVMSym_New).
 */
typedef BoxTask (*BoxVMSymResolver)(BoxVM *vm, BoxVMSymID sym_id,
                                    BoxUInt sym_type, int defined,
                                    void *def, size_t def_size,
                                    void *ref, size_t ref_size);

/**
 * @brief The details about a symbol.
 */
typedef struct {
  BoxVMSymID sym_id;    /**< The symbol ID number */
  BoxName    name;      /**< The name of the symbol */
  int        defined;   /**< Has a definition been given for the symbol? */
  size_t     def_size,  /**< Size of the symbol definition data. */
             def_addr;  /**< Address of the data in the array 'data' */
  BoxUInt    sym_type,  /**< Type of the symbol */
             first_ref; /**< Number of the first reference to the symbol */
} BoxVMSym;

/**
 * @brief A reference...
 */
typedef struct {
  BoxVMSymID
          sym_id,  /**< ID-number of the referenced symbol */
          next;     /**< ID-number of the next reference to the same symbol */
  size_t  ref_size, /**< Size of the data containing the symbol reference */
          ref_addr; /**< Address of the data in the array 'data' */
  int     resolved; /**< Has the reference been resolved? */
  BoxVMSymResolver
          resolver; /**< Function to be called for resolution */
} BoxVMSymRef;

/* Data types used by the BoxVMSym_Code_* functions */
typedef BoxTask (*BoxVMSymCodeGen)(BoxVM *vm, BoxVMSymID sym_id,
                                   BoxUInt sym_type, int defined,
                                   void *def, size_t def_size,
                                   void *ref, size_t ref_size);

typedef struct {
  BoxUInt proc_num,
          pos,
          size;
  BoxVMSymCodeGen
          code_gen;
} BoxVMSymCodeRef;

/** Initialize the symbol table of the program.
 * @param vmp is the VM-program.
 */
BOXEXPORT void
BoxVMSymTable_Init(BoxVMSymTable *t);

/** Destroy the symbol table of the program.
 * @param vmp is the VM-program.
 */
BOXEXPORT void
BoxVMSymTable_Finish(BoxVMSymTable *t);

/** Create a new symbol with name 'n' and type 'sym_type'.
 * '*sym_id' is set with the allocated symbol number.
 */
BOXEXPORT BoxVMSymID
BoxVMSym_New(BoxVM *vmp, BoxUInt sym_type, BoxUInt def_size);

/**
 * @brief Create a new symbol.
 *
 * @param vm The virtual machine associated to the symbol.
 * @param sym_type The type of the symbol.
 * @param def Pointer to the symbol data.
 * @param def_size Size of the symbol data.
 * @return A new symbol identifier (an integer).
 */
BOXEXPORT BoxVMSymID
BoxVMSym_Create(BoxVM *vm, BoxUInt sym_type,
                const void *def, size_t def_size);

/** Associate a name to the symbol sym_id.
 */
BOXEXPORT void
BoxVMSym_Set_Name(BoxVM *vm, BoxVMSymID sym_id, const char *name);

/** Get the name of the given symbol sym_id (NULL if the symbol has not
 * a name)
 */
const char *BoxVMSym_Get_Name(BoxVM *vm, BoxVMSymID sym_id);

/**
 * @brief Find a symbol with the given name.
 */
BOXEXPORT BoxVMSymID
BoxVM_Find_Sym(BoxVM *vm, const char *name);

/**
 * @brief Get the definition data for a symbol.
 *
 * @param vm The virtual machine the symbol refers to.
 * @param sym_id The symbol identifier.
 * @return The pointer to the definion data. The caller can write the
 *   definition data before defining the symbol. If this is what the caller
 *   decides to do, then it should also define the symbol by passing @c NULL
 *   as a last argument for BoxVMSym_Define() to avoid overwriting the symbol
 *   data.
 * @note The pointer returned by this function must be used immediately,
 *   before adding new symbols (as data may be relocated when doing so).
 */
BOXEXPORT void *
BoxVMSym_Get_Definition(BoxVM *vm, BoxVMSymID sym_id);

/**
 * @brief Define a symbol which was previously created with BoxVMSym_Create().
 *
 * @param vm The virtual machine the symbol refers to.
 * @param sym_id The symbol identifier.
 * @param def The symbol data. Use @c NULL to leave the symbol data as it
 *   currently is (as set using BoxVMSym_Get_Definition()).
 * @return @c BOXTASK_OK if the symbol was defined, else @c BOXTASK_FAILURE
 *   (which typically means that a symbol was defined twice).
 */
BOXEXPORT BoxTask
BoxVMSym_Define(BoxVM *vm, BoxVMSymID sym_id, void *def);

/** Return whether the symbol 'sym_id' has been defined or not */
BOXEXPORT int
BoxVMSym_Is_Defined(BoxVM *vm, BoxVMSymID sym_id);

typedef enum {
  BOXVMSYM_AUTO,
  BOXVMSYM_RESOLVED,
  BOXVMSYM_UNRESOLVED,
} BoxVMSymStatus;

/** Add a reference to the symbol 'sym_id'. The reference data is pointed
 * by 'ref' and has size 'ref_size'. 'r' is the function to be called for
 * resolving the symbol. If 'resolved' == BOXVMSYM_RESOLVED, the reference
 * is marked as resolved, if 'resolved' == BOXVMSYM_UNRESOLVED, it is marked
 * as unresolved. If 'resolved' == BOXVMSYM_AUTO, then it is marked as resolved
 * only if the symbol is defined.
 */
BOXEXPORT void
BoxVMSym_Ref(BoxVM *vm, BoxVMSymID sym_id, BoxVMSymResolver r,
             void *ref, size_t ref_size, BoxVMSymStatus resolved);

#if 0
/** Set the symbol resolver */
BOXEXPORT BoxTask
BoxVMSym_Resolver_Set(BoxVM *vm, BoxVMSymID sym_id, VMSymResolver r);
#endif

/** Set *all_resolved = 1 only if there are no unresolved references */
void BoxVMSym_Ref_Check(BoxVM *vm, int *all_resolved);

/** Print an error message for every unresolved reference */
BOXEXPORT void
BoxVMSym_Ref_Report(BoxVM *vm);

/**
 * @brief Resolve the symbol.
 *
 * A symbol can be resolved once it has been defined with BoxVMSym_Define().
 * The resolution consist in iterating over all the reference to the symbol
 * and calling the resolutor (a callback).
 * @param vm The virtual machine the symbol refers to.
 * @param sym_id The symbol identifier.
 * @return Whether the operation succeeded.
 * @note If <pp>sym_id=0</pp>, then try to resolve all the defined symbols
 *   (undefined symbols are ignored).
 */
BOXEXPORT BoxTask
BoxVMSym_Resolve(BoxVM *vm, BoxVMSymID sym_id);

/**
 * @brief Print the symbol table for the given symbol.
 *
 * The symbol table is a list of all the references made to the symbol. If @p
 * sym_id is zero the references to all symbols will be printed.
 * @param vm The virtual machine.
 * @param out The output stream.
 * @param sym_id The symbol identifier.
 */
BOXEXPORT void
BoxVMSym_Table_Print(BoxVM *vm, FILE *out, BoxVMSymID sym_id);

/**
 * @brief Check that the type of the symbol @p sym_id is @p sym_type.
 */
BOXEXPORT BoxTask
BoxVMSym_Check_Type(BoxVM *vm, BoxVMSymID sym_id, BoxUInt sym_type);

/**
 * @brief Open the given dynamic library to resolve symbols.
 *
 * If the file is not found then this function appends an appropriate extension
 * (.so for linux, .dll for windows, etc.) and tries again.
 */
BOXEXPORT BoxTask
BoxVMSym_Resolve_CLib(BoxVM *vm, const char *lib_file);

/**
 * @brief Resolution of functions defined in external dynamically loaded
 *   C libraries.
 */
BOXEXPORT BoxTask
BoxVMSym_Resolve_CLibs(BoxVM *vm, BoxList *lib_paths, BoxList *libs);

#  define BoxVMSym_Resolve_All(vm) \
  BoxVMSym_Resolve((vm), 0)

#endif /* _BOX_VMSYM_H */
