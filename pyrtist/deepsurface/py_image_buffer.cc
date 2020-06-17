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

#include <string.h>

#include "deep_surface.h"
#include "py_image_buffer.h"
#include "py_helper.h"

// Forward declarations.
static PyObject* PyImageBuffer_New(PyTypeObject* type,
                                   PyObject* args, PyObject* kwds);
static void PyImageBuffer_Dealloc(PyObject* py_obj);
static PyObject* PyImageBuffer_SetFromString(PyObject* ib, PyObject* args);
static PyObject* PyImageBuffer_Clear(PyObject* ib, PyObject* args);
static PyObject* PyImageBuffer_SaveToFile(PyObject* ib, PyObject* args);

// PyImageBuffer object methods.
static PyMethodDef pyimagebuffer_methods[] = {
  {"set_from_string", PyImageBuffer_SetFromString, METH_VARARGS},
  {"clear", PyImageBuffer_Clear, METH_NOARGS},
  {"save_to_file", PyImageBuffer_SaveToFile, METH_VARARGS},
  {NULL, NULL, 0, NULL}
};

///////////////////////////////////////////////////////////////////////////////
// Implement the new buffer API.

static int PyImageBuffer_GetBuffer(PyObject* exporter,
                                   Py_buffer* view, int flags) {
  PyImageBuffer* py_ib = reinterpret_cast<PyImageBuffer*>(exporter);
  ARGBImageBuffer* ib = py_ib->image_buffer;
  return PyBuffer_FillInfo(view, exporter, ib->GetPtr(), ib->GetSizeInBytes(),
                           /* readonly */ 0, flags);
}

PyTypeObject* PyImageBuffer_GetType() {
  struct Setter {
    Setter()
        : buffer_procs{},
          py_type{PyObject_HEAD_INIT(NULL)} {
      // Set the buffer protocol functions.
      buffer_procs.bf_getbuffer = PyImageBuffer_GetBuffer;

      // Set the type struct.
      auto& t = py_type;
      t.tp_name = "deepsurface.ImageBuffer";
      t.tp_basicsize = sizeof(PyImageBuffer);
      t.tp_dealloc = PyImageBuffer_Dealloc;
      t.tp_as_buffer = &buffer_procs;
#if PY_MAJOR_VERSION >= 3
      t.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
#else
      t.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_NEWBUFFER;
#endif
      t.tp_methods = pyimagebuffer_methods;
      t.tp_new = PyImageBuffer_New;
    }

    PyBufferProcs buffer_procs;
    PyTypeObject py_type;
  };

  static Setter instance{};
  return &instance.py_type;
}

///////////////////////////////////////////////////////////////////////////////


PyObject* PyImageBuffer_FromC(PyTypeObject* type, ARGBImageBuffer* ib,
                              PyObject* base) {
  if (!ib->IsValid()) {
    // Destroy ib and raise an exception.
    delete ib;
    PyErr_SetString(PyExc_ValueError, "Invalid width/height");
    return nullptr;
  }

  PyObject *py_obj = PyImageBuffer_GetType()->tp_alloc(type, 0);
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
  DS_GET_PY_HEAD(py_ib)->ob_type->tp_free(py_obj);
}

static bool SetFromString(ARGBImageBuffer* dst,
                          Py_buffer* src, int src_stride, int src_pixel_size,
                          std::function<void(uint8_t*, uint8_t*)> setter) {
  int dst_width = dst->GetWidth();
  int dst_height = dst->GetHeight();
  int dst_stride = dst->GetStride();
  int src_size = src_pixel_size*src_stride*dst_height;

  if (src_size < src->len)
    return false;

  uint8_t* initial_src_ptr = reinterpret_cast<uint8_t*>(src->buf);
  uint32_t* dst_bol_ptr = dst->GetPtr();
  uint32_t* dst_end_ptr = dst_bol_ptr + dst_stride*dst_height;
  src_stride *= src_pixel_size;
  while (dst_bol_ptr < dst_end_ptr) {
    uint32_t* dst_eol_ptr = dst_bol_ptr + dst_width;
    uint32_t* dst_ptr = dst_bol_ptr;
    dst_bol_ptr += dst_stride;
    uint8_t* src_ptr = initial_src_ptr;
    initial_src_ptr += src_stride;
    while (dst_ptr < dst_eol_ptr) {
      setter(reinterpret_cast<uint8_t*>(dst_ptr), src_ptr);
      src_ptr += src_pixel_size;
      dst_ptr++;
    }
  }

  return true;
}

static PyObject* PyImageBuffer_SetFromString(PyObject* py_obj, PyObject* args) {
  Py_buffer buffer;
  int src_stride;
  const char* mode;
  if (!PyArg_ParseTuple(args, "s*is:ImageBuffer.set_from_string",
                        &buffer, &src_stride, &mode))
    return nullptr;

  if (buffer.shape != nullptr || buffer.strides != nullptr ||
      buffer.suboffsets != nullptr || buffer.ndim != 1) {
    PyErr_SetString(PyExc_ValueError, "Only simple buffers are supported");
    return nullptr;
  }

  bool success = false;
  ARGBImageBuffer* ib = reinterpret_cast<PyImageBuffer*>(py_obj)->image_buffer;
  if (strcmp(mode, "RGBA") == 0) {
    auto setter = [](uint8_t* dst, uint8_t* src)->void {
      uint32_t a = src[3];
      dst[0] = (src[2]*a)/255;
      dst[1] = (src[1]*a)/255;
      dst[2] = (src[0]*a)/255;
      dst[3] = a;
    };
    success = SetFromString(ib, &buffer, src_stride, 4, setter);
  } else if (strcmp(mode, "RGB") == 0) {
    auto setter = [](uint8_t* dst, uint8_t* src)->void {
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = 0xff;
    };
    success = SetFromString(ib, &buffer, src_stride, 3, setter);
  }
  PyBuffer_Release(&buffer);

  if (!success) {
    PyErr_SetString(PyExc_ValueError, "Invalid mode");
    return nullptr;
  }

  Py_RETURN_TRUE;
}

static PyObject* PyImageBuffer_Clear(PyObject* ib, PyObject*) {
  PyImageBuffer* py_ib = reinterpret_cast<PyImageBuffer*>(ib);
  py_ib->image_buffer->Clear();
  Py_RETURN_NONE;
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
