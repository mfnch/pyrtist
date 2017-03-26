/* error.c - Autore: Franchin Matteo - 18 ottobre 2002
 *
 * Questo file implementa una semplice convenzione di gestione degli errori.
 * Il codice che vuole segnalare un errore deve includere il file error.h e
 * usare le macro ERRORMSG, WARNINGMSG oppure FATALMSG
 * per segnalare il verificarsi di un errore.
 * Le tre macro aggiungono l'errore ad una lista di errori recenti
 * che pu� essere visualizzata con la funzione err_print o err_printclr.
 * La macro WARNINGMSG deve essere utilizzata per segnalare un "avvertimento"
 * mentre ERRORMSG segnala un vero e proprio errore.
 * FATALMSG dovrebbe essere utilizzata per segnalare un errore grave.
 * Vedere error.h per ulteriori informazioni.
 */

#include <stdio.h>

/* Numero di messaggi di errore */
#define error_list_dim  20

/* Numero di tipi d'errore */
#define num_error_types  5

/* Definisce un buffer circolare contenente gli errori verificatisi */
static struct msg_item {
  char *location;
  char *message;
  unsigned int type;
  long linenum;
} error_list[error_list_dim];

static unsigned int first_err  = 0;
static unsigned int last_err  = 0;
static unsigned int num_err    = 0;

static char *err_type[num_error_types] = {
  "Warning in %s", "Errore in %s", "Errore grave in %s",
  "Warning alla riga %d", "Errore alla riga %d"
};

/* Procedure definite in questo file */
void err_add(char *loc, char *msg, unsigned int type, long linenum);
void err_clear(void);
int err_test(void);
void err_print(FILE *stream);
void err_prnclr(FILE *stream);
int err_num(void);

/* NOME: err_add
 * DESCRIZIONE: segnala che � avvenuto un errore.
 *  loc indica in quale parte del programma,
 *  msg il messaggio corrispondente,
 *  type indica il tipo di errore (0 = error, 1 = warning, ...)
 */
void err_add(char *loc, char *msg, unsigned int type, long linenum)
{
  if (++num_err <= error_list_dim) {
    error_list[last_err].location = loc;
    error_list[last_err].message = msg;
    error_list[last_err].type = type % num_error_types;
    error_list[last_err].linenum = linenum;
    last_err = (last_err + 1) % error_list_dim;
  }
  err_prnclr(stdout);
  return;
}

/* NOME: err_clear
 * DESCRIZIONE: pulisce la lista degli errori, o meglio:
 *  induce il programma a considerarsi nello stato:
 * "nessun nuovo errore verificatosi".
 */
void err_clear(void)
{
  first_err = last_err;
  num_err = 0;

  return;
}

/* NOME: err_test
 * DESCRIZIONE: restituisce 0 se nessun nuovo errore si � verificato,
 *  1 in caso contrario.
 */
int err_test(void)
{
  if (last_err == first_err)
    return 0;
  else
    return 1;
}

/* NOME: err_print
 * DESCRIZIONE: stampa la lista degli errori verificatisi
 */
void err_print(FILE *stream)
{
  unsigned int i;

  for (i = first_err; i != last_err; i = ((i+1) % error_list_dim))
    if (error_list[i].type > 2) {
      fprintf(stream, err_type[ error_list[i].type ], error_list[i].linenum);
      fprintf(stream, ": %s\n", error_list[i].message);

    } else {
      fprintf(stream, err_type[ error_list[i].type ], error_list[i].location);
      fprintf(stream, ": %s\n", error_list[i].message);
    }

  return;
}

/* NOME: err_print
 * DESCRIZIONE: stampa la lista degli errori verificatisi
 *  e la "pulisce".
 */
void err_prnclr(FILE *stream)
{
  err_print(stream);
  err_clear();

  return;
}

/* NOME: err_num
 * DESCRIZIONE: Restituisce il numero di errori recenti verificatisi
 */
int err_num(void) { return num_err; }
