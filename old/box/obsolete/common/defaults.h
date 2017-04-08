/* defaults.h - Autore: Franchin Matteo - maggio 2004
 *
 * Questo file contiene la definizione di alcune costanti che regolano
 * il funzionamento del programma.
 */

/******************************************************************************/
/*                                COSTANTI COMUNI                             */
/******************************************************************************/

/* NUM_TYPES: Numero totale dei tipi di registri disponibili.
 * NUM_INTRINSICS e' il numero di tipi intrinseci (il tipo TYPE_OBJ non
 *  e' considerato un tipo intrinseco, in quanto serve per gestire i tipi
 *  definiti dall'utente).
 */
#define NUM_TYPES 5
#define NUM_INTRINSICS 4

/******************************************************************************/
/*                     COSTANTI UTILIZZATE NEL FILE main.c                    */
/******************************************************************************/

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

/******************************************************************************/
/*                   COSTANTI UTILIZZATE NEL FILE messages.c                  */
/******************************************************************************/

/* MSG_NUM_ARGS_MAX e' il numero massimo di argomenti (caratteri %)
 * che possono essere contenuti in un messaggio (cioe' nell'argomento fmt
 * di Msg_PrintF). Gli argomenti %s verranno filtrati con
 * Str_Cut(s, MSG_MAX_LENGHT, MSG_CUT_POSITION), tutti gli altri verranno
 * passati a sprintf o fprintf cosi' come sono.
 */
#define MSG_NUM_ARGS_MAX 8
#define MSG_MAX_LENGHT 40
#define MSG_CUT_POSITION 25
#define MSG_BUFFER_SIZE 1024

/******************************************************************************/
/*                COSTANTI UTILIZZATE NEL FILE tokenizer.lex.c                */
/******************************************************************************/

/* TOK_MAX_INCLUDE e' il numero massimo di file di include che il tokenizer
 * accettera' di aprire (annidare) l'uno dentro l'altro. Serve soltanto
 * per evitare il problema della inclusione circolare (ad esempio, se nel file
 * programma.txt e' presente una: include "programma.txt")
 * TOK_TYPICAL_MAX_INCLUDE e' il numero massimo tipico(che ci si aspetta)
 * di file annidati inclusi con la direttiva "include".
 */
#define TOK_MAX_INCLUDE 10
#define TOK_TYPICAL_MAX_INCLUDE 4

/******************************************************************************/
/*                   COSTANTI UTILIZZATE NEL FILE symbols.c                   */
/******************************************************************************/

/* SYM_HASHSIZE e' la dimensione della tavola di hash, SYM_PARAMETER
 * e' un parametro utilizzato dalla funzione di hash.
 */
#define SYM_HASHSIZE 101
#define SYM_PARAMETER 31

/******************************************************************************/
/*                  COSTANTI UTILIZZATE NEL FILE virtmach.c                   */
/******************************************************************************/

/* VM_TYPICAL_NUM_MODULES e' il numero tipico di moduli installati nella VM,
 * VM_TYPICAL_MOD_DIM e' la dimensione tipica di un modulo di programma
 *  compilato (espressa in numero di "parole di istruzione" = InstrWord),
 * VM_SAFE_EXEC1 (se definito) provoca l'inserimento in fase di compilazione
 *  di codice per il controllo degli indici di registro(per evitare di accedere
 *  per sbaglio a registri non allocati).
 * VM_TYPICAL_TMP_SIZE: dimensione tipica del codice temporaneo prodotto.
 */
#define VM_TYPICAL_NUM_MODULES 10
#define VM_TYPICAL_MOD_DIM 1024
#define VM_TYPICAL_TMP_SIZE 1024
/* Commentare la riga seguente per rendere la VM piu' veloce, ma meno sicura! */
#define VM_SAFE_EXEC1

/******************************************************************************/
/*                  COSTANTI UTILIZZATE NEL FILE compiler.c                   */
/******************************************************************************/

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