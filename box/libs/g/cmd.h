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

#ifndef _BOX_LIBG_RAW_H
#  define _BOX_LIBG_RAW_H

#  include <box/types.h>

#  include "obj.h"

/** Raw graphics commands (COMMANDS FOR BoxGWin_Interpret_Obj). */
typedef enum {
  BOXGCMD_SAVE=0, BOXGCMD_RESTORE, BOXGCMD_SET_ANTIALIAS,
  BOXGCMD_MOVE_TO, BOXGCMD_LINE_TO, BOXGCMD_CURVE_TO,
  BOXGCMD_CLOSE_PATH, BOXGCMD_NEW_PATH, BOXGCMD_NEW_SUB_PATH,
  BOXGCMD_STROKE, BOXGCMD_STROKE_PRESERVE, BOXGCMD_FILL,
  BOXGCMD_FILL_PRESERVE, BOXGCMD_CLIP, BOXGCMD_CLIP_PRESERVE,
  BOXGCMD_RESET_CLIP, BOXGCMD_PUSH_GROUP, BOXGCMD_POP_GROUP_TO_SOURCE,
  BOXGCMD_SET_OPERATOR, BOXGCMD_PAINT, BOXGCMD_PAINT_WITH_ALPHA,
  BOXGCMD_COPY_PAGE, BOXGCMD_SHOW_PAGE, BOXGCMD_SET_LINE_WIDTH,
  BOXGCMD_SET_LINE_CAP, BOXGCMD_SET_LINE_JOIN, BOXGCMD_SET_MITER_LIMIT,
  BOXGCMD_SET_DASH, BOXGCMD_SET_FILL_RULE, BOXGCMD_SET_SOURCE_RGBA,
  BOXGCMD_TEXT_PATH, BOXGCMD_TRANSLATE, BOXGCMD_SCALE, BOXGCMD_ROTATE,
  BOXGCMD_PATTERN_CREATE_RGB, BOXGCMD_PATTERN_CREATE_RGBA,
  BOXGCMD_PATTERN_CREATE_LINEAR, BOXGCMD_PATTERN_CREATE_RADIAL,
  BOXGCMD_PATTERN_CREATE_IMAGE,
  BOXGCMD_PATTERN_ADD_COLOR_STOP_RGB, BOXGCMD_PATTERN_ADD_COLOR_STOP_RGBA,
  BOXGCMD_PATTERN_SET_EXTEND, BOXGCMD_PATTERN_SET_FILTER,
  BOXGCMD_PATTERN_DESTROY, BOXGCMD_SET_SOURCE,
  BOXGCMD_EXT_SET_AUTO_BBOX, BOXGCMD_EXT_UNSET_BBOX,
  BOXGCMD_EXT_JOINARC_TO, BOXGCMD_EXT_ARC_TO,
  BOXGCMD_EXT_SET_FONT, BOXGCMD_EXT_TEXT_PATH, BOXGCMD_EXT_TRANSFORM,
  BOXGCMD_EXT_BORDER_EXCHANGE, BOXGCMD_EXT_BORDER_SAVE,
  BOXGCMD_EXT_BORDER_RESTORE, BOXGCMD_EXT_FILL_AND_STROKE,
  BOXG_NUM_CMDS

} BoxGCmd;

/** Possible types for arguments of a raw graphical command */
typedef enum {
  BOXGCMDARGKIND_INT,    /**< Integer: no transformation (I) */
  BOXGCMDARGKIND_REAL,   /**< Real: no transformation (R) */
  BOXGCMDARGKIND_STR,    /**< String: no transformation (S) */
  BOXGCMDARGKIND_POINT,  /**< Point: transformed at drawing (P) */
  BOXGCMDARGKIND_VECTOR, /**< Vector: transformed as a non applied
                              vector (V) */
  BOXGCMDARGKIND_REALP,  /**< Real couple: no transformation (p) */
  BOXGCMDARGKIND_WIDTH  /**< Transformed as a width (real number) (W) */

} BoxGCmdArgKind;

/** Maximum number of arguments among all the available commands. */
#define BOXG_MAX_NUM_CMD_ARGS 6

/** Used to store pointers to arguments */
typedef union {
  void     *ptr;
  BoxInt   *i;
  BoxReal  *r,
           w;
  BoxStr   *s;
  BoxPoint *pp,
           p,
           v;
  BoxGObj  *o;

} BoxGCmdArg;

/** Signature of a raw command. */
typedef enum {
  BOXGCMDSIG_NONE,
  BOXGCMDSIG_I,
  BOXGCMDSIG_W,
  BOXGCMDSIG_R,
  BOXGCMDSIG_P,
  BOXGCMDSIG_V,
  BOXGCMDSIG_S,
  BOXGCMDSIG_PP,
  BOXGCMDSIG_RRR,
  BOXGCMDSIG_PPP,
  BOXGCMDSIG_PPPS,
  BOXGCMDSIG_RRRR,
  BOXGCMDSIG_RRRRR,
  BOXGCMDSIG_PPPRR,
  BOXGCMDSIG_PPPpS,
  BOXGCMDSIG_PPPPRR,
  BOXGCMDSIG_W_

} BoxGCmdSig;

/** Function called while iterating over the commands. 
 * 'cmd' is the command, 'sig' is the signature, 'num_args' the number of
 * arguments of the command, 'kinds' an array of 'num_args' BoxGCmdArgKind
 * items with the argument kinds, 'args' an array of 'num_args' pointers to
 * the argument values, 'aux' a pointer to an auxiliary unset region of
 * memory which can contain up to 'num_args' BoxGCmdArg items (useful for
 * modifying the command arguments, if necessary). Finally, 'pass' is a pointer
 * passed to the function as given by BoxGCmdIter_Iter.
 */
typedef BoxTask (*BoxGCmdIter)(BoxGCmd cmd, BoxGCmdSig sig, int num_args,
                               BoxGCmdArgKind *kinds, void **args,
                               BoxGCmdArg *aux, void *pass);

/** Return a BoxGObjKind which can contain a command argument of the given
 * type. 
 */
BoxGObjKind BoxGCmdArgKind_To_Obj_Kind(BoxGCmdArgKind t);

/** Get the signature of the given command. */
BoxGCmdSig BoxGCmdSig_Get(BoxGCmd cmd_id);

/** Get the number of arguments for the command with the given signature. */
int BoxGCmdSig_Get_Num_Args(BoxGCmdSig sig);

/** Write in kinds[0], kinds[1], ... the BoxGCmdArgKind for all the arguments
 * of the command with the given signature. Returns the number of arguments
 * of the command.
 */
int BoxGCmdSig_Get_Arg_Kinds(BoxGCmdSig sig, BoxGCmdArgKind *kinds);

/** Iterate over the commands stored inside an Obj object. The iterator 'iter'
 * is called for every command in the Obj, passing the pointer 'pass'.
 */
BoxTask BoxGCmdIter_Iter(BoxGCmdIter iter, BoxGObj *obj, void *pass);

/** Append the commands in 'src' to 'dst', filtering them through the iterator
 * 'filter'. The iterator can filter the command only by changing the
 * arguments (for example applying a scale transformation to all the points).
 */
BoxTask BoxGCmdIter_Filter_Append(BoxGCmdIter filter,
                                  BoxGObj *dst, BoxGObj *src, void *pass);

#endif /* _BOX_LIBG_RAW_H */
