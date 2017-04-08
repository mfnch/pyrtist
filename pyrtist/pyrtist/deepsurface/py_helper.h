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

#ifndef _PY_HELPER_H
#define _PY_HELPER_H

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