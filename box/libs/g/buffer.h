/* buffer.h - Autore: Franchin Matteo - aprile 2002
 *
 * Questo file contiene le definizioni necessarie per poter
 * gestire le chiamate a buffer.c
 */

#ifndef _BUFFER_H

#  define _BUFFER_H

/* Struttura di controllo del buffer */
typedef struct {
	long id;	/* Costante identificativa di struttura inizializzata */
	void *ptr;	/* Puntatore alla zona di memoria che contiene i dati */
	long dim;	/* Numero massimo di elementi contenibili nel buffer */
	long size;	/* Dimensione in bytes della zona di memoria allocata */
	long mindim;	/* Valore minimo di dim */
	short elsize;	/* Dimensione in bytes di ogni elemento */
	long numel;		/* Numero di elementi attualmente inseriti */
	long chain;		/* Quantita' inutilizzata (utile per eventuali catene di elementi) */
} buff;

/* Procedure definite in buffer.c */
int buff_create(buff *buffer, short elsize, long mindim);
int buff_recycle(buff *buffer, short elsize, long mindim);
int buff_push(buff *buffer, void *elem);
int buff_mpush(buff *buffer, void *elem, long numel);
int buff_bigenough(buff *buffer, long numel);
int buff_smallenough(buff *buffer, long numel);
int buff_clear(buff *buffer);
void buff_free(buff *buffer);

#define buffID		0x66626468

#define buff_just_allocated(buffer) buffer->id = 0
#define buff_created(buffer)	((buffer)->id == buffID ? 1 : 0)
#define buff_chain(buffer)		((buffer)->chain)
#define buff_numitem(buffer)	((buffer)->numel)
#define buff_ptr(buffer)		((buffer)->ptr)
#define buff_item(buffer, type, n)	*((type *) ((buffer)->ptr + ((n)-1)*((long) (buffer)->elsize)))
#define buff_firstitem(buffer, type)	*((type *) ((buffer)->ptr))
#define buff_lastitem(buffer, type)	*((type *) ((buffer)->ptr + ((buffer)->numel-1)*((long) (buffer)->elsize)))
#define buff_itemptr(buffer, type, n)	((type *) ((buffer)->ptr + ((n)-1)*((long) (buffer)->elsize)))
#define buff_firstitemptr(buffer, type)	((type *) ((buffer)->ptr))
#define buff_lastitemptr(buffer, type)	((type *) ((buffer)->ptr + ((buffer)->numel-1)*((long) (buffer)->elsize)))
#define buff_dec(buffer)	buff_smallenough((buffer), --(buffer)->numel)

#endif
