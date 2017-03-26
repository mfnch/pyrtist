/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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
#include <box/vm_priv.h>
#include "graphic.h"
#include "hsv.h"

/* Color@HSV */
BoxTask hsv_color(BoxVMX *vmp) {
  Color c = BOX_VM_ARG1(vmp, Color);
  HSV *hsv = BOX_VM_THIS_PTR(vmp, HSV);
  Color_Trunc(& c);
  HSV_From_Color(hsv, & c);
  return BOXTASK_OK;
}

/* HSV@Color */
BoxTask color_hsv(BoxVMX *vmp) {
  HSV hsv = BOX_VM_ARG1(vmp, HSV);
  Color *c = BOX_VM_THIS_PTR(vmp, Color);
  HSV_Trunc(& hsv);
  HSV_To_Color(c, & hsv);
  return BOXTASK_OK;
}
