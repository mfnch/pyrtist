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
  w->put.scale.x = w->put.scale.y = 0.0;
  w->put.auto_transforms = 0;
  w->put.got.constraints = 0;
  w->put.got.compute = 0;
  w->put.got.figure = 0;

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

#if 0
static int put_begin(void)
{
  list *cur_list;

  PRNMSG("put_begin: Inizio istruzione...");

  put_status = CONSTRAINTS_USED;
  put_figtype = NO_FIG_SPECIFIED;

  /* Stato predefinito: tutto manuale! */
  put_needed = 0;

  /* Setto i parametri di default per la trasformazione */
  put_rot_angle = 0.0;
  put_scale_y = put_scale_x = 1.0;
  put_rot_center = put_trsl_vect = (Point) {0.0, 0.0};

  /* Inizializzo le due liste di punti (plist) che conterranno i vincoli */
  cur_list = list_status;
  list_status = B_list;
  if ( ! list_begin(OBJID_PLIST, sizeof(Point)) ) {
    EXIT_ERR("Impossibile creare la plist!\n");
  }
  B_list = list_status;

  list_status = F_list;
  if ( ! list_begin(OBJID_PLIST, sizeof(Point)) ) {
    EXIT_ERR("Impossibile creare la plist!\n");
  }
  F_list = list_status;

  /* Creo la lista che conterra' i pesi relativi a ciascun vincolo */
  list_status = weight_list;
  if ( ! list_begin(OBJID_FLIST, sizeof(Real)) ) {
    EXIT_ERR("Impossibile creare la flist!\n");
  }
  weight_list = list_status;
  list_status = cur_list;

  EXIT_OK("Ok!\n");
}
#endif


Task window_put_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  grp_window *cur_win = grp_win;
  Window *figure;

  /* Se sono gia' stati inseriti dei vincoli (con !near[...]), ma essi non
   * sono ancora stati utilizzati (cioe' .Compute non e' stata invocata), allora
   * li uso adesso e mi calcolo la trasformazione che meglio li soddisfa!
   */
  if ( !(w->put.got.compute) ) { TASK( put_calculate(w) ); }

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


Task window_put_window(VMProgram *vmp) {return Success;}
Task window_put_point(VMProgram *vmp) {return Success;}
Task window_put_real(VMProgram *vmp) {return Success;}
Task window_put_string(VMProgram *vmp) {return Success;}

Task window_put_scale_real(VMProgram *vmp) {return Success;}
Task window_put_near_begin(VMProgram *vmp) {return Success;}
Task window_put_near_end(VMProgram *vmp) {return Success;}
Task window_put_near_real(VMProgram *vmp) {return Success;}
Task window_put_near_string(VMProgram *vmp) {return Success;}
