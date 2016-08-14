#ifndef _PY_DEEP_SURFACE_H
#define _PY_DEEP_SURFACE_H

#include "deep_surface.h"

#include "Python.h"

/// The internal C structure of our PyDeepSurface.
typedef struct {
  PyObject_HEAD
  DeepSurface* deep_surface;
} PyDeepSurface;

/// The Python type for a DeepSurface wrapper.
extern PyTypeObject PyDeepSurface_Type;

#endif  // _PY_DEEP_SURFACE_H
