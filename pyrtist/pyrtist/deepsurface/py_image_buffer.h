#ifndef _PY_IMAGE_BUFFER_H
#define _PY_IMAGE_BUFFER_H

#include "image_buffer.h"

#include "Python.h"

/// The internal C structure of our PyDeepBuffer.
typedef struct {
  PyObject_HEAD
  ARGBImageBuffer* image_buffer;  ///< The C++ object.
  PyObject* base;                 ///< The Python object owning this object.
                                  ///  This is nullptr for standalone objects.
} PyImageBuffer;

/// The Python type for a DeepBuffer wrapper.
extern PyTypeObject PyImageBuffer_Type;

PyObject* PyImageBuffer_FromC(PyTypeObject* type, ARGBImageBuffer* ib,
                              PyObject* base);

#endif  // _PY_IMAGE_BUFFER_H
