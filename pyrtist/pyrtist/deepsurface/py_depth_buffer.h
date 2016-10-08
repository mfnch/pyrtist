#ifndef _PY_DEPTH_BUFFER_H
#define _PY_DEPTH_BUFFER_H

#include "depth_buffer.h"

#include "Python.h"

/// The internal C structure of our PyDeepBuffer.
typedef struct {
  PyObject_HEAD
  DeepBuffer* deep_buffer;  ///< The C++ object.
  PyObject* base;           ///< The Python object owning this object.
                            /// This is nullptr for standalone objects.
} PyDeepBuffer;

/// The Python type for a DeepBuffer wrapper.
extern PyTypeObject PyDeepBuffer_Type;

PyObject* PyDeepBuffer_FromC(PyTypeObject* type, DeepBuffer* db,
                             PyObject* base);

#endif  // _PY_DEPTH_BUFFER_H
