#include "Python.h"

#include "py_depth_buffer.h"
#include "py_image_buffer.h"
#include "deep_surface.h"

static PyObject* Py_Transfer(PyObject* ds, PyObject* args) {
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

static PyMethodDef deepsurface_methods[] = {
  {"transfer", Py_Transfer, METH_VARARGS},
  {nullptr, nullptr, 0, nullptr}
};

#define PYTYPE_READY(t) \
  if (PyType_Ready(&(t)) < 0) return

#define PYMODULE_ADDOBJECT(m, n, t) \
  do { Py_INCREF(&(t)); \
       PyModule_AddObject(m, n, reinterpret_cast<PyObject*>(&(t))); } while (0)

extern "C" {

  PyMODINIT_FUNC initdeepsurface() {
    PYTYPE_READY(PyImageBuffer_Type);
    PYTYPE_READY(PyDepthBuffer_Type);

    PyObject* m = Py_InitModule("deepsurface", deepsurface_methods);

    PYMODULE_ADDOBJECT(m, "ImageBuffer", PyImageBuffer_Type);
    PYMODULE_ADDOBJECT(m, "DepthBuffer", PyDepthBuffer_Type);
  }

}  // extern "C"
