/* autoput.h - Autore: Matteo Franchin
 *
 * Dichiarazione delle funzioni definite in autoput.c
 */

void aput_matrix(Point *t, Point *rcntr, Real rang, Real sx, Real sy, Real *matrix);
void aput_get(Point *rot_center, Point *trsl_vect,
              Real *rot_angle, Real *scale_x, Real *scale_y );
void aput_set(Point *rot_center, Point *trsl_vect,
              Real *rot_angle, Real *scale_x, Real *scale_y );
int aput_autoput(Point *F, Point *R, Real *weight, int n, int needed);
int aput_allow(char *permissions, int *needed);
