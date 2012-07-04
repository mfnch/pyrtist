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

#include "types.h"
#include "mem.h"
#include "vm_private.h"
#include "vmx.h"

#if 0
/* Initialize a new VM executor in the given portion of memory. */
void BoxVMX_Init(BoxVMX *vmx) {
  vmx->fail_msg = NULL;
  BoxArr_Init(& vmx->backtrace, sizeof(BoxVMTrace), 32);
}

/* Finalize the given VM executor. */
void BoxVMX_Finish(BoxVMX *vmx) {
  if (vmx->fail_msg)
    BoxMem_Free(vmx->fail_msg);
  BoxArr_Finish(& vmx->backtrace);
}
#endif

/* Set a failure message. */
void BoxVMX_Set_Fail_Msg(BoxVMX *vmx, const char *msg) {
  BoxVM *vm = vmx->vm;
  if (vm->fail_msg)
    BoxMem_Free(vm->fail_msg);
  vm->fail_msg = msg ? BoxMem_Strdup(msg) : NULL;
}

/* Get the last failure message. */
char *BoxVMX_Get_Fail_Msg(BoxVMX *vmx, BoxBool steal) {
  BoxVM *vm = vmx->vm;
  if (vm->fail_msg) {
    if (steal) {
      char *msg = vm->fail_msg;
      vm->fail_msg = NULL;
      return msg;

    } else
      return vm->fail_msg;

  } else
    return NULL;
}
