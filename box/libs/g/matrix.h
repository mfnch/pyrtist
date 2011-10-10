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

#ifndef _BOX_LIBG_MATRIX_H
#  define _BOX_LIBG_MATRIX_H

#  include <box/types.h>

/** Note: this definition is coherent with the following Box definition
 * Matrix = ++(Real m11, m12, m13, m21, m22, m23)
 */
typedef struct {
  BoxReal m11, m12, m13, m21, m22, m23;

} BoxGMatrix;

/** Obsolete. Use BoxGMatrix instead. */
typedef BoxGMatrix Matrix;

/** Calculate the transformation matrix corresponding to:
 * translation vector 't', rotation center 'rcntr', rotation angle 'rang',
 * scale factors 'sx' and 'sy'. The computed matrix is put in 'm'.
 */
void BoxGMatrix_Set(BoxGMatrix *m,
                    BoxPoint *t, BoxPoint *rcntr, BoxReal rang,
                    BoxReal sx, BoxReal sy);

/** Set the matrix 'm' to the identity matrix. */
void BoxGMatrix_Set_Identity(BoxGMatrix *m);

/** Apply the matrix 'm' to the 'num_points' points in 'in' and store the
 * result in 'out'.
 */
void BoxGMatrix_Map_Points(BoxGMatrix *m, BoxPoint *out, BoxPoint *in,
                           size_t num_points);

/** Apply the matrix 'm' to the point 'in' and store the result in 'out'. */
void BoxGMatrix_Map_Point(BoxGMatrix *m, BoxPoint *out, BoxPoint *in);

/** Apply the matrix 'm' to the 'num_vecs' vectors in 'vecs'. */
void BoxGMatrix_Map_Vectors(BoxGMatrix *m, BoxPoint *out, BoxPoint *in,
                            size_t num_vecs);

/** Apply the matrix 'm' to the vector 'in' and store the result in 'out'. */
void BoxGMatrix_Map_Vector(BoxGMatrix *m, BoxPoint *out, BoxPoint *in);

#endif /* _BOX_LIBG_MATRIX_H */
