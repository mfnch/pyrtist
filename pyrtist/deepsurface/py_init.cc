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

#include "Python.h"

#include "py_depth_buffer.h"
#include "py_image_buffer.h"
#include "py_mesh.h"
#include "deep_surface.h"
#include "mesh.h"


static PyObject* Py_Transfer(PyObject* ds, PyObject* args) {
  PyObject* py_src_image;
  PyObject* py_src_depth;
  PyObject* py_dst_image;
  PyObject* py_dst_depth;
  if (!PyArg_ParseTuple(args, "OOOO:DeepSurface.transfer",
                        &py_src_depth, &py_src_image,
                        &py_dst_depth, &py_dst_image))
    return nullptr;

  if (!(PyObject_TypeCheck(py_src_depth, PyDepthBuffer_GetType()) &&
        PyObject_TypeCheck(py_src_image, PyImageBuffer_GetType()) &&
        PyObject_TypeCheck(py_dst_depth, PyDepthBuffer_GetType()) &&
        PyObject_TypeCheck(py_dst_image, PyImageBuffer_GetType()))) {
    PyErr_SetString(PyExc_ValueError,
                    "Invalid arguments to DeepSurface.transfer");
    return nullptr;
  }

  auto src_depth = reinterpret_cast<PyDepthBuffer*>(py_src_depth)->depth_buffer;
  auto src_image = reinterpret_cast<PyImageBuffer*>(py_src_image)->image_buffer;
  auto dst_depth = reinterpret_cast<PyDepthBuffer*>(py_dst_depth)->depth_buffer;
  auto dst_image = reinterpret_cast<PyImageBuffer*>(py_dst_image)->image_buffer;
  DeepSurface::Transfer(src_depth, src_image, dst_depth, dst_image);
  Py_RETURN_NONE;
}

static PyObject* Py_Sculpt(PyObject* ds, PyObject* args) {
  PyObject* py_src_image;
  PyObject* py_src_depth;
  PyObject* py_dst_image;
  PyObject* py_dst_depth;
  if (!PyArg_ParseTuple(args, "OOOO:DeepSurface.sculpt",
                        &py_src_depth, &py_src_image,
                        &py_dst_depth, &py_dst_image))
    return nullptr;

  if (!(PyObject_TypeCheck(py_src_depth, PyDepthBuffer_GetType()) &&
        PyObject_TypeCheck(py_src_image, PyImageBuffer_GetType()) &&
        PyObject_TypeCheck(py_dst_depth, PyDepthBuffer_GetType()) &&
        PyObject_TypeCheck(py_dst_image, PyImageBuffer_GetType()))) {
    PyErr_SetString(PyExc_ValueError,
                    "Invalid arguments to DeepSurface.sculpt");
    return nullptr;
  }

  auto src_depth = reinterpret_cast<PyDepthBuffer*>(py_src_depth)->depth_buffer;
  auto src_image = reinterpret_cast<PyImageBuffer*>(py_src_image)->image_buffer;
  auto dst_depth = reinterpret_cast<PyDepthBuffer*>(py_dst_depth)->depth_buffer;
  auto dst_image = reinterpret_cast<PyImageBuffer*>(py_dst_image)->image_buffer;
  DeepSurface::Sculpt(src_depth, src_image, dst_depth, dst_image);
  Py_RETURN_NONE;
}

#define PYTYPE_READY(t) \
  if (PyType_Ready(t) < 0) return nullptr

#define PYMODULE_ADDOBJECT(m, n, t) \
  do { Py_INCREF(t); \
       PyModule_AddObject(m, n, reinterpret_cast<PyObject*>(t)); } while (0)

static PyObject* Module_Init() {
  static PyMethodDef method_def[] = {
    {"transfer", Py_Transfer, METH_VARARGS},
    {"sculpt", Py_Sculpt, METH_VARARGS},
    {nullptr, nullptr, 0, nullptr}
  };

#if PY_MAJOR_VERSION >= 3
  static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "deepsurface",
    nullptr,  // Documentation.
    -1,
    method_def
  };

  PyObject* m = PyModule_Create(&module_def);
  if (m == nullptr)
    return nullptr;
#elif PY_MAJOR_VERSION >= 2
  PyObject* m = Py_InitModule("deepsurface", method_def);
#else
#error "Unsupported Python version"
#endif
  PYTYPE_READY(PyImageBuffer_GetType());
  PYTYPE_READY(PyDepthBuffer_GetType());
  PYTYPE_READY(PyMesh_GetType());

  PYMODULE_ADDOBJECT(m, "ImageBuffer", PyImageBuffer_GetType());
  PYMODULE_ADDOBJECT(m, "DepthBuffer", PyDepthBuffer_GetType());
  PYMODULE_ADDOBJECT(m, "Mesh", PyMesh_GetType());

  return m;
}

extern "C" {

#if PY_MAJOR_VERSION >= 3
  PyMODINIT_FUNC PyInit_deepsurface() {
    return Module_Init();
  }
#else
  PyMODINIT_FUNC initdeepsurface() {
    Module_Init();
  }
#endif

}  // extern "C"
