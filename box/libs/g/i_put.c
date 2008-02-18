#include <math.h>
#include <assert.h>

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"
#include "autoput.h"
#include "fig.h"

Task put_window_init(Window *w) {
  if ( ! buff_create(& w->put.fig_points, sizeof(Point), 8) ) {
    g_error("put_window_init: buff_create failed (fig_points)!");
    return Failed;
  }
  if ( ! buff_create(& w->put.back_points, sizeof(Point), 8) ) {
    g_error("put_window_init: buff_create failed (back_points)!");
    return Failed;
  }
  if ( ! buff_create(& w->put.weights, sizeof(Real), 8) ) {
    g_error("put_window_init: buff_create failed (weights)!");
    return Failed;
  }
  return Success;
}

void put_window_destroy(Window *w) {
  buff_free(& w->put.fig_points);
  buff_free(& w->put.back_points);
  buff_free(& w->put.weights);
}

Task window_put_begin(VMProgram *vmp) {
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

  if ( ! (   buff_clear(& w->put.fig_points)
          && buff_clear(& w->put.back_points)
          && buff_clear(& w->put.weights)) ) {
    g_error("window_put_begin: buff_clear failed!");
    return Failed;
  }

  return Success;
}

static Task put_calculate(Window *w) {
  long F_num, B_num, weight_num;
  Point *F_ptr, *B_ptr;
  Real *weight_ptr;

  /* Accedo alle liste */
  F_ptr = buff_firstitemptr(& w->put.fig_points, Point);
  B_ptr = buff_firstitemptr(& w->put.back_points, Point);
  weight_ptr = buff_firstitemptr(& w->put.weights, Real);

  F_num = buff_numitem(& w->put.fig_points);
  B_num = buff_numitem(& w->put.back_points);
  weight_num = buff_numitem(& w->put.weights);
  assert(B_num == F_num && weight_num == F_num);

  /* Setto i parametri prima dei calcoli */
  aput_set(& w->put.rot_center, & w->put.translation,
           & w->put.rot_angle, & w->put.scale.x, & w->put.scale.y);

  /* Calcolo i parametri della trasformazione in base ai vincoli introdotti */
  if (!aput_autoput(F_ptr, B_ptr, weight_ptr, F_num, w->put.auto_transforms))
    return Failed;

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
  return Success;
}


Task window_put_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  grp_window *cur_win = grp_win;
  Window *figure;

  /* Se sono gia' stati inseriti dei vincoli (con !near[...]), ma essi non
   * sono ancora stati utilizzati (cioe' .Compute non e' stata invocata), allora
   * li uso adesso e mi calcolo la trasformazione che meglio li soddisfa!
   */
  if ( w->put.got.constraints ) { TASK( put_calculate(w) ); }

  if ( !w->put.got.figure ) {
    g_warning("You did not provide any figure to Put[].");
    return Success;
  }

  /* Calcolo la matrice di trasformazione */
  aput_matrix(& w->put.translation,
              & w->put.rot_center, w->put.rot_angle,
              w->put.scale.x, w->put.scale.y,
              fig_matrix);

  /* Disegno l'oggetto */
  figure = (Window *) w->put.figure;
  grp_win = w->window;
  fig_draw_fig(figure->window);
  grp_win = cur_win;
  return Success;
}

Task window_put_window(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  WindowPtr *wp = BOX_VM_ARGPTR1(vmp, WindowPtr);
  w->put.figure = (void *) *wp;
  w->put.got.figure = 1;
  return Success;
}

Task window_put_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *translation = BOX_VM_ARGPTR1(vmp, Point);
  w->put.translation = *translation;
  if (w->put.got.translation)
    g_warning("ignoring previously specified translation vector!");
  w->put.got.translation = 1;
  return Success;
}

Task window_put_real(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real *rot_angle = BOX_VM_ARGPTR1(vmp, Real);
  w->put.rot_angle = *rot_angle;
  if (w->put.got.rot_angle)
    g_warning("ignoring previously specified rotation angle!");
  w->put.got.rot_angle = 1;
  return Success;
}

Task window_put_string(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  char *auto_transformations_str = BOX_VM_ARGPTR1(vmp, char);

  if (!aput_allow(auto_transformations_str, & w->put.auto_transforms)) {
    g_warning("aput_allow failed!");
    return Success;
  }

  return Success;
}

Task window_put_scale_real(VMProgram *vmp) {
  Subtype *scale_of_window_put = BOX_VM_CURRENTPTR(vmp, Subtype);
  Subtype *put_of_window = SUBTYPE_PARENT_PTR(scale_of_window_put, Subtype);
  Window *w = *((Window **) SUBTYPE_PARENT_PTR(put_of_window, WindowPtr));
  Real *scale = BOX_VM_ARGPTR1(vmp, Real);
  w->put.scale.y = w->put.scale.x = *scale;
  if (w->put.got.scale)
    g_warning("ignoring previously specified scale factors!");
  w->put.got.scale = 1;
  return Success;
}

Task window_put_near_begin(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->put.near.have.on_src = 0;
  w->put.near.have.on_dest = 0;
  w->put.near.have.weight = 0;
  w->put.near.weight = 1.0;
  return Success;
}

Task window_put_near_end(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  if (! w->put.near.have.on_src || ! w->put.near.have.on_src) {
    g_warning("Ignoring .Near[] specification: source or destination point "
              "is missing");
    return Success;
  }

  if (   ! buff_push(& w->put.fig_points, & w->put.near.on_src)
      || ! buff_push(& w->put.back_points, & w->put.near.on_dest)
      || ! buff_push(& w->put.weights, & w->put.near.weight)) {
    g_error("window_put_near_end: buff_push() failed!");
    return Failed;
  }

  w->put.got.constraints = 1;
  return Success;
}

static Task _window_put_near_real(Window *w, Real weight){
  if (! w->put.near.have.weight) {
    g_warning("Window.Put.Near got already the weight for this constrain.");
    return Success;
  }
  if (weight < 0.0) {
    g_warning("The weight has to be a positive Real number!");
    return Success;
  }
  w->put.near.weight = weight;
  w->put.near.have.weight = 1;
  return Success;
}

Task window_put_near_real(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  Real weight = BOX_VM_ARG1(vmp, Real);
  return _window_put_near_real(w, weight);
}

Task window_put_near_int(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  Int int_num = BOX_VM_ARG1(vmp, Int);
  if (w->put.near.have.on_src)
    return _window_put_near_real(w, (Real) int_num);

  else {
    Window *figure;
    Point *on_src;
    if (!w->put.got.figure) {
      g_error("Figure has not been specified. Cannot refer to its hot points "
              "from Window.Put.Near!");
      return Failed;
    }
    figure = (Window *) w->put.figure;

    on_src = pointlist_get(& figure->pointlist, int_num);
    if (on_src == (Point *) NULL) {
      g_error("The point index you gave to Window.Put.Near "
              "goes out of bounds.");
      return Failed;
    }

    w->put.near.on_src = *on_src;
    w->put.near.have.on_src = 1;
    return Success;
  }
}

Task window_put_near_point(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  Point *p = BOX_VM_ARGPTR1(vmp, Point);
  if (!w->put.near.have.on_src) {
    w->put.near.on_src = *p;
    w->put.near.have.on_src = 1;
    return Success;

  } else if (!w->put.near.have.on_dest) {
    w->put.near.on_dest = *p;
    w->put.near.have.on_dest = 1;
    return Success;

  } else {
    g_warning("Window.Put.Near already got the source and destination point: "
              "this point will be ignored!");
    return Success;
  }
}

Task window_put_near_string(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  char *name = BOX_VM_ARGPTR1(vmp, char);
  if (!w->put.near.have.on_src) {
    Window *figure;
    Point *on_src;
    if (!w->put.got.figure) {
      g_error("Figure has not been specified. Cannot refer to its hot points "
              "from Window.Put.Near!");
      return Failed;
    }
    figure = (Window *) w->put.figure;
    on_src = pointlist_find(& figure->pointlist, name);
    if (on_src == (Point *) NULL) {
      g_error("The name you provided to Window.Put.Near does not correspond "
              "to any of the hot points of the figure.");
      return Failed;
    }

    w->put.near.on_src = *on_src;
    w->put.near.have.on_src = 1;
    return Success;

  } else {
    g_warning("Window.Put.Near already got the source point. "
              "String will be ignored!");
    return Success;
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
