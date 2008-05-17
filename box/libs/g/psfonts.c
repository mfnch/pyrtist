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

#include <stdlib.h>

/* It seems there are 35 fonts that are guaranteed to be available.
 * Others may be available, but maybe not. Therefore we stick only to these.
 */
static const char *guaranteed_ps_fonts[] = {
  "AvantGarde-Book",
  "AvantGarde-BookOblique",
  "AvantGarde-Demi",
  "AvantGarde-DemiOblique",
  "Bookman-Demi",
  "Bookman-DemiItalic",
  "Bookman-Light",
  "Bookman-LightItalic",
  "Courier",
  "Courier-Bold",
  "Courier-BoldOblique",
  "Courier-Oblique",
  "Helvetica",
  "Helvetica-Bold",
  "Helvetica-BoldOblique",
  "Helvetica-Narrow",
  "Helvetica-Narrow-Bold",
  "Helvetica-Narrow-BoldOblique",
  "Helvetica-Narrow-Oblique",
  "Helvetica-Oblique",
  "NewCenturySchlbk-Bold",
  "NewCenturySchlbk-BoldItalic",
  "NewCenturySchlbk-Italic",
  "NewCenturySchlbk-Roman",
  "Palatino-Bold",
  "Palatino-BoldItalic",
  "Palatino-Italic",
  "Palatino-Roman",
  "Symbol",
  "Times-Bold",
  "Times-BoldItalic",
  "Times-Italic",
  "Times-Roman",
  "ZapfChancery-MediumItalic",
  "ZapfDingbats",
  (char *) NULL
};

const char *findfont(const char *font_name) {
  return font_name;
}
