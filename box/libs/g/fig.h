/* fig.h - Autore: Franchin Matteo
 *
 * Questo file contiene le dichiarazioni delle funzioni del file fig.c
 */

#ifndef _FIG_H
#  define _FIG_H

/* Matrice per la trasformazione lineare di fig_ltransform */
extern Real fig_matrix[6];

/* Procedure per gestire i layers */
grp_window *fig_open_win(int numlayers);
int fig_destroy_layer(int l);
int fig_new_layer(void);
void fig_select_layer(int l);
void fig_clear_layer(int l);
void fig_ltransform(Point *p, int n);
void fig_draw_layer(grp_window *source, int l);
void fig_draw_fig(grp_window *source);

#endif
