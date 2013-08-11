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

#include <box/argparse_priv.h>

#include <box/mem.h>
#include <box/print.h>

#include <stdlib.h>
#include <string.h>


/* Shorthand used in the code below to set BCArgParser.err_msg */
#define MY_ERR(ap, ...)                                  \
  BCArgParser_Set_Err_Msg((ap), Box_SPrintF(__VA_ARGS__))

void BCArgParser_Init(BCArgParser *ap, BCArgParserOption *options,
                      int num_options)
{
  ap->stuff = NULL;
  ap->err_msg = NULL;
  ap->prog = NULL;
  ap->first = NULL;
  ap->args = NULL;
  ap->opts = options;
  ap->num_opts = num_options;
  ap->num_args = 0;
  ap->opt_char = '-';
}

void BCArgParser_Finish(BCArgParser *ap)
{
  if (ap) {
    Box_Mem_Free(ap->prog);
    Box_Mem_Free(ap->first);
    Box_Mem_Free(ap->err_msg);
  }
}

BCArgParser *BCArgParser_Create(BCArgParserOption *options, int num_options)
{
  BCArgParser *ap = Box_Mem_Alloc(sizeof(BCArgParser));
  if (ap)
    BCArgParser_Init(ap, options, num_options);
  return ap;
}

void BCArgParser_Destroy(BCArgParser *ap)
{
  BCArgParser_Finish(ap);
  Box_Mem_Free(ap);
}

/**
 * @brief Parse a short option.
 *
 * Examples: -a, -abc, -oarg, -o arg, -abcoarg.
 * Where the string "arg" in the examples is to be interpreted as the argument
 * for the option -o.
 * @return The number of argument parsed. 0 in case of errors.
 */
static int My_Parse_Short(BCArgParser *ap, int arg_index)
{
  const char *cp, *arg = ap->args[arg_index];
  char c;

  for (cp = & arg[1]; (c = *cp); cp++) {
    int opt_index;
    BCArgParserOption *opt;

    /* Short option? Try to determine which one... */
    for (opt_index = 0, opt = NULL; opt_index < ap->num_opts; opt_index++) {
      if (ap->opts[opt_index].opt_char == c) {
        opt = & ap->opts[opt_index];
        break;
      }
    }

    if (!opt) {
      MY_ERR(ap, "Unrecognized option `-%c'", c);
      return 0;
    }

    if (opt->opt_arg) {
      /* The option takes an argument. We have two cases: (1) the argument just
       * follows the option character or (2) the argument to the option is in
       * the next args item.
       */
      const char *rhs = cp + 1;
      if (*rhs) {
        if (opt->fn(ap, opt, cp + 1))
          return 1;
      } else {
        if (arg_index + 1 >= ap->num_args) {
          MY_ERR(ap, "Missing argument to option `-%c'", c);
          return 0;
        }

        if (opt->fn(ap, opt, ap->args[arg_index + 1]))
          return 2;
      }

      if (!ap->err_msg)
        MY_ERR(ap, "Failure while parsing option `-%c'", c);
      return 0;

    } else if (!opt->fn(ap, opt, NULL))
      return 0;
  }

  return 1;
}

/**
 * @brief Parse a long option.
 *
 * Examples: --long-opt, --arg-opt=argument, --arg-opt argument.
 * @return The number of argument parsed. 0 in case of errors.
 */
static int My_Parse_Long(BCArgParser *ap, int arg_index)
{
  const char *arg = & ap->args[arg_index][2], *arg_end;
  size_t arg_len;
  int opt_index;
  BCArgParserOption *opt;

  arg_end = strchr(arg, '=');
  arg_len = (arg_end) ? arg_end - arg : strlen(arg);

  for (opt_index = 0, opt = NULL; opt_index < ap->num_opts; opt_index++) {
    if (strncmp(arg, ap->opts[opt_index].opt_str, arg_len) == 0) {
      if (opt) {
        MY_ERR(ap, "Ambiguous option `--%s': cannot choose between `--%s' and "
               "`--%s'", arg, opt->opt_str, ap->opts[opt_index].opt_str);
        return  0;
      }
      opt = & ap->opts[opt_index];
    }
  }

  if (!opt) {
    MY_ERR(ap, "Unrecognized option `--%s'", arg);
    return 0;
  }

  /* Option requires argument? */
  if (opt->opt_arg) {
    /* Yes. What syntax? --option=arg or --option arg? */
    if (arg_end) {
      /* Syntax is --option=arg.*/
      if (opt->fn(ap, opt, arg_end + 1))
        return 1;
    } else {
      /* Syntax is --option arg.*/
      if (arg_index + 1 >= ap->num_args) {
        MY_ERR(ap, "Missing argument of option `--%s'", arg);
        return 0;
      }

      if (opt->fn(ap, opt, ap->args[arg_index + 1]))
        return 2;
    }

    if (!ap->err_msg)
      MY_ERR(ap, "Failure while parsing option `--%s'", arg);
    return 0;
  }
 

  /* No argument required. */
  if (arg_end) {
    MY_ERR(ap, "Option `--%s' takes no argument, but you provided `--%s'",
           opt->opt_str, arg);
    return 0;
  }

  /* No argument required. */
  return (opt->fn(ap, opt, NULL)) ? 1 : 0;
}

/* Use the argument parser to parse the given arguments. */
BoxBool BCArgParser_Parse(BCArgParser *ap, const char **argv, int argc)
{
  int arg_index;
  char opt_char = ap->opt_char;

  Box_Mem_Free(ap->err_msg);
  ap->err_msg = NULL;
  ap->args = argv;
  ap->num_args = argc;
  ap->prog = (argc >= 1) ? Box_Mem_Strdup(argv[0]) : NULL;

  for (arg_index = 1; arg_index < argc; arg_index++) {
    const char *arg = argv[arg_index];
    size_t sz = strlen(arg);
    int num_parsed = 0;

    /* Paranoia... */
    if (sz == 0)
      continue;

    if (sz < 2 || arg[0] != opt_char) {
      /* This is the first non option argument. Record it and stop parsing. */
      ap->first = Box_Mem_Strdup(arg);
      break;
    }

    /* Options have at least two characters, the first being -. */ 
    if (arg[1] == opt_char) {
      /* Option terminator? Just quit! */
      if (sz == 2) {
        arg_index++;
        break;
      }
      num_parsed = My_Parse_Long(ap, arg_index);
    } else
      num_parsed = My_Parse_Short(ap, arg_index);

    if (num_parsed < 1)
      break;
    arg_index += num_parsed - 1;
  }

  if (ap->err_msg)
    return BOXBOOL_FALSE;

  ap->num_args_left = argc - arg_index;
  if (ap->num_args_left > 0)
    ap->args_left = & argv[arg_index];

  return BOXBOOL_TRUE;
}

void BCArgParser_Set_Err_Msg(BCArgParser *ap, char *err_msg) {
  Box_Mem_Free(ap->err_msg);
  ap->err_msg = err_msg;
}

char *BCArgParser_Get_Err_Msg(BCArgParser *ap)
{
  char *err_msg = ap->err_msg;
  ap->err_msg = NULL;
  return err_msg;
}

void BCArgParser_Set_Stuff(BCArgParser *ap, void *stuff)
{
  ap->stuff = stuff;
}

void *BCArgParser_Get_Stuff(BCArgParser *ap)
{
  return ap->stuff;
}

BoxBool BCArgParser_Get_Args_Left(BCArgParser *ap,
                                  const char ***args_left, int *num_args_left)
{
  if (ap->num_args_left > 0) {
    *num_args_left = ap->num_args_left;
    *args_left = ap->args_left;
    return BOXBOOL_TRUE;
  }
  return BOXBOOL_FALSE;
}

#ifdef TEST_ARGPARSE

#include <stdio.h>

BoxBool My_Parse_Option(BCArgParser *ap, BCArgParserOption *opt,
                        const char *arg)
{
  printf("Option '%s', arg='%s'\n", opt->opt_str, (arg) ? arg : "");
  return BOXBOOL_TRUE;
}

int main(int argc, const char **argv) {
  BCArgParser ap;
  BCArgParserOption opts[] = {
    {'h', "help", NULL, "Show usage.", My_Parse_Option},
    {'v', "version", NULL, "Show version.", My_Parse_Option}
  };

  BCArgParser_Init(& ap,  & opts[0],
                   sizeof(opts)/sizeof(BCArgParserOption));
  if (!BCArgParser_Parse(& ap, argv, argc))
    printf("Error parsing options: %s\n", ap.err_msg);
  BCArgParser_Finish(& ap);
  exit(EXIT_SUCCESS);
}

#endif



