#ifndef _PY_MESH_H
#define _PY_MESH_H

#include "mesh.h"

#include "Python.h"

/// The internal C structure of our PyMesh.
typedef struct {
  PyObject_HEAD
  deepsurface::Mesh* mesh;  ///< The C++ object.
} PyMesh;

/// The Python type for a DepthBuffer wrapper.
extern PyTypeObject PyMesh_Type;

PyObject* PyMesh_FromC(PyTypeObject* type, ARGBImageBuffer* ib,
                       PyObject* base);

#endif  // _PY_MESH_H
