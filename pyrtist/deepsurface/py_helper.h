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

#ifndef _PY_HELPER_H
#define _PY_HELPER_H

#include "Python.h"

#if PY_MAJOR_VERSION >= 3
#define DS_GET_PY_HEAD(py_obj) (&((py_obj)->ob_base))
#define DS_PyLong_Check PyLong_Check
#define DS_PyLong_FromLong PyLong_FromLong
#define DS_PyLong_AsLong PyLong_AsLong
#elif PY_MAJOR_VERSION >= 2
#define DS_GET_PY_HEAD(py_obj) (py_obj)
#define DS_PyLong_Check PyInt_Check
#define DS_PyLong_FromLong PyInt_FromLong
#define DS_PyLong_AsLong PyInt_AsLong
#else
#error "Unsupported version of Python"
#endif

#include "Python.h"

namespace deepsurface {

class PyRef {
 public:
  PyRef(PyObject* obj) : ref_(obj) { Py_XINCREF(ref_); }
  ~PyRef() { Py_XDECREF(ref_); }

 private:
  PyObject* ref_;
};

}  // namespace deepsurface

#endif  /* _PY_HELPER_H */
