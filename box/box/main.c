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

/* Elenco dei flags che regolano il funzionamento del programma: */
enum {
  FLAG_HELP=1, FLAG_EXECUTE=2, FLAG_OUTPUT=4, FLAG_OVERWRITE=8,
  FLAG_STDIN=0x10, FLAG_INPUT=0x20, FLAG_SETUP=0x40,
  FLAG_VERBOSE=0x80, FLAG_ERRORS=0x100, FLAG_SILENT=0x200,
  FLAG_FORCE_EXEC=0X400
};

/* The BoxVM object where the source is compiled */
static BoxVM *target_vm = NULL;
static BoxPaths box_paths;

/* Variabili interne */
static BoxUInt flags = FLAG_EXECUTE; /* Stato di partenza dei flags */
static char *prog_name;
static char *file_input = NULL;
static char *file_output;
static char *file_setup;
static char *query = NULL;

/* Functions called when the argument of an option is found */
static void My_Set_File_Input(BoxPaths *bp, char *arg) {file_input = arg;}
static void My_Set_File_Output(BoxPaths *bp, char *arg) {file_output = arg;}
static void My_Set_File_Setup(BoxPaths *bp, char *arg) {file_setup = arg;}
static void My_Set_Query(BoxPaths *bp, char *arg) {query = arg;}
static void My_Exec_Query(char *query);

#define NO_ARG ((void (*)(BoxPaths *, char *)) NULL)

/* Tabella contenente i nomi delle opzioni e i dati relativi */
static struct opt {
  char     *name;  /* Nome dell'opzione */
  BoxUInt  cflag,  /* Se part = PAR_NONE, esegue *((BoxUInt *) arg) &= ~cflag */
           sflag,  /* Se part = PAR_NONE, esegue *((BoxUInt *) arg) |= sflag */
           xflag,  /* Se part = PAR_NONE, esegue *((BoxUInt *) arg) ^= xflag */
           repeat, /* Numero di volte che l'opzione puo' essere invocata */
           *flags; /* Puntatore all'insieme dei flags */
  void     (*use_argument)(BoxPaths *bp, char *arg);
  
} opt_tab[] = {
  {"help",    0, FLAG_HELP, 0, -1, & flags, NO_ARG},
  {"?",       0, FLAG_HELP, 0, -1, & flags, NO_ARG},
  {"test",    0, 0, FLAG_EXECUTE, -1, & flags, NO_ARG},
  {"verbose", FLAG_ERRORS + FLAG_SILENT, 0, FLAG_VERBOSE, -1, & flags, NO_ARG},
  {"errors",  FLAG_VERBOSE + FLAG_SILENT, 0, FLAG_ERRORS, -1, & flags, NO_ARG},
  {"silent",  FLAG_VERBOSE + FLAG_ERRORS, 0, FLAG_SILENT, -1, & flags, NO_ARG},
  {"force",   0, 0, FLAG_FORCE_EXEC, -1, & flags, NO_ARG},
  {"stdin",   0, FLAG_INPUT+FLAG_STDIN, 0, 1, & flags, NO_ARG},
  {"input",   FLAG_STDIN, FLAG_INPUT, 0, 1, & flags, My_Set_File_Input},
  {"output",  FLAG_OVERWRITE, FLAG_OUTPUT, 0, 1, & flags, My_Set_File_Output},
  {"write",   0, FLAG_OVERWRITE + FLAG_OUTPUT, 0, 1, & flags, My_Set_File_Output},
  {"setup",   0, FLAG_SETUP, 0, 1, & flags, My_Set_File_Setup},
  {"library", 0, 0, 0, -1, & flags, BoxPaths_Add_Lib},
  {"Include-path", 0, 0, 0, -1, & flags, BoxPaths_Add_Pkg_Dir},
  {"Lib-path", 0, 0, 0, -1, & flags, BoxPaths_Add_Lib_Dir},
  {"query",    0, 0, 0, -1, & flags, My_Set_Query},
  {NULL}
}, opt_default =  {"input", 0, FLAG_INPUT, 0, 1, & flags, My_Set_File_Input};

/* Funzioni definite inquesto file */
int main(int argc, char** argv);
void Main_Error_Exit(char *msg);
void Main_Cmnd_Line_Help(void);
BoxTask Main_Prepare(void);
BoxTask Main_Execute(BoxUInt main_module);

/******************************************************************************/

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
My_Stage_Parse_Command_Line(BoxUInt *flags, int argc, char **argv)
{
  int i;
  BoxUInt j;

  MSG_CONTEXT_BEGIN("Reading the command line options");

  /* Impostazioni iniziali dei flag */
  *flags = FLAG_EXECUTE;

  prog_name = argv[0];

  /* Ciclo su tutti gli argomenti passati */
  for (i = 1; i < argc; i++) {
    char *option = argv[i];
    char *opt_prefix = "";
    struct opt *opt_desc;

    if (*option == OPTION_CHAR) {
      static char single_opt_char[2] = {OPTION_CHAR, '\0'};
      static char double_opt_char[3] = {OPTION_CHAR, OPTION_CHAR, '\0'};
      BoxUInt oplen = strlen(++option), opnum = -1;

      opt_prefix = single_opt_char;
      if (oplen > 0)
        if (*option == OPTION_CHAR) {
          opt_prefix = double_opt_char;
          ++option;
          --oplen;
        }

      for (j = 0; opt_tab[j].name != NULL; j++)
        if (strncmp(option, opt_tab[j].name, oplen) == 0)
          opnum = (opnum == -1) ? j : -2;

      if ( opnum == -1 ) {
        MSG_ERROR("%s%s <-- Illegal option!", opt_prefix, option);
        Main_Error_Exit(CMD_LINE_HELP);
      } else if ( opnum == -2 ) {
        MSG_ERROR("%s%s <-- Ambiguous option!", opt_prefix, option);
        Main_Error_Exit(CMD_LINE_HELP);
      }

      opt_desc = & opt_tab[opnum];

    } else {
      /* Se non inizia con - allora e' l'argomento
       * dell'opzione di default!
       */
      opt_desc = & opt_default;
      --i;
    }

    if ( opt_desc->repeat > 0 )
      --opt_desc->repeat;
    else if ( opt_desc->repeat == 0 ) {
      MSG_ERROR("%s%s <-- Should be used only once!", opt_prefix, option);
      Main_Error_Exit(CMD_LINE_HELP);
    }

    *opt_desc->flags &= ~opt_desc->cflag;
    *opt_desc->flags |= opt_desc->sflag;
    *opt_desc->flags ^= opt_desc->xflag;

    if (opt_desc->use_argument != NO_ARG) {
      if (++i >= argc) {
        MSG_ERROR("%s%s <-- Option requires an argument!", opt_prefix, option);
        Main_Error_Exit(CMD_LINE_HELP);
      }

      opt_desc->use_argument(& box_paths, argv[i]);
    }
  } /* Fine del ciclo for */

  MSG_CONTEXT_END();
  return BOXTASK_OK;
}

static BoxTask My_Stage_Interpret_Command_Line(BoxUInt *f) {
  BoxUInt flags = *f;

  /* Controllo se e' stata specificata l'opzione di help */
  if (flags & FLAG_HELP) Main_Cmnd_Line_Help();

  if (query != NULL) My_Exec_Query(query);

  /* Re-inizializzo la gestione dei messaggi! */
  if (flags & FLAG_VERBOSE)
    Msg_Main_Show_Level_Set(MSG_LEVEL_ADVICE); /* Mostro tutti i messaggi! */
  else if (flags & FLAG_ERRORS)
    Msg_Main_Show_Level_Set(MSG_LEVEL_ERROR); /* Mostro solo i messaggi d'errore/gravi! */
  else if (flags & FLAG_SILENT)
    Msg_Main_Show_Level_Set(MSG_LEVEL_MAX+1); /* Non mostro alcun messaggio! */
  else
    Msg_Main_Show_Level_Set(MSG_LEVEL_WARNING);

  /* Controllo che tutto sia apposto */
  if ((flags & FLAG_INPUT) == 0) {
    MSG_ERROR("You should specify an input file!");
    Main_Error_Exit(CMD_LINE_HELP);

  } else {
    if ((flags & FLAG_STDIN) == 0) {
      if (freopen(file_input, "rt", stdin) == NULL) {
        MSG_ERROR("%s <-- Cannot open the file for reading: %s",
                  file_input, strerror(errno));
        Main_Error_Exit(NULL);
      }
    } else {
      MSG_ADVICE("Reading the source program from standard input");
    }
  }

  /* Controllo se c'e' un file da includere automaticamente */
  if (flags & FLAG_SETUP) {
    MSG_ADVICE("Inizio dal file di setup: \"%s\"", file_setup);
  } else
    file_setup = (char *) NULL;

  return BOXTASK_OK;
}

static BoxTask My_Stage_Add_Default_Paths(void) {
  BoxPaths_Set_All_From_Env(& box_paths);
  BoxPaths_Add_Script_Path_To_Inc_Dir(& box_paths, file_input);
  return BOXTASK_OK;
}

static BoxTask My_Stage_Compilation(char *file, BoxVMCallNum *main_module) {
  Msg_Main_Counter_Clear_All();
  MSG_CONTEXT_BEGIN("Compilation");

  target_vm = Box_Compile_To_VM_From_File(main_module, /*target_vm*/ NULL,
                                          /*file*/ NULL,
                                          /*filename*/ file_input,
                                          /*setup_file_name*/ file,
                                          /*paths*/ & box_paths);

  MSG_ADVICE("Compilaton finished. %U errors and %U warnings were found.",
             MSG_GT_ERRORS, MSG_NUM_WARNINGS);

  MSG_CONTEXT_END();
  return BOXTASK_OK;
}

/* Enter symbol resolution stage */
static BoxTask My_Stage_Symbol_Resolution(BoxUInt *flags) {
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
    *flags &= ~FLAG_EXECUTE;
    status = BOXTASK_ERROR;
  }

  MSG_CONTEXT_END();
  return status;
}

static BoxTask My_Stage_Execution(BoxUInt *flags, BoxUInt main_module) {
  BoxTask status ;
  /* Controllo se e' possibile procedere all'esecuzione del file compilato! */
  if ((*flags & FLAG_EXECUTE) == 0)
    return BOXTASK_OK;

  if (MSG_GT_WARNINGS > 0) {
    if (*flags & FLAG_FORCE_EXEC) {
      if ( MSG_GT_ERRORS > 0 ) {
        *flags &= ~FLAG_EXECUTE;
        MSG_ADVICE("Errors found: Execution will not be started!");
        return BOXTASK_ERROR;
      } else {
        MSG_ADVICE("Warnings found: Execution will be started anyway!");
      }

    } else {
      *flags &= ~FLAG_EXECUTE;
      MSG_ADVICE("Warnings/errors found: Execution will not be started!" );
      return BOXTASK_ERROR;
    }
  }

  MSG_CONTEXT_BEGIN("Execution");
  Msg_Main_Counter_Clear_All();
  status = Main_Execute(main_module);
  MSG_ADVICE("Execution finished. %U errors and %U warnings were found.",
              MSG_GT_ERRORS, MSG_NUM_WARNINGS);
  MSG_CONTEXT_END();
  return status;
}

static BoxTask My_Stage_Write_Asm(BoxUInt flags) {
  /* Fase di output */
  if (flags & FLAG_OUTPUT) {
    FILE *out = stdout;
    int close_file = 0;
    if (strcmp(file_output, "-") == 0) {
      MSG_ADVICE("Writing the assembly code for the compiled program "
                 "on the standart output.");
    } else {
      MSG_ADVICE("Writing the assembly code for the compiled program "
                 "into \"%s\"!", file_output);
      out = fopen(file_output, "w");
      if (out == (FILE *) NULL) {
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

BoxTask Main_Execute(BoxUInt main_module) {
  BoxTask t;
  t = BoxVM_Module_Execute(target_vm, main_module);
  if (t == BOXTASK_FAILURE && (flags & FLAG_SILENT) == 0)
    BoxVM_Backtrace_Print(target_vm, stderr);
  return t;
}

/* DESCRIZIONE: Questa funzione viene chiamata, nel caso si sia verificato
 *  un errore, per uscire immediatamente dall'esecuzione del programma.
 *  msg spiega perche' il programma sia stato interrotto improvvisamente.
 *  Se msg contiene "%s", questi verranno sostituiti con la stringa usata
 *  per avviare il programma.
 */
void Main_Error_Exit(char *msg) {
  if (msg != NULL && (flags & FLAG_SILENT) == 0) {
    fprintf(stderr, msg, prog_name);
    fprintf(stderr, "\n");
  }

  exit(EXIT_FAILURE);
}

void Main_Cmnd_Line_Help(void) {
  fprintf(stderr,
  BOX_VERSTR " " RELEASE_STRING " - Language to describe graphic figures."
  "\n Created and implemented by Matteo Franchin.\n\n"
  "USAGE: " PROGRAM_NAME " options inputfile\n\n"
  "options: are the following:\n"
  " -h(elp)               show this help screen\n"
  " -st(din)              read the input file from stadard input\n"
  " -i(nput) filename     specify the input file\n"
  " -o(utput) filename    compile to filename (refuse to overwrite it)\n"
  " -w(rite) filename     compile to filename (overwrite if it exists)\n"
  " -se(tup) filename     this file will be included automatically at the beginning\n"
  " -l(ibrary) lib        add a new C library to be used when linking\n"
  " -L(ib-path) dir       add dir to the list of directories to be searched for -l\n"
  " -I(nclude-path) dir   add a new directory to be searched when including files\n"
  " -t(est)               just a test: compilation with no execution\n"
  " -f(orce)              force execution, even if warning messages have been shown\n"
  " -v(erbose)            show all the messages, also warning messages\n"
  " -e(rrors)             show only error messages\n"
  " -si(lent)             do not show any message\n"
  "\n inputfile: the name of the input file\n\n"
  "NOTE: some options can be used more than once.\n"
  " Some of them cancel out two by two. Example: using two times the option -t\n"
  " has the same effect of not using it at all.\n"
  );
  exit(EXIT_SUCCESS);
}

static void My_Exec_Query(char *query) {
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

/******************************************************************************/

/* main function of the program. */
int main(int argc, char** argv) {
  BoxUInt main_module;
  int exit_status = EXIT_SUCCESS;
  BoxTask status;

  if (My_Stage_Init() != BOXTASK_OK)
    Main_Error_Exit("Initialization failed!");

  status = My_Stage_Parse_Command_Line(& flags, argc, argv);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Interpret_Command_Line(& flags);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Add_Default_Paths();
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Compilation(file_setup, & main_module);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Symbol_Resolution(& flags);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Write_Asm(flags);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  status = My_Stage_Execution(& flags, main_module);
  if (status != BOXTASK_OK) exit_status = EXIT_FAILURE;

  My_Stage_Finalize();
  exit(exit_status);
}
