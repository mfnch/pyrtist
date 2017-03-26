#include <stdlib.h>

/* DESCRIZIONE: Cerca il separatore sep nella stringa text e lo sostituisce
 *  con '\0', restituendo il puntatore al carattere successivo, oppure NULL
 *  se non e' stato trovato nessun separatore sep nella stringa.
 */
char *str_get_next(char *text, char sep)
{
	for (; *text != '\0'; text++)
		if ( *text == sep ) {
			*text = '\0';
			return (text + 1);
		}
	
	return NULL;
}

void parse_layer_string(char *this_job)
{
	char *next_job;

	/* Tratto separatamente ogni sub-stringa (| e' il relativo separatore) */
	do {
		char *dest_layers, *next_arg;

		next_job = str_get_next(this_job, '|');

		/* Ottengo la sub-stringa contenente i layer di destinazione */
		dest_layers = str_get_next(this_job, '>');

		/* Prelevo ciascun argomento che seleziona i layer di origine */
		do {
			next_arg = str_get_next(this_job, ',');

		} while ( (this_job = next_arg) != NULL );

	} while ( (this_job = next_job) != NULL );

	return;
}

#define SKIP_SPACES(s) \
	while ( (*s == ' ') && (*s != '\0') ) s++;

/* DESCRIZIONE: Questa macro permette di usare una indicizzazione "circolare",
 *  secondo cui, data una lista di num_items elementi, l'indice 1 si riferisce
 *  al primo elemento, 2 al secondo, ..., num_items all'ultimo, num_items+1 al primo,
 *  num_items+2 al secondo, ...
 *  Inoltre l'indice 0 si riferisce all'ultimo elemento, -1 al pen'ultimo, ...
 */
#define CIRCULAR_INDEX(num_items, index) \
	( (index > 0) ? \
		( (index - 1) % num_items ) + 1 \
		: num_items - ( (-index) % num_items ) \
	)

/* DESCRIZIONE: Legge dall'inizio di text una stringa nei seguenti possibili
 *  formati:
 *   1) "n"  --> seleziona un layer in base al suo numero assoluto,
 *	 2) "xn" --> seleziona il layer numero (n + active)
 *  n e' un numero con segno. Gli eventuali spazi saranno ignorati.
 *  Esempi sono "1" = "+1", "-3", "x1" = " x + 1 ", " x - 2 " = "x-2".
 * NOTA: Restituisce il numero del layer o -1 se c'e' un errore.
 */
static int parse_layer_arg(char *text, int active, int numlayers)
{
	int x = 0, y, n;

	SKIP_SPACES(text);
	if ( tolower(*text) == 'x' ) {
		x = active;
		++text;
		SKIP_SPACES(text);
	} else if (*text == '\0')
		return -1; /* Errore! */

	if ( sscanf(text, "%d%n", y, n) < 1 )
		return -1; /* Errore! */

	text += n;
	SKIP_SPACES(text);

	if ( *text == '\0' ) 
		return -1; /* Errore! */

	return CIRCULAR_INDEX(numlayers, x);
}
