#include "deep_surface.h"
#include "py_image_buffer.h"
#include "py_deep_buffer.h"
#include "py_deep_surface.h"

// Forward declarations.
static PyObject* PyDeepSurface_New(PyTypeObject* type,
                                   PyObject* args, PyObject* kwds);
static void PyDeepSurface_Dealloc(PyObject* py_obj);
static PyObject* PyDeepSurface_GetImageBuffer(PyObject* ds, PyObject* args);
static PyObject* PyDeepSurface_GetDepthBuffer(PyObject* ds, PyObject* args);
static PyObject* PyDeepSurface_Clear(PyObject* ds, PyObject* args);
static PyObject* PyDeepSurface_Transfer(PyObject* ds, PyObject* args);
static PyObject* PyDeepSurface_SaveToFiles(PyObject* ds, PyObject* args);

// PyDeepSurface object methods.
static PyMethodDef pydeepsurface_methods[] = {
  {"get_image_buffer", PyDeepSurface_GetImageBuffer, METH_VARARGS},
  {"get_depth_buffer", PyDeepSurface_GetDepthBuffer, METH_VARARGS},
  {"clear", PyDeepSurface_Clear, METH_VARARGS},
  {"transfer", PyDeepSurface_Transfer, METH_VARARGS},
  {"save_to_files", PyDeepSurface_SaveToFiles, METH_VARARGS},
  {NULL, NULL, 0, NULL}
};

// PyDeepSurface object type.
PyTypeObject PyDeepSurface_Type = {
  PyObject_HEAD_INIT(NULL)
  0,                                         // ob_size
  "deepsurface.DeepSurface",                 // tp_name
  sizeof(PyDeepSurface),                     // tp_basicsize
  0,                                         // tp_itemsize
  PyDeepSurface_Dealloc,                     // tp_dealloc
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
  0,                                         // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
  0,                                         // tp_doc
  0,                                         // tp_traverse
  0,                                         // tp_clear
  0,                                         // tp_richcompare
  0,                                         // tp_weaklistoffset
  0,                                         // tp_iter
  0,                                         // tp_iternext
  pydeepsurface_methods,                     // tp_methods
  0,                                         // tp_members
  0,                                         // tp_getset
  0,                                         // tp_base
  0,                                         // tp_dict
  0,                                         // tp_descr_get
  0,                                         // tp_descr_set
  0,                                         // tp_dictoffset
  0,                                         // tp_init
  0,                                         // tp_alloc
  PyDeepSurface_New,                         // tp_new
  0,                                         // tp_free
  0,                                         // tp_is_gc
  0,                                         // tp_bases
};

static PyObject* PyDeepSurface_New(PyTypeObject* type,
                                   PyObject* args, PyObject* kwds) {
  int width, height;
  if (!PyArg_ParseTuple(args, "ii:DeepSurface.__new__", &width, &height))
    return nullptr;

  DeepSurface* ds = new DeepSurface{width, width, height};
  if (!ds->IsValid()) {
    // Destroy ds and raise an exception.
    delete ds;
    PyErr_SetString(PyExc_ValueError, "Invalid width/height");
    return nullptr;
  }

  PyObject *py_obj = PyDeepSurface_Type.tp_alloc(type, 0);
  if (py_obj == nullptr) {
    delete ds;
    return nullptr;
  }

  PyDeepSurface* py_ds = reinterpret_cast<PyDeepSurface*>(py_obj);
  py_ds->deep_surface = ds;
  return py_obj;
}

static void PyDeepSurface_Dealloc(PyObject* py_obj) {
  PyDeepSurface* py_ds = reinterpret_cast<PyDeepSurface*>(py_obj);
  delete py_ds->deep_surface;
  py_ds->ob_type->tp_free(py_obj);
}

static PyObject*
PyDeepSurface_GetImageBuffer(PyObject* self, PyObject* args) {
  if (!PyArg_ParseTuple(args, ":DeepSurface.get_src_cairo_image"))
    return nullptr;

  PyDeepSurface* py_ds = reinterpret_cast<PyDeepSurface*>(self);
  ARGBImageBuffer* ib = py_ds->deep_surface->GetImageBuffer();
  return PyImageBuffer_FromC(&PyImageBuffer_Type, ib, self);
}

static PyObject*
PyDeepSurface_GetDepthBuffer(PyObject* self, PyObject* args) {
  if (!PyArg_ParseTuple(args, ":DeepSurface.get_depth_buffer"))
    return nullptr;

  PyDeepSurface* py_ds = reinterpret_cast<PyDeepSurface*>(self);
  DeepBuffer* db = py_ds->deep_surface->GetDeepBuffer();
  return PyDeepBuffer_FromC(&PyDeepBuffer_Type, db, self);
}

static PyObject* PyDeepSurface_Clear(PyObject* ds, PyObject* args) {
  if (!PyArg_ParseTuple(args, ":DeepSurface.clear"))
    return nullptr;

  PyDeepSurface* py_ds = reinterpret_cast<PyDeepSurface*>(ds);
  py_ds->deep_surface->Clear();
  Py_RETURN_NONE;
}

static PyObject* PyDeepSurface_Transfer(PyObject* ds, PyObject* args) {
  int and_clear = 1;
  if (!PyArg_ParseTuple(args, "|i:DeepSurface.transfer", &and_clear))
    return nullptr;

  PyDeepSurface* py_ds = reinterpret_cast<PyDeepSurface*>(ds);
  py_ds->deep_surface->Transfer(and_clear != 0);
  Py_RETURN_NONE;
}

static PyObject* PyDeepSurface_SaveToFiles(PyObject* ds, PyObject* args) {
  const char* image_file_name = nullptr;
  const char* normals_file_name = nullptr;
  if (!PyArg_ParseTuple(args, "zz:DeepSurface.save_to_files",
                        &image_file_name, &normals_file_name))
    return nullptr;

  PyDeepSurface* py_ds = reinterpret_cast<PyDeepSurface*>(ds);
  if (py_ds->deep_surface->SaveToFiles(image_file_name, normals_file_name))
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}
