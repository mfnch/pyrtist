/***************************************************************************
 *   Copyright (C) 2006-2010 by Matteo Franchin (fnch@users.sf.net)        *
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

#include <box/argparse.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef BOX_RELEASE_DATE
#  define RELEASE_STRING "("BOX_RELEASE_DATE")"
#else
#  define RELEASE_STRING
#endif

#ifdef PACKAGE_STRING
#  define BOX_VERSTR PACKAGE_STRING
#else
#  define BOX_VERSTR "box"
#endif

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "mem.h"
#include "strutils.h"
#include "array.h"
#include "vm_priv.h"
#include "vmproc.h"
#include "vmsym.h"
#include "vm2c.h"
#include "registers.h"
#include "paths.h"
#include "compiler.h"

/* Visualizzo questo messaggio quando ho errori nella riga di comando: */
#define CMD_LINE_HELP "Try '%s -h' to get some help!"

/**
 * @brief Structure used to accumulate information from command line parsing.
 */
typedef struct {
  struct {
    unsigned int show_help :1,
                 show_version :1,
                 silent :1,
                 only_errors :1,
                 verbose :1,
                 stdin : 1,
                 force_exec : 1,
                 execute :1;
  } flags;
  const char     *prog_name,
                 *file_input,
                 *file_output,
                 *file_setup,
                 *query;
} MyArgParserResult;


/* The BoxVM object where the source is compiled */
static BoxVM *target_vm = NULL;
static BoxPaths box_paths;

/* Function called with option `--query' to show configuration parameters. */
static void My_Exec_Query(const char *query) {
  char *pkg_path = NULL,
       *lib_path = NULL;
  int exit_status = EXIT_FAILURE;

  Box_Get_Bltin_Pkg_And_Lib_Paths(& pkg_path, & lib_path);

  struct {
    char *name;
    char *value;
  } *v, vars[] = {
#ifdef PACKAGE_VERSION
    {"VERSION", PACKAGE_VERSION},
#endif
#ifdef BOX_RELEASE_DATE
    {"RELEASE_DATE", BOX_RELEASE_DATE},
#endif
#if defined(__DATE__) && defined(__TIME__)
    {"BUILD_DATE", __DATE__", "__TIME__},
#endif
#ifdef BUILTIN_LIBRARY_PATH
    {"BUILTIN_LIBRARY_PATH",
     (lib_path != NULL) ? lib_path : BUILTIN_LIBRARY_PATH},
#endif
#ifdef BUILTIN_PKG_PATH
    {"BUILTIN_PKG_PATH",
     (pkg_path != NULL) ? pkg_path : BUILTIN_PKG_PATH},
#endif
#ifdef C_INCLUDE_PATH
    {"C_INCLUDE_PATH", C_INCLUDE_PATH},
#endif
#ifdef C_LIBRARY_PATH
    {"C_LIBRARY_PATH", C_LIBRARY_PATH},
#endif
#ifdef BOXCORE_NAME
    {"BOXCORE_NAME", BOXCORE_NAME},
#endif
    {NULL, NULL}
  };

  if (strcasecmp(query, "list") == 0) {
    for (v = & vars[0]; v->name != NULL; v++)
      printf("%s\n", v->name);
    exit_status = EXIT_SUCCESS;

  } else if (strcasecmp(query, "all") == 0) {
    for (v = & vars[0]; v->name != NULL; v++)
      printf("%s=\"%s\"\n", v->name, v->value);
    exit_status = EXIT_SUCCESS;

  } else {
    for (v = & vars[0]; v->name != NULL; v++) {
      if (strcasecmp(v->name, query) == 0) {
        printf("%s\n", v->value);
        exit_status = EXIT_SUCCESS;
        break;
      }
    }
  }

  Box_Mem_Free(pkg_path);
  Box_Mem_Free(lib_path);

  exit(exit_status);
}

/* Show a message and exit the program. */
void Main_Error_Exit(MyArgParserResult *result, char *msg) {
  if (msg && !result->flags.silent) {
    fprintf(stderr, msg, result->prog_name);
    fprintf(stderr, "\n");
  }

  exit(EXIT_FAILURE);
}

/* Parser handler. Used with BCArgParser. */
BoxBool My_Parse_Option(BCArgParser *ap, BCArgParserOption *opt,
                        const char *arg)
{
  MyArgParserResult *result = BCArgParser_Get_Stuff(ap);

  switch (opt->opt_char) {
  case 'f':
    result->flags.force_exec ^= 1;
    break;
  case 'e':
    result->flags.verbose = result->flags.silent = 0;
    result->flags.only_errors = 1;
    break;
  case 'h':
    result->flags.show_help = 1;
    break;
  case 'I':
    BoxPaths_Add_Pkg_Dir(& box_paths, arg);
    break;
  case 'i':
    result->flags.stdin = 1;
    break;
  case 'L':
    BoxPaths_Add_Lib_Dir(& box_paths, arg);
    break;
  case 'l':
    BoxPaths_Add_Lib(& box_paths, arg);
    break;
  case 'o':
    result->file_output = arg;
    break;
  case 'q':
    result->query = arg;
    break;
  case 'S':
    result->file_setup = arg;
    break;
  case 's':
    result->flags.only_errors = result->flags.verbose = 0;
    result->flags.silent = 1;
    break;
  case 't':
    result->flags.execute ^= 1;
    break;
  case 'V':
    result->flags.silent = result->flags.only_errors = 0;
    result->flags.verbose ^= 1;
    break;
  case 'v':
    result->flags.show_version = 1;
    break;
  default:
    BCArgParser_Set_Err_Msg(ap, Box_Mem_Strdup("Unexpected failure"));
    return BOXBOOL_FALSE;
  }

  return BOXBOOL_TRUE;
}

BCArgParserOption my_opts[] = {
  {'h', "help", NULL, "Show this help screen", My_Parse_Option},
  {'v', "version", NULL, "Show the program version", My_Parse_Option},
  {'i', "stdin", NULL, "Read input from standard input", My_Parse_Option},
  {'o', "output", "FILENAME", "Output file", My_Parse_Option},
  {'S', "setup", "FILENAME", "Setup file", My_Parse_Option},
  {'l', "library", "LIBNAME", "Add a new C library to be used when linking",
   My_Parse_Option},
  {'L', "lib-path", "PATH", "Add directory to the list of directories "
   "searched with -l", My_Parse_Option},
  {'I', "include-path", "PATH", "Add a new directory to be searched when "
   "including files", My_Parse_Option},
  {'t', "test", NULL, "Test mode: compilation with no execution",
   My_Parse_Option},
  {'f', "force", NULL, "Force execution, even when warning messages "
   "are shown", My_Parse_Option},
  {'s', "silent", NULL, "Do not show any messages", My_Parse_Option},
  {'e', "errors", NULL, "Show only error messages", My_Parse_Option},
  {'V', "verbose", NULL, "Show all the messages, also warning messages",
   My_Parse_Option},
  {'q', "query", "PARNAME", "Show configuration parameters", My_Parse_Option}
};

void Main_Show_Help(void) {
  printf(
  BOX_VERSTR " " RELEASE_STRING " - Language to describe graphic figures.\n"
  "Created and implemented by Matteo Franchin.\n\n"
  "USAGE: " PROGRAM_NAME " [options] inputfile arg1 arg2 ...\n\n"
  "inputfile is the name of the Box source file.\n"
  "arg1, arg2, ... are the arguments passed to inputfile.\n"
  "Options can be one or more of:\n"
  " -h, --help          show this help screen\n"
  " -v, --version       show the program version and exit\n"
  " -i, --stdin         read the input from standard input\n"
  " -o, --output FILE   compile to filename (refuse to overwrite it)\n"
  " -S, --setup FILE    this file will be included automatically at the beginning\n"
  " -l, --library LIB   add a new C library to be used when linking\n"
  " -L, --lib-path DIR  add dir to the list of directories to be searched for -l\n"
  " -I, --include-path DIR add a new directory to be searched when including files\n"
  " -t, --test          just a test: compilation with no execution\n"
  " -f, --force         force execution, even if warning messages have been shown\n"
  " -V, --verbose       show all the messages, also warning messages\n"
  " -e, --errors        show only error messages\n"
  " -s, --silent        do not show any messages\n"
  );
  exit(EXIT_SUCCESS);
}

/* Show version information and exit. */
void Main_Show_Version(void)
{
  printf(BOX_VERSTR "\n");
  exit(EXIT_SUCCESS);
}

static BoxTask My_Stage_Init(void) {
  BoxPaths_Init(& box_paths);
  /* Initialisation of the message module */
  Msg_Main_Init(MSG_LEVEL_WARNING);
  return BOXTASK_OK;
}

static void My_Stage_Finalize(void) {
  if (target_vm != NULL)
    BoxVM_Destroy(target_vm);

  BoxPaths_Finish(& box_paths);

  Msg_Main_Destroy();
}

static BoxTask
My_Stage_Parse_Command_Line(MyArgParserResult *result,
                            int argc, const char **argv)
{
  BCArgParser *ap;
  const char **args_left;
  int num_args_left;

  MSG_CONTEXT_BEGIN("Reading the command line options");

  ap = BCArgParser_Create(& my_opts[0],
                          sizeof(my_opts)/sizeof(BCArgParserOption));
  if (!ap)
    MSG_FATAL("Cannot initialize command line argument parser");

  result->flags.show_help = 0;
  result->flags.show_version = 0;
  result->flags.silent = 0;
  result->flags.only_errors = 0;
  result->flags.verbose = 0;
  result->flags.stdin = 0;
  result->flags.force_exec = 0;
  result->flags.execute = 1;
  result->prog_name = (argc >= 1) ? argv[0] : "box";
  result->file_input = NULL;
  result->file_output = NULL;
  result->file_setup = NULL;
  result->query = NULL;
  BCArgParser_Set_Stuff(ap, result);

  if (!BCArgParser_Parse(ap, argv, argc)) {
    char *err_msg = BCArgParser_Get_Err_Msg(ap);
    if (err_msg)
      MSG_ERROR("%~s", err_msg);
    else
      MSG_ERROR("Error while parsing the command line arguments.");
    BCArgParser_Destroy(ap);
    Main_Error_Exit(result, NULL);
  }

  if (BCArgParser_Get_Args_Left(ap, & args_left, & num_args_left)) {
    result->file_input = args_left[0];
  }

  BCArgParser_Destroy(ap);
  MSG_CONTEXT_END();
  return BOXTASK_OK;
}

static BoxTask My_Stage_Interpret_Command_Line(MyArgParserResult *result) {
  /* Controllo se e' stata specificata l'opzione di help */
  if (result->flags.show_help)
    Main_Show_Help();

  if (result->flags.show_version)
    Main_Show_Version();

  if (result->query)
    My_Exec_Query(result->query);

  /* Re-inizializzo la gestione dei messaggi! */
  if (result->flags.verbose)
    Msg_Main_Show_Level_Set(MSG_LEVEL_ADVICE);
  else if (result->flags.only_errors)
    Msg_Main_Show_Level_Set(MSG_LEVEL_ERROR);
  else if (result->flags.silent)
    Msg_Main_Show_Level_Set(MSG_LEVEL_MAX + 1);
  else
    Msg_Main_Show_Level_Set(MSG_LEVEL_WARNING);

  /* Check that we did get the input file. */
  if (result->flags.stdin) {
      MSG_ADVICE("Reading the source program from standard input");
  } else {
    if (!result->file_input) {
      MSG_ERROR("You should specify an input file!");
      Main_Error_Exit(result, CMD_LINE_HELP);
    } else {
      if (!freopen(result->file_input, "rt", stdin)) {
        MSG_ERROR("%s <-- Cannot open the file for reading: %s",
                  result->file_input, strerror(errno));
        Main_Error_Exit(result, NULL);
      }
    }
  }

  /* Check wheter the user has provided a setup file. */
  if (result->file_setup)
    MSG_ADVICE("Reading setup file: \"%s\"", result->file_setup);

  return BOXTASK_OK;
}

static BoxTask My_Stage_Add_Default_Paths(MyArgParserResult *result) {
  BoxPaths_Set_All_From_Env(& box_paths);
  BoxPaths_Add_Script_Path_To_Inc_Dir(& box_paths, result->file_input);
  return BOXTASK_OK;
}

static BoxTask My_Stage_Compilation(MyArgParserResult *result,
                                    BoxVMCallNum *main_module) {
  Msg_Main_Counter_Clear_All();
  MSG_CONTEXT_BEGIN("Compilation");

  target_vm =
    Box_Compile_To_VM_From_File(main_module,
                                /*target_vm*/ NULL,
                                /*file*/ NULL,
                                /*filename*/ result->file_input,
                                /*setup_file_name*/ result->file_setup,
                                /*paths*/ & box_paths);

  MSG_ADVICE("Compilaton finished. %U errors and %U warnings were found.",
             MSG_GT_ERRORS, MSG_NUM_WARNINGS);

  MSG_CONTEXT_END();
  return BOXTASK_OK;
}

/* Enter symbol resolution stage */
static BoxTask My_Stage_Symbol_Resolution(MyArgParserResult *result) {
  int all_resolved;
  BoxTask status = BOXTASK_OK;

  MSG_CONTEXT_BEGIN("Symbol resolution");

  status = BoxVMSym_Resolve_CLibs(target_vm,
                                  BoxPaths_Get_Lib_Dir(& box_paths),
                                  BoxPaths_Get_Libs(& box_paths));
  if (status != BOXTASK_OK)
    return status;

  status = BoxVMSym_Resolve_All(target_vm);
  if (status != BOXTASK_OK)
    return status;

  BoxVMSym_Ref_Check(target_vm, & all_resolved);
  if (!all_resolved) {
    BoxVMSym_Ref_Report(target_vm);
    MSG_ERROR("Unresolved references: program cannot be executed.");
    result->flags.execute = 0;
    status = BOXTASK_ERROR;
  }

  MSG_CONTEXT_END();
  return status;
}


BoxTask Main_Execute(MyArgParserResult *result, BoxUInt main_module) {
  BoxTask t;
  t = BoxVM_Module_Execute(target_vm, main_module);
  if (t == BOXTASK_FAILURE && !result->flags.silent)
    BoxVM_Backtrace_Print(target_vm, stderr);
  return t;
}

static BoxTask My_Stage_Execution(MyArgParserResult *result,
                                  BoxUInt main_module) {
  BoxTask status;
  /* Controllo se e' possibile procedere all'esecuzione del file compilato! */
  if (!result->flags.execute)
    return BOXTASK_OK;

  if (MSG_GT_WARNINGS > 0) {
    if (result->flags.force_exec) {
      if (MSG_GT_ERRORS > 0) {
        result->flags.execute = 0;
        MSG_ADVICE("Errors found: Skipping execution.");
        return BOXTASK_ERROR;
      } else {
        MSG_ADVICE("Warnings found: Execution forced.");
      }
    } else {
      result->flags.execute = 0;
      MSG_ADVICE("Warnings/errors found: Skipping execution!" );
      return BOXTASK_ERROR;
    }
  }

  MSG_CONTEXT_BEGIN("Execution");
  Msg_Main_Counter_Clear_All();
  status = Main_Execute(result, main_module);
  MSG_ADVICE("Execution finished. %U errors and %U warnings were found.",
              MSG_GT_ERRORS, MSG_NUM_WARNINGS);
  MSG_CONTEXT_END();
  return status;
}

static BoxTask My_Stage_Write_Asm(MyArgParserResult *result) {
  const char *file_output = result->file_output;
  /* Fase di output */
  if (file_output) {
    FILE *out = stdout;
    int close_file = 0;
    if (strcmp(result->file_output, "-") == 0) {
      MSG_ADVICE("Writing the assembly code for the compiled program "
                 "on the standart output.");
    } else {
      MSG_ADVICE("Writing the assembly code for the compiled program "
                 "into \"%s\"!", result->file_output);
      out = fopen(file_output, "w");
      if (!out) {
        MSG_ERROR("Cannot open '%s' for writing: %s",
                  file_output, strerror(errno));
        return BOXTASK_ERROR;
      }
      close_file = 1;
    }

    /* If the file name ends with '.c' then we produce a C file. */
    if (Box_CStr_Ends_With(file_output, ".c"))
      BoxVM_Export_To_C_Code(target_vm, out);
    else {
      /* If the file name ends with '.bvmx' then we also include the hex. */
      if (Box_CStr_Ends_With(file_output, ".bvmx"))
        BoxVM_Set_Attr(target_vm, BOXVM_ATTR_DASM_WITH_HEX,
                       BOXVM_ATTR_DASM_WITH_HEX);
      if (BoxVM_Proc_Disassemble_All(target_vm, out) != BOXTASK_OK)
        return BOXTASK_FAILURE;
    }


    if (close_file)
      (void) fclose(out);
  }

  return BOXTASK_OK;
}

/******************************************************************************/

/* main function of the program. */
int main(int argc, const char **argv) {
  BoxUInt main_module;
  int exit_status = EXIT_SUCCESS;
  BoxTask status;
  MyArgParserResult result;

  if (My_Stage_Init() != BOXTASK_OK)
    Main_Error_Exit(& result, "Initialization failed!");

  status = My_Stage_Parse_Command_Line(& result, argc, argv);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Interpret_Command_Line(& result);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Add_Default_Paths(& result);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Compilation(& result, & main_module);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Symbol_Resolution(& result);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Write_Asm(& result);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Execution(& result, main_module);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  My_Stage_Finalize();
  exit(exit_status);
}
