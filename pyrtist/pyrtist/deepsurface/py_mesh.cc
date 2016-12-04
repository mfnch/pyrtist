#include "mesh.h"
#include "py_mesh.h"
#include "py_depth_buffer.h"
#include "py_image_buffer.h"

#include <memory>

// Forward declarations.
static PyObject* PyMesh_New(PyTypeObject* type,
                            PyObject* args, PyObject* kwds);
static void PyMesh_Dealloc(PyObject* py_obj);
static PyObject* PyMesh_Draw(PyObject* m, PyObject* args);

// PyMesh object methods.
static PyMethodDef pymesh_methods[] = {
  {"draw", PyMesh_Draw, METH_VARARGS},
  {NULL, NULL, 0, NULL}
};

///////////////////////////////////////////////////////////////////////////////

// PyMesh object type.
PyTypeObject PyMesh_Type = {
  PyObject_HEAD_INIT(NULL)
  0,                                         // ob_size
  "deepsurface.Mesh",                        // tp_name
  sizeof(PyMesh),                            // tp_basicsize
  0,                                         // tp_itemsize
  PyMesh_Dealloc,                            // tp_dealloc
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
  pymesh_methods,                            // tp_methods
  0,                                         // tp_members
  0,                                         // tp_getset
  0,                                         // tp_base
  0,                                         // tp_dict
  0,                                         // tp_descr_get
  0,                                         // tp_descr_set
  0,                                         // tp_dictoffset
  0,                                         // tp_init
  0,                                         // tp_alloc
  PyMesh_New,                                // tp_new
  0,                                         // tp_free
  0,                                         // tp_is_gc
  0,                                         // tp_bases
};

static PyObject* PyMesh_New(PyTypeObject* type,
                            PyObject* args, PyObject* kwds) {
  const char* file_name = nullptr;
  if (!PyArg_ParseTuple(args, "|z:Mesh.__new__", &file_name))
    return nullptr;

  using Mesh = deepsurface::Mesh;
  std::unique_ptr<Mesh> mesh(file_name != nullptr ?
                             Mesh::LoadObj(file_name) : new Mesh);
  if (!mesh) {
    PyErr_SetString(PyExc_ValueError, "Mesh creation failed");
    return nullptr;
  }

  PyObject *py_obj = PyMesh_Type.tp_alloc(type, 0);
  if (py_obj == nullptr)
    return nullptr;

  PyMesh* py_mesh = reinterpret_cast<PyMesh*>(py_obj);
  py_mesh->mesh = mesh.release();
  return py_obj;
}

static void PyMesh_Dealloc(PyObject* py_obj) {
  PyMesh* py_mesh = reinterpret_cast<PyMesh*>(py_obj);
  delete py_mesh->mesh;
  py_mesh->ob_type->tp_free(py_obj);
}

static bool
SetScalarFromPy(float* out, PyObject* in) {
  if (in == nullptr)
    return false;
  else if (PyFloat_Check(in))
    *out = static_cast<float>(PyFloat_AsDouble(in));
  else if (PyInt_Check(in))
    *out = static_cast<float>(PyInt_AsLong(in));
  else
    return false;
  return true;
}

static bool
SetAffine3FromPy(deepsurface::Affine3<float>& matrix, PyObject* py_matrix) {
  PyObject* row_iter = PyObject_GetIter(py_matrix);
  bool ok = (row_iter != nullptr);
  for (int i = 0; ok && i < 3; i++) {
    PyObject* row = PyIter_Next(row_iter);
    PyObject* col_iter = PyObject_GetIter(row);
    ok = (col_iter != nullptr);
    for (int j = 0; ok && j < 4; j++) {
      PyObject* col = PyIter_Next(col_iter);
      ok = SetScalarFromPy(&matrix[i][j], col);
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

static PyObject* PyMesh_Draw(PyObject* mesh, PyObject* args) {
  PyObject* py_image;
  PyObject* py_depth;
  PyObject* py_matrix;
  if (!PyArg_ParseTuple(args, "OOO:Mesh.draw",
                        &py_depth, &py_image, &py_matrix))
    return nullptr;

  if (!(PyObject_TypeCheck(py_depth, &PyDepthBuffer_Type) &&
        PyObject_TypeCheck(py_image, &PyImageBuffer_Type))) {
    PyErr_SetString(PyExc_ValueError,
                    "Invalid arguments to Mesh.draw");
    return nullptr;
  }

  auto depth = reinterpret_cast<PyDepthBuffer*>(py_depth)->depth_buffer;
  auto image = reinterpret_cast<PyImageBuffer*>(py_image)->image_buffer;
  PyMesh* py_mesh = reinterpret_cast<PyMesh*>(mesh);
  if (py_mesh->mesh != nullptr) {
    deepsurface::Affine3<float> matrix;
    if (SetAffine3FromPy(matrix, py_matrix)) {
      py_mesh->mesh->Draw(depth, image, matrix);
      Py_RETURN_TRUE;
    }
  }
  Py_RETURN_FALSE;
}
