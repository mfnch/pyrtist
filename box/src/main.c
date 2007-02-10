/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
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

/* $Id$ */

/* main.c, maggio 2004
 *
 * Questo file legge le opzioni ed esegue le azioni associate.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "virtmach.h"
#include "vmproc.h"
#include "vmsym.h"
#include "registers.h"
#include "compiler.h"
#include "parserh.h"

/* Visualizzo questo messaggio quando ho errori nella riga di comando: */
#define CMD_LINE_HELP "Try '%s -h' to get some help!"

/* Definisco i tipi possibili per i parametri degli argomenti
 * della riga di comando
 */
typedef enum {
  PAR_NONE, PAR_INTG, PAR_UINT,
  PAR_REAL, PAR_POINT, PAR_STRING
} ParType;

/* Elenco dei flags che regolano il funzionamento del programma: */
enum {
  FLAG_HELP=1, FLAG_EXECUTE=2, FLAG_OUTPUT=4, FLAG_OVERWRITE=8,
  FLAG_STDIN=0x10, FLAG_INPUT=0x20, FLAG_SETUP=0x40,
  FLAG_VERBOSE=0x80, FLAG_ERRORS=0x100, FLAG_SILENT=0x200,
  FLAG_FORCE_EXEC=0X400
};

/* Variabili interne */
static UInt flags = FLAG_EXECUTE; /* Stato di partenza dei flags */
static char *prog_name;
static char *file_input;
static char *file_output;
static char *file_setup;

/* Tabella contenente i nomi delle opzioni e i dati relativi */
struct opt {
  char     *name;  /* Nome dell'opzione */
  ParType  part;   /* Tipo dell'eventuale parametro associato all'opzione */
  UInt     cflag;  /* Se part = PAR_NONE, esegue *((UInt *) arg) &= ~cflag */
  UInt     sflag;  /* Se part = PAR_NONE, esegue *((UInt *) arg) |= sflag */
  UInt     xflag;  /* Se part = PAR_NONE, esegue *((UInt *) arg) ^= xflag */
  UInt     repeat; /* Numero di volte che l'opzione puo' essere invocata */
  UInt     *flags; /* Puntatore all'insieme dei flags */
  void     *arg;   /* Puntatore al parametro da settare */
} opt_tab[] = {
  { "help", PAR_NONE, 0, FLAG_HELP, 0, -1, & flags, NULL },
  { "?",  PAR_NONE, 0, FLAG_HELP, 0, -1, & flags, NULL },
  { "test", PAR_NONE, 0, 0, FLAG_EXECUTE, -1, & flags, NULL },
  { "verbose",  PAR_NONE, FLAG_ERRORS + FLAG_SILENT, 0, FLAG_VERBOSE, -1, & flags, NULL },
  { "errors", PAR_NONE, FLAG_VERBOSE + FLAG_SILENT, 0, FLAG_ERRORS, -1, & flags, NULL },
  { "silent", PAR_NONE, FLAG_VERBOSE + FLAG_ERRORS, 0, FLAG_SILENT, -1, & flags, NULL },
  { "force",  PAR_NONE, 0, 0, FLAG_FORCE_EXEC, -1, & flags, NULL },
  { "stdin",  PAR_NONE, 0, FLAG_INPUT+FLAG_STDIN, 0, 1, & flags, NULL },
  { "input",  PAR_STRING, FLAG_STDIN, FLAG_INPUT, 0, 1, & flags, & file_input },
  { "output", PAR_STRING, FLAG_OVERWRITE, FLAG_OUTPUT, 0, 1, & flags, & file_output },
  { "write",  PAR_STRING, 0, FLAG_OVERWRITE + FLAG_OUTPUT, 0, 1, & flags, & file_output },
  { "setup",  PAR_STRING, 0, FLAG_SETUP, 0, 1, & flags, & file_setup },
  { NULL }
}, opt_default =  { "input", PAR_STRING, 0, FLAG_INPUT, 0, 1, & flags, & file_input };


static VMProgram *program = NULL;

/* Funzioni definite inquesto file */
int main(int argc, char** argv);
void Main_Error_Exit(char *msg);
void Main_Cmnd_Line_Help(void);
Task Main_Prepare(void);
Task Main_Install(UInt *main_module);
Task Main_Execute(UInt main_module);
void Temporaneo(void);

/******************************************************************************/

/* main function of the program. Gets the command line arguments and set up
 * the program accrodingly.
 */
int main(int argc, char** argv) {
  int i;
  UInt j, main_module;

  /* Inizializzo la gestione dei messaggi */
  Msg_Main_Init(MSG_LEVEL_WARNING);

  MSG_CONTEXT_BEGIN("Reading the command line options");

  /* Impostazioni iniziali dei flag */
  flags = FLAG_EXECUTE;

  prog_name = argv[0];

  /* Ciclo su tutti gli argomenti passati */
  for ( i = 1; i < argc; i++ ) {
    char *option = argv[i];
    struct opt *opt_desc;

    if ( *option == OPTION_CHAR ) {
      UInt oplen = strlen(++option), opnum = -1;

      for ( j = 0; opt_tab[j].name != NULL; j++ )
        if ( strncasecmp(option, opt_tab[j].name, oplen) == 0 ) {
          if ( opnum == -1 ) opnum = j; else opnum = -2;
        }

      if ( opnum == -1 ) {
        MSG_ERROR( "-%s <-- Illegal option!", option );
        Main_Error_Exit( CMD_LINE_HELP );
      }

      if ( opnum == -2 ) {
        MSG_ERROR( "-%s <-- Ambiguous option!", option );
        Main_Error_Exit( CMD_LINE_HELP );
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
      MSG_ERROR("-%s <-- This option should be used only once!", option );
      Main_Error_Exit( CMD_LINE_HELP );
    }

    *opt_desc->flags &= ~opt_desc->cflag;
    *opt_desc->flags |= opt_desc->sflag;
    *opt_desc->flags ^= opt_desc->xflag;

    if ( opt_desc->part != PAR_NONE ) {
      if ( ++i >= argc ) {
        MSG_ERROR("-%s <-- This option requires an argument!", option );
        Main_Error_Exit( CMD_LINE_HELP );
      }

      switch ( opt_desc->part ) {
       case PAR_STRING:
        *((char **) opt_desc->arg) = argv[i]; break;
       default:
         MSG_ERROR("Internal error in the option parsing :-(");
        Main_Error_Exit( NULL );
      }
    }
  } /* Fine del ciclo for */

  MSG_CONTEXT_END();

  /* Controllo se e' stata specificata l'opzione di help */
  if ( flags & FLAG_HELP ) Main_Cmnd_Line_Help();

  /* Re-inizializzo la gestione dei messaggi! */
  if ( flags & FLAG_VERBOSE )
    Msg_Main_Show_Level_Set(MSG_LEVEL_ADVICE); /* Mostro tutti i messaggi! */
  else if ( flags & FLAG_ERRORS )
    Msg_Main_Show_Level_Set(MSG_LEVEL_ERROR); /* Mostro solo i messaggi d'errore/gravi! */
  else if ( flags & FLAG_SILENT )
    Msg_Main_Show_Level_Set(MSG_LEVEL_MAX+1); /* Non mostro alcun messaggio! */
  else
    Msg_Main_Show_Level_Set(MSG_LEVEL_ERROR); /* Mostro solo i messaggi importanti! */

  Temporaneo();

  /* Controllo che tutto sia apposto */
  if ( (flags & FLAG_INPUT) == 0 ) {
    MSG_ERROR("You should specify an input file!");
    Main_Error_Exit( CMD_LINE_HELP );

  } else {
    if ( (flags & FLAG_STDIN) == 0 ) {
      if ( freopen(file_input, "rt", stdin) == NULL ) {
        MSG_ERROR("%s <-- Cannot open the file for reading: %s",
         file_input, strerror(errno));
        Main_Error_Exit( NULL );
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

  Msg_Main_Counter_Clear_All();
  MSG_CONTEXT_BEGIN("Compilation");

  if IS_FAILED( Parser_Init(TOK_MAX_INCLUDE, file_setup) )
    Main_Error_Exit( NULL );

  if IS_FAILED( VM_Init(& program) ) Main_Error_Exit( NULL );
  if IS_FAILED( Cmp_Init(program) ) Main_Error_Exit( NULL );

  /* Avvio il parsing! */
  (void) yyparse();

  MSG_ADVICE( "Compilaton finished. "
   "%U errors and %U warnings were found.",
   MSG_GT_ERRORS, MSG_NUM_WARNINGS );
  MSG_CONTEXT_END();

  Parser_Finish();
  Cmp_Finish();

  if IS_FAILED( Main_Prepare() ) Main_Error_Exit( NULL );
  if IS_FAILED( Main_Install(& main_module) ) Main_Error_Exit( NULL );


  /* Controllo se e' possibile procedere all'esecuzione del file compilato! */
  if ( (flags & FLAG_EXECUTE) && (MSG_GT_WARNINGS > 0) ) {
    if (flags & FLAG_FORCE_EXEC) {
      if ( MSG_GT_ERRORS > 0 ) {
        flags &= ~FLAG_EXECUTE;
        MSG_ADVICE(
         "Errors found: Execution will not be started!" );
      } else {
        MSG_ADVICE(
         "Warings found: Execution will be started anyway!" );
      }

    } else {
      flags &= ~FLAG_EXECUTE;
      MSG_ADVICE("Warnings/errors found: Execution will not be started!" );
    }
  }

  /* Fase di output */
  if (flags & FLAG_OUTPUT) {
    FILE *out = stdout;
    int close_file = 0;
    if (strcmp(file_output, "-") == 0) {
      MSG_ADVICE("Writing the assembly code for the compiled program"
       "on the standart output.");
    } else {
      MSG_ADVICE("Writing the assembly code for the compiled program"
       "into \"%s\"!", file_output );
      out = fopen(file_output, "w");
      if (out == (FILE *) NULL) {
        MSG_ERROR("Cannot open '%s' for writing: %s",
         file_output, strerror(errno));
        Main_Error_Exit(NULL);
      }
      close_file = 1;
    }

    (void) Cmp_Data_Display(out);
    fprintf(out, "\n");
    VM_DSettings(program, 1);
    if IS_FAILED( VM_Proc_Disassemble_All(program, out) )
      Main_Error_Exit( NULL );
    if (close_file) (void) fclose(out);
  }

  /* Fase di esecuzione */
  if (flags & FLAG_EXECUTE) {
    if IS_FAILED( VM_Sym_Resolve_All(program) ) {
      exit(EXIT_FAILURE);
    }
    /*VM_Sym_Table_Print(program, stderr, 0);*/

    Msg_Main_Counter_Clear_All();
    MSG_CONTEXT_BEGIN("Execution");
    (void) Main_Execute(main_module);
    MSG_ADVICE( "Execution finished. "
     "%U errors and %U warnings were found.",
     MSG_GT_ERRORS, MSG_NUM_WARNINGS );
    MSG_CONTEXT_END();
  }

  VM_Destroy(program); /* This function accepts program = NULL */
  exit( EXIT_SUCCESS );
}

/* FASE DI POST-COMPILAZIONE */
Task Main_Prepare(void) {
  int i;
  Intg num_var[NUM_TYPES], num_reg[NUM_TYPES];
  RegVar_Get_Nums(num_var, num_reg);
  TASK( VM_Code_Prepare(program, num_var, num_reg) );
  /* Preparo i registri globali */
  for(i = 0; i < NUM_TYPES; i++) {num_var[i] = 0; num_reg[i] = 3;}
  TASK( VM_Module_Globals(program, num_var, num_reg) );
 /* The following function sets gro0 to point to the data segment */
  TASK( Cmp_Data_Prepare() );
  return Success;
}

/* FASE DI ESECUZIONE */
Task Main_Install(UInt *main_module) {
  return VM_Proc_Install_Code(program, main_module,
   VM_Proc_Target_Get(program), "main", "Description...");
}

Task Main_Execute(UInt main_module) {
  /*Msg_Num_Reset_All();

  Msg_Context_Enter("Controllo lo stato di definizione dei moduli.\n");
  status = VM_Module_Check(program, 1);
  Msg_Context_Exit(0);
  if ( status == Failed ) {
    MSG_ADVICE( "Trovati " SUInt " errori: l'esecuzione non verra' avviata!",
     Msg_Num(MSG_NUM_ERRFAT) );
    return Failed;
  }
  */

  return VM_Module_Execute(program, main_module);
}

/* DESCRIZIONE: Questa funzione viene chiamata, nel caso si sia verificato
 *  un errore, per uscire immediatamente dall'esecuzione del programma.
 *  msg spiega perche' il programma sia stato interrotto improvvisamente.
 *  Se msg contiene "%s", questi verranno sostituiti con la stringa usata
 *  per avviare il programma.
 */
void Main_Error_Exit(char *msg) {
  if ( (msg != NULL) && ((flags & FLAG_SILENT) == 0) ) {
    fprintf(stderr, msg, prog_name);
    fprintf(stderr, "\n");
  }

  VM_Destroy(program); /* This function accepts program = NULL */
  exit( EXIT_FAILURE );
}

void Main_Cmnd_Line_Help(void) {
  fprintf( stderr,
  PROGRAM_NAME " " VERSION_STR " - Language to describe graphic figures."
  "\n Created and implemented by Matteo Franchin - "
   VERSION_DATE ", " VERSION_TIME "\n\n"
  "USAGE: " PROGRAM_NAME " options inputfile\n\n"
  "options: are the following:\n"
  " -h(elp)               show this help screen\n"
  " -st(din)              read the input file from stadard input\n"
  " -i(nput) filename     specify the input file\n"
  " -o(utput) filename    compile to filename (refuse to overwrite it)\n"
  " -w(rite) filename     compile to filename (overwrite if it exists)\n"
  " -se(tup) filename     this file will be included automatically at the beginning\n"
  " -t(est)               just a test: compilation with no execution\n"
  " -v(erbose)            show all the messages, also warning messages\n"
  " -e(rrors)             show only error messages\n"
  " -si(lent)             do not show any message\n"
  " -f(orce)              force execution, even if warning messages have been shown\n"
  "\n inputfile: the name of the input file\n\n"
  "NOTE: some options can be used more than once.\n"
  " Some of them cancel out two by two. Example: using two times the option -t\n"
  " has the same effect of not using it at all.\n"
  );

  exit( EXIT_SUCCESS );
}

void Temporaneo(void) {

  return;

/*   o = VM_Asm_Out_New(-1);
  VM_Asm_Out_Set(program, o);

  printf("Writing program...\n");
  Cmp_Assemble(ASM_PROJX_P, CAT_GREG, 15);
  Cmp_Assemble(ASM_PROJY_P, CAT_LREG, 7);
  Cmp_Assemble(ASM_INTG_R, CAT_LREG, 1);
  Cmp_Assemble(ASM_REAL_I, CAT_LREG, 2);
  Cmp_Assemble(ASM_REAL_I, CAT_IMM, 27);
  Cmp_Assemble(ASM_POINT_II, CAT_LREG, 3, CAT_GREG, 4);
  Cmp_Assemble(ASM_POINT_RR, CAT_LREG, 5, CAT_LREG, 6);
  Cmp_Assemble(ASM_MOV_Iimm, CAT_LREG, 3, CAT_IMM, 1003);
  Cmp_Assemble(ASM_MOV_Iimm, CAT_LREG, 3, CAT_IMM, 127);
  Cmp_Assemble(ASM_MOV_Iimm, CAT_LREG, 3, CAT_IMM, 128);
  Cmp_Assemble(ASM_MOV_Pimm, CAT_LREG, 2, CAT_IMM, 3.1415, -2.7);
  Cmp_Assemble(ASM_MOV_Rimm, CAT_LREG, 23, CAT_IMM, 1.234567);
  Cmp_Assemble(ASM_MOV_II, CAT_LREG, 3, CAT_IMM, -128);
  Cmp_Assemble(ASM_MOV_Iimm, CAT_LREG, 3, CAT_IMM, -129);
  Cmp_Assemble(ASM_CALL_Iimm, CAT_LREG, 13);
  Cmp_Assemble(ASM_MOV_II, CAT_GREG, 1, CAT_LREG, 3);
  Cmp_Assemble(ASM_MOV_II, CAT_LREG, 2, CAT_PTR, -2000);
  Cmp_Assemble(ASM_ADD_II, CAT_GREG, 1, CAT_IMM, 37);
  Cmp_Assemble(ASM_RET);
  printf("Completed!\nDisassembling...\n");
  VM_DSettings(program, 1);
  if IS_FAILED(
   VM_Disassemble(program, stdout, (o->program)->ptr, (o->program)->numel) ) {
    printf("Ho qualche problema nella scrittura del programma!\n");
    return;
  }
  printf("Completed!\n");
*/
  return;
}
