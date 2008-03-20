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

/* PROCEDURE DI TRACCIATURA DI LINEE SPESSE
 */

#include <math.h>
#include <stdio.h>

#include "error.h"
#include "graphic.h"

/* Di seguito specifichiamo le strutture e le dichiarazioni necessarie
 * per le procedure di tracciatura di linee "spesse"
 */

/* Questa struttura tiene conto di cio' che si sta tracciando */
typedef struct {
  Point pnt1, pnt2; /* Vertici dell'asse della linea */
  Real sp1, sp2;    /* Spessori in corrispondenza dei due vertici */
  Point v;          /* Vettori dell'asse della linea */
  Point u;          /* Vettore v normalizzato */
  Point o;          /* Versore ortogonale al vettore dell'asse */
  Point vb[2];      /* Vettori dei due lati della linea */
  Point p[2];       /* I due punti laterali della linea */
  Point cong[2];    /* Punti di congiuntura fra i lati delle linee */
  Point vci;        /* Vettore di congiuntura interno */
  Point vertex[4];  /* Vertici della linea precedente */
  Real mod, mod2;   /* Lunghezza della linea */
} line_desc;

typedef struct {
  /* Lunghezze di giunzione interne ed esterne della linea corrente */
  double ti, te;
  /* Lunghezze di giunzione interne ed esterne della prossima linea */
  double ni, ne;
} join_style;

static int line_is_closed;
static int line_segment;
static line_desc line1, line2, firstline;
static line_desc *thsl;
static line_desc *nxtl;
static join_style *curjs;
static double cutting = 8.0;

/* Macro utilizzate in seguito */
#define MOD2(x, y)  (x*x + y*y)

/* DESCRIZIONE: Setta lo stile di giunzione. userjs e' un puntatore ad un array
 *  di 4 numeri di tipo float, che descrivono il tipo di giunzione da usare
 *  per collegare le linee fra loro.
 */
void grp_join_style(Real *userjs)
{
  curjs = (join_style *) userjs;
  return;
}

/* DESCRIZIONE: Setta il valore di cutting. Quando un angolo della spezzata
 *  e' a gomito molto stretto la congiuntura puo' sporgere troppo.
 *  Allora si puo' pensare di tagliare questa sporgenza.
 *  Il parametro di cutting serve a questo: piu' piccolo e' e piu' grande sara'
 *  il taglio. Esso deve essere positivo ed e' consigliabile sceglierlo
 *  piu' grande di 1.
 * NOTA: Se non c'e' una sporgenza rilevante il taglio non sara' praticato.
 */
void grp_cutting(Real c)
{
  if (c > 0.0)
    cutting = c;

  return;
}

/* DESCRIZIONE: Specifica gli estremi della prima linea da tracciare.
 *  Le altre linee che continuano la spezzata devono essere specificate
 *  con grp_next_line
 *  La prima parte della linea pu essere "tagliata".
 *  A tal proposito startlenght specifica la lunghezza della prima parte
 *  della linea che si vuole "omettere".
 *  sp1 e sp2 sono gli spessori della linea in corrispondenza ai due punti
 *  (x1, y1) e (x2, y2).
 */
void grp_first_line(Real x1, Real y1, Real sp1,
 Real x2, Real y2, Real sp2, Real startlenght, int is_closed)
{
  Real sl;

  /* Setto i puntatori alle strutture che contengono i dati sulle linee */
  thsl = &line1;
  nxtl = &line2;

  /* Calcolo il vettore relativo alla linea */
  thsl->v.x = (thsl->pnt2.x = x2) - (thsl->pnt1.x = x1);
  thsl->v.y = (thsl->pnt2.y = y2) - (thsl->pnt1.y = y1);
  thsl->mod = sqrt(thsl->mod2 = MOD2(thsl->v.x, thsl->v.y));

  /* Calcolo il versore relativo e il versore ortogonale */
  thsl->o.x = thsl->u.y = thsl->v.y / thsl->mod;
  thsl->o.y = -(thsl->u.x = thsl->v.x / thsl->mod);

  /* Calcolo i due punti laterali relativi alla linea */
  thsl->p[0].x = thsl->pnt1.x + sp1 * thsl->o.x;
  thsl->p[0].y = thsl->pnt1.y + sp1 * thsl->o.y;

  thsl->p[1].x = thsl->pnt1.x - sp1 * thsl->o.x;
  thsl->p[1].y = thsl->pnt1.y - sp1 * thsl->o.y;

  /* Calcolo i vettori dei relativi lati e i loro moduli */
  thsl->vb[0].x = thsl->v.x + (sp2 - sp1) * thsl->o.x;
  thsl->vb[0].y = thsl->v.y + (sp2 - sp1) * thsl->o.y;

  thsl->vb[1].x = thsl->v.x - (sp2 - sp1) * thsl->o.x;
  thsl->vb[1].y = thsl->v.y - (sp2 - sp1) * thsl->o.y;

  /* Determino quanta parte della linea devo "tagliare" */
  sl = startlenght/thsl->mod;

  /* Calcolo i due punti di partenza */
  thsl->vertex[0].x = thsl->p[0].x + sl * thsl->vb[0].x;
  thsl->vertex[0].y = thsl->p[0].y + sl * thsl->vb[0].y;
  thsl->vertex[1].x = thsl->p[1].x + sl * thsl->vb[1].x;
  thsl->vertex[1].y = thsl->p[1].y + sl * thsl->vb[1].y;

  thsl->sp1 = sp1;
  thsl->sp2 = sp2;
  line_is_closed = is_closed;
  line_segment = 0;

  return;
}

/* DESCRIZIONE: Conclude la tracciatura della spezzata disegnando
 *  la sua ultima linea.
 *  La linea puo' essere accorciata nella sua ultima parte.
 *  A tal proposito lastlenght specifica la lunghezza della parte
 *  che si vuole sia "tagliata". Specificando lastlenght = 0
 *  l'ultima linea verra' tracciata nella sua intera lunghezza.
 */
void grp_last_line(double lastlenght, int is_closed)
{
  if ( is_closed ) {
    /* La linea e' chiusa */
    thsl->vertex[2] = firstline.vertex[2];
    thsl->vertex[3] = firstline.vertex[3];

    /* Traccia l'ultima linea della spezzata */
    grp_rline( thsl->vertex[0], thsl->vertex[1] );
    grp_rline( thsl->vertex[1], thsl->vertex[2] );
    grp_rline( thsl->vertex[2], thsl->vertex[3] );
    grp_rline( thsl->vertex[3], thsl->vertex[0] );

  } else {
    /* La linea e' aperta */
    double ll = 1 - lastlenght/thsl->mod;

    thsl->vertex[3].x = thsl->p[0].x + ll * thsl->vb[0].x;
    thsl->vertex[3].y = thsl->p[0].y + ll * thsl->vb[0].y;
    thsl->vertex[2].x = thsl->p[1].x + ll * thsl->vb[1].x;
    thsl->vertex[2].y = thsl->p[1].y + ll * thsl->vb[1].y;

    /* Traccia l'ultima linea della spezzata */
    grp_rline( thsl->vertex[0], thsl->vertex[1] );
    grp_rline( thsl->vertex[1], thsl->vertex[2] );
    grp_rline( thsl->vertex[2], thsl->vertex[3] );
    grp_rline( thsl->vertex[3], thsl->vertex[0] );
  }

  return;
}

/*
 * DESCRIZIONE: Continua la tracciatura della spezzata, specificando un altro
 *  suo vertice. sp1 e sp2 sono gli spessori della linea in corrispondenza
 *  ai due punti (xl, yl) e (x, y), dove (xl, yl) e' l'ultimo punto immesso.
 *  style specifica il modo in cui le linee della spezzata devono essere
 *  congiunte.............
 */
void grp_next_line(double x, double y, double sp1, double sp2, int style)
{
  double thscongpos[2], nxtcongpos[2];

  /* Calcolo il vettore relativo alla seconda linea */
  nxtl->v.x = (nxtl->pnt2.x = x) - (nxtl->pnt1.x = thsl->pnt2.x);
  nxtl->v.y = (nxtl->pnt2.y = y) - (nxtl->pnt1.y = thsl->pnt2.y);
  nxtl->mod = sqrt(nxtl->mod2 = MOD2(nxtl->v.x, nxtl->v.y));

  /* Calcolo il versore relativo e il versore ortogonale */
  nxtl->o.x = nxtl->u.y = nxtl->v.y / nxtl->mod;
  nxtl->o.y = -(nxtl->u.x = nxtl->v.x / nxtl->mod);

  /* Calcolo i due punti laterali relativi alla seconda linea */
  nxtl->p[0].x = nxtl->pnt1.x + sp1 * nxtl->o.x;
  nxtl->p[0].y = nxtl->pnt1.y + sp1 * nxtl->o.y;

  nxtl->p[1].x = nxtl->pnt1.x - sp1 * nxtl->o.x;
  nxtl->p[1].y = nxtl->pnt1.y - sp1 * nxtl->o.y;

  /* Calcolo i vettori dei relativi lati e i loro moduli */
  nxtl->vb[0].x = nxtl->v.x + (sp2 - sp1) * nxtl->o.x;
  nxtl->vb[0].y = nxtl->v.y + (sp2 - sp1) * nxtl->o.y;

  nxtl->vb[1].x = nxtl->v.x - (sp2 - sp1) * nxtl->o.x;
  nxtl->vb[1].y = nxtl->v.y - (sp2 - sp1) * nxtl->o.y;

  nxtl->sp1 = sp1;
  nxtl->sp2 = sp2;

  /* Calcolo i punti di giunzione */
  /* Primo punto di giunzione */
  if ( ! grp_intersection2(
   & thsl->p[0], & thsl->vb[0],  /* Prima retta */
   & nxtl->p[0], & nxtl->vb[0],  /* Seconda retta */
   & thscongpos[0], & nxtcongpos[0]) ) {printf("1 C'e' qualche problema!!!\n");}

  /* Secondo punto di giunzione */
  if ( ! grp_intersection2(
   & thsl->p[1], & thsl->vb[1],  /* Prima retta */
   & nxtl->p[1], & nxtl->vb[1],  /* Seconda retta */
   & thscongpos[1], & nxtcongpos[1]) ) {printf("2 C'e' qualche problema!!!\n");}

  /* Devo controllare che thscongpos>0 e nxtcongpos<1*/
  if ((thscongpos[0] <= 0) || (nxtcongpos[0] >= 1)) printf("3 C'e' qualche problema!!!\n");
  if ((thscongpos[1] <= 0) || (nxtcongpos[1] >= 1)) printf("4 C'e' qualche problema!!!\n");

  /* Completo il calcolo dei punti di giunzione */
  nxtl->cong[0].x = nxtl->p[0].x + nxtcongpos[0] * nxtl->vb[0].x;
  nxtl->cong[0].y = nxtl->p[0].y + nxtcongpos[0] * nxtl->vb[0].y;

  nxtl->cong[1].x = nxtl->p[1].x + nxtcongpos[1] * nxtl->vb[1].x;
  nxtl->cong[1].y = nxtl->p[1].y + nxtcongpos[1] * nxtl->vb[1].y;

  if ( style == 0 ) {
    /* In questo caso le linee si congiungono senza arrotondamenti */
    thsl->vertex[2].x = nxtl->vertex[1].x = nxtl->cong[1].x;
    thsl->vertex[2].y = nxtl->vertex[1].y = nxtl->cong[1].y;
    thsl->vertex[3].x = nxtl->vertex[0].x = nxtl->cong[0].x;
    thsl->vertex[3].y = nxtl->vertex[0].y = nxtl->cong[0].y;

    /* Ora non resta che tracciare la linea */
  if ( (++line_segment != 1) || (! line_is_closed)  ) {
    grp_rline( thsl->vertex[0], thsl->vertex[1] );
    grp_rline( thsl->vertex[1], thsl->vertex[2] );
    grp_rline( thsl->vertex[2], thsl->vertex[3] );
    grp_rline( thsl->vertex[3], thsl->vertex[0] );
  } else {
    firstline = *thsl;
  }

  } else {
    /* In questo caso le linee vengono collegate smussando gli spigoli */
    int inn, ext;
    double thsproj[2], nxtproj[2];
    double thsposzero, nxtposzero;
    double thsipos, thsepos, nxtipos, nxtepos;

    /* Determino quale dei due lati della linea e' interno e quale e' esterno */
    thsproj[0] = (nxtl->cong[0].x - thsl->pnt1.x)*thsl->v.x +
     (nxtl->cong[0].y - thsl->pnt1.y)*thsl->v.y;

    thsproj[1] = (nxtl->cong[1].x - thsl->pnt1.x)*thsl->v.x +
     (nxtl->cong[1].y - thsl->pnt1.y)*thsl->v.y;

    nxtproj[0] = (nxtl->cong[0].x - nxtl->pnt1.x)*nxtl->v.x +
     (nxtl->cong[0].y - nxtl->pnt1.y)*nxtl->v.y;

    nxtproj[1] = (nxtl->cong[1].x - nxtl->pnt1.x)*nxtl->v.x +
     (nxtl->cong[1].y - nxtl->pnt1.y)*nxtl->v.y;

    if (thsproj[0] < thsproj[1]) {
      inn = 0;
      ext = 1;
      thsposzero = thsproj[0] / thsl->mod2;
    } else {
      inn = 1;
      ext = 0;
      thsposzero = thsproj[1] / thsl->mod2;
    }

    if (nxtproj[0] > nxtproj[1])
      nxtposzero = nxtproj[0] / nxtl->mod2;
    else
      nxtposzero = nxtproj[1] / nxtl->mod2;

    /* Trovo punto di terminazione del bordo interno della prima linea */
    if (curjs->ti < 0.0) {
      if (curjs->ti < -1.0)
        thsipos = thscongpos[inn];
      else
        thsipos = thsposzero - curjs->ti * (thscongpos[inn] - thsposzero);

    } else {
      thsipos = thsposzero - curjs->ti * thsl->sp2 / thsl->mod;
      if (thsipos < 0.0) thsipos = 0.0;
    }

    /* Trovo punto di terminazione del bordo esterno della prima linea */
    if (curjs->te < 0.0) {
      if (curjs->te < -1.0)
        thsepos = thscongpos[ext];
      else
        thsepos = thsposzero - curjs->te * (thscongpos[ext] - thsposzero);

    } else {
      thsepos = thsposzero - curjs->te * thsl->sp2 / thsl->mod;
      if (thsepos < 0.0) thsepos = 0.0;
    }

      /* Trovo punto di terminazione del bordo interno della seconda linea */
    if (curjs->ni < 0.0) {
      if (curjs->ni < -1.0)
        nxtipos = nxtcongpos[inn];
      else
        nxtipos = nxtposzero - curjs->ni * (nxtcongpos[inn] - nxtposzero);

    } else {
      nxtipos = nxtposzero + curjs->ni * nxtl->sp1 / nxtl->mod;
      if (nxtipos > 1.0) nxtipos = 1.0;
    }

      /* Trovo punto di terminazione del bordo esterno della seconda linea */
    if (curjs->ne < 0.0) {
      if (curjs->ne < -1.0)
        nxtepos = nxtcongpos[ext];
      else
        nxtepos = nxtposzero - curjs->ne * (nxtcongpos[ext] - nxtposzero);

    } else {
      nxtepos = nxtposzero + curjs->ne * nxtl->sp1 / nxtl->mod;
      if (nxtepos > 1.0) nxtepos = 1.0;
    }

    /* Completo il calcolo dei punti */
    thsl->vertex[3-inn].x = thsl->p[inn].x + thsipos * thsl->vb[inn].x;
    thsl->vertex[3-inn].y = thsl->p[inn].y + thsipos * thsl->vb[inn].y;

    thsl->vertex[2+inn].x = thsl->p[ext].x + thsepos * thsl->vb[ext].x;
    thsl->vertex[2+inn].y = thsl->p[ext].y + thsepos * thsl->vb[ext].y;

    nxtl->vertex[inn].x = nxtl->p[inn].x + nxtipos * nxtl->vb[inn].x;
    nxtl->vertex[inn].y = nxtl->p[inn].y + nxtipos * nxtl->vb[inn].y;

    nxtl->vertex[ext].x = nxtl->p[ext].x + nxtepos * nxtl->vb[ext].x;
    nxtl->vertex[ext].y = nxtl->p[ext].y + nxtepos * nxtl->vb[ext].y;

    /* Ora traccio la linea */
    if ( (++line_segment != 1) || (! line_is_closed)  ) {
      grp_rline( thsl->vertex[0], thsl->vertex[1] );
      grp_rline( thsl->vertex[1], thsl->vertex[2] );
      grp_rline( thsl->vertex[2], thsl->vertex[3] );
      grp_rline( thsl->vertex[3], thsl->vertex[0] );
    } else {
      firstline = *thsl;
    }

    /* Ora controllo l'angolo per decidere il tipo di congiuntura
     * da tracciare
     */
    if ( (cutting > 0.0) &&
     (thsl->u.x * nxtl->u.x + thsl->u.y * nxtl->u.y < 0.0) ) {

      /* Angolo acuto: curva a gomito: uso una congiuntura smorzata */
      double cgwidth, cgrefwidth, alpha;
      Point cgvertex[5];

      /* La congiuntura ha 4 lati, 2 sono linee, 2 sono curve.
       * Il lato interno �una curva semplice, mentre quello esterno
       * �una curva smorzata (costituita da due curve successive)
       * Ora traccio il lato esterno, ma devo fare un sacco di conti!
       */

      /* Trovo la larghezza di riferimento per la congiuntura */
       {
       register double s1, s2, x1, y1, x2, y2, r;

       s1 = thsl->sp2;
       s2 = nxtl->sp1;
       x1 = thsl->mod;
       y1 = s1 - thsl->sp1;
       x2 = s2 - nxtl->sp2;
       y2 = nxtl->mod;

       r = (x2 * s1  + y2 * s2) * x1;
       s2 = (x1 * s1  + y1 * s2) * y2;
       s1 = x1*y2 - x2*y1;
       cgrefwidth = sqrt(r*r + s2*s2) / s1;
       }

      /* Trovo la "larghezza" della congiuntura */
       {
       register double r1, r2;

       cgvertex[2].x = r1 = nxtl->cong[ext].x;
       cgvertex[2].y = r2 = nxtl->cong[ext].y;
       cgvertex[3].x = r1 = nxtl->pnt1.x - r1;
       cgvertex[3].y = r2 = nxtl->pnt1.y - r2;
       cgwidth = sqrt(r1*r1 + r2*r2);
       }

      /* Trovo il punto centrale della curva ( = lato esterno) */
      alpha = 1.0 - cutting * 0.707106781 * cgrefwidth / cgwidth;

      if ( (alpha > 0) && (alpha < 1) ) {
        /* In questo caso ho cutting! */
        double beta1, beta2;
        Point cutdir;

        /* Continuo il calcolo che stavo facendo. */
        cgvertex[2].x += alpha * cgvertex[3].x;

        /* Questi sono i punti iniziale e finale */
        cgvertex[0].x = thsl->vertex[2+inn].x;
        cgvertex[0].y = thsl->vertex[2+inn].y;
        cgvertex[4].x = nxtl->vertex[ext].x;
        cgvertex[4].y = nxtl->vertex[ext].y;

        /* Ora devo trovare i punti 1 e 3 ! */

        /* Trovo la direzione della tangente alla curva nel punto
         * centrale del lato curvo (cioe' cgvertex[2])
         */
        cutdir.x = cgvertex[0].x - cgvertex[4].x;
        cutdir.y = cgvertex[0].y - cgvertex[4].y;

        /* Trovo il punto 1: �intersezione di 2 rette! */
        if ( ! grp_intersection(
         /* Prima retta: bordo esterno della linea 1 */
         & cgvertex[0], & thsl->vb[ext],
         /* Seconda retta: passante per 2 con direzione cutdir */
         & cgvertex[2], & cutdir, & beta1) ) {goto nxln_err1;}

        /* Trovo il punto 2: �intersezione di 2 rette! */
        if ( ! grp_intersection(
         /* Prima retta: bordo esterno della linea 2 */
         & cgvertex[4], & nxtl->vb[ext],
         /* Seconda retta: passante per 2 con direzione cutdir */
         & cgvertex[2], & cutdir, & beta2) ) {goto nxln_err1;}

        if ( (beta1 < 0.0) || (beta2 > 0.0) ) goto nxln_err1;

        cgvertex[1].x = cgvertex[0].x + beta1 * thsl->vb[ext].x;
        cgvertex[1].y = cgvertex[0].y + beta1 * thsl->vb[ext].y;
        cgvertex[3].x = cgvertex[4].x + beta2 * nxtl->vb[ext].x;
        cgvertex[3].y = cgvertex[4].y + beta2 * nxtl->vb[ext].y;

        /* Finalmente posso tracciare il lato esterno */
        grp_rcong( cgvertex[0], cgvertex[1], cgvertex[2] );
        grp_rcong( cgvertex[2], cgvertex[3], cgvertex[4] );

        /* Ora traccio il lato interno */
        cgvertex[0].x = thsl->vertex[2+ext].x;
        cgvertex[0].y = thsl->vertex[2+ext].y;
        cgvertex[1].x = nxtl->cong[inn].x;
        cgvertex[1].y = nxtl->cong[inn].y;
        cgvertex[2].x = nxtl->vertex[inn].x;
        cgvertex[2].y = nxtl->vertex[inn].y;
        grp_rcong( cgvertex[0], cgvertex[1], cgvertex[2] );

        /* E infine traccio i lati di tipo linea */
        grp_rline( thsl->vertex[2], thsl->vertex[3] );
        grp_rline( nxtl->vertex[0], nxtl->vertex[1] );

        goto nxln_exit;
      }
    }

     /* Angolo ottuso: curva dolce: uso una congiuntura semplice! */
     {
     Point cgvertex[3];

     /* La congiuntura ha 4 lati, 2 sono linee, 2 sono curve.
      * Ora traccio i lati uno in seguito all'altro:
      * curva, linea, curva, linea!
      */

     /* Linea */
     grp_rline( thsl->vertex[2], thsl->vertex[3] );

     /* Curva */
     cgvertex[0].x = thsl->vertex[3].x;
     cgvertex[0].y = thsl->vertex[3].y;
     cgvertex[1].x = nxtl->cong[0].x;
     cgvertex[1].y = nxtl->cong[0].y;
     cgvertex[2].x = nxtl->vertex[0].x;
     cgvertex[2].y = nxtl->vertex[0].y;
     grp_rcong( cgvertex[0], cgvertex[1], cgvertex[2] );

     /* Linea */
     grp_rline( nxtl->vertex[0], nxtl->vertex[1] );

     /* Curva */
     cgvertex[0].x = nxtl->vertex[1].x;
     cgvertex[0].y = nxtl->vertex[1].y;
     cgvertex[1].x = nxtl->cong[1].x;
     cgvertex[1].y = nxtl->cong[1].y;
     cgvertex[2].x = thsl->vertex[2].x;
     cgvertex[2].y = thsl->vertex[2].y;
     grp_rcong( cgvertex[0], cgvertex[1], cgvertex[2] );

     }
  }

nxln_exit:
  /* Preparo l'elaborazione della prossima linea */
   {
   register line_desc *tmpl;

   tmpl = thsl;
   thsl = nxtl;
   nxtl = tmpl;
   }

  return;

nxln_err1:
  printf("Impossibile tracciare lo spigolo! Disattivare cutting!\n");
  goto nxln_exit;
}

/* DESCRIZIONE: Questa funzione calcola l'intersezione fra le seguenti linee:
 *  1) la linea passante per il punto p1 e diretta lungo il vettore d1;
 *  2) la linea passante per p2 e diretta lungo d2.
 *  Se il calcolo riesce restituisce 1, 0 altrimenti (se le linee sono
 *  parallele oppure coincidenti).
 *  La procedura restituisce alpha1, che mi permette di ottenere il punto
 *  d'intersezione nel modo seguente: intersez = p1 + alpha1 * d1
 */
int grp_intersection(Point *p1, Point *d1, Point *p2, Point *d2,
 double *alpha1)
{
  Point p2mp1;
  double d1vectd2;

  p2mp1.x = p2->x - p1->x;
  p2mp1.y = p2->y - p1->y;

  /* Primo punto di giunzione */
  d1vectd2 = d1->x * d2->y - d1->y * d2->x;

  if (d1vectd2 == 0.0) return 0;

  *alpha1 = (p2mp1.x * d2->y - p2mp1.y * d2->x) / d1vectd2;

  return 1;
}

/* DESCRIZIONE: Questa funzione calcola l'intersezione fra le seguenti linee:
 *  1) la linea passante per il punto p1 e diretta lungo il vettore d1;
 *  2) la linea passante per p2 e diretta lungo d2.
 *  Se il calcolo riesce restituisce 1, 0 altrimenti (se le linee sono
 *  parallele oppure coincidenti).
 *  La procedura restituisce alpha1, che mi permette di ottenere il punto
 *  d'intersezione nel modo seguente: intersez = p1 + alpha1 * d1
 *  e alpha2 che permette di ottenere lo stesso punto come:
 *  intersez = p2 + alpha2 * d2
 */
int grp_intersection2(Point *p1, Point *d1, Point *p2, Point *d2,
 double *alpha1, double *alpha2)
{
  Point p2mp1;
  double d1vectd2;

  p2mp1.x = p2->x - p1->x;
  p2mp1.y = p2->y - p1->y;

  /* Primo punto di giunzione */
  d1vectd2 = d1->x * d2->y - d1->y * d2->x;

  if (d1vectd2 == 0.0) return 0;

  *alpha1 = (p2mp1.x * d2->y - p2mp1.y * d2->x) / d1vectd2;
  *alpha2 = (p2mp1.x * d1->y - p2mp1.y * d1->x) / d1vectd2;

  return 1;
}
