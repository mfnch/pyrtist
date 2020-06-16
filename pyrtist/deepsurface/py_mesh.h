// Copyright (C) 2017 Matteo Franchin
//
// This file is part of Pyrtist.
//   Pyrtist is free software: you can redistribute it and/or modify it
//   under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, either version 2.1 of the License, or
//   (at your option) any later version.
//
//   Pyrtist is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _PY_MESH_H
#define _PY_MESH_H

#include "mesh.h"

#include "Python.h"

/// The internal C structure of our PyMesh.
typedef struct {
  PyObject_HEAD
  deepsurface::Mesh* mesh;  ///< The C++ object.
} PyMesh;

/// @brief Get the Python type for a DepthBuffer wrapper.
PyTypeObject* PyMesh_GetType();

PyObject* PyMesh_FromC(PyTypeObject* type, ARGBImageBuffer* ib,
                       PyObject* base);

#endif  // _PY_MESH_H
