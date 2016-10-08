#ifndef _PY_DEPTH_BUFFER_H
#define _PY_DEPTH_BUFFER_H

#include "depth_buffer.h"

#include "Python.h"

/// The internal C structure of our PyDepthBuffer.
typedef struct {
  PyObject_HEAD
  DepthBuffer* depth_buffer;  ///< The C++ object.
  PyObject* base;           ///< The Python object owning this object.
                            /// This is nullptr for standalone objects.
} PyDepthBuffer;

/// The Python type for a DepthBuffer wrapper.
extern PyTypeObject PyDepthBuffer_Type;

PyObject* PyDepthBuffer_FromC(PyTypeObject* type, DepthBuffer* db,
                             PyObject* base);

#endif  // _PY_DEPTH_BUFFER_H
