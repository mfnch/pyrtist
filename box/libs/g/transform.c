/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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

/* This file implements an algorithm to automatically compute a transformation
 * matrix given a specification of allowed transformations and constraints.
 * The constraints are given as triples (source_point, target_point, weight),
 * where:
 *
 *   'source_point': is a point in the source figure (the one which is
 *                   being placed),
 *   'target_point': is a point in the destination figure,
 *         'weight': is a positive real number which decides how important the
 *                   constraint is.
 *
 * The algorithm will translate, rotate, scale or/and invert the source figure
 * (depending on what transformations are allowed) to make sure that all the
 * source points get as close as possible to the corresponding target points,
 * weighting each constraint by the appropriate weight.
 * The algorithm is non-iterative, it is an exact analytical solution to the
 * problem above.
 */

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <box/types.h>

#include "error.h"
#include "graphic.h"
#include "transform.h"

/*#define DEBUG*/

#if 0
/* Quantita' che definiscono la trasformazione */
static Real Qx, Qy, Tx, Ty;
static Real theta, cos_theta, sin_theta, s, cos_tau, sin_tau/*,tau*/;
#endif

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

typedef enum {
  BOXGAUTOTRANSFORMERR_NO_POINTS = 1,
  BOXGAUTOTRANSFORMERR_ZERO_WEIGHTS,
  BOXGAUTOTRANSFORMERR_NOT_IMPLEMENTED
} BoxGAutoTransformErr;


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
BoxGAutoTransformErr
BoxG_Auto_Transform(BoxGTransform *transform,
                    BoxPoint *src, BoxPoint *dst, BoxReal *weight, int n,
		    BoxGAllow allowed_transforms) {
  int i;
  BoxReal weights_sum;

  if (n < 1)
    return BOXGAUTOTRANSFORMERR_NO_POINTS;

#ifdef DEBUG
  for (i = 0; i < n; i++) {
    printf("src[%d] = (%g, %g)\t<--near[%g]-->\tdst[%d] = (%g, %g)\n...",
	   i, src[i].x, src[i].y, weight[i], i, dst[i].x, dst[i].y);
  }
#endif

  /* Compute the sum of the weights */
  weights_sum = 0.0;
  for (i = 0; i < n; i++)
    weights_sum += weight[i];

  if (weights_sum == 0.0)
    return BOXGAUTOTRANSFORMERR_ZERO_WEIGHTS;

  if (allowed_transforms & BOXGALLOW_TRANSLATE) {
    int allowed_translations = allowed_transforms & BOXGALLOW_TRANSLATE;

    /* One or both of the translational degrees of freedom have to be
     * handled automatically: we automatically determine Q and T.
     */

    /* Compute weighted averages of F and R: F_avg and R_avg */
    BoxPoint src_avg = (BoxPoint) {0.0, 0.0},
             dst_avg = (BoxPoint) {0.0, 0.0};

    for (i = 0; i < n; i++) {
      BoxReal w = weight[i];
      src_avg.x += src[i].x * w;
      src_avg.y += src[i].y * w;
      dst_avg.x += dst[i].x * w;
      dst_avg.y += dst[i].y * w;
    }

    src_avg.x /= weights_sum;
    src_avg.y /= weights_sum;
    dst_avg.x /= weights_sum;
    dst_avg.y /= weights_sum;

    /* Set the rotation center */
    transform->rotation_center = src_avg;

    /* Compute the translation vector */
    if (allowed_transforms == BOXGALLOW_TRANSLATE_X) {
      /* x: automatic, y: manual */
      transform->translation.x = dst_avg.x - src_avg.x;
      transform->translation.y -= src_avg.y;

    } else if (allowed_transforms == BOXGALLOW_TRANSLATE_Y) {
      /* x: manual, y: automatic */
      transform->translation.x -= src_avg.x;
      transform->translation.y = dst_avg.y - src_avg.y;

    } else {
      /* x, y: automatic */
      transform->translation.x = dst_avg.x - src_avg.x;
      transform->translation.y = dst_avg.y - src_avg.y;
    }
  }

  /* Exit if translating was the only thing we had to do */
  if ((allowed_transforms & BOXGALLOW_ALL & ~BOXGALLOW_TRANSLATE) == 0)
    return 0;

  else {	
    /* Now we compute all the averages we need in the next steps */
    BoxPoint g_avg, g2_avg, i_avg, j_avg;
    BoxPoint U = 
      (BoxPoint) {transform->translation.x + transform->rotation_center.x,
		  transform->translation.y + transform->rotation_center.y};

    for (i = 0; i < n; i++) {
      BoxReal w = weight[i];
      BoxPoint
	 g = (BoxPoint) {src[i].x - transform->rotation_center.x,
		         src[i].y - transform->rotation_center.y},

	wg = (BoxPoint) {w * g.x,
			 w * g.y},

         s = (BoxPoint) {dst[i].x - U.x,
			 dst[i].y - U.y};

      g2_avg.x += wg.x * g.x;
      g2_avg.y += wg.y * g.y;

      i_avg.x += wg.x * s.x;
      i_avg.y += wg.y * s.y;

      j_avg.x += wg.x * s.y;
      j_avg.y += wg.y * s.x;
    }

    i_avg.x /= weights_sum; /*(w = wsum * sg2x);*/
    j_avg.x /= weights_sum;
    i_avg.y /= weights_sum; /*(w = wsum * sg2y);*/
    j_avg.y = -(j_avg.y/weights_sum);
    g2_avg.x /= weights_sum;
    g2_avg.y /= weights_sum;
    g_avg.x = sqrt(g2_avg.x);
    g_avg.y = sqrt(g2_avg.y);

#ifdef DEBUG
    printf("g2x = %g\tg2y = %g\n...", g2_avg.x, g2_avg.y);
    printf("sg2x = %g\tsg2y = %g\n...", g_avg.x, g_avg.y);
    printf("ix = %g\tiy = %g\n...", i_avg.x, i_avg.y);
    printf("jx = %g\tjy = %g\n...", j_avg.x, j_avg.y);
#endif

    /* We now have three different cases to deal with */
    if ((allowed_transforms & BOXGALLOW_DEFORM) == 0) {
      /* CASE 1: Proportions are manually fixed. */
      BoxReal cos_tau = transform->scaling_cos,
              sin_tau = transform->scaling_sin;

      BoxReal A = i_avg.x * cos_tau + i_avg.y * sin_tau,
              B = j_avg.x * cos_tau + j_avg.y * sin_tau;

      if (allowed_transforms & BOXGALLOW_ROTATE) {
	/* Determino l'angolo di rotazione, se e' selezionata
	 * la rotazione automatica!
	 */
	BoxReal modAB = sqrt(A*A + B*B);
	transform->rotation_cos = A/modAB,
	transform->rotation_sin = B/modAB;
	transform->rotation_angle =
          atan2(transform->rotation_sin, transform->rotation_sin);

      } else {
	/* Se la rotazione e' manuale, conosco theta, ma non sin e cos! */
	transform->rotation_cos = cos(transform->rotation_angle);
	transform->rotation_sin = sin(transform->rotation_angle);
      }

      /* What is missing is just the scaling factor! */
      if (allowed_transforms & BOXGALLOW_SCALE) {

	BoxReal C = cos_tau*cos_tau*g2_avg.x + sin_tau*sin_tau*g2_avg.y;
	transform->scale_factor = 
          (A*transform->rotation_cos + B*transform->rotation_sin)/C;
      }
      
    } else if (allowed_transforms & BOXGALLOW_ANISOTROPIC) {
      /* CASO 2: Proportions have to be determined automatically. */
      return BOXGAUTOTRANSFORMERR_NOT_IMPLEMENTED;
      
    } else if (allowed_transforms & BOXGALLOW_INVERT) {
      /* CASO 3: Proportions are fixed, but can invert. */
      return BOXGAUTOTRANSFORMERR_NOT_IMPLEMENTED;      
    }
  }

  return 1;
}

#if 0
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
int aput_allow(const char *permissions, int *needed) {
  char p;
  int allow = 0;
  enum {NORMAL, WAIT_COMPONENT} status = NORMAL;
  enum {CLEAR=0, SET=-1} mode = SET;

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
        return 1;
      default:
        ERRORMSG("aput_allow", "La lettera non corrisponde "
                 "ad una trasformazione ammessa");
        return 0;
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

  return 0;
}
#endif
