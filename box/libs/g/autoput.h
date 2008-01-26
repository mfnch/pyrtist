/* autoput.h - Autore: Franchin Matteo
 *
 * Dichiarazione delle funzioni definite in autoput.c
 */

void aput_matrix(point *t, point *rcntr, real rang, real sx, real sy, real *matrix);
void aput_get( point *rot_center, point *trsl_vect,
 real *rot_angle, real *scale_x, real *scale_y );
void aput_set( point *rot_center, point *trsl_vect,
 real *rot_angle, real *scale_x, real *scale_y );
int aput_autoput(point *F, point *R, real *weight, int n, int needed);
int aput_allow(char *permissions, int *needed);
