
#ifndef _G_H
#  define _G_H

typedef struct {
  Real r, g, b;
} Color;

typedef Real LineStyle[4];

void g_error(const char *msg);

#  define g_warning g_error

#endif
