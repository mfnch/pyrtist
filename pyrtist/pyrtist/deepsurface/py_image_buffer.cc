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

#include "deep_surface.h"
#include "py_image_buffer.h"

// Forward declarations.
static PyObject* PyImageBuffer_New(PyTypeObject* type,
                                   PyObject* args, PyObject* kwds);
static void PyImageBuffer_Dealloc(PyObject* py_obj);
static PyObject* PyImageBuffer_Clear(PyObject* ib, PyObject* args);
static PyObject* PyImageBuffer_GetData(PyObject* ib, PyObject* args);
static PyObject* PyImageBuffer_SaveToFile(PyObject* ib, PyObject* args);

// PyImageBuffer object methods.
static PyMethodDef pyimagebuffer_methods[] = {
  {"clear", PyImageBuffer_Clear, METH_NOARGS},
  {"get_data", PyImageBuffer_GetData, METH_NOARGS},
  {"save_to_file", PyImageBuffer_SaveToFile, METH_VARARGS},
  {NULL, NULL, 0, NULL}
};

///////////////////////////////////////////////////////////////////////////////
// Implement the old buffer API.

static Py_ssize_t
PyImageBuffer_GetReadBuf(PyObject* py_obj, Py_ssize_t segment, void** ptr) {
  if (segment != 0) {
    PyErr_SetString(PyExc_SystemError, "ImageBuffer has only one segment");
    return -1;
  }
  ARGBImageBuffer* ib = reinterpret_cast<PyImageBuffer*>(py_obj)->image_buffer;
  *ptr = ib->GetPtr();
  return ib->GetSizeInBytes();
}

static Py_ssize_t
PyImageBuffer_GetSegCount(PyObject* py_obj, Py_ssize_t* lenp) {
  if (lenp) {
    ARGBImageBuffer* ib =
      reinterpret_cast<PyImageBuffer*>(py_obj)->image_buffer;
    *lenp = ib->GetSizeInBytes();
  }
  return 1;  // Only one segment.
}

static PyBufferProcs imagebuffer_as_buffer = {
  PyImageBuffer_GetReadBuf,
  PyImageBuffer_GetReadBuf,
  PyImageBuffer_GetSegCount,
  nullptr,
};

///////////////////////////////////////////////////////////////////////////////

// PyImageBuffer object type.
PyTypeObject PyImageBuffer_Type = {
  PyObject_HEAD_INIT(NULL)
  0,                                         // ob_size
  "deepsurface.ImageBuffer",                 // tp_name
  sizeof(PyImageBuffer),                     // tp_basicsize
  0,                                         // tp_itemsize
  PyImageBuffer_Dealloc,                     // tp_dealloc
  0,                                         // tp_print
  0,                                         // tp_getattr
  0,                                         // tp_setattr
  0,                                         // tp_compare
  0,                                         // tp_repr
  0,                                         // tp_as_number
  0,                                         // tp_as_sequence
  0,                                         // tp_as_mapping
  0,                                         // tp_hash
  0,                                         // tp_call
  0,                                         // tp_str
  0,                                         // tp_getattro
  0,                                         // tp_setattro
  &imagebuffer_as_buffer,                    // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
  0,                                         // tp_doc
  0,                                         // tp_traverse
  0,                                         // tp_clear
  0,                                         // tp_richcompare
  0,                                         // tp_weaklistoffset
  0,                                         // tp_iter
  0,                                         // tp_iternext
  pyimagebuffer_methods,                     // tp_methods
  0,                                         // tp_members
  0,                                         // tp_getset
  0,                                         // tp_base
  0,                                         // tp_dict
  0,                                         // tp_descr_get
  0,                                         // tp_descr_set
  0,                                         // tp_dictoffset
  0,                                         // tp_init
  0,                                         // tp_alloc
  PyImageBuffer_New,                         // tp_new
  0,                                         // tp_free
  0,                                         // tp_is_gc
  0,                                         // tp_bases
};

PyObject* PyImageBuffer_FromC(PyTypeObject* type, ARGBImageBuffer* ib,
                              PyObject* base) {
  if (!ib->IsValid()) {
    // Destroy ib and raise an exception.
    delete ib;
    PyErr_SetString(PyExc_ValueError, "Invalid width/height");
    return nullptr;
  }

  PyObject *py_obj = PyImageBuffer_Type.tp_alloc(type, 0);
  if (py_obj == nullptr) {
    delete ib;
    return nullptr;
  }

  PyImageBuffer* py_ib = reinterpret_cast<PyImageBuffer*>(py_obj);
  py_ib->image_buffer = ib;
  Py_XINCREF(base);
  py_ib->base = base;
  return py_obj;
}

static PyObject* PyImageBuffer_New(PyTypeObject* type,
                                   PyObject* args, PyObject* kwds) {
  int width, height;
  if (!PyArg_ParseTuple(args, "ii:ImageBuffer.__new__", &width, &height))
    return nullptr;

  ARGBImageBuffer* ib = new ARGBImageBuffer{width, width, height};
  return PyImageBuffer_FromC(type, ib, nullptr);
}

static void PyImageBuffer_Dealloc(PyObject* py_obj) {
  PyImageBuffer* py_ib = reinterpret_cast<PyImageBuffer*>(py_obj);
  if (py_ib->base == nullptr)
    // This is a standalone buffer: delete it here.
    delete py_ib->image_buffer;
  else
    // This buffer is embedded inside py_ib->base: just do a DECREF.
    Py_DECREF(py_ib->base);
  py_ib->ob_type->tp_free(py_obj);
}

static PyObject* PyImageBuffer_Clear(PyObject* ib, PyObject*) {
  PyImageBuffer* py_ib = reinterpret_cast<PyImageBuffer*>(ib);
  py_ib->image_buffer->Clear();
  Py_RETURN_NONE;
}

static PyObject* PyImageBuffer_GetData(PyObject* ib, PyObject* args) {
  return PyBuffer_FromReadWriteObject(ib, 0, Py_END_OF_BUFFER);
}

static PyObject* PyImageBuffer_SaveToFile(PyObject* ib, PyObject* args) {
  const char* file_name = nullptr;
  if (!PyArg_ParseTuple(args, "z:ImageBuffer.save_to_file", &file_name))
    return nullptr;

  PyImageBuffer* py_ib = reinterpret_cast<PyImageBuffer*>(ib);
  if (py_ib->image_buffer->SaveToFile(file_name))
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}
