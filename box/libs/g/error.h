/* error.h - Autore: Franchin Matteo - 18 ottobre 2002
 *
 * Questo file deve essere incluso in tutti i file di codice che vogliono
 * segnalare i loro errori secondo le convenzioni di error.c.
 * Qui vengono definite tre macro: FATALMSG, ERRORMSG e WARNINGMSG
 * che hanno una sintassi identica e comunicano che si è verificato un errore
 * più o meno grave.
 * La sintassi è questa ERRORMSG(Locazione, Messaggio)
 * Locazione è una stringa che indica la parte del programma in cui si è
 * verificato l'errore, Messaggio è il messaggio d'errore corrispondente.
 */

/* Procedura usata per la notifica di errori */
extern void err_add(char *loc, char *msg, unsigned int type, long linenum);
extern void err_clear(void);
extern int err_test(void);
extern int err_num(void);

#define WARNINGMSG(loc, msg) \
 err_add(loc, msg, 0, -1)

#define ERRORMSG(loc, msg) \
 err_add(loc, msg, 1, -1)

#define FATALMSG(loc, msg) \
 err_add(loc, msg, 2, -1)

#define LWARNINGMSG(linenum, msg) \
 err_add("", msg, 3, linenum)

#define LERRORMSG(linenum, msg) \
 err_add("", msg, 4, linenum)
