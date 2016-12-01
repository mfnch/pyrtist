#include "Python.h"

#include "py_depth_buffer.h"
#include "py_image_buffer.h"
#include "deep_surface.h"
#include "obj_parser.h"

#include <memory>


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

static PyObject* Py_Sculpt(PyObject* ds, PyObject* args) {
  PyObject* py_src_image;
  PyObject* py_src_depth;
  PyObject* py_dst_image;
  PyObject* py_dst_depth;
  if (!PyArg_ParseTuple(args, "OOOO:DeepSurface.sculpt",
                        &py_src_depth, &py_src_image,
                        &py_dst_depth, &py_dst_image))
    return nullptr;

  if (!(PyObject_TypeCheck(py_src_depth, &PyDepthBuffer_Type) &&
        PyObject_TypeCheck(py_src_image, &PyImageBuffer_Type) &&
        PyObject_TypeCheck(py_dst_depth, &PyDepthBuffer_Type) &&
        PyObject_TypeCheck(py_dst_image, &PyImageBuffer_Type))) {
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

#include <iostream>

static bool
MatrixFromPy(deepsurface::Affine3<float>& matrix, PyObject* py_matrix) {
  PyObject* row_iter = PyObject_GetIter(py_matrix);
  bool ok = (row_iter != nullptr);
  for (int i = 0; ok && i < 3; i++) {
    PyObject* row = PyIter_Next(row_iter);
    PyObject* col_iter = PyObject_GetIter(row);
    ok = (col_iter != nullptr);
    for (int j = 0; ok && j < 4; j++) {
      PyObject* col = PyIter_Next(col_iter);
      ok = PyFloat_Check(col);
      if (ok)
        matrix[i][j] = static_cast<float>(PyFloat_AsDouble(col));
      Py_DECREF(col);
    }
    Py_DECREF(row);
  }
  Py_DECREF(row_iter);

  if (!ok) {
    PyErr_Clear();
    return false;
  }

  return true;
}

static PyObject* Py_LoadObj(PyObject* ds, PyObject* args) {
  const char* file_name = nullptr;
  PyObject* py_image;
  PyObject* py_depth;
  PyObject* py_matrix;
  if (!PyArg_ParseTuple(args, "zOOO:load_obj",
                        &file_name, &py_depth, &py_image, &py_matrix))
    return nullptr;

  std::unique_ptr<ObjFile> obj_file{ObjFile::Load(file_name)};
  if (obj_file) {
    auto depth = reinterpret_cast<PyDepthBuffer*>(py_depth)->depth_buffer;
    auto image = reinterpret_cast<PyImageBuffer*>(py_image)->image_buffer;
    deepsurface::Affine3<float> matrix;
    if (MatrixFromPy(matrix, py_matrix)) {
      obj_file->Draw(depth, image, matrix);
      Py_RETURN_TRUE;
    }
  }
  Py_RETURN_FALSE;
}

static PyMethodDef deepsurface_methods[] = {
  {"transfer", Py_Transfer, METH_VARARGS},
  {"sculpt", Py_Sculpt, METH_VARARGS},
  {"load_obj", Py_LoadObj, METH_VARARGS},
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
