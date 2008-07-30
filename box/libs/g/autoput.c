/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/* $Id$
 *
 * Questo file contiene le procedure che sono usate per posizionare
 * automaticamente le figure (servono per calcolare angoli di rotazione,
 * vettori di traslazione, etc. in maniera automatica).
 */

#include <stdio.h>
#include <math.h>
#include <ctype.h>

/* De-commentare per includere i messaggi di debug */
/*#define DEBUG*/

#include "debug.h"
#include "types.h"
#include "error.h"
#include "graphic.h"
#include "autoput.h"

/* Quantita' che definiscono la trasformazione */
static Real Qx, Qy, Tx, Ty;
static Real theta, cos_theta, sin_theta, s, cos_tau, sin_tau/*,tau*/;

#define AUTO_TRANSLATION(n) ( ((n) & 0x3) != 0 )
#define AUTO_TRANSLATION_X(n) ( ((n) & 0x1) != 0 )
#define AUTO_TRANSLATION_Y(n) ( ((n) & 0x2) != 0 )
#define AUTO_ROTATION(n) ( ((n) & 0x4) != 0 )
#define AUTO_SCALE(n) ( ((n) & 0x8) != 0 )
#define AUTO_PROPORTION(n) ( ((n) & 0x10) != 0)
#define AUTO_INVERSION(n) ( ((n) & 0x20) != 0)
#define FIXED_PROPORTION(n) ( ((n) & 0x30) == 0)

#define ONLY_TRANSLATION(n) ( (n & ~0x3) == 0 )

#define SET_TRANSLATION_X(n, mode)  ( n = (n & ~0x1) | (mode & 0x1) )
#define SET_TRANSLATION_Y(n, mode)  ( n = (n & ~0x2) | (mode & 0x2) )
#define SET_ROTATION(n, mode)  ( n = (n & ~0x4) | (mode & 0x4) )
#define SET_SCALE(n, mode)    ( n = (n & ~0x8) | (mode & 0x8) )
#define SET_DEFORM(n, mode)    ( n = (n & ~0x10) | (mode & 0x10) )
#define SET_INVERSION(n, mode)  ( n = (n & ~0x20) | (mode & 0x20) )


/* DESCRIZIONE: Questa funzione serve a posizionare una figura.
 *  Essa esegue i calcoli necessari per determinare come la figura debba essere
 *  traslata, ruotata e/o ridimensionata per soddisfare i vincoli imposti
 *  dall'utente. Il numero di vincoli e' specificato da n e ciascuno di essi
 *  comprende 3 argomenti:
 *    vincolo n-esimo ---> ( F[n], R[n], weight[n] )
 *  F[n] e' un punto della figura, R e' un punto di Riferimento dello sfondo
 *  (sfondo = "foglio" su cui verra' disposta la figura), mentre weight
 *  e' un numero reale che specifica la "forza" del vincolo.
 *  Ogni vincolo puo' essere pensato come una "molla" che collega F con R:
 *  la figura ruotera'/traslera' e/o si ridimensionera' in modo che le molle
 *  si accorcino. weight e' la forza della molla, e serve per rendere un vincolo
 *  (= una molla) piu' importante degli altri.
 *  L'utente puo' decidere quali saranno le "liberta'" dell'oggetto.
 *  Puo' cioe' stabilire se le molle provocheranno solo una rotazione
 *  dell'oggetto o se provocheranno solo una sua traslazione, o ridimensionamento,
 *  o una qualsiasi combinazione di tali trasformazioni.
 *  Le liberta' sono determinate dall'argomento chiamato needed, secondo
 *  lo schema seguente:
 *   BIT 0 di needed  -->  L'oggetto traslera' lungo X (automaticamente)
 *   BIT 1 di needed  -->  L'oggetto traslera' lungo Y (automaticamente)
 *   BIT 2 di needed  -->  L'oggetto ruotera' (automaticamente)
 *   BIT 3 di needed  -->  L'oggetto sara' ridimensionato (automaticamente)
 *   BIT 4 di needed  -->  L'oggetto sara' "sproporzionato" (automaticamente)
 *   BIT 5 di needed  -->  L'oggetto sara' riflesso (automaticamente)
 */
int aput_autoput(Point *F, Point *R, Real *weight, int n, int needed)
{
  register int i;
  register Real wsum, *wptr;
  register Real ix, iy, jx, jy, g2x, g2y, sg2x, sg2y;
  Point *Fptr, *Rptr;

  PRNMSG("aput_autoput: Calcolo la trasformazione dai vincoli...\n");
  PRNMSG("...needed = "); PRNINTG(needed); PRNMSG("\n...");

  /* n > 0 */
  if (n < 1) {
    WARNINGMSG("autoput", "Numero di punti inferiore a 1");
    EXIT_ERR("n < 1\n");
  }

  /* Stampo i vincoli, se sono in fase di DEBUG */
  #ifdef DEBUG
    for (i=0; i<n; i++) {
      printf( "F[%d] = (%g, %g)\t<--near[%g]-->\tR[%d] = (%g, %g) \n...",
       i, F[i].x, F[i].y, weight[i], i, R[i].x, R[i].y );
    }
  #endif

  if ( AUTO_TRANSLATION(needed) ) {
    /* Entrambi o solo uno dei due gradi traslazionali
     * vanno trattati automaticamente: aggiusto Q e T!
     */
    if ( ! AUTO_TRANSLATION_X(needed) ) {
      /* x manuale, y automatico */
      register Real MFx, MFy, MRy, w;

      PRNMSG("auto-traslazione lungo y\n...");

      /* Calcoliamo le medie pesate di F.y e R.y */
      wsum = *(wptr = weight);
      MFx = (Fptr = F)->x * wsum;
      MFy = Fptr->y * wsum;
      MRy = (Rptr = R)->y * wsum;
      for (i=1; i<n; i++) {
        wsum += (w = *(++wptr));
        MFx += (++Fptr)->x * w;
        MFy += Fptr->y * w;
        MRy += (++Rptr)->y * w;
      }
      MFx /= wsum;
      MFy /= wsum;
      MRy /= wsum;

      Qx = MFx;  /* Ignoro il valore di Q impostato dall'utente */
      Qy = MFy;
      Ty = MRy - MFy;
      Tx -= MFx;

    } else if ( ! AUTO_TRANSLATION_Y(needed) ) {
      /* x automatico , y manuale */
      /* Calcoliamo le medie pesate di f.x e r.x */

      PRNMSG("auto-traslazione lungo x\n...");
      printf("Non ancora implementato!\n");
      return 0;

    } else {
      /* x e y entrambi automatici */
      register Real MFx, MFy, MRx, MRy, w;

      PRNMSG("auto-traslazione(lungo x-y)\n...");

      /* Calcoliamo le medie pesate di F e R */
      wsum = *(wptr = weight);
      MFx = (Fptr = F)->x * wsum;
      MFy = Fptr->y * wsum;
      MRx = (Rptr = R)->x * wsum;
      MRy = Rptr->y * wsum;
      for (i=1; i<n; i++) {
        wsum += (w = *(++wptr));
        MFx += (++Fptr)->x * w;
        MFy += Fptr->y * w;
        MRx += (++Rptr)->x * w;
        MRy += Rptr->y * w;
      }
      MFx /= wsum;
      MFy /= wsum;
      MRx /= wsum;
      MRy /= wsum;

      Qx = MFx;  /* Ignoro il valore di Q impostato dall'utente */
      Qy = MFy;
      Tx = MRx - MFx;
      Ty = MRy - MFy;
    }

  } else {
    PRNMSG("traslazione manuale\n...");

    /* Calcoliamo wsum */
    wsum = *(wptr = weight);
    for (i=1; i<n; i++)
      wsum += *(++wptr);
  }

  /* Se non c'e' altro da fare ESCO! */
  if ( ONLY_TRANSLATION(needed) ) return 1;

  /* Ora calcoliamo tutte le medie di cui abbiamo bisogno
   * per eseguire la minimizzazione
   */
  {
    register Real
     w, gx, gy, wgx, wgy, sx, sy, Ux, Uy;

    Ux = Tx + Qx;
    Uy = Ty + Qy;

    gx = (Fptr = F)->x - Qx;
    gy = Fptr->y - Qy;
    wgx = gx * (w = *(wptr = weight));
    wgy = gy * w;
    g2x = wgx*gx;
    g2y = wgy*gy;
    sx = (Rptr = R)->x - Ux;
    sy = Rptr->y - Uy;
    ix = wgx * sx;
    iy = wgy * sy;
    jx = wgx * sy;
    jy = wgy * sx;
    for (i=1; i<n; i++) {
      gx = (++Fptr)->x - Qx;
      gy = Fptr->y - Qy;
      wgx = gx * (w = *(++wptr));
      wgy = gy * w;
      g2x += wgx*gx;
      g2y += wgy*gy;
      sx = (++Rptr)->x - Ux;
      sy = Rptr->y - Uy;
      ix += wgx * sx;
      iy += wgy * sy;
      jx += wgx * sy;
      jy += wgy * sx;
    }
    sg2x = sqrt(g2x = g2x/wsum);
    sg2y = sqrt(g2y = g2y/wsum);
    ix /= wsum; /*(w = wsum * sg2x);*/
    jx /= wsum;
    iy /= wsum; /*(w = wsum * sg2y);*/
    jy = -(jy/wsum);
  }

  #ifdef DEBUG
    printf("g2x = %g\tg2y = %g\n...", g2x, g2y);
    printf("sg2x = %g\tsg2y = %g\n...", sg2x, sg2y);
    printf("ix = %g\tiy = %g\n...", ix, iy);
    printf("jx = %g\tjy = %g\n...", jx, jy);
  #endif

  /* Abbiamo ora 3 distinti casi da trattare */
  if ( FIXED_PROPORTION(needed) ) {
    /* CASO 1: Proporzioni fissate manualmente */
    PRNMSG("proporzioni fisse\n...");

    Real A = ix * cos_tau + iy * sin_tau;
    Real B = jx * cos_tau + jy * sin_tau;

    if ( AUTO_ROTATION(needed) ) {
      /* Determino l'angolo di rotazione, se e' selezionata
       * la rotazione automatica!
       */
      Real modAB = sqrt(A*A + B*B);
      cos_theta = A/modAB;
      sin_theta = B/modAB;
      theta = atan2(sin_theta, cos_theta);

    } else {
      /* Se la rotazione e' manuale, conosco theta, ma non sin e cos! */
      cos_theta = cos(theta);
      sin_theta = sin(theta);
    }

    /* Ora non resta che calcolare il fattore di ingrandimento! */
    if ( AUTO_SCALE(needed) ) {
      Real C = cos_tau * cos_tau * g2x + sin_tau * sin_tau * g2y;
      s = ( A * cos_theta + B * sin_theta ) / C;
    }

  } else if ( AUTO_PROPORTION(needed) ) {
    /* CASO 2: Proporzioni automatiche */
    PRNMSG("proporzioni automatiche\n...");

  } else if ( AUTO_INVERSION(needed) ) {
    /* CASO 3: Proporzioni fisse a meno di un inversione */
    PRNMSG("proporzioni fisse, inversione automatica\n...");

  }

  return 1;
}

/* DESCRIZIONE: Setta i parametri della trasformazione prima di avviare
 *  il calcolo automatico con aput_autoput.
 */
void aput_set(Point *rot_center, Point *trsl_vect,
              Real *rot_angle, Real *scale_x, Real *scale_y ) {
  register Real sx, sy;

  Qx = rot_center->x;
  Qy = rot_center->y;
  Tx = trsl_vect->x;
  Ty = trsl_vect->y;

  theta = *rot_angle;

  sx = *scale_x; sy = *scale_y;
  s = sqrt( sx*sx + sy*sy );
  cos_tau = sx/s; sin_tau = sy/s;

  return;
}

/* DESCRIZIONE: Preleva i parametri calcolati in modo automatico da aput_autoput
 */
void aput_get(Point *rot_center, Point *trsl_vect,
              Real *rot_angle, Real *scale_x, Real *scale_y ) {
  rot_center->x = Qx;
  rot_center->y = Qy;
  trsl_vect->x = Tx;
  trsl_vect->y = Ty;

  *rot_angle = theta;
  *scale_x = s * cos_tau;
  *scale_y = s * sin_tau;
}

/* DESCRIZIONE: Converte una stringa in un valore di "needed" da usare nella
 *  funzione aput_autoput(). La stringa specifica quali trasformazioni
 *  debbano essere eseguite automaticamente e quali manualmente.
 *  Essa e' costituita da una successione di lettere, ciascuna delle quali
 *  specifica una trasformazione da eseguire. Se - ad esempio -
 *  voglio traslare e ruotare automaticamente, allora permission = "tr"
 *  oppure = "rt" (l'ordine non conta!). Le associazioni lettere-trasform. sono:
 *   "t" -> traslazione, "r" -> rotazione, "s" -> scala,
 *   "d" -> deformazione (ovvero: cambio di proporzioni X-Y),
 *   "i" -> inversione (ovvero: riflessione)
 *  Si puo' anche usare "tx" per traslazione solo lungo x, analogamente "ty".
 *  E' possibile premettere a ciascuna lettera un segno: "-" e' opposto al "+",
 *  nel senso che "+" aggiunge, mentre "-" toglie la trasformazione
 *  (esempio "rt" = "+r+t"). Il segno vale per tutte le lettere seguenti.
 *  Se il primo carattere della stringa e' uno spazio, allora sara' continuata
 *  la stringa a cui corrisponde il valore needed.
 *  (esempio: se olp_perm = aput_allow("s", 0) e x = aput_allow(" d", old_perm),
 *  allora x = aput_allow("sd", 0) )
 *  Gli altri spazi della stringa vengono ignorati.
 */
int aput_allow(char *permissions, int *needed)
{
  char p;
  int allow = 0;
  enum {NORMAL, WAIT_COMPONENT} status = NORMAL;
  enum {CLEAR=0, SET=-1} mode = SET;

  PRNMSG("aput_allow: Inizio conversione stringa allow -> numero...\n...");

  p = tolower(*permissions);
  if ( p == ' ' ) { allow = *needed; }

  for (;; p = tolower(*permissions) ) {

    switch (status) {
     case NORMAL:
      switch (p) {
       case 't':
        status = WAIT_COMPONENT; break;
       case 'r':
        SET_ROTATION(allow, mode); break;
       case 's':
        SET_SCALE(allow, mode); break;
       case 'd':
        SET_DEFORM(allow, mode); break;
       case 'i':
        SET_INVERSION(allow, mode); break;
       case ' ':
        break;
       case '+':
        mode = SET;
        break;
       case '-':
        mode = CLEAR;
        break;
       case '\0':
        *needed = allow;
        EXIT_OK("Ok!\n");
       default:
        ERRORMSG("aput_allow",
         "La lettera non corrisponde ad una trasformazione ammessa" );
        EXIT_ERR("fallito(1)\n");
      }
      ++permissions;
      break;

     case WAIT_COMPONENT:
      switch (p) {
       case 'x':
        SET_TRANSLATION_X(allow, mode);
        ++permissions; break;
       case 'y':
        SET_TRANSLATION_Y(allow, mode);
        ++permissions; break;
       default:
        SET_TRANSLATION_X(allow, mode);
        SET_TRANSLATION_Y(allow, mode);
        break;
      }
      status = NORMAL;
      break;
    }

  }

  EXIT_ERR("Dovrebbe essere impossibile vedere questo messaggio!\n");
}
