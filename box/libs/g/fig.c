/* fig.c - Autore: Franchin Matteo
 *
 * Questo file contiene quanto basta per poter disegnare su una "finestra
 * virtuale".
 * I comandi vengono semplicemente salvati in diverse "liste di comandi",
 * che chiamo "layer". I layer servono a ordinare gli oggetti grafici
 * in modo da realizzare la giusta sovrapposizione.
 */

#include <stdio.h>
#include <stdlib.h>

/* De-commentare per includere i messaggi di debug */
/*#define DEBUG*/

#include "debug.h"
#include "error.h"
#include "buffer.h"
#include "graphic.h"  /* Dichiaro alcune strutture grafiche generali */
#include "fig.h"

/* Queste funzioni sono definite nella seconda parte di questo file
 * e invece di eseguire effettivamente i comandi, provvedono soltanto
 * a memorizzarli nel layer attualmente attivo.
 */
static void not_available(void);
void fig_rreset(void);
void fig_rinit(void);
void fig_rdraw(void);
void fig_rline(Point a, Point b);
void fig_rcong(Point a, Point b, Point c);
void fig_rcurve(Point a, Point b, Point c, Real cut);
void fig_rcircle(Point ctr, Point a, Point b);
void fig_rfgcolor(Real r, Real g, Real b);
void fig_rbgcolor(Real r, Real g, Real b);

/* Lista delle funzioni di basso livello (non disponibili) */
static void (*fig_lowfn[])() = {
  not_available,
  not_available,
  not_available,
  not_available
};

/* Lista delle funzioni di rasterizzazione */
void (*fig_midfn[])() = {
  fig_rreset, fig_rinit, fig_rdraw,
  fig_rline, fig_rcong, fig_rcurve,
  fig_rcircle, fig_rfgcolor, fig_rbgcolor
};

/*#define grp_activelayer (((struct fig_header *) grp_win->wrdep)->current)*/

/* Dimensione iniziale di un layer in bytes = dimensione iniziale dello spazio
 * in cui vengono memorizzate le istruzioni di ciascun layer
 */
#define LAYER_INITIAL_SIZE 128

/* DESCRIZIONE: Questa macro permette di usare una indicizzazione "circolare",
 *  secondo cui, data una lista di num_items elementi, l'indice 1 si riferisce
 *  al primo elemento, 2 al secondo, ..., num_items all'ultimo, num_items+1 al primo,
 *  num_items+2 al secondo, ...
 *  Inoltre l'indice 0 si riferisce all'ultimo elemento, -1 al pen'ultimo, ...
 */
#ifdef CIRCULAR_INDEX
#  undef CIRCULAR_INDEX
#endif

#  define CIRCULAR_INDEX(num_items, index) \
    ( (index > 0) ? ( (index - 1) % num_items ) + 1 : \
                    num_items - ( (-index) % num_items ) )

struct fig_header {
  int numlayers;    /* Numero dei layer della figura */
  int current;    /* Layer attualmente in uso */
  int top;      /* Primo layer della lista */
  int bottom;      /* Ultimo layer della lista */
  int firstfree;    /* Primo posto libero nella lista dei layer (0 = nessuno) */
  buff layerlist;    /* Buffer che gestisce la lista dei layer */
};

struct layer_header {
  unsigned long ID;
//  ... color;
  int numcmnd;
  int previous;
  int next;
  buff layer;
};


/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/* I puntatori alle funzioni non disponibili con finestre di tipo "fig"
 * vengono fatti puntare a questa funzione
 */
static void not_available(void)
{
  WARNINGMSG("fig low function", "Caratteristica non disponibile");
  return;
}

/* DESCRIZIONE: Apre una finestra di tipo "fig", che consiste essenzialmente
 *  in un "registratore" di comandi grafici. Infatti ogni comando che
 *  la finestra riceve viene memorizzato in diversi "contenitori": i layer.
 *  Questi sono ordinati dal piu' basso, cioe' quello che viene disegnato
 *  per primo (e quindi sara' ricoperto da tutti i successivi), al piu' alto,
 *  che viene disegnato per ultimo (e quindi ricopre tutti gli altri).
 *  numlayers specifica proprio il numero dei layer della figura.
 *  L'ordine dei layer puo' essere modificato (altre procedure di questo file).
 *  Ad ogni layer e' associato un numero (da 1 a numlayers) e questo viene
 *  utilizzato per far riferimento ad esso.
 */
grp_window *fig_open_win(int numlayers)
{
  grp_window *wd;
  struct fig_header *figh;
  struct layer_header *layh;
  buff *laylist;
  int i;

  PRNMSG("fig_open_win:\n  ");

  if ( numlayers < 1 ) {
    ERRORMSG("fig_open_win", "Figura senza layers");
    return NULL;
  }

  /* Creo gli headers della figura (con tutte le informazioni utili
   * per la gestione dei layers)
   */
  figh = (struct fig_header *) malloc( sizeof(struct fig_header) );

  if ( (figh == NULL) ) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  /* Creo la lista dei layers con numlayers elementi */
  laylist = & figh->layerlist;
  if ( ! buff_create( laylist, sizeof(struct layer_header), numlayers) ) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  /* buff_create crea una lista vuota e usa malloc solo quando si tenta
   * di riempirla (con buff_push, etc).
   * Quindi ora forzo l'allocazione della lista!
   */
  if ( ! buff_bigenough( laylist, buff_numitem(laylist) = numlayers ) ) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  /* Compilo l'header della figura */
  figh->numlayers = numlayers;
  figh->top = 1;
  figh->bottom = numlayers;
  figh->firstfree = 0;  /* Nessun layer libero */

  /* Adesso creo la lista dei layer */
  layh = buff_firstitemptr(laylist, struct layer_header);
  i = 0;
  while (i < numlayers) {
    /* Definisco lo spazio dove verranno memorizzate le istruzioni */
    if ( ! buff_create( & layh->layer, 1, LAYER_INITIAL_SIZE) ) {
      ERRORMSG("fig_open_win", "Memoria esaurita");
      return NULL;
    }

    /* Compilo l'header del layer */
    layh->ID = 0x7279616c;  /* 0x7279616c = "layr" */
    //layh->color = ???;
    layh->numcmnd = 0;
    layh->previous = i++;
    layh->next = (i + 1) % numlayers;

    ++layh;    /* Passo al prossimo layer */
  }

  wd = (grp_window *) malloc( sizeof(grp_window) );
  if ( (wd == NULL) ) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  wd->ptr = buff_ptr(laylist);

  wd->wrdep = (void *) figh;
  wd->save = not_available;
  wd->lowfn = fig_lowfn;
  wd->midfn = fig_midfn;

  PRNMSG("Ok!\n");
  return wd;
}

/* DESCRIZIONE: Elimina un layer con tutto il suo contenuto.
 *  l e' il numero del layer da distruggere.
 */
int fig_destroy_layer(int l)
{
  struct fig_header *figh;
  struct layer_header *flayh, *llayh, *layh;
  buff *laylist;
  int p, n;

  PRNMSG("fig_destroy_layer:\n  ");

  /* Trovo l'header della figura attualmente attiva */
  figh = (struct fig_header *) grp_win->wrdep;

  if (figh->numlayers < 2) {
    ERRORMSG("fig_destroy_layer", "Figura senza layers");
    return 0;
  }

  l = CIRCULAR_INDEX(figh->numlayers, l);

  /* Trovo l'header del layer l */
  laylist = & figh->layerlist;
  flayh = buff_ptr(laylist);
  llayh = flayh + l - 1;

  /* Iniziamo col cancellare i comandi inseriti nel layer */
  buff_free(& llayh->layer);

  /* Togliamo l'header del layer dalla lista */
  p = llayh->previous;
  n = llayh->next;
  if (p == 0) {
    /* l e' il primo elemento della lista, quindi non e' l'ultimo, cioe'
     * n e' diverso da 0. n sara' il nuovo primo layer!
     */
    figh->top = n;
    layh = flayh + n - 1;
    layh->previous = 0;

  } else if (n == 0) {
    /* l e' l'ultimo elemento della lista, quindi non e' il primo, cioe'
     * p e' diverso da 0. p sara' il nuovo ultimo layer!
     */
    figh->bottom = p;
    layh = flayh + p - 1;
    layh->next = 0;

  } else {
    /* l non e' ne' il primo, ne' l'ultimo elemento della lista, cioe'
     * n e p diversi entrambi da 0.
     */
    layh = flayh + p - 1;
    layh->next = n;
    layh = flayh + n - 1;
    layh->previous = p;
  }

  /* Contrassegno l'header come vuoto e lo metto nella catena
   * degli header vuoti
   */
  llayh->ID = 0x65657266; /* 0x65657266 = "free" */
  llayh->next = figh->firstfree;
  figh->firstfree = l;

  /* Aggiorno i dati sulla figura */
  --figh->numlayers;
  if ( (figh->current == l) ) {
    WARNINGMSG("fig_destroy_layer", "Layer attivo distrutto: nuovo layer attivo = 1");
    fig_select_layer(1);
  }

  return 1;
}

/* DESCRIZIONE: Crea un nuovo layer vuoto e ne restituisce il numero
 *  identificativo (> 0) o 0 in caso di errore.
 */
int fig_new_layer(void)
{
  struct fig_header *figh;
  struct layer_header *flayh, *llayh, *layh;
  buff *laylist;
  int l;

  PRNMSG("fig_new_layer:\n  ");

  /* Trovo l'header della figura attualmente attiva */
  figh = (struct fig_header *) grp_win->wrdep;

  laylist = & figh->layerlist;

  if (figh->firstfree == 0) {
    /* Devo ingrandire la lista */
    l = ++buff_numitem(laylist);
    if ( ! buff_bigenough( laylist, l ) ) {
      ERRORMSG("fig_new_layer", "Memoria esaurita");
      return 0;
    }

    /* Trovo l'header del nuovo layer */
    flayh = buff_ptr(laylist);
    llayh = flayh + l - 1;

    /* buff_bigenough puo' cambiare l'indirizzo della lista dei layer */
    fig_select_layer( figh->current );

  } else {
    /* Esiste un posto vuoto: lo occupo! */
    /* Trovo l'header del layer libero (figh->firstfree) */
    l = figh->firstfree;
    flayh = buff_ptr(laylist);
    llayh = flayh + l - 1;

    /* Verifica che si tratta di un header libero */
    if ( llayh->ID != 0x65657266 ) {  /* 0x65657266 = "free" */
      ERRORMSG("fig_new_layer", "Errore interno (bad layer ID, 1)");
      return 0;
    }
    /* Ora posso prendermelo! */
    figh->firstfree = llayh->next;
  }

  /* Definisco lo spazio dove verranno memorizzate le istruzioni */
  if ( ! buff_create( & llayh->layer, 1, LAYER_INITIAL_SIZE) ) {
    ERRORMSG("fig_new_layer", "Memoria esaurita");
    return 0;
  }

  /* Allungo la lista dei layers: il nuovo andra' sopra gli altri! */
  layh = flayh + figh->bottom - 1;
  layh->next = l;

  /* Compilo l'header del layer */
  layh->ID = 0x7279616c;  /* 0x7279616c = "layr" */

  //layh->color = ???;
  llayh->numcmnd = 0;
  llayh->previous = figh->bottom;
  llayh->next = 0;
  figh->bottom = l;
  ++figh->numlayers;

  return l;
}

/* DESCRIZIONE: Seleziona il layer l: fino alla prossima istruzione
 * fig_select_layer, i comandi grafici saranno inviati a quel layer.
 */
void fig_select_layer(int l)
{
  buff *laylist;
  struct fig_header *figh;
  struct layer_header *layh;

  PRNMSG("fig_select_layer:\n  ");

  /* Trovo l'header della figura attualmente attiva */
  figh = (struct fig_header *) grp_win->wrdep;

  /* Setto il layer attivo a l */
  l = CIRCULAR_INDEX(figh->numlayers, l);
  figh->current = l;

  /* Trovo l'header del layer l */
  laylist = & figh->layerlist;
  layh = buff_ptr(laylist) + l - 1;
  /* Per convenzione grp_win->ptr punta a tale header: lo setto! */
  grp_win->ptr = layh;

  return;
}

/* DESCRIZIONE: Pulisce il contenuto del layer l.
 */
void fig_clear_layer(int l)
{
  buff *laylist;
  struct fig_header *figh;
  struct layer_header *layh;

  PRNMSG("fig_clear_layer:\n  ");

  /* Trovo l'header della figura attualmente attiva */
  figh = (struct fig_header *) grp_win->wrdep;

  /* Trovo l'header del layer l */
  l = CIRCULAR_INDEX(figh->numlayers, l);
  laylist = & figh->layerlist;
  layh = buff_ptr(laylist) + l - 1;

  /* Cancello il contenuto del layer */
  layh->numcmnd = 0;
  if ( !buff_clear( & layh->layer ) ) {
    ERRORMSG("fig_clear_layer", "Memoria esaurita");
  }

  if ( figh->current == l )
    fig_select_layer(l);

  return;
}

/***************************************************************************************/
/* PROCEDURE CORRISPONDENTI AI COMANDI DA "CATTURARE" */

/* Nei layers le istruzioni vengono memorizzate argomento per argomento.
 * Ogni istruzione e' costituita da un header iniziale, fatto cosi':
 *  Nome Tipo  Descrizione
 *------|-----|-------------------------------------
 *  ID   long  Numero che identifica l'istruzione
 *  dim  long  Dimesione in byte dell'istruzione (header escluso)
 * Questo header e' poi seguito da una struttura contenente i valori
 * degli argomenti del comando specificato.
 */

/* Enumero gli ID di tutti i comandi */
enum ID_type {
  ID_rreset = 1, ID_rinit, ID_rdraw, ID_rline,
  ID_rcong, ID_rcurve, ID_rcircle, ID_rfgcolor, ID_rbgcolor
};

struct cmnd_header {
  long  ID;
  long  size;
};

/* Questa macro contiene gran parte del codice comune a tutte le ... */
#define BEGIN_CMND(name, argsize) \
  struct layer_header *layh; \
  buff *layer; \
  struct cmnd_header *cmndh; \
  int ni; \
  register void *ip; \
\
  PRNMSG(name":\n  ");\
  layh = (struct layer_header *) grp_win->ptr; \
  if ( layh->ID != 0x7279616c) {  /* 0x7279616c = "layr" */ \
    ERRORMSG(name, "Errore interno (bad layer ID, 2)"); \
    PRNMSG("l'ID del header di layer e' errato\n");\
    return; \
  } \
\
  layer = & layh->layer; \
  ni = buff_numitem(layer); \
  if ( !buff_bigenough( layer, \
   buff_numitem(layer) += sizeof(struct cmnd_header) + argsize) \
  ) { \
    ERRORMSG(name, "Memoria esaurita"); \
    return; \
  } \
\
  ip = (void *) buff_ptr(layer) + (int) ni;  /* Instruction-Pointer */ \
\
  /* Trovo l'header di istruzione e la lista argomenti */ \
  cmndh = (struct cmnd_header *) ip; \
\
  /* Incremento il numero di comandi inseriti nel layer */ \
  ++layh->numcmnd

void fig_rreset(void)
{
  BEGIN_CMND( "fig_rreset", 0 );
  *cmndh = (struct cmnd_header) {ID_rreset, 0};
  PRNMSG("Ok!\n");
  return;
}

void fig_rinit(void)
{
  BEGIN_CMND( "fig_rinit", 0 );
  *cmndh = (struct cmnd_header) {ID_rinit, 0};
  PRNMSG("Ok!\n");
  return;
}

void fig_rdraw(void)
{
  BEGIN_CMND( "fig_rdraw", 0 );
  *cmndh = (struct cmnd_header) {ID_rdraw, 0};
  PRNMSG("Ok!\n");
  return;
}

void fig_rline(Point a, Point b)
{
  struct cmnd_args {Point a, b;} *cmnda;
  BEGIN_CMND( "fig_rline", sizeof(struct cmnd_args) );
  cmnda = (struct cmnd_args *) ( ip + sizeof(struct cmnd_header) );

  *cmndh = (struct cmnd_header) {ID_rline, sizeof(struct cmnd_args) };
  *cmnda = (struct cmnd_args) {a, b};
  PRNMSG("Ok!\n");
  return;
}

void fig_rcong(Point a, Point b, Point c)
{
  struct cmnd_args {Point a, b, c;} *cmnda;
  BEGIN_CMND( "fig_rcong", sizeof(struct cmnd_args) );
  cmnda = (struct cmnd_args *) ( ip + sizeof(struct cmnd_header) );

  *cmndh = (struct cmnd_header) {ID_rcong, sizeof(struct cmnd_args) };
  *cmnda = (struct cmnd_args) {a, b, c};
  PRNMSG("Ok!\n");
  return;
}

void fig_rcurve(Point a, Point b, Point c, Real cut)
{
  struct cmnd_args {Point a, b, c; Real cut;} *cmnda;
  BEGIN_CMND( "fig_rcurve", sizeof(struct cmnd_args) );
  cmnda = (struct cmnd_args *) ( ip + sizeof(struct cmnd_header) );

  *cmndh = (struct cmnd_header) {ID_rcurve, sizeof(struct cmnd_args) };
  *cmnda = (struct cmnd_args) {a, b, c, cut};
  PRNMSG("Ok!\n");
  return;
}

void fig_rcircle(Point ctr, Point a, Point b)
{
  struct cmnd_args {Point ctr, a, b;} *cmnda;
  BEGIN_CMND( "fig_rcircle", sizeof(struct cmnd_args) );
  cmnda = (struct cmnd_args *) ( ip + sizeof(struct cmnd_header) );

  *cmndh = (struct cmnd_header) {ID_rcircle, sizeof(struct cmnd_args) };
  *cmnda = (struct cmnd_args) {ctr, a, b};
  PRNMSG("Ok!\n");
  return;
}

void fig_rfgcolor(Real r, Real g, Real b)
{
  struct cmnd_args {Real r, g, b;} *cmnda;
  BEGIN_CMND( "fig_rfgcolor", sizeof(struct cmnd_args) );
  cmnda = (struct cmnd_args *) ( ip + sizeof(struct cmnd_header) );

  *cmndh = (struct cmnd_header) {ID_rfgcolor, sizeof(struct cmnd_args) };
  *cmnda = (struct cmnd_args) {r, g, b};
  PRNMSG("Ok!\n");
  return;
}

void fig_rbgcolor(Real r, Real g, Real b)
{
  struct cmnd_args {Real r, g, b;} *cmnda;
  BEGIN_CMND( "fig_rbgcolor", sizeof(struct cmnd_args) );
  cmnda = (struct cmnd_args *) ( ip + sizeof(struct cmnd_header) );

  *cmndh = (struct cmnd_header) {ID_rbgcolor, sizeof(struct cmnd_args) };
  *cmnda = (struct cmnd_args) {r, g, b};
  PRNMSG("Ok!\n");
  return;
}

/***************************************************************************************/
/* PROCEDURE PER DISEGNARE I LAYER SULLA FINESTRA ATTIVA */

/* Matrice usata nella trasformazione lineare di fig_ltransform */
Real fig_matrix[6] = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};

/* DESCRIZIONE: Applica una trasformazione lineare agli n punti p[].
 */
void fig_ltransform(Point *p, int n)
{
  register Real m11, m12, m13, m21, m22, m23;
  register Real px;

  m11 = fig_matrix[0]; m12 = fig_matrix[1];
  m21 = fig_matrix[2]; m22 = fig_matrix[3];
  m13 = fig_matrix[4]; m23 = fig_matrix[5];

  for (; n-- > 0; p++) {
    p->x = m11 * (px = p->x) + m12 * p->y + m13;
    p->y = m21 * px + m22 * p->y + m23;
  }

  return;
}

/* DESCRIZIONE: Disegna il layer l della figura source sulla finestra grafica
 *  attualmente in uso. Questo disegno puo' essere "filtrato" e cioe' puo' essere
 *  ruotato, ridimensionato, traslato, ...
 *  A tal scopo occorre impostare la trasformazione con ???????????????????????
 * NOTA: Non e' possibile copiare un layer in se' stesso: infatti l'esecuzione
 *  di comandi sul layer puo' causare un suo ridimensionamento, cioe' una realloc.
 *  In tal caso il fig_draw_layer continuera' a leggere sui vecchi indirizzi,
 *  senza controllare questa eventualita'! (problema   R I M O S S O !))
 */
void fig_draw_layer(grp_window *source, int l)
{
  struct fig_header *figh;
  struct layer_header *layh;
  buff *laylist, *layer;
  union {
   struct cmnd_header *hdr;
   void *ptr;
  } cmnd;
  long ID, ip, nc;
  Point tp[3];

  PRNMSG("fig_draw_layer:\n  ");

  /* Trovo l'header della figura "source" */
  figh = (struct fig_header *) source->wrdep;

  l = CIRCULAR_INDEX(figh->numlayers, l);

  /* Trovo l'header del layer l */
  laylist = & figh->layerlist;
  layh = buff_itemptr(laylist, struct layer_header, l);
  if ( layh->ID != 0x7279616c) {  /* 0x7279616c = "layr" */
    ERRORMSG( "fig_draw_layer", "Errore interno (bad layer ID), 3" );
    return;
  }

  layer = & layh->layer;

  ip = 0;    /* ip tiene traccia della posizione del buffer */
  nc = layh->numcmnd;      /* Numero dei comandi inseriti nel layer */

  PRNMSG("Layer contenente "); PRNINTG(nc); PRNMSG(" comandi.\n");

  /* Continuo fino ad aver eseguito ogni comando */
  while (nc > 0) {

    /* Trovo l'indirizzo dell'istruzione corrente.
     * NOTA: devo ricalcolarmelo ogni volta, dato che durante il ciclo while
     *  il buffer potrebbe essere ri-allocato e buff_ptr(layer) potrebbe
     *  cambiare! Questo accade quando copio un layer in se' stesso,
     *  in tal caso le istruzioni lette dal layer vengono anche scritte
     *  sullo stesso layer, cioe' le dimensioni del layer aumentano
     *  e potrebbe essere necessaria una realloc.
     */
    cmnd.ptr = (void *) buff_ptr(layer) + (long) ip;

    ID = cmnd.hdr->ID;
    ip += sizeof(struct cmnd_header) + cmnd.hdr->size;

    cmnd.ptr += sizeof(struct cmnd_header);

    switch (ID) {
     case ID_rreset:
      grp_rreset();
      break;

     case ID_rinit:
      grp_rinit();
      break;

     case ID_rdraw:
      grp_rdraw();
      break;

     case ID_rline: {
      struct {Point a, b;} *args = cmnd.ptr;

      tp[0] = args->a; tp[1] = args->b;
      fig_ltransform( tp, 2 );
      grp_rline( tp[0], tp[1] );
      } break;

     case ID_rcong: {
      struct {Point a, b, c;} *args = cmnd.ptr;

      tp[0] = args->a; tp[1] = args->b; tp[2] = args->c;
      fig_ltransform( tp, 3 );
      grp_rcong( tp[0], tp[1], tp[2] );
      } break;

     case ID_rcurve: {
      struct {Point a, b, c; Real cut;} *args = cmnd.ptr;

      tp[0] = args->a; tp[1] = args->b; tp[2] = args->c;
      fig_ltransform( tp, 3 );
      grp_rcurve( tp[0], tp[1], tp[2], args->cut );
      } break;

     case ID_rcircle: {
      struct {Point ctr, a, b;} *args = cmnd.ptr;

      tp[0] = args->ctr; tp[1] = args->a; tp[2] = args->b;
      fig_ltransform( tp, 3 );
      grp_rcircle( tp[0], tp[1], tp[2] );
      } break;

     case ID_rfgcolor: {
      struct {Real r, g, b;} *args = cmnd.ptr;

      grp_rfgcolor( args->r, args->g, args->b );
      } break;

     case ID_rbgcolor: {
      struct {Real r, g, b;} *args = cmnd.ptr;

      grp_rbgcolor( args->r, args->g, args->b );
      } break;

     default:
      ERRORMSG( "fig_draw_layer", "Comando grafico non riconosciuto" );
      return;
      break;
    }

    /* Passo al prossimo comando */
    --nc;
  }

  return;
}

/* DESCRIZIONE: Disegna la figura source sulla finestra grafica attualmente
 *  in uso. I layer vengono disegnati uno dietro l'altro con fig_draw_layer
 *  a partire dal "layer bottom", fino ad arrivare al "layer top".
 */
void fig_draw_fig(grp_window *source)
{
  struct fig_header *figh;
  struct layer_header *layh;
  buff *laylist;
  long nl, cl;

  PRNMSG("fig_draw_fig:\n  ");

  /* Trovo l'header della figura "source" */
  figh = (struct fig_header *) source->wrdep;

  laylist = & figh->layerlist;

  /* Parto dall'header che sta sotto tutti gli altri */
  cl = figh->bottom;

  PRNMSG("Inizio a disegnare i "); PRNINTG(figh->numlayers);
  PRNMSG(" layer(s)!\n  ");

  for ( nl = figh->numlayers; nl > 0; nl-- ) {
    /* Disegno il layer cl */
    fig_draw_layer(source, cl);

    /* Ora passo a quello successivo che lo ricopre */
    /* Trovo l'header del layer cl */
    layh = (void *) buff_ptr(laylist) + (long) (cl - 1);
    cl = layh->previous;
  }

  if (cl != 0) {
    ERRORMSG( "fig_draw_fig", "Errore interno (layer chain)" );
    PRNMSG("Catena dei layer danneggiata!\n");
    return;
  }

  PRNMSG("Ok!\n");
  return;
}
