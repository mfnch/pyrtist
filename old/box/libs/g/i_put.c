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

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "types.h"
#include <box/vm_priv.h>
#include "str.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"
#include "autoput.h"
#include "fig.h"
#include "pointlist.h"
#include "i_pointlist.h"

BoxTask put_window_init(Window *w) {
  if ( ! buff_create(& w->put.fig_points, sizeof(BoxPoint), 8) ) {
    g_error("put_window_init: buff_create failed (fig_points)!");
    return BOXTASK_FAILURE;
  }
  if ( ! buff_create(& w->put.back_points, sizeof(BoxPoint), 8) ) {
    g_error("put_window_init: buff_create failed (back_points)!");
    return BOXTASK_FAILURE;
  }
  if ( ! buff_create(& w->put.weights, sizeof(BoxReal), 8) ) {
    g_error("put_window_init: buff_create failed (weights)!");
    return BOXTASK_FAILURE;
  }
  return BOXTASK_OK;
}

void put_window_destroy(Window *w) {
  buff_free(& w->put.fig_points);
  buff_free(& w->put.back_points);
  buff_free(& w->put.weights);
}

BoxTask window_put_begin(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);

  w->put.rot_angle = 0.0;
  w->put.rot_center.x = w->put.rot_center.y = 0.0;
  w->put.translation.x = w->put.translation.y = 0.0;
  w->put.scale.x = w->put.scale.y = 1.0;
  w->put.auto_transforms = 0;
  w->put.got.constraints = 0;
  w->put.got.compute = 0;
  w->put.got.figure = 0;
  w->put.got.translation = 0;
  w->put.got.rot_angle = 0;
  w->put.got.scale = 0;
  w->put.got.matrix = 0;

  if ( ! (   buff_clear(& w->put.fig_points)
          && buff_clear(& w->put.back_points)
          && buff_clear(& w->put.weights)) ) {
    g_error("window_put_begin: buff_clear failed!");
    return BOXTASK_FAILURE;
  }

  return BOXTASK_OK;
}

static BoxTask put_calculate(Window *w) {
  long F_num, B_num, weight_num;
  BoxPoint *F_ptr, *B_ptr;
  BoxReal *weight_ptr;

  /* Accedo alle liste */
  F_ptr = buff_firstitemptr(& w->put.fig_points, BoxPoint);
  B_ptr = buff_firstitemptr(& w->put.back_points, BoxPoint);
  weight_ptr = buff_firstitemptr(& w->put.weights, BoxReal);

  F_num = buff_numitem(& w->put.fig_points);
  B_num = buff_numitem(& w->put.back_points);
  weight_num = buff_numitem(& w->put.weights);
  assert(B_num == F_num && weight_num == F_num);

  /* Setto i parametri prima dei calcoli */
  aput_set(& w->put.rot_center, & w->put.translation,
           & w->put.rot_angle, & w->put.scale.x, & w->put.scale.y);

  /* Calcolo i parametri della trasformazione in base ai vincoli introdotti */
  if (!aput_autoput(F_ptr, B_ptr, weight_ptr, F_num, w->put.auto_transforms))
    return BOXTASK_FAILURE;

  /* Preleva i risultati dei calcoli */
  aput_get(& w->put.rot_center, & w->put.translation,
           & w->put.rot_angle, & w->put.scale.x, & w->put.scale.y);

  w->put.got.constraints = 1;

#ifdef DEBUG
  printf("Vettore di traslazione: T = (%g, %g)\n", put_trsl_vect.x, put_trsl_vect.y);
  printf("Centro di rotazione: Q = (%g, %g)\n", put_rot_center.x, put_rot_center.y);
  printf("Angolo di rotazione: theta = %g\n", put_rot_angle);
  printf("Scala x: %g\n", put_scale_x);
  printf("Scala y: %g\n", put_scale_y);
#endif
  return BOXTASK_OK;
}

static BoxTask _transform_pl(BoxInt index, char *name, void *object, void *data) {
  Matrix *m = (Matrix *) data;
  BoxGMatrix_Map_Point(m, (BoxPoint *) object, (BoxPoint *) object);
  return BOXTASK_OK;
}

BoxTask window_put_end(BoxVMX *vmp) {
  PROC_OF_WINDOW_SUBTYPE(vmp, w, out_pl, IPointList *);
  Window *figure;
  IPointList *returned_pl;

  /* Se sono gia' stati inseriti dei vincoli (con !near[...]), ma essi non
   * sono ancora stati utilizzati (cioe' .Compute non e' stata invocata), allora
   * li uso adesso e mi calcolo la trasformazione che meglio li soddisfa!
   */
  if ( w->put.got.constraints ) { BOXTASK( put_calculate(w) ); }

  if ( !w->put.got.figure ) {
    g_warning("You did not provide any figure to Put[].");
    return BOXTASK_OK;
  }

  if (!w->put.got.matrix) {
    /* the matrix has not been given by the user, we calculate it! */
    BoxGMatrix_Set(& w->put.matrix,
                   & w->put.translation, & w->put.rot_center,
                   w->put.rot_angle, w->put.scale.x, w->put.scale.y);
  }

  /* Disegno l'oggetto */
  figure = (Window *) w->put.figure;
  BoxGWin_Fig_Draw_Fig_With_Matrix(w->window, figure->window,
                                   & w->put.matrix);
  returned_pl = (IPointList *) malloc(sizeof(IPointList));
  if (returned_pl == (IPointList *) NULL) {
    g_error("window_put_end: malloc failed!");
    return BOXTASK_FAILURE;
  }
  returned_pl->name = (char *) NULL;
  pointlist_dup(& returned_pl->pl, & figure->pointlist);
  (void) pointlist_iter(& returned_pl->pl, _transform_pl, & w->put.matrix);
  *out_pl = returned_pl;
  return BOXTASK_OK;
}

BoxTask window_put_window(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  WindowPtr *wp = BOX_VM_ARG1_PTR(vmp, WindowPtr);
  w->put.figure = (void *) *wp;
  w->put.got.figure = 1;
  return BOXTASK_OK;
}

BoxTask window_put_point(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  BoxPoint *translation = BOX_VM_ARG1_PTR(vmp, BoxPoint);
  w->put.translation = *translation;
  if (w->put.got.translation)
    g_warning("ignoring previously specified translation vector!");
  w->put.got.translation = 1;
  return BOXTASK_OK;
}

BoxTask window_put_real(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  BoxReal *rot_angle = BOX_VM_ARG1_PTR(vmp, BoxReal);
  w->put.rot_angle = *rot_angle;
  if (w->put.got.rot_angle)
    g_warning("ignoring previously specified rotation angle!");
  w->put.got.rot_angle = 1;
  return BOXTASK_OK;
}

BoxTask window_put_string(BoxVMX *vm) {
  SUBTYPE_OF_WINDOW(vm, w);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *auto_transformations_str = (char *) s->ptr;

  if (!aput_allow(auto_transformations_str, & w->put.auto_transforms)) {
    g_warning("aput_allow failed!");
    return BOXTASK_OK;
  }

  return BOXTASK_OK;
}

BoxTask window_put_matrix(BoxVMX *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  Matrix *m = BOX_VM_ARG1_PTR(vmp, Matrix);
  w->put.matrix = *m;
  w->put.got.matrix = 1;
  return BOXTASK_OK;
}

BoxTask window_put_scale_real(BoxVMX *vmp) {
  BoxSubtype *scale_of_window_put = BOX_VM_THIS_PTR(vmp, BoxSubtype);
  BoxSubtype *put_of_window =
    SUBTYPE_PARENT_PTR(scale_of_window_put, BoxSubtype);
  Window *w = *((Window **) SUBTYPE_PARENT_PTR(put_of_window, WindowPtr));
  BoxReal *scale = BOX_VM_ARG1_PTR(vmp, BoxReal);
  w->put.scale.y = w->put.scale.x = *scale;
  if (w->put.got.scale)
    g_warning("ignoring previously specified scale factors!");
  w->put.got.scale = 1;
  return BOXTASK_OK;
}

BoxTask window_put_scale_point(BoxVMX *vmp) {
  BoxSubtype *scale_of_window_put = BOX_VM_THIS_PTR(vmp, BoxSubtype);
  BoxSubtype *put_of_window =
    SUBTYPE_PARENT_PTR(scale_of_window_put, BoxSubtype);
  Window *w =
    *((Window **) SUBTYPE_PARENT_PTR(put_of_window, WindowPtr));
  BoxPoint *p = BOX_VM_ARG1_PTR(vmp, BoxPoint);
  w->put.scale.x = p->x;
  w->put.scale.y = p->y;
  if (w->put.got.scale)
    g_warning("ignoring previously specified scale factors!");
  w->put.got.scale = 1;
  return BOXTASK_OK;
}

BoxTask window_put_near_begin(BoxVMX *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->put.near.have.on_src = 0;
  w->put.near.have.on_dest = 0;
  w->put.near.have.weight = 0;
  w->put.near.weight = 1.0;
  return BOXTASK_OK;
}

BoxTask window_put_near_end(BoxVMX *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  if (! w->put.near.have.on_src || ! w->put.near.have.on_src) {
    g_warning("Ignoring .Near[] specification: source or destination point "
              "is missing");
    return BOXTASK_OK;
  }

  if (   ! buff_push(& w->put.fig_points, & w->put.near.on_src)
      || ! buff_push(& w->put.back_points, & w->put.near.on_dest)
      || ! buff_push(& w->put.weights, & w->put.near.weight)) {
    g_error("window_put_near_end: buff_push() failed!");
    return BOXTASK_FAILURE;
  }

  w->put.got.constraints = 1;
  return BOXTASK_OK;
}

static BoxTask _window_put_near_real(Window *w, BoxReal weight){
  if (! w->put.near.have.weight) {
    g_warning("Window.Put.Near got already the weight for this constrain.");
    return BOXTASK_OK;
  }
  if (weight < 0.0) {
    g_warning("The weight has to be a positive Real number!");
    return BOXTASK_OK;
  }
  w->put.near.weight = weight;
  w->put.near.have.weight = 1;
  return BOXTASK_OK;
}

BoxTask window_put_near_real(BoxVMX *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  BoxReal weight = BOX_VM_ARG1(vmp, BoxReal);
  return _window_put_near_real(w, weight);
}

BoxTask window_put_near_int(BoxVMX *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  BoxInt int_num = BOX_VM_ARG1(vmp, BoxInt);
  if (w->put.near.have.on_src)
    return _window_put_near_real(w, (BoxReal) int_num);

  else {
    Window *figure;
    BoxPoint *on_src;
    if (!w->put.got.figure) {
      g_error("Figure has not been specified. Cannot refer to its hot points "
              "from Window.Put.Near!");
      return BOXTASK_FAILURE;
    }
    figure = (Window *) w->put.figure;

    on_src = pointlist_get(& figure->pointlist, int_num);
    if (on_src == (BoxPoint *) NULL) {
      g_error("The point index you gave to Window.Put.Near "
              "goes out of bounds.");
      return BOXTASK_FAILURE;
    }

    w->put.near.on_src = *on_src;
    w->put.near.have.on_src = 1;
    return BOXTASK_OK;
  }
}

BoxTask window_put_near_point(BoxVMX *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  BoxPoint *p = BOX_VM_ARG1_PTR(vmp, BoxPoint);
  if (!w->put.near.have.on_src) {
    w->put.near.on_src = *p;
    w->put.near.have.on_src = 1;
    return BOXTASK_OK;

  } else if (!w->put.near.have.on_dest) {
    w->put.near.on_dest = *p;
    w->put.near.have.on_dest = 1;
    return BOXTASK_OK;

  } else {
    g_warning("Window.Put.Near already got the source and destination point: "
              "this point will be ignored!");
    return BOXTASK_OK;
  }
}

BoxTask window_put_near_string(BoxVMX *vm) {
  SUBTYPE2_OF_WINDOW(vm, w);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *name = (char *) s->ptr;
  if (!w->put.near.have.on_src) {
    Window *figure;
    BoxPoint *on_src;
    if (!w->put.got.figure) {
      g_error("Figure has not been specified. Cannot refer to its hot points "
              "from Window.Put.Near!");
      return BOXTASK_FAILURE;
    }
    figure = (Window *) w->put.figure;
    on_src = pointlist_find(& figure->pointlist, name);
    if (on_src == NULL) {
      g_error("The name you provided to Window.Put.Near does not correspond "
              "to any of the hot points of the figure.");
      return BOXTASK_FAILURE;
    }

    w->put.near.on_src = *on_src;
    w->put.near.have.on_src = 1;
    return BOXTASK_OK;

  } else {
    g_warning("Window.Put.Near already got the source point. "
              "String will be ignored!");
    return BOXTASK_OK;
  }
}

#if 0
/* Sintassi possibili per !near:
 * 1) !near[ punto1, punto2 {, weight} ]
 * 2) !near[ stringa, punto2 {, weight} ]
 * 3) !near[ numero, punto2 {, weight} ]
 * 4) !near[ stringa, numero, punto2 {, weight} ]
 */
static int put_near(void)
{
  int arg = 1;
  TYPE_FLT weight;
  TYPE_PNT F, B;

  PRNMSG("put_near: Inserimento di un vincolo...");

  /* Segnalo che i vincoli non sono ancora stati usati per i calcoli! */
  put_status &= CONSTRAINTS_NOT_USED;

  if ( vrmc_numargs() < 2 ) {
    vrmc_liberr("Argomenti errati");
    EXIT_ERR("Numero di argomenti insufficiente!\n");
  }

  switch ( vrmc_argtypen(arg) ) {
   case 'p':
    /* weight non e' specificato */
    weight = 1.0; /* Valore di default */
    break;

   case 'n':
    /* weight e' specificato */
    weight = vrmc_getfa(arg++);
    if ( weight <= 0.0 ) {
      vrmc_liberr("Il peso deve essere positivo");
      EXIT_ERR("weight <= 0!\n");
    }
    if ( vrmc_argtypen(arg) == 'p' ) break;

   default:
    vrmc_liberr("Argomenti errati");
    EXIT_ERR("L'ultimo o penultimo argomento non e' un punto!\n");
  }

  /* Prelevo il punto di riferimento B */
  B = *vrmc_getpa(arg);

  /* Manca solo il punto della figura:
   * elimino gli argomenti gia' considerati!
   */
  vrmc_rmargs(arg);

  /* Tutto gli argomenti che restano servono a specificare
   * un punto della figura. Ho 2 casi...
   */
  if ( vrmc_chkargs("p") ) {
    /* 1) Resta proprio solo un punto */
    F = *vrmc_getpa(1);

  } else {
    /* 2) La lista di argomenti che rimangono non e' costituita
     *  di un solo punto: tratto questi argomenti come se fossero
     *  stati inseriti usando punti_figura->argomenti, dove
     *  punti_figura e' l'oggetto plist contenente i punti della
     *  figura da disporre sul "foglio".
     *  Questo mi consente di usare sintassi come:
     *   >< Put triangolo, !near["vertice_A", (1, 2)]
     *  che e' equivalente a:
     *   >< Put triangolo, !near[triangolo->"vertice_A", (1, 2)]
     */
    if ( put_figure->child == NULL ) {
      /* La figura da disporre non possiede hot-points: errore! */
      vrmc_liberr("Argomenti errati");
      EXIT_ERR("La figura non possiede una lista di hot-points!\n");
    }

    if ( ! plist_position2( LIST_GET_FROM_STATUS(put_figure->child),
     & F ) ) {
      EXIT_ERR("plist_position2() fallita!\n");
    }
  }

  /* Aggiungo i due punti e il peso weight alle rispettive liste */
  {
    list *cur_list = list_status;

    list_status = B_list;
    if ( ! list_item( & B ) ) {EXIT_ERR("fallito(1)!\n");};
    if ( ! list_next() ) {EXIT_ERR("fallito(2)!\n");};
    B_list = list_status;

    list_status = F_list;
    if ( ! list_item( & F ) ) {EXIT_ERR("fallito(3)!\n");};
    if ( ! list_next() ) {EXIT_ERR("fallito(4)!\n");};
    F_list = list_status;

    list_status = weight_list;
    if ( ! list_item( & weight ) ) {EXIT_ERR("fallito(5)!\n");};
    if ( ! list_next() ) {EXIT_ERR("fallito(6)!\n");};
    weight_list = list_status;

    list_status = cur_list;
  }

  EXIT_OK("Ok!\n");
}
#endif
