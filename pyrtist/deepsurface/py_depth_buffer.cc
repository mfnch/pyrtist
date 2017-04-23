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
#include "py_depth_buffer.h"
#include "py_image_buffer.h"
#include "sampler.h"

#include <iostream>

// Sample Python callbacks to achieve better performance.
static constexpr bool kSamplePythonCallbacks = true;

// Forward declarations.
static PyObject* PyDepthBuffer_New(PyTypeObject* type,
                                  PyObject* args, PyObject* kwds);
static void PyDepthBuffer_Dealloc(PyObject* py_obj);
static PyObject* PyDepthBuffer_GetWidth(PyObject* db, PyObject*);
static PyObject* PyDepthBuffer_GetHeight(PyObject* db, PyObject*);
static PyObject* PyDepthBuffer_Clear(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_DrawPlane(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_DrawStep(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_DrawSphere(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_DrawCylinder(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_DrawCircular(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_DrawCrescent(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_ComputeNormals(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_SaveNormals(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_GetData(PyObject* db, PyObject* args);
static PyObject* PyDepthBuffer_SaveToFile(PyObject* ib, PyObject* args);

// PyDepthBuffer object methods.
static PyMethodDef pydepthbuffer_methods[] = {
  {"clear", PyDepthBuffer_Clear, METH_NOARGS},
  {"get_width", PyDepthBuffer_GetWidth, METH_NOARGS},
  {"get_height", PyDepthBuffer_GetHeight, METH_NOARGS},
  {"draw_plane", PyDepthBuffer_DrawPlane, METH_VARARGS},
  {"draw_step", PyDepthBuffer_DrawStep, METH_VARARGS},
  {"draw_sphere", PyDepthBuffer_DrawSphere, METH_VARARGS},
  {"draw_cylinder", PyDepthBuffer_DrawCylinder, METH_VARARGS},
  {"draw_circular", PyDepthBuffer_DrawCircular, METH_VARARGS},
  {"draw_crescent", PyDepthBuffer_DrawCrescent, METH_VARARGS},
  {"compute_normals", PyDepthBuffer_ComputeNormals, METH_NOARGS},
  {"save_normals", PyDepthBuffer_SaveNormals, METH_VARARGS},
  {"get_data", PyDepthBuffer_GetData, METH_NOARGS},
  {"save_to_file", PyDepthBuffer_SaveToFile, METH_VARARGS},
  {NULL, NULL, 0, NULL}
};

///////////////////////////////////////////////////////////////////////////////
// Implement the old buffer API.

static Py_ssize_t
PyDepthBuffer_GetReadBuf(PyObject* py_obj, Py_ssize_t segment, void** ptr) {
  if (segment != 0) {
    PyErr_SetString(PyExc_SystemError, "DepthBuffer has only one segment");
    return -1;
  }
  DepthBuffer* db = reinterpret_cast<PyDepthBuffer*>(py_obj)->depth_buffer;
  *ptr = db->GetPtr();
  return db->GetSizeInBytes();
}

static Py_ssize_t
PyDepthBuffer_GetSegCount(PyObject* py_obj, Py_ssize_t* lenp) {
  if (lenp) {
    DepthBuffer* db = reinterpret_cast<PyDepthBuffer*>(py_obj)->depth_buffer;
    *lenp = db->GetSizeInBytes();
  }
  return 1;  // Only one segment.
}

static PyBufferProcs depthbuffer_as_buffer = {
  PyDepthBuffer_GetReadBuf,
  PyDepthBuffer_GetReadBuf,
  PyDepthBuffer_GetSegCount,
  nullptr,
};

///////////////////////////////////////////////////////////////////////////////

// PyDepthBuffer object type.
PyTypeObject PyDepthBuffer_Type = {
  PyObject_HEAD_INIT(NULL)
  0,                                         // ob_size
  "deepsurface.DepthBuffer",                  // tp_name
  sizeof(PyDepthBuffer),                      // tp_basicsize
  0,                                         // tp_itemsize
  PyDepthBuffer_Dealloc,                      // tp_dealloc
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
  &depthbuffer_as_buffer,                    // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
  0,                                         // tp_doc
  0,                                         // tp_traverse
  0,                                         // tp_clear
  0,                                         // tp_richcompare
  0,                                         // tp_weaklistoffset
  0,                                         // tp_iter
  0,                                         // tp_iternext
  pydepthbuffer_methods,                     // tp_methods
  0,                                         // tp_members
  0,                                         // tp_getset
  0,                                         // tp_base
  0,                                         // tp_dict
  0,                                         // tp_descr_get
  0,                                         // tp_descr_set
  0,                                         // tp_dictoffset
  0,                                         // tp_init
  0,                                         // tp_alloc
  PyDepthBuffer_New,                          // tp_new
  0,                                         // tp_free
  0,                                         // tp_is_gc
  0,                                         // tp_bases
};

PyObject* PyDepthBuffer_FromC(PyTypeObject* type, DepthBuffer* db,
                             PyObject* base) {
  if (!db->IsValid()) {
    // Destroy db and raise an exception.
    delete db;
    PyErr_SetString(PyExc_ValueError, "Invalid width/height");
    return nullptr;
  }

  PyObject *py_obj = PyDepthBuffer_Type.tp_alloc(type, 0);
  if (py_obj == nullptr) {
    delete db;
    return nullptr;
  }

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(py_obj);
  py_db->depth_buffer = db;
  Py_XINCREF(base);
  py_db->base = base;
  return py_obj;
}

static PyObject* PyDepthBuffer_New(PyTypeObject* type,
                                   PyObject* args, PyObject* kwds) {
  int width, height;
  if (!PyArg_ParseTuple(args, "ii:DepthBuffer.__new__", &width, &height))
    return nullptr;

  DepthBuffer* db = new DepthBuffer{width, width, height};
  return PyDepthBuffer_FromC(type, db, nullptr);
}

static void PyDepthBuffer_Dealloc(PyObject* py_obj) {
  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(py_obj);
  if (py_db->base == nullptr)
    // This is a standalone buffer: delete it here.
    delete py_db->depth_buffer;
  else
    // This buffer is embedded inside py_db->base: just do a DECREF.
    Py_DECREF(py_db->base);
  py_db->ob_type->tp_free(py_obj);
}

static PyObject* PyDepthBuffer_GetWidth(PyObject* db, PyObject*) {
  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  long v = py_db->depth_buffer->GetWidth();
  return PyLong_FromLong(static_cast<long>(v));
}

static PyObject* PyDepthBuffer_GetHeight(PyObject* db, PyObject*) {
  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  long v = py_db->depth_buffer->GetHeight();
  return PyLong_FromLong(static_cast<long>(v));
}

static PyObject* PyDepthBuffer_Clear(PyObject* db, PyObject*) {
  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  py_db->depth_buffer->Clear();
  Py_RETURN_NONE;
}

static PyObject* PyDepthBuffer_DrawPlane(PyObject* db, PyObject* args) {
  float clip_start_x, clip_start_y, clip_end_x, clip_end_y;
  float mx[6];
  float z00, z10, z01;
  if (!PyArg_ParseTuple(args, "fffffffffffff:DepthBuffer.draw_plane",
                        &clip_start_x, &clip_start_y, &clip_end_x, &clip_end_y,
                        mx, mx + 1, mx + 2, mx + 3, mx + 4, mx + 5,
                        &z00, &z10, &z01))
    return nullptr;

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  py_db->depth_buffer->DrawPlane(clip_start_x, clip_start_y, clip_end_x,
                                 clip_end_y, mx, z00, z10, z01);
  Py_RETURN_NONE;
}

static PyObject* PyDepthBuffer_DrawStep(PyObject* db, PyObject* args) {
  float clip_start_x, clip_start_y, clip_end_x, clip_end_y,
      start_x, start_y, start_z, end_x, end_y, end_z;
  if (!PyArg_ParseTuple(args, "ffffffffff:DepthBuffer.draw_step",
                        &clip_start_x, &clip_start_y, &clip_end_x, &clip_end_y,
                        &start_x, &start_y, &start_z, &end_x, &end_y, &end_z))
    return nullptr;

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  py_db->depth_buffer->DrawStep(clip_start_x, clip_start_y,
                                clip_end_x, clip_end_y,
                                start_x, start_y, start_z,
                                end_x, end_y, end_z);
  Py_RETURN_NONE;
}

static PyObject* PyDepthBuffer_DrawSphere(PyObject* db, PyObject* args) {
  float clip_start_x, clip_start_y, clip_end_x, clip_end_y;
  float mx[6];
  float z_start, z_end;

  if (!PyArg_ParseTuple(args, "ffffffffffff:DepthBuffer.draw_sphere",
                        &clip_start_x, &clip_start_y, &clip_end_x, &clip_end_y,
                        mx, mx + 1, mx + 2, mx + 3, mx + 4, mx + 5,
                        &z_start, &z_end))
    return nullptr;

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  py_db->depth_buffer->
    DrawSphere(clip_start_x, clip_start_y, clip_end_x, clip_end_y, mx,
               z_start, z_end);
  Py_RETURN_NONE;
}

static PyObject* PyDepthBuffer_DrawCylinder(PyObject* db, PyObject* args) {
  float clip_start_x, clip_start_y, clip_end_x, clip_end_y;
  float mx[6];
  float end_radius, z_of_axis, start_radius_z, end_radius_z;
  if (!PyArg_ParseTuple(args, "ffffffffffffff:DepthBuffer.draw_cylinder",
                        &clip_start_x, &clip_start_y, &clip_end_x, &clip_end_y,
                        mx, mx + 1, mx + 2, mx + 3, mx + 4, mx + 5,
                        &end_radius, &z_of_axis, &start_radius_z,
                        &end_radius_z))
    return nullptr;

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  py_db->depth_buffer->
    DrawCylinder(clip_start_x, clip_start_y, clip_end_x, clip_end_y, mx,
                 end_radius, z_of_axis, start_radius_z, end_radius_z);
  Py_RETURN_NONE;
}

static PyObject* PyDepthBuffer_DrawCircular(PyObject* db, PyObject* args) {
  float clip_start_x, clip_start_y, clip_end_x, clip_end_y;
  float mx[6];
  float translate_z, scale_z;
  PyObject* callback;
  if (!PyArg_ParseTuple(args, "ffffffffffffO:DepthBuffer.draw_circular",
                        &clip_start_x, &clip_start_y, &clip_end_x, &clip_end_y,
                        mx, mx + 1, mx + 2, mx + 3, mx + 4, mx + 5,
                        &scale_z, &translate_z, &callback))
    return nullptr;

  if (!PyCallable_Check(callback)) {
    PyErr_SetString(PyExc_TypeError,
                    "DepthBuffer.draw_circular: object cannot be called");
    return nullptr;
  }

  // Wrap the Python callback inside a std::function<float(float)>.
  bool ok = true;
  auto callback_wrapper =
    [callback, &ok](float x)->float {
      PyObject* py_args = Py_BuildValue("(f)", x);
      PyObject* py_result = PyObject_CallObject(callback, py_args);
      Py_DECREF(py_args);

      float result = -1.0f;
      if (PyFloat_Check(py_result)) {
        result = static_cast<float>(PyFloat_AsDouble(py_result));
      } else {
        ok = false;
      }
      Py_DECREF(py_result);
      return result;
    };

  std::function<float(float)> radius_fn = callback_wrapper;
  Sampler sampler;
  if (kSamplePythonCallbacks) {
    // Sample the Python callback, so that we don't have to go back to Python
    // to evaluate it.

    // First, let's find how fine the sampling should be.
    float n = std::max(std::abs(clip_end_x - clip_start_x),
                       std::abs(clip_end_y - clip_start_y));
    int num_samples = std::max(2, static_cast<int>(n));

    // Now create the sampler.
    sampler.Sample(num_samples, callback_wrapper);
    if (!ok) {
      PyErr_SetString(PyExc_TypeError,
                      "DepthBuffer.draw_circular: callback returned non-float");
      return nullptr;
    }

    radius_fn = [&sampler](float x)->float {return sampler(x);};
  }

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  py_db->depth_buffer->
    DrawCircular(clip_start_x, clip_start_y, clip_end_x, clip_end_y, mx,
                 scale_z, translate_z, radius_fn);
  Py_RETURN_NONE;
}

static PyObject* PyDepthBuffer_DrawCrescent(PyObject* db, PyObject* args) {
  float clip_start_x, clip_start_y, clip_end_x, clip_end_y;
  float mx[6];
  float scale_z, translate_z, y0, y1;

  if (!PyArg_ParseTuple(args, "ffffffffffffff:DepthBuffer.draw_crescent",
                        &clip_start_x, &clip_start_y, &clip_end_x, &clip_end_y,
                        mx, mx + 1, mx + 2, mx + 3, mx + 4, mx + 5,
                        &scale_z, &translate_z, &y0, &y1))
    return nullptr;

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  py_db->depth_buffer->
    DrawCrescent(clip_start_x, clip_start_y, clip_end_x, clip_end_y, mx,
                 scale_z, translate_z, y0, y1);
  Py_RETURN_NONE;

}

static PyObject* PyDepthBuffer_ComputeNormals(PyObject* db, PyObject* args) {
  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  ARGBImageBuffer* normals = py_db->depth_buffer->ComputeNormals();
  return PyImageBuffer_FromC(&PyImageBuffer_Type, normals, nullptr);
}

static PyObject* PyDepthBuffer_SaveNormals(PyObject* db, PyObject* args) {
  const char* file_name = nullptr;
  if (!PyArg_ParseTuple(args, "s:DepthBuffer.save_normals", &file_name))
    return nullptr;

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  ARGBImageBuffer* normals = py_db->depth_buffer->ComputeNormals();
  bool success = normals->SaveToFile(file_name);
  delete normals;
  if (success)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static PyObject* PyDepthBuffer_GetData(PyObject* db, PyObject* args) {
  return PyBuffer_FromReadWriteObject(db, 0, Py_END_OF_BUFFER);
}

static PyObject* PyDepthBuffer_SaveToFile(PyObject* db, PyObject* args) {
  const char* file_name = nullptr;
  if (!PyArg_ParseTuple(args, "z:DepthBuffer.save_to_file", &file_name))
    return nullptr;

  PyDepthBuffer* py_db = reinterpret_cast<PyDepthBuffer*>(db);
  if (py_db->depth_buffer->SaveToFile(file_name))
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}
