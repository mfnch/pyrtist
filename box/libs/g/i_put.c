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

Task window_put_string(VMProgram *vmp) {return Success;}

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

Task window_put_near_begin(VMProgram *vmp) {return Success;}
Task window_put_near_end(VMProgram *vmp) {return Success;}
Task window_put_near_real(VMProgram *vmp) {return Success;}
Task window_put_near_string(VMProgram *vmp) {return Success;}
