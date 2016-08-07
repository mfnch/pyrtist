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


#ifndef _BOX_LIBG_TRANSFORM_H
#  define _BOX_LIBG_TRANSFORM_H

#  include <box/types.h>
#  include "graphic.h"

/** Flags used to specify which transformations are allowed. */
typedef enum {
  BOXGALLOW_TRANSLATE_X = 0x1,  /**< Allow automatic translation along x */
  BOXGALLOW_TRANSLATE_Y = 0x2,  /**< Allow automatic translation along y */
  BOXGALLOW_TRANSLATE   = 0x3,  /**< Allow automatic translation in x, y */
  BOXGALLOW_ROTATE      = 0x4,  /**< Allow automatic rotation */
  BOXGALLOW_SCALE       = 0x8,  /**< Allow automatic scaling */
  BOXGALLOW_ANISOTROPIC = 0x10, /**< Allow different scaling in x and y */
  BOXGALLOW_INVERT      = 0x20, /**< Allow mirroring the figure */
  BOXGALLOW_DEFORM      = 0x30, /**< Allow mirroring and anisotropic scaling */
  BOXGALLOW_ALL         = 0x3f  /**< All flags or-ed together */

} BoxGAllow;

/** Structure containing a transformation in terms of translation vector,
 * rotation center and angle and scaling factors. From this structure
 * a transformation matrix can be obtained.
 */
typedef struct {
  BoxPoint translation,
           rotation_center;
  BoxReal  rotation_angle,
           rotation_cos,
           rotation_sin,
           scale_factor,
           scaling_angle,
           scaling_cos,
           scaling_sin;
} BoxGTransform;

/** Used by BoxG_Auto_Transform to signal errors. */
typedef enum {
  BOXGAUTOTRANSFORMERR_NO_ERR=0,
  BOXGAUTOTRANSFORMERR_NOT_ENOUGH_POINTS,
  BOXGAUTOTRANSFORMERR_ZERO_WEIGHTS,
  BOXGAUTOTRANSFORMERR_NOT_IMPLEMENTED
} BoxGAutoTransformErr;


/** Provide a string representation of the given BoxGAutoTransformErr. */
const char *BoxGAutoTransformErr_To_String(BoxGAutoTransformErr n);

/** From an initial transformation 'transform', 'n' constraints (provided in
 * 'src', 'dst' and 'weight') and a set of allowed transformations,
 * 'allowed_transformation', finds the one which best satisfies the
 * constraints. In particular, the objective is to minimise the function:
 *
 *   f(q) = sum_i {weight[i]*(T(q) src[i] - dst[i])^2} / sum_i {weight[i]}
 *
 * where T[q] is a transformation parametrized by q, which is a vector made
 * by all the available degrees of freedom. The objective is then to determine
 * 'q' and T(q). T(q) is then stored inside 'transform'.
 */
BoxGAutoTransformErr
BoxG_Auto_Transform(BoxGTransform *transform,
                    BoxPoint *src, BoxPoint *dst, BoxReal *weight, int n,
                    BoxGAllow allowed_transforms);

/** Convert a string to a BoxGAllow value to use with BoxG_Auto_Transform. This
 * function allows quick specification of the allowed transformations. Allowed
 * transformations are specified using letters. Each letter corresponds to an
 * allowed transformation. For example: "rt" means that "r"otations and
 * "t"ranslations are allowed. The order of letters does not matter, e.g. "rt"
 * is equivalent to "tr". The following table collects the associations
 * between letters and transformations:
 *  "t" -> translation, "r" -> rotation, "s" -> scale,
 *  "a" -> allows anisotropic scaling, "i" -> allows inversion (mirroring) It
 * is also possible to use "tx" or "ty" to enable translation only along one of
 * the axes. It is also possible to give sign "+" and "-". The sign change the
 * behaviour of the function. "+" means that the following characters should be
 * used to enable transformations (default), while "-" means that they should
 * be used to disable transformations. This is handy for concatenated
 * strings. Example: allowed + "-txr" means that we should do whathever
 * specified by 'allowed', but we should not translate along-x nor rotate.
 * There are then a few characters to fine tune the behaviour of the function.
 * When 'string' starts with a white space " ", the flags are or-ed over the
 * pre-existing flags in '*allow' (while if the string does not start with a
 * space the initial value of '*allow' is ignored, i.e. set to zero). Spaces in
 * the middle of the string are ignored.
 */
BoxTask BoxGAllow_Of_String(BoxGAllow *allow, const char *string);

#endif /*_BOX_LIBG_TRANSFORM_H */
