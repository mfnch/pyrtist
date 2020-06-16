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

#ifndef _PY_IMAGE_BUFFER_H
#define _PY_IMAGE_BUFFER_H

#include "image_buffer.h"

#include "Python.h"

/// The internal C structure of our PyImageBuffer.
typedef struct {
  PyObject_HEAD
  ARGBImageBuffer* image_buffer;  ///< The C++ object.
  PyObject* base;                 ///< The Python object owning this object.
                                  ///  This is nullptr for standalone objects.
} PyImageBuffer;

/// @brief Get the Python type for a ImageBuffer wrapper.
PyTypeObject* PyImageBuffer_GetType();

PyObject* PyImageBuffer_FromC(PyTypeObject* type, ARGBImageBuffer* ib,
                              PyObject* base);

#endif  // _PY_IMAGE_BUFFER_H
