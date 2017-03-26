/* types.h - Autore: Franchin Matteo - maggio 2004
 *
 * Questo file contiene le dichiarazioni dei tipi utilizzati nel programma.
 */

/* Numeri interi senza segno */
typedef unsigned long UInt;

/* Qui definisco la "precisione" dei numeri interi e reali.
 * Dopo tali definizioni, definisco pure quelle macro che devono essere
 * cambiate, qualora si cambino le definizioni di Intg e Real.
 */
typedef long Intg;	/* Numeri interi */
typedef double Real; /* Numeri in virgola mobile */
#define strtointg strtol /* Conversione stringa->Intg */
#define strtoreal strtod /* Conversione stringa->Real */

/* Definisco il tipo Char, che e' esattamente un char */
typedef unsigned char Char;

/* Tipo di dato "punto" */
typedef struct {
  Real x, y;
} Point;

#define NAME(str) ((Name) {sizeof(str)-1, str})
typedef struct {
  UInt length;
  char *text;
} Name;

/* Stringhe da usare nelle printf per stampare i vari tipi */
#define SUInt "%lu"
#define SChar "%c"
#define SIntg "%ld"
#define SReal "%g"
#define SPoint "(%g, %g)"

/* Le funzioni spesso restituiscono un intero per indicare se tutto e' andato
 * bene o se si e' verificato un errore. Definisco il relativo tipo di dato:
 */
typedef enum {Success = 0, Failed = 1} Task;

/* Macro per testare il successo o il fallimento di una funzione.
 */
#define IS_SUCCESSFUL(x) (!x)
#define IS_FAILED(x) (x)

/* Macro per chiamare una funzione che restituisce Task,
 * da una funzione che restituisce Task.
 */
#define TASK(x) if ( x ) return Failed

/* DESCRIZIONE: Questa macro permette di usare una indicizzazione "circolare",
 *  secondo cui, data una lista di num_items elementi, l'indice 1 si riferisce
 *  al primo elemento, 2 al secondo, ..., num_items all'ultimo, num_items+1 al primo,
 *  num_items+2 al secondo, ...
 *  Inoltre l'indice 0 si riferisce all'ultimo elemento, -1 al pen'ultimo, ...
 */
#define CIRCULAR_INDEX(num_items, index) \
  ( (index > 0) ? \
    ( (index - 1) % num_items ) \
    : num_items - 1 - ( (-index) % num_items ) \
  )
