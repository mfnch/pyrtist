/****************************************************************************
 * Copyright (C) 2011 by Matteo Franchin                                    *
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

#include <box/types.h>
#include <box/vm_private.h>

#include "bridge.h"
#include "i_window.h"
#include "fig.h"

BoxTask Box_Lib_G_SimplePut_At_Window(BoxVM *vm) {
  BoxGWindow *w = BoxVM_Get_Parent_Target(vm);
  BoxGSimplePut *sp = BoxVM_Get_Child_Target(vm);

  BoxGWin_Fig_Draw_Fig_With_Matrix((*w)->window, sp->src->window,
                                   & sp->matrix);
  return BOXTASK_OK;
}
