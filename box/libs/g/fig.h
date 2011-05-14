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
GrpWindow *fig_open_win(int numlayers);
int fig_destroy_layer(int l);
int fig_new_layer(void);
void fig_select_layer(int l);
void fig_clear_layer(int l);
Real fig_transform_factor(Real angle);

void Fig_Draw_Fig(GrpWindow *src);
void Fig_Draw_Fig_With_Matrix(GrpWindow *src, Matrix *m);

int fig_save_fig(GrpWindow *figure, GrpWindowPlan *plan);

#endif
