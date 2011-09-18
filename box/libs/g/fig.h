/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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

/* Questo file contiene le dichiarazioni delle funzioni del file fig.c
 */

#ifndef _FIG_H
#  define _FIG_H

#  include "graphic.h"

/* Procedure per gestire i layers */
BoxGWin *BoxGWin_Create_Fig(int numlayers);
int BoxGWin_Fig_Destroy_Layer(BoxGWin *w, int l);
int BoxGWin_Fig_New_Layer(BoxGWin *w);
void BoxGWin_Fig_Select_Layer(BoxGWin *w, int l);
void BoxGWin_Fig_Clear_Layer(BoxGWin *w, int l);
Real fig_transform_factor(Real angle);

void BoxGWin_Fig_Draw_Fig(BoxGWin *dest, BoxGWin *src);
void BoxGWin_Fig_Draw_Fig_With_Matrix(BoxGWin *dest, BoxGWin *src, Matrix *m);

int BoxGWin_Fig_Save_Fig(BoxGWin *src, BoxGWinPlan *plan);

#endif
