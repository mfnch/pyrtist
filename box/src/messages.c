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

/* messages.c - maggio 2004
 *
 * Questo file gestisce i messaggi (di errore, avvertimento, etc) che
 * il programma deve segnalare.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "types.h"
#include "str.h"
#include "messages.h"
#include "defaults.h"

struct context {
	char *msg;
	struct context *next;
	struct context *previous;
};

typedef struct context Context;

static char msg_buffer[MSG_BUFFER_SIZE];

static struct {
	unsigned int always_print_context: 1;
} msg_settings = { 0 };

static Context *msg_current_context = NULL;
static Context *msg_displayed_context = NULL;
static UInt msg_cur_lev = 0;
static UInt msg_max_ignored_lev = 0;
static UInt msg_action_lev = 17;
static UInt msg_num_errors[4] = {0, 0, 0, 0};
/* Funzione che viene chiamata in caso di errore con livello > msg_action_lev
 * essa puo' modificare errlev, cioe' la gravita dell'errore (ponendo errlev=0,
 * si evita che il messaggio di errore venga stampato!)
 */
static void (*Msg_Action)(UInt *errlev) = NULL;

static const char *msg_qualifier[4] = {
	"Avviso", "Attenzione", "Errore", "Errore grave"
};

/* Procedure da usare solo all'interno di questo file */
static void Msg__Context_Exit(void);
static void Msg__Exit_Now(char *msg);

/* Procedura da chiamare per terminare il programma */
void (*Msg_Exit_Now)(char *msg) = Msg__Exit_Now;

/* DESCRIZIONE: Notifica un messaggio, il verificarsi di un errore o l'inizio
 *  di un nuovo contesto per il programma.
 *  level specifica la gravita' del messaggio:
 *   1-3   --> messaggio di semplice comunicazione
 *   4-7   --> messaggio di warning (non implica interruzione dell'esecuzione)
 *   8-11  --> messaggio d'errore
 *   12-15 --> messaggio d'errore grave
 *  Se invece level = 0, viene specificato un nuovo contesto.
 */
void Msg_PrintF(UInt level, const char *fmt, ...) {
  va_list ap;
  UInt i = 0, j = 0; /* Indici per le array s[] e news[] */
  void *s[MSG_NUM_ARGS_MAX], *news[MSG_NUM_ARGS_MAX];

  MSG_LOCATION("Msg_PrintF");

  if ( (level == 0) || (level > msg_max_ignored_lev) ) {
    const register char *c;

    va_start(ap, fmt);

    for( c = fmt; *c != '\0'; c++ ) {
      if ( *c == '%' ) {
        void *p;

        if ( i >= MSG_NUM_ARGS_MAX ) break;

        s[i] = p = va_arg(ap, void *);

        if ( *(c+1) == 's' ) {
          UInt leng = strlen((char *) p);
          if ( leng > MSG_MAX_LENGHT ) {
            s[i] = news[++j]
             = Str_Cut( (char *) p, MSG_MAX_LENGHT, MSG_CUT_POSITION );
          }
        }
        ++i;
      }
    }

    va_end(ap);
  }

  if ( level == 0 ) {
    /* Si tratta di un contesto */
    UInt leng;
    Context *c;

    leng = sprintf(msg_buffer, fmt,
     s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7] );

    /* Cancello tutte le stringhe allocate */
    for(; j > 0; j--) free(news[j]);

    if ( leng > MSG_BUFFER_SIZE )
      Msg_Exit_Now("Overflow nel buffer di messaggi!");

    c = (Context *) malloc(sizeof(Context));
    if ( c == NULL )
      Msg_Exit_Now("Memoria esaurita!");

    c->msg = strdup(msg_buffer);
    c->previous = msg_current_context;
    c->next = NULL;

    if ( msg_settings.always_print_context ) {
      /* Aggiorno il contesto! */
      fprintf( stderr, "%s", c->msg );
      msg_displayed_context = NULL;

    } else {
      if ( msg_displayed_context == NULL )
        msg_displayed_context = c;
    }

    if ( msg_current_context != NULL )
      msg_current_context->next = c;

    msg_current_context = c;
    ++msg_cur_lev;
    return;

  } else {
    UInt l = (level >> 2) & 3;

    ++msg_num_errors[l];

    if ( level > msg_action_lev ) {
      if ( Msg_Action != NULL )
        Msg_Action( & level );
    }

    if ( level > msg_max_ignored_lev ) {
      /* Aggiorno il contesto! */
      while ( msg_displayed_context != NULL ) {
        fprintf( stderr, "%s", msg_displayed_context->msg );
        msg_displayed_context = msg_displayed_context->next;
      }

      /* Stampo il messaggio */
      fprintf( stderr, "%s: ", msg_qualifier[l] );
      fprintf( stderr, fmt,
        s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7] );
      fprintf( stderr, "\n" );

      /* Cancello tutte le stringhe allocate */
      for(; j > 0; j--) free(news[j]);
    }
    return;
  }
}

/* DESCRIZIONE: Inizializza il sistema di gestione dei messaggi.
 *  ignore_level(0-17) specifica il livello di gravita' minimo richiesto
 *  affinche' un messaggio sia considerato (in tutti gli altri casi verra'
 *  ignorato, cioe' non verra' mostrato, anche se incrementera'
 *  il corrispondente contatore dei messaggi, a cui si accede con Msg_Num()).
 *  act_level(0-17) specifica il livello di gravita' minimo affinche' venga chiamata
 *  la funzione di gestione degli errori (che viene impostata proprio
 *  con Msg_Init, vedi fn, piu' avanti).
 *  ctxt_always indica se il contesto deve essere mostrato sempre, anche se non
 *  ci sono messaggi da visualizzare (ctxt_always = 1 --> visualizza sempre)
 *  fn e' la funzione che viene chiamata se il livello di gravita' del messaggio
 *  supera act_level.
 */
void Msg_Init(UInt ignore_level, UInt act_level, UInt ctxt_always, MsgAction fn)
{
	UInt i;

	for ( i = 0; i < 4; i++ )
		msg_num_errors[i] = 0;

	if ( ctxt_always )
		msg_settings.always_print_context = 1;
	else
		msg_settings.always_print_context = 0;

	/* Esco da tutti i contesti (sempre che ci sia dentro!) */
	Msg_Context_Exit(0);

	msg_max_ignored_lev = (ignore_level > 1) ? ignore_level-1 : 0;
	msg_action_lev = (act_level > 1) ? act_level-1 : 0;

	if  ( fn != NULL )
		Msg_Action = fn;

	return;
}

/* DESCRIZIONE: Restituisce il numero che identifica il contesto corrente.
 */
UInt Msg_Context_Num(void)
{
	return msg_cur_lev;
}

/* DESCRIZIONE: Esce dal contesto corrente.
 */
static void Msg__Context_Exit(void)
{
	Context *c = msg_current_context;

	if ( c == NULL ) {
		msg_displayed_context = NULL;
		msg_cur_lev = 0;
		return;
	}

	msg_current_context = c->previous;

	if ( msg_displayed_context == c )
		msg_displayed_context = NULL;
	free(c->msg);
	free(c);

	if ( msg_current_context == NULL ) {
		msg_displayed_context = NULL;
		msg_cur_lev = 0;
		return;

	} else {
		msg_current_context->next = NULL;
		--msg_cur_lev;
		return;
	}
}

/* DESCRIZIONE: Esce dagli ultimi n contesti specificati.
 *  L'indicizzazione e' circolare. Quindi se n = 0, esce da tutti i contesti,
 *  se n = -1, da tutti tranne il primo e cosi' via...
 */
void Msg_Context_Exit(Intg n)
{
	if ( n > 0 )
		n = CIRCULAR_INDEX(msg_cur_lev, n);

	for ( ; n >= 0; n-- )
		Msg__Context_Exit();

	return;
}

/* DESCRIZIONE: Ritorna al contesto di livello n.
 */
void Msg_Context_Return(UInt n)
{
	if ( n >= msg_cur_lev ) return;

	Msg_Context_Exit(msg_cur_lev - n);
	return;
}

/* DESCRIZIONE: Restituisce il numero dei messaggi dei vari tipi.
 *  Se t = 0, 1, 2, 3 restituisce rispettivamente il numero dei messaggi di
 *  avviso, attenzione, errore, errore fatale.
 *  Per t negativi esegue le somme di tali errori, cosicche':
 *  Msg_Num(-1) e' equivalente a Msg_Num(1) + Msg_Num(2) + Msg_Num(3)
 *  Msg_Num(-2) e' equivalente a Msg_Num(2) + Msg_Num(3)
 *  Msg_Num( < -2) e' equivalente a Msg_Num(3)
 */
UInt Msg_Num(Intg t)
{
	if ( t >= 0 ) {
		if ( t > 3 ) t = 3;
		return msg_num_errors[t];

	} else {
		UInt i;

		if ( t < -3 ) t = 2; else t = -(t + 1);

		for ( i = 0; t < 3; t++ )
			i += msg_num_errors[t];

		return i;
	}
}

/* DESCRIZIONE: Azzera i contatori degli errori.
 */
void Msg_Num_Reset_All(void)
{
	UInt i;
	for ( i = 0; i < 4; i++ ) msg_num_errors[i] = 0;
	return;
}

/* DESCRIZIONE: Viene chiamata quando il messaggio non si puo' gestire,
 *  a causa di un errore. Questa funzione tenta di terminare il programma.
 */
void Msg__Exit_Now(char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit( EXIT_FAILURE );
}

/*int main(void)
{
	Msg_Init(1, 17, 0, NULL);

	Msg_Context_Enter("--> Inizia il run del programma"PROGRAM_NAME"...\n");
	Msg_Context_Enter("File \"%s\": ",
	 "questo_file_lo_scelgo_in_modo_che_sia_veramente_lungo.txt");

	MSG_ERROR("Simulo il verificarsi di un errore!\n");
	Msg_Context_Enter("Linea 100: ");
	Msg_Context_Exit(1);
	MSG_ERROR("Un secondo errore!\n");
	MSG_ERROR("Un terzo errore!\n");
	MSG_ERROR("Questo non funzionera'!!! 1.0 = %g!\n", 1.0);

	return EXIT_SUCCESS;
}*/
