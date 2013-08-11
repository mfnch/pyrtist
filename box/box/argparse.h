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

#ifndef _BOX_ARGPARSE_H
#  define _BOX_ARGPARSE_H

#  include <box/types.h>


/**
 * @brief Argument parser.
 */
typedef struct BCArgParser_struct BCArgParser;

/**
 * @brief Descriptor for a command line option.
 */
typedef struct BCArgParserOption_struct BCArgParserOption;

/**
 * @brief Function used to notify that an option has been correctly parsed.
 */
typedef BoxBool (*BCArgParserFn)(BCArgParser *ap, BCArgParserOption *opt,
                                 const char *arg);

/**
 * @brief Descriptor for a command line option.
 */
struct BCArgParserOption_struct {
  char       opt_char; /**< Single character for the short form (e.g. -x). */
  const char *opt_str, /**< String for the long form (e.g. --long-form). */
             *opt_arg, /**< String given if the option has an argument. */
             *help;    /**< String used in the help screen. */
  BCArgParserFn
             fn;       /**< Used to notify that the option has been parsed. */
};

/**
 * @brief Create a new argument parser.
 *
 * @param options A #BCArgParserOption array describing the options to parse.
 * @param num_options The size of the @p options array.
 * @return A new argument parser object or @c NULL in case of errors.
 */
BOXEXPORT BCArgParser *
BCArgParser_Create(BCArgParserOption *options, int num_options);

/**
 * @param Destroy an argument parser created with BCArgParser_Create().
 */
BOXEXPORT void
BCArgParser_Destroy(BCArgParser *ap);

/**
 * @brief Use the argument parser to parse the given arguments.
 *
 * @param ap The argument parser to use.
 * @param argv A list of pointers to the arguments (similar to C's main()).
 * @param argc The number of arguments in @p argv (similar to C's main()).
 * @return Whether parsing was successful.
 */
BOXEXPORT BoxBool
BCArgParser_Parse(BCArgParser *ap, const char **argv, int argc);

/**
 * @brief Set the error message in the argument parser.
 *
 * @param ap The argument parser.
 * @param err_msg A newly allocated string.
 *
 * @note The caller passes the responsability to deallocate the memory
 *   occupied by the error message to the argument parser, which will use
 *   Box_Mem_Free() to do this.
 */
BOXEXPORT void
BCArgParser_Set_Err_Msg(BCArgParser *ap, char *err_msg);

/**
 * @brief Return the last error message occurred during argument parsing.
 *
 * @param ap The argument parser.
 * @return A newly allocated string containing the message or @c NULL if
 *   no errors occurred.
 *
 * @note The caller is given the responsability to deallocate the memory
 *   occupied by the error message string with Box_Mem_Free().
 */
BOXEXPORT char *
BCArgParser_Get_Err_Msg(BCArgParser *ap);

/**
 * @brief Set the stuff pointer in #BCArgParser.
 *
 * The stuff pointer is a pointer which can be stored inside the #BCArgParser
 * object and is ignored by it (it is not used for parsing). The pointer can
 * be retrieved with BCArgParser_Get_stuff() and can be used to pass
 * information to the parse handlers #BCArgParserFn.
 */
BOXEXPORT void
BCArgParser_Set_Stuff(BCArgParser *ap, void *stuff);

/**
 * @brief Get the stuff pointer as set by BCArgParser_Set_Stuff().
 *
 * @see BCArgParser_Set_Stuff
 */
BOXEXPORT void *
BCArgParser_Get_Stuff(BCArgParser *ap);

/**
 * @brief Get the arguments left after parse.
 *
 * @param ap Object from which to extract the arguments.
 * @param args_left This is written with the pointer to the arguments left.
 * @param num_args_left This is written with the number of arguments left.
 * @return Whether there are any arguments left.
 */
BOXEXPORT BoxBool
BCArgParser_Get_Args_Left(BCArgParser *ap,
                          const char ***args_left, int *num_args_left);

#endif /* _BOX_ARGPARSE_H */
