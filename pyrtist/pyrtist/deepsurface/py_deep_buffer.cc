#include "deep_surface.h"
#include "py_deep_buffer.h"
#include "py_image_buffer.h"

// Forward declarations.
static PyObject* PyDeepBuffer_New(PyTypeObject* type,
                                  PyObject* args, PyObject* kwds);
static void PyDeepBuffer_Dealloc(PyObject* py_obj);
static PyObject* PyDeepBuffer_DrawStep(PyObject* db, PyObject* args);
static PyObject* PyDeepBuffer_DrawSphere(PyObject* db, PyObject* args);
static PyObject* PyDeepBuffer_DrawCylinder(PyObject* db, PyObject* args);
static PyObject* PyDeepBuffer_ComputeNormals(PyObject* db, PyObject* args);
static PyObject* PyDeepBuffer_SaveNormals(PyObject* db, PyObject* args);
static PyObject* PyDeepBuffer_GetData(PyObject* db, PyObject* args);

// PyDeepBuffer object methods.
static PyMethodDef pydeepbuffer_methods[] = {
  {"draw_step", PyDeepBuffer_DrawStep, METH_VARARGS},
  {"draw_sphere", PyDeepBuffer_DrawSphere, METH_VARARGS},
  {"draw_cylinder", PyDeepBuffer_DrawCylinder, METH_VARARGS},
  {"compute_normals", PyDeepBuffer_ComputeNormals, METH_NOARGS},
  {"save_normals", PyDeepBuffer_SaveNormals, METH_VARARGS},
  {"get_data", PyDeepBuffer_GetData, METH_NOARGS},
  {NULL, NULL, 0, NULL}
};

///////////////////////////////////////////////////////////////////////////////
// Implement the old buffer API.

static Py_ssize_t
PyDeepBuffer_GetReadBuf(PyObject* py_obj, Py_ssize_t segment, void** ptr) {
  if (segment != 0) {
    PyErr_SetString(PyExc_SystemError, "DeepBuffer has only one segment");
    return -1;
  }
  DeepBuffer* db = reinterpret_cast<PyDeepBuffer*>(py_obj)->deep_buffer;
  *ptr = db->GetPtr();
  return db->GetSizeInBytes();
}

static Py_ssize_t
PyDeepBuffer_GetSegCount(PyObject* py_obj, Py_ssize_t* lenp) {
  if (lenp) {
    DeepBuffer* db = reinterpret_cast<PyDeepBuffer*>(py_obj)->deep_buffer;
    *lenp = db->GetSizeInBytes();
  }
  return 1;  // Only one segment.
}

static PyBufferProcs deepbuffer_as_buffer = {
  PyDeepBuffer_GetReadBuf,
  PyDeepBuffer_GetReadBuf,
  PyDeepBuffer_GetSegCount,
  nullptr,
};

///////////////////////////////////////////////////////////////////////////////

// PyDeepBuffer object type.
PyTypeObject PyDeepBuffer_Type = {
  PyObject_HEAD_INIT(NULL)
  0,                                         // ob_size
  "deepsurface.DeepBuffer",                  // tp_name
  sizeof(PyDeepBuffer),                      // tp_basicsize
  0,                                         // tp_itemsize
  PyDeepBuffer_Dealloc,                      // tp_dealloc
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
  &deepbuffer_as_buffer,                     // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
  0,                                         // tp_doc
  0,                                         // tp_traverse
  0,                                         // tp_clear
  0,                                         // tp_richcompare
  0,                                         // tp_weaklistoffset
  0,                                         // tp_iter
  0,                                         // tp_iternext
  pydeepbuffer_methods,                      // tp_methods
  0,                                         // tp_members
  0,                                         // tp_getset
  0,                                         // tp_base
  0,                                         // tp_dict
  0,                                         // tp_descr_get
  0,                                         // tp_descr_set
  0,                                         // tp_dictoffset
  0,                                         // tp_init
  0,                                         // tp_alloc
  PyDeepBuffer_New,                          // tp_new
  0,                                         // tp_free
  0,                                         // tp_is_gc
  0,                                         // tp_bases
};

PyObject* PyDeepBuffer_FromC(PyTypeObject* type, DeepBuffer* db,
                             PyObject* base) {
  if (!db->IsValid()) {
    // Destroy db and raise an exception.
    delete db;
    PyErr_SetString(PyExc_ValueError, "Invalid width/height");
    return nullptr;
  }

  PyObject *py_obj = PyDeepBuffer_Type.tp_alloc(type, 0);
  if (py_obj == nullptr) {
    delete db;
    return nullptr;
  }

  PyDeepBuffer* py_db = reinterpret_cast<PyDeepBuffer*>(py_obj);
  py_db->deep_buffer = db;
  Py_XINCREF(base);
  py_db->base = base;
  return py_obj;
}

static PyObject* PyDeepBuffer_New(PyTypeObject* type,
                                  PyObject* args, PyObject* kwds) {
  int width, height;
  if (!PyArg_ParseTuple(args, "ii:DeepBuffer.__new__", &width, &height))
    return nullptr;

  DeepBuffer* db = new DeepBuffer{width, width, height};
  return PyDeepBuffer_FromC(type, db, nullptr);
}

static void PyDeepBuffer_Dealloc(PyObject* py_obj) {
  PyDeepBuffer* py_db = reinterpret_cast<PyDeepBuffer*>(py_obj);
  if (py_db->base == nullptr)
    // This is a standalone buffer: delete it here.
    delete py_db->deep_buffer;
  else
    // This buffer is embedded inside py_db->base: just do a DECREF.
    Py_DECREF(py_db->base);
  py_db->ob_type->tp_free(py_obj);
}

static PyObject* PyDeepBuffer_DrawStep(PyObject* db, PyObject* args) {
  float clip_start_x, clip_start_y, clip_end_x, clip_end_y,
      start_x, start_y, start_z, end_x, end_y, end_z;
  if (!PyArg_ParseTuple(args, "ffffffffff:DeepSurface.draw_step",
                        &clip_start_x, &clip_start_y, &clip_end_x, &clip_end_y,
                        &start_x, &start_y, &start_z, &end_x, &end_y, &end_z))
    return nullptr;

  PyDeepBuffer* py_db = reinterpret_cast<PyDeepBuffer*>(db);
  py_db->deep_buffer->DrawStep(clip_start_x, clip_start_y,
                               clip_end_x, clip_end_y,
                               start_x, start_y, start_z,
                               end_x, end_y, end_z);
  Py_RETURN_NONE;
}

static PyObject* PyDeepBuffer_DrawSphere(PyObject* db, PyObject* args) {
  int x, y, z, radius;
  float z_scale;
  if (!PyArg_ParseTuple(args, "iiiif:DeepSurface.draw_sphere",
                        &x, &y, &z, &radius, &z_scale))
    return nullptr;

  PyDeepBuffer* py_db = reinterpret_cast<PyDeepBuffer*>(db);
  py_db->deep_buffer->DrawSphere(x, y, z, radius, z_scale);
  Py_RETURN_NONE;
}

static PyObject* PyDeepBuffer_DrawCylinder(PyObject* db, PyObject* args) {
  float clip_start_x, clip_start_y, clip_end_x, clip_end_y;
  float mx[6];
  float end_radius, z_of_axis, start_radius_z, end_radius_z;
  if (!PyArg_ParseTuple(args, "ffffffffffffff:DeepSurface.draw_cylinder",
                        &clip_start_x, &clip_start_y, &clip_end_x, &clip_end_y,
                        mx, mx + 1, mx + 2, mx + 3, mx + 4, mx + 5,
                        &end_radius, &z_of_axis, &start_radius_z,
                        &end_radius_z))
    return nullptr;

  PyDeepBuffer* py_db = reinterpret_cast<PyDeepBuffer*>(db);
  py_db->deep_buffer->
    DrawCylinder(clip_start_x, clip_start_y, clip_end_x, clip_end_y, mx,
                 end_radius, z_of_axis, start_radius_z, end_radius_z);
  Py_RETURN_NONE;
}

static PyObject* PyDeepBuffer_ComputeNormals(PyObject* db, PyObject* args) {
  PyDeepBuffer* py_db = reinterpret_cast<PyDeepBuffer*>(db);
  ARGBImageBuffer* normals = py_db->deep_buffer->ComputeNormals();
  return PyImageBuffer_FromC(&PyImageBuffer_Type, normals, nullptr);
}

static PyObject* PyDeepBuffer_SaveNormals(PyObject* db, PyObject* args) {
  const char* file_name = nullptr;
  if (!PyArg_ParseTuple(args, "s:DeepSurface.save_normals", &file_name))
    return nullptr;

  PyDeepBuffer* py_db = reinterpret_cast<PyDeepBuffer*>(db);
  ARGBImageBuffer* normals = py_db->deep_buffer->ComputeNormals();
  bool success = normals->SaveToFile(file_name);
  delete normals;
  if (success)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static PyObject* PyDeepBuffer_GetData(PyObject* db, PyObject* args) {
  return PyBuffer_FromReadWriteObject(db, 0, Py_END_OF_BUFFER);
}
