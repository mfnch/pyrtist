
#ifndef _G_H
#  define _G_H

typedef struct {
  Real r, g, b;
} Color;

typedef struct __OptColor {
  Color local;
  struct __OptColor *alternative, *selected;
} OptColor;

typedef Real LineStyle[4];

void g_error(const char *msg);
Task g_optcolor_set(OptColor *oc, Color *c);
Task g_optcolor_set_rgb(OptColor *oc, Real r, Real g, Real b);
Color *g_optcolor_get(OptColor *oc);
void g_optcolor_alternative_set(OptColor *oc, OptColor *alternative);

#  define g_warning g_error
#  define g_optcolor_unset(oc) g_optcolor_set_rgb((oc), -1.0, 0.0, 0.0);
#endif
