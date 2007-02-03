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

/**
 * @file defaults.h
 * @brief The defaults settings for the Box compiler stuff.
 *
 * This file contains the definitions of the constants which tune
 * the performance of the Box compiler, VM, etc.
 * These include the default size of hashtables, arrays, etc.
 */

/* Questo file contiene la definizione di alcune costanti che regolano
 * il funzionamento del programma.
 */
#ifndef _DEFAULT_H
#define _DEFAULT_H
/*****************************************************************************
 *                                COSTANTI COMUNI                            *
 *****************************************************************************/

/* NUM_TYPES: Numero totale dei tipi di registri disponibili.
 * NUM_INTRINSICS e' il numero di tipi intrinseci (il tipo TYPE_OBJ non
 *  e' considerato un tipo intrinseco, in quanto serve per gestire i tipi
 *  definiti dall'utente).
 */
#define NUM_TYPES 5
#define NUM_INTRINSICS 4

/*****************************************************************************
 *                     COSTANTI UTILIZZATE NEL FILE main.c                   *
 *****************************************************************************/

/* PROGRAM_NAME e' il nome del programma.
 * VERSION_STR e VERSION_NUM sono la stringa e l'intero che identificano
 * la versione del programma.
 * VERSION_DATE e' la data di creazione del programma.
 * OPTION_CHAR e' il carattere usato per le opzioni da riga di comando
 * (in unix e' '-' in DOS e' '/')
 */
#define PROGRAM_NAME "box"

#define VERSION_STR "0.00"
#define VERSION_NUM 000
#define VERSION_DATE __DATE__
#define VERSION_TIME __TIME__

#define OPTION_CHAR '-'

/*****************************************************************************
 *                  COSTANTI UTILIZZATE NEL FILE messages.c                  *
 *****************************************************************************/

/* MSG_NUM_ARGS_MAX e' il numero massimo di argomenti (caratteri %)
 * che possono essere contenuti in un messaggio (cioe' nell'argomento fmt
 * di Msg_PrintF). Gli argomenti %s verranno filtrati con
 * Str_Cut(s, MSG_MAX_LENGHT, MSG_CUT_POSITION), tutti gli altri verranno
 * passati a sprintf o fprintf cosi' come sono.
 */
#define MSG_TYPICAL_NUM_MSGS 16

#define MSG_NUM_ARGS_MAX 8
#define MSG_MAX_LENGHT 40
#define MSG_CUT_POSITION 25
#define MSG_BUFFER_SIZE 1024

/*****************************************************************************
 *               COSTANTI UTILIZZATE NEL FILE tokenizer.lex.c                *
 *****************************************************************************/

/* TOK_MAX_INCLUDE e' il numero massimo di file di include che il tokenizer
 * accettera' di aprire (annidare) l'uno dentro l'altro. Serve soltanto
 * per evitare il problema della inclusione circolare (ad esempio, se nel file
 * programma.txt e' presente una: include "programma.txt")
 * TOK_TYPICAL_MAX_INCLUDE e' il numero massimo tipico(che ci si aspetta)
 * di file annidati inclusi con la direttiva "include".
 */
#define TOK_MAX_INCLUDE 10
#define TOK_TYPICAL_MAX_INCLUDE 4

/*****************************************************************************
 *                  COSTANTI UTILIZZATE NEL FILE symbols.c                   *
 *****************************************************************************/

/* SYM_HASHSIZE e' la dimensione della tavola di hash, SYM_PARAMETER
 * e' un parametro utilizzato dalla funzione di hash.
 */
#define SYM_HASHSIZE 101
#define SYM_PARAMETER 31

/*****************************************************************************
 *                 COSTANTI UTILIZZATE NEL FILE virtmach.c                   *
 *****************************************************************************/

/* VM_TYPICAL_NUM_MODULES e' il numero tipico di moduli installati nella VM,
 * VM_TYPICAL_MOD_DIM e' la dimensione tipica di un modulo di programma
 *  compilato (espressa in numero di "parole di istruzione" = InstrWord),
 * VM_SAFE_EXEC1 (se definito) provoca l'inserimento in fase di compilazione
 *  di codice per il controllo degli indici di registro(per evitare di accedere
 *  per sbaglio a registri non allocati).
 * VM_TYPICAL_TMP_SIZE: dimensione tipica del codice temporaneo prodotto.
 */
#define VM_TYPICAL_NUM_MODULES 10
#define VM_TYPICAL_NUM_SHEETS 10
#define VM_TYPICAL_NUM_LABELS 64
#define VM_TYPICAL_MOD_DIM 1024
#define VM_TYPICAL_TMP_SIZE 1024
/* Commentare la riga seguente per rendere la VM piu' veloce, ma meno sicura! */
#define VM_SAFE_EXEC1

/** Initial size of the hashtable containing the symbols for the VM */
#define VMSYM_SYM_HT_SIZE 1024
/** Initial size of the array containing the definitions of the VM symbols */
#define VMSYM_DEF_ARR_SIZE 1024
/** Initial size of the array containing the data of the definition
 * and reference structures
 */
#define VMSYM_DATA_ARR_SIZE 1024
/** Initial size of the array containing the references to the VM symbols */
#define VMSYM_REF_ARR_SIZE 2048
/** Initial size of the array containing the names of the VM symbols */
#define VMSYS_NAME_ARR_SIZE 8192
/** Initial size of the array containing the installed procedures */
#define VMPROC_INST_ARR_SIZE 16
/** Initial size of the collection containing the uninstalled procedures */
#define VMPROC_UNINST_CLC_SIZE 16
/** Initial size of the code of a procedure */
#define VM_PROC_CODE_SIZE 1024

/*****************************************************************************
 *                 COSTANTI UTILIZZATE NEL FILE compiler.c                   *
 *****************************************************************************/

/* CMP_TYPICAL_NUM_TYPES e' il numero tipico di tipi (intrinseci+user defined)
 * di cui fara' uso il programma.
 * CMP_TYPICAL_DATA_SIZE dimensione iniziale in byte dell'area di memoria
 *  dedicata al contenimento dei dati (stringhe, etc).
 * CMP_TYPICAL_IMM_SIZE dimensione iniziale in byte dell'area di memoria
 *  dedicata al contenimento dei dati temporanei (ossia immediati).
 * CMP_PRIVILEGED tipi la cui elaborazione e' privilegiata (piu' veloce
 *  ed efficente).
 * CMP_TYPICAL_TYPE_NAME_SIZE lunghezza tipica dei nomi dei tipi (un nome,
 *  per esempio, potrebbe essere questo 'array di 10 puntatori a Int')
 * SYM_BOX_LIST_DIM: Numero massimo tipico di box annidate.
 * TYM_NUM_TYPE_NAMES: Numero massimo di utilizzi "contemporanei"
 *  della funzione Tym_Type_Names (numero di volte che si puo' usare
 *  Tym_Type_Names nella stessa istruzione printf).
 * CMP_TYPICAL_STRUC_DIM numero tipico massimo di elementi delle strutture.
 * CMP_TYPICAL_STRUC_SIZE dimensione tipica massima di una struttura.
 */
#define CMP_TYPICAL_NUM_TYPES 10
#define CMP_TYPICAL_DATA_SIZE 8192
#define CMP_TYPICAL_IMM_SIZE 4096
#define CMP_PRIVILEGED 4
#define TYM_TYPICAL_TYPE_NAME_SIZE 128
#define TYM_NUM_TYPE_NAMES 3
#define SYM_BOX_LIST_DIM 5
#define CMP_TYPICAL_STRUC_DIM  32
#define CMP_TYPICAL_STRUC_SIZE 1024

/*****************************************************************************
 *                 COSTANTI UTILIZZATE NEL FILE compiler.c                   *
 *****************************************************************************/
#define BOX_ARR_SIZE 16

#endif
