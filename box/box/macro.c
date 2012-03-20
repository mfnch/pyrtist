/***************************************************************************
 *   Copyright (C) 2012 by Matteo Franchin                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* For test purposes, build as:

     gcc -o testmacro -I.. -Wall array.c error.c mem.c print.c messages.c \
       msgbase.c strutils.c srcpos.c -DBUILD_TEST macro.c

   and run without arguments: ./testmacro
 */

#include <ctype.h>

#include <box/types.h>
#include <box/array.h>
#include <box/macro.h>


#define BOXMACRO_MAX_ARGS 3

struct BoxMacro_struct {
  char   *text;     /**< A NUL terminated string */
  char   *name;     /**< A pointer to the macro name in 'text' */
  char   *args[BOXMACRO_MAX_ARGS];
                    /**< The arguments of the macro in 'text' */
  char   *delim;    /**< First space after argument */
  size_t num_args;  /**< Number of arguments in 'args' */
};


/* Currently we implement this parser just as a state machine.
 * Later we may want to use a flex scanner for this.
 */
BoxMacroErr BoxMacro_Parse(BoxMacro *bm) {
  char *text = bm->text;
  int keep_parsing;
  size_t i;

  /* Parser states */
  enum {
    MY_WAIT_NAME,
    MY_GET_NAME,
    MY_GOT_NAME,
    MY_WAIT_ARG,
    MY_GET_ARG,
    MY_GET_STRING,
    MY_GET_STRING_ESCAPE

  } state = MY_WAIT_NAME;

  bm->name = NULL;
  bm->num_args = 0;

  /* Main parse loop */
  for (i = 0, keep_parsing = 1; keep_parsing; i++) {
    char this_char = text[i];
    keep_parsing = (text[i] != '\0');

    switch (state) {
    case MY_WAIT_NAME:
      if (!isspace(this_char)) {
        if (!isalpha(this_char))
          return BOXMACROERR_NAME;
        bm->name = & text[i];
        state = MY_GET_NAME;
      }
      break;

    case MY_GET_NAME:
      if (!isalnum(this_char) && this_char != '_' && this_char != '-') {
        if (!isspace(this_char) && this_char != ':' && this_char != '\0')
          return BOXMACROERR_COLON;
        text[i] = '\0';
        state = (this_char == ':') ? MY_WAIT_ARG : MY_GOT_NAME;
      }
      break;

    case MY_GOT_NAME:
      if (!isspace(this_char)) {
        if (this_char != ':' && this_char != '\0')
          return BOXMACROERR_COLON;
        state = MY_WAIT_ARG;
      }
      break;

    case MY_WAIT_ARG:
      if (!isspace(this_char)) {
        if (this_char == ',' || this_char == '\0')
          text[i] = '\0';

        else {
          bm->delim = NULL;
          state = (this_char == '"') ? MY_GET_STRING : MY_GET_ARG;
        }

        if (bm->num_args >= BOXMACRO_MAX_ARGS)
          return BOXMACROERR_NUM_ARGS;
        bm->args[bm->num_args++] = & text[i];
      }
      break;

    case MY_GET_ARG:
      if (isspace(this_char)) {
        if (!bm->delim)
          bm->delim = & text[i];

      } else if (this_char == ',' || this_char == '\0') {
        char *delim = bm->delim ? bm->delim : & text[i];
        *delim = '\0';
        state = MY_WAIT_ARG;

      } else {
        bm->delim = NULL;
        if (this_char == '"')
          state = MY_GET_STRING;
      }
      break;

    case MY_GET_STRING:
      if (this_char == '"') {
        bm->delim = NULL;
        state = MY_GET_ARG;

      } else if (this_char == '\\')
        state = MY_GET_STRING_ESCAPE;
 
      else if (this_char == '\0')
        return BOXMACROERR_STRING;
      break;

    case MY_GET_STRING_ESCAPE:
      if (this_char != '\0')
        state = MY_GET_STRING;
      else
        return BOXMACROERR_STRING;
      break;

    default:
      return BOXMACROERR_UNKNOWN;
    }    
  }

  return 0;
}

#ifdef BUILD_TEST

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static void My_Test_Macro_Parse(const char *input, BoxMacroErr status,
                                const char *name, size_t num_args,
                                const char **args) {
  BoxMacro bm;
  bm.text = strdup(input);
  assert(bm.text != NULL);
  BoxMacroErr my_status = BoxMacro_Parse(& bm);

  if (my_status == status) {
    if (bm.name == name || strcmp(bm.name, name) == 0) {
      size_t max_num_args = (num_args >= bm.num_args) ? num_args : bm.num_args;
      size_t i;
      int diff_num_args = (bm.num_args != num_args);

      if (diff_num_args)
        printf("Num. args: %ld (expected %ld)\n", bm.num_args, num_args);

      for (i = 0; i < max_num_args; i++) {
        if (diff_num_args || strcmp(bm.args[i], args[i]) != 0)
          printf("Arg. %ld: '%s' (expected '%s')\n", i,
                 (i < bm.num_args) ? bm.args[i] : "N/A",
                 (i < num_args) ? args[i] : "N/A");
      }
    } else
      printf("Name: '%s' (expected: '%s')\n", bm.name, name);

  } else
    printf("Status: %d (expected: %d)\n", my_status, status);

  free(bm.text);
}

int main(void) {
  My_Test_Macro_Parse("", BOXMACROERR_NAME, NULL, 0, (const char *[]) {});
  My_Test_Macro_Parse("macro-without-args", 0,
                      "macro-without-args", 0, (const char *[]) {});
  My_Test_Macro_Parse("  macro-without-args", 0,
                      "macro-without-args", 0, (const char *[]) {});
  My_Test_Macro_Parse("macro-without-args  ", 0,
                      "macro-without-args", 0, (const char *[]) {});
  My_Test_Macro_Parse("  macro-without-args   ", 0,
                      "macro-without-args", 0, (const char *[]) {});
  My_Test_Macro_Parse("  macro-without-args : ", 0,
                      "macro-without-args", 1, (const char *[]) {""});
  My_Test_Macro_Parse("  file  :  \"fds  af \"  ,  c  ", 0,
                      "file", 2, (const char *[]) {"\"fds  af \"", "c"});
  My_Test_Macro_Parse("  file  : \"fds \\\" af \"  ,  c  ", 0,
                      "file", 2, (const char *[]) {"\"fds \\\" af \"", "c"});
  exit(EXIT_SUCCESS);
}
#endif
