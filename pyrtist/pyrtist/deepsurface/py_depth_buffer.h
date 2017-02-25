// Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
//
// This file is part of Pyrtist.
//   Pyrtist is free software: you can redistribute it and/or modify it
//   under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Pyrtist is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

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
