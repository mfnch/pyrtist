/****************************************************************************
 * Copyright (C) 2010-2012 by Matteo Franchin                               *
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
 * @file vm.h
 * @brief Box virtual machine public API.
 */

#ifndef _BOX_VM_H
#  define _BOX_VM_H

#  include <stdio.h>
#  include <stdlib.h>

#  include <box/types.h>

/** A virtual machine object, containing the code to be executed.
 * A BoxVM object can be used to construct a VM executor, BoxVMX, which can
 * execute the code contained in the BoxVM object.
 */
typedef struct BoxVM_struct BoxVM;

/** A virtual machine executor. An object which can be used to execute
 * code from a virtual machine.
 */
typedef struct BoxVMX_struct BoxVMX;
typedef BoxVMX VMStatus;

/* Data type used to write/read binary codes for the instructions */
typedef unsigned char BoxVMByte;

typedef char BoxVMSByte;

typedef unsigned long BoxVMWord;

#  define BoxVMWord_Fmt "%8.8lx"

/** The opcodes for the operations (instructions) understandable by the Box
 * virtual machine.
 * NOTE: the order of the enumeration matters! It corresponds to the order
 *   in the table vm_instr_desc_table[]
 */
typedef enum {
  BOXOP_CALL_I=0, BOXOP_CALL_Iimm,
  BOXOP_NEWC_II, BOXOP_NEWI_II, BOXOP_NEWR_II, BOXOP_NEWP_II, BOXOP_NEWO_II,
  BOXOP_MOV_Cimm, BOXOP_MOV_Iimm, BOXOP_MOV_Rimm, BOXOP_MOV_Pimm,
  BOXOP_MOV_CC, BOXOP_MOV_II, BOXOP_MOV_RR, BOXOP_MOV_PP, BOXOP_MOV_OO,
  BOXOP_BNOT_I, BOXOP_BAND_II, BOXOP_BXOR_II, BOXOP_BOR_II,
  BOXOP_SHL_II, BOXOP_SHR_II,
  BOXOP_INC_I, BOXOP_INC_R, BOXOP_DEC_I, BOXOP_DEC_R,
  BOXOP_POW_II, BOXOP_POW_RR,
  BOXOP_ADD_II, BOXOP_ADD_RR, BOXOP_ADD_PP,
  BOXOP_SUB_II, BOXOP_SUB_RR, BOXOP_SUB_PP,
  BOXOP_MUL_II, BOXOP_MUL_RR, BOXOP_DIV_II, BOXOP_DIV_RR, BOXOP_REM_II,
  BOXOP_NEG_I, BOXOP_NEG_R, BOXOP_NEG_P, BOXOP_PMULR_P, BOXOP_PDIVR_P,
  BOXOP_EQ_II, BOXOP_EQ_RR, BOXOP_EQ_PP, BOXOP_EQ_OO,
  BOXOP_NE_II, BOXOP_NE_RR, BOXOP_NE_PP, BOXOP_NE_OO,
  BOXOP_LT_II, BOXOP_LT_RR, BOXOP_LE_II, BOXOP_LE_RR,
  BOXOP_GT_II, BOXOP_GT_RR, BOXOP_GE_II, BOXOP_GE_RR,
  BOXOP_LNOT_I, BOXOP_LAND_II, BOXOP_LOR_II,
  BOXOP_REAL_C, BOXOP_REAL_I, BOXOP_INT_C, BOXOP_INT_R,
  BOXOP_POINT_II, BOXOP_POINT_RR,
  BOXOP_PROJX_P, BOXOP_PROJY_P, BOXOP_PPTRX_P, BOXOP_PPTRY_P,
  BOXOP_RET,
  BOXOP_CREATE_I, BOXOP_MALLOC_I, BOXOP_MLN_O, BOXOP_MUNLN_O,
  BOXOP_MCOPY_OO, BOXOP_RELOC_OO, BOXOP_SHIFT_OO, BOXOP_REF_OO, BOXOP_NULL_O,
  BOXOP_LEA_C, BOXOP_LEA_I, BOXOP_LEA_R, BOXOP_LEA_P, BOXOP_LEA_OO,
  BOXOP_PUSH_O, BOXOP_POP_O,
  BOXOP_JMP_I, BOXOP_JC_I,
  BOXOP_ADD_O,
  BOXOP_ARINIT_I, BOXOP_ARSIZE_I, BOXOP_ARADDR_II, BOXOP_ARGET_OO,
  BOXOP_ARNEXT_OO, BOXOP_ARDEST_O,
  BOX_NUM_OPS

} BoxOp;

/** Generic opcodes (type independent) */
typedef enum {
  BOXGOP_CALL=0, BOXGOP_NEWC, BOXGOP_MOV,
  BOXGOP_BNOT, BOXGOP_BAND, BOXGOP_BXOR, BOXGOP_BOR, BOXGOP_SHL, BOXGOP_SHR,
  BOXGOP_INC, BOXGOP_DEC, BOXGOP_POW, BOXGOP_ADD, BOXGOP_SUB, BOXGOP_MUL,
  BOXGOP_DIV, BOXGOP_REM, BOXGOP_NEG, BOXGOP_PMULR, BOXGOP_PDIVR,
  BOXGOP_EQ, BOXGOP_NE, BOXGOP_LT, BOXGOP_LE, BOXGOP_GT, BOXGOP_GE,
  BOXGOP_LNOT, BOXGOP_LAND, BOXGOP_LOR, BOXGOP_REAL, BOXGOP_INT, BOXGOP_POINT,
  BOXGOP_PROJX, BOXGOP_PROJY, BOXGOP_PPTRX, BOXGOP_PPTRY,
  BOXGOP_RET,
  BOXGOP_CREATE, BOXGOP_MALLOC, BOXGOP_MLN, BOXGOP_MUNLN,
  BOXGOP_MCOPY, BOXGOP_RELOC, BOXGOP_SHIFT, BOXGOP_REF, BOXGOP_NULL,
  BOXGOP_LEA, BOXGOP_PUSH, BOXGOP_POP, BOXGOP_JMP, BOXGOP_JC,
  BOXGOP_ARINIT, BOXGOP_ARSIZE, BOXGOP_ARADDR, BOXGOP_ARGET, BOXGOP_ARNEXT,
  BOXGOP_ARDEST, BOX_NUM_GOPS

} BoxGOp;

/** The attributes corresponding to different behaviours of the Box virtual
 * machine.
 * @see BoxVM_Set_Attr
 */
typedef enum {
  BOXVM_ATTR_ASM_LONG_FMT=1,  /**< Use long format when assembling code */
  BOXVM_ATTR_DASM_WITH_HEX=2, /**< Show also hex values when disassembling */
  BOXVM_ATTR_ADD_DATA_IDENT=4 /**< Add identity info (debug) to data blocks */

} BoxVMAttr;

/** Allocate space for a BoxVM object and initialise it with BoxVM_Init.
 * You'll need to call BoxVM_Destroy to destroy the object.
 * @see BoxVM_Destroy, BoxVM_Init
 */
BOXEXPORT BoxVM *BoxVM_Create(void);

/** Destroy a BoxVM object created with BoxVM_Create
 * @see BoxVM_Create
 */
BOXEXPORT void BoxVM_Destroy(BoxVM *vm);

/** Set or unset the attributes which control the behaviour of the Box
 * virtual machine.
 * @param vmp an instance of the Box virtual machine
 * @param mask specify what values to set/unset
 * @param value specify to set/unset the values specified in mask
 * @see BoxVMAttr
 */
void BoxVM_Set_Attr(BoxVM *vm, BoxVMAttr mask, BoxVMAttr value);

/** This function adds a new piece of data to the data segment.
 * NOTE: It returns the address of the data item with respect to the beginning
 *  of the data segment.
 * @param vm the VM object.
 * @param data the pointer to the data to add.
 * @param size the size of the data pointed by data.
 * @param type an integer identifying the type of data.
 */
BOXEXPORT size_t BoxVM_Data_Add(BoxVM *vm, const void *data, size_t size,
                                BoxInt type);

/** Produce a human readable representation of the data segment of 'vm'
 * and send it to the output stream 'stream'.
 */
void BoxVM_Data_Display(BoxVM *vm, FILE *stream);

/** Sets the force-long flag and return what was its previous value. */
BOXEXPORT int BoxVM_Set_Force_Long(BoxVM *vm, int force_long);

BOXEXPORT void BoxVM_ASettings(BoxVM *vmp, int forcelong, int error,
                               int inhibit);

BOXEXPORT void BoxVM_VA_Assemble(BoxVM *vm, BoxOp instr, va_list ap);

BOXEXPORT void BoxVM_Assemble(BoxVM *vmp, BoxOp instr, ...);

/** Similar to VM_Assemble, but use the long bytecode format. */
#define BoxVM_Assemble_Long(vm, instr, ...)                     \
  do {                                                          \
    int is_long = BoxVM_Set_Force_Long((vm), 1);                \
    BoxVM_Assemble((vm), (instr), __VA_ARGS__);                 \
    (void) BoxVM_Set_Force_Long((vm), is_long);} while(0)

BOXEXPORT BoxTask BoxVM_Disassemble(BoxVM *vm, FILE *output,
                                    const void *prog, size_t dim);

/** Specifies the number of global registers and variables used by the BoxVM.
 */
BOXEXPORT BoxTask BoxVM_Alloc_Global_Regs(BoxVM *vm, BoxInt num_var[],
                                          BoxInt num_reg[]);

BOXEXPORT void BoxVM_Module_Global_Set(BoxVM *vmp, BoxInt type, BoxInt reg,
                                       void *value);

#endif /* _BOX_VM_H */
