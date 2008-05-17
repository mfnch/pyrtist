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
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "graphic.h"
#include "psfonts.h"

/* It seems there are 35 fonts that are guaranteed to be available.
 * Others may be available, but maybe not. Therefore we stick only to these.
 */
static struct ps_font_desc {
  const char *full_name;
  const char *name;
  FontSlant slant;
  FontWeight weight;

} guaranteed_ps_fonts[] = {
  {"AvantGarde-Book",        "AvantGarde-Book", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"AvantGarde-BookOblique", "AvantGarde-Book", FONT_SLANT_OBLIQUE, FONT_WEIGHT_NORMAL},
  {"AvantGarde-Demi",        "AvantGarde-Demi", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"AvantGarde-DemiOblique", "AvantGarde-Demi", FONT_SLANT_OBLIQUE, FONT_WEIGHT_NORMAL},
  {"Bookman-Demi",        "Bookman-Demi", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"Bookman-DemiItalic",  "Bookman-Demi", FONT_SLANT_ITALIC, FONT_WEIGHT_NORMAL},
  {"Bookman-Light",       "Bookman-Light", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"Bookman-LightItalic", "Bookman-Light", FONT_SLANT_ITALIC, FONT_WEIGHT_NORMAL},
  {"Courier",             "Courier", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"Courier-Oblique",     "Courier", FONT_SLANT_OBLIQUE, FONT_WEIGHT_NORMAL},
  {"Courier-Bold",        "Courier", FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD},
  {"Courier-BoldOblique", "Courier", FONT_SLANT_OBLIQUE, FONT_WEIGHT_BOLD},
  {"Helvetica",             "Helvetica", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"Helvetica-Oblique",     "Helvetica", FONT_SLANT_OBLIQUE, FONT_WEIGHT_NORMAL},
  {"Helvetica-Bold",        "Helvetica", FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD},
  {"Helvetica-BoldOblique", "Helvetica", FONT_SLANT_OBLIQUE, FONT_WEIGHT_BOLD},
  {"Helvetica-Narrow",             "Helvetica-Narrow", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"Helvetica-Narrow-Oblique",     "Helvetica-Narrow", FONT_SLANT_OBLIQUE, FONT_WEIGHT_NORMAL},
  {"Helvetica-Narrow-Bold",        "Helvetica-Narrow", FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD},
  {"Helvetica-Narrow-BoldOblique", "Helvetica-Narrow", FONT_SLANT_OBLIQUE, FONT_WEIGHT_BOLD},
  {"NewCenturySchlbk-Roman",      "NewCenturySchlbk", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"NewCenturySchlbk-Italic",     "NewCenturySchlbk", FONT_SLANT_ITALIC, FONT_WEIGHT_NORMAL},
  {"NewCenturySchlbk-Bold",       "NewCenturySchlbk", FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD},
  {"NewCenturySchlbk-BoldItalic", "NewCenturySchlbk", FONT_SLANT_ITALIC, FONT_WEIGHT_BOLD},
  {"Palatino-Roman",      "Palatino", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"Palatino-Italic",     "Palatino", FONT_SLANT_ITALIC, FONT_WEIGHT_NORMAL},
  {"Palatino-Bold",       "Palatino", FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD},
  {"Palatino-BoldItalic", "Palatino", FONT_SLANT_ITALIC, FONT_WEIGHT_BOLD},
  {"Symbol",           "Symbol", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"Times-Roman",      "Times", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {"Times-Italic",     "Times", FONT_SLANT_ITALIC, FONT_WEIGHT_NORMAL},
  {"Times-Bold",       "Times", FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD},
  {"Times-BoldItalic", "Times", FONT_SLANT_ITALIC, FONT_WEIGHT_BOLD},
  {"ZapfChancery-MediumItalic", "ZapfChancery", FONT_SLANT_ITALIC, FONT_WEIGHT_NORMAL},
  {"ZapfDingbats", "ZapfDingbats", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL},
  {(char *) NULL, "", FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL}
};

static struct ps_font_desc *ps_find_font(const char *font_name,
                                         FontSlant s, FontWeight w) {
  struct ps_font_desc *known_font = guaranteed_ps_fonts;
  for(; known_font->full_name != (const char *) NULL; known_font++) {
    if (strcasecmp(known_font->full_name, font_name) == 0) {
      return known_font;

    } else {
      if (strcasecmp(known_font->name, font_name) == 0) {
        if (known_font->slant == s && known_font->weight == w)
          return known_font;
      }
    }
  }
  return (struct ps_font_desc *) NULL;
}

const char *ps_font_get_full_name(const char *font_name,
                                  FontSlant s, FontWeight w) {
  struct ps_font_desc *fd = ps_find_font(font_name, s, w);
  if (fd == (struct ps_font_desc *) NULL) return (const char *) NULL;
  return fd->full_name;
}

int ps_font_get_info(const char *full_name,
                     const char **name, FontSlant *s, FontWeight *w) {
  FontSlant dummy_s = FONT_SLANT_NORMAL;
  FontWeight dummy_w = FONT_WEIGHT_NORMAL;
  const char *dummy_name;
  struct ps_font_desc *fd;

  if (s == (FontSlant *) NULL) s = & dummy_s;
  if (w == (FontWeight *) NULL) w = & dummy_w;
  if (name != (const char **) NULL) name = & dummy_name;

  fd = ps_find_font(full_name, *s, *w);
  if (fd == (struct ps_font_desc *) NULL) {
    *name = (const char *) NULL;
    return 0;
  }
  *name = fd->name;
  *s = fd->slant;
  *w = fd->weight;
  return 1;
}

static const char *font_type(FontSlant s, FontWeight w) {
  int s_num, w_num;
  const char *types[] = {"Normal", "Bold",
                         "Italic", "Bold+Italic",
                         "Oblique", "Bold+Oblique"};
  switch(s) {
  case FONT_SLANT_NORMAL:  s_num = 0; break;
  case FONT_SLANT_ITALIC:  s_num = 1; break;
  case FONT_SLANT_OBLIQUE: s_num = 2; break;
  default: assert(0);
  }

  switch(w) {
  case FONT_WEIGHT_NORMAL:  w_num = 0; break;
  case FONT_WEIGHT_BOLD:  w_num = 1; break;
  default: assert(0);
  }

  return types[s_num*2 + w_num];
}

void ps_print_available_fonts(FILE *out) {
  const char *previous_name = (char *) NULL;
  struct ps_font_desc *known_font = guaranteed_ps_fonts;
  for(; known_font->full_name != (const char *) NULL; known_font++) {
    int same_as_previous;
    if (previous_name == (char *) NULL)
      same_as_previous = 0;
    else
      same_as_previous = (strcmp(known_font->name, previous_name) == 0);

    if (!same_as_previous) {
      if (previous_name != (const char *) NULL)
        fprintf(out, ")\n");

        fprintf(out, "%s (%s",
                known_font->name,
                font_type(known_font->slant, known_font->weight));

    } else
        fprintf(out, ", %s",
                font_type(known_font->slant, known_font->weight));

    previous_name = known_font->name;
  }
  if (previous_name != (const char *) NULL)
    fprintf(out, ")\n");
}

const char *ps_default_font_full_name(void) {
  const char *full_name;
  full_name = ps_font_get_full_name("Courier",
                                    FONT_SLANT_NORMAL,
                                    FONT_WEIGHT_NORMAL);
  assert(full_name != (const char *) NULL);
  return full_name;
}
