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

#ifndef _PSFONTS_H
#  define _PSFONTS_H

#  include "graphic.h"

/** Returns the full name of the postscript font whose name is 'font_name',
 * and whose style is described by 's' and 'w'.
 * The function returns NULL if there is no default postscript font
 * corresponding to the given description.
 */
const char *ps_font_get_full_name(const char *font_name,
                                  FontSlant s, FontWeight w);

/** Returns details about the postscript font whose full name is 'full_name'.
 * The details are returned in '*name' (the main name of the font),
 * '*s' (the slant attribute of the font) and in '*w' (the weight attribute).
 * If one or more of these pointers are set to NULL, the corresponding
 * attribute is not returned.
 * The function returns 0 if the font has not been found (1 otherwise).
 */
int ps_font_get_info(const char *full_name,
                     const char **name, FontSlant *s, FontWeight *w);

/** Print a list of the available postscript fonts to the stream 'out'.
 * The list is formatted as follows:
 *   Font1 (Normal, Bold, Italic, Bold+Italic)
 *   Font2 (Normal, Italic)
 *   ...
 */
void ps_print_available_fonts(FILE *out);

/** Returns the full name of one available font, to be used
 * as the default font.
 */
const char *ps_default_font_full_name(void);

#endif
