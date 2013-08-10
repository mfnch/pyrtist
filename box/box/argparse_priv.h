/****************************************************************************
 * Copyright (C) 2013 by Matteo Franchin                                    *
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

#ifndef _BOX_ARGPARSE_PRIV_H
#  define _BOX_ARGPARSE_PRIV_H

#  include <box/argparse.h>


struct BCArgParser_struct {
  char              *err_msg,
                    *prog,
                    *first;
  const char        **args,
                    **args_left;
  BCArgParserOption *opts;
  int               num_opts,
                    num_args,
                    num_args_left;
  char              opt_char;
};

#endif /* _BOX_ARGPARSE_PRIV_H */
