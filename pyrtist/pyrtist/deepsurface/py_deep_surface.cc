#include "deep_surface.h"
#include "py_image_buffer.h"
#include "py_depth_buffer.h"
#include "py_deep_surface.h"

// Forward declarations.
static PyObject* PyDeepSurface_New(PyTypeObject* type,
                                   PyObject* args, PyObject* kwds);
static void PyDeepSurface_Dealloc(PyObject* py_obj);
static PyObject* PyDeepSurface_GetImageBuffer(PyObject* ds, PyObject* args);
static PyObject* PyDeepSurface_GetDepthBuffer(PyObject* ds, PyObject* args);
static PyObject* PyDeepSurface_Transfer(PyObject* ds, PyObject* args);

// PyDeepSurface object methods.
static PyMethodDef pydeepsurface_methods[] = {
  {"get_image_buffer", PyDeepSurface_GetImageBuffer, METH_VARARGS},
  {"get_depth_buffer", PyDeepSurface_GetDepthBuffer, METH_VARARGS},
  {"transfer", PyDeepSurface_Transfer, METH_VARARGS | METH_STATIC},
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
  if (!PyArg_ParseTuple(args, ":DeepSurface.get_image_buffer"))
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
  DepthBuffer* db = py_ds->deep_surface->GetDepthBuffer();
  return PyDepthBuffer_FromC(&PyDepthBuffer_Type, db, self);
}

static PyObject* PyDeepSurface_Transfer(PyObject* ds, PyObject* args) {
  PyObject* py_src_image;
  PyObject* py_src_depth;
  PyObject* py_dst_image;
  PyObject* py_dst_depth;
  if (!PyArg_ParseTuple(args, "OOOO:DeepSurface.transfer",
                        &py_src_depth, &py_src_image,
                        &py_dst_depth, &py_dst_image))
    return nullptr;

  if (!(PyObject_TypeCheck(py_src_depth, &PyDepthBuffer_Type) &&
        PyObject_TypeCheck(py_src_image, &PyImageBuffer_Type) &&
        PyObject_TypeCheck(py_dst_depth, &PyDepthBuffer_Type) &&
        PyObject_TypeCheck(py_dst_image, &PyImageBuffer_Type))) {
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
