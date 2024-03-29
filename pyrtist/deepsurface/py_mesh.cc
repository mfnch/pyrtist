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

#include "mesh.h"
#include "py_mesh.h"
#include "py_depth_buffer.h"
#include "py_image_buffer.h"
#include "py_helper.h"

#include <memory>

// Forward declarations.
static PyObject* PyMesh_New(PyTypeObject* type,
                            PyObject* args, PyObject* kwds);
static void PyMesh_Dealloc(PyObject* py_obj);
static PyObject* PyMesh_Draw(PyObject* m, PyObject* args);
static PyObject* PyMesh_AddVertices(PyObject* mesh, PyObject* args);
static PyObject* PyMesh_AddTexCoords(PyObject* mesh, PyObject* args);
static PyObject* PyMesh_AddPolygons(PyObject* mesh, PyObject* args);

// PyMesh object methods.
static PyMethodDef pymesh_methods[] = {
  {"draw", PyMesh_Draw, METH_VARARGS},
  {"add_vertices", PyMesh_AddVertices, METH_VARARGS},
  {"add_tex_coords", PyMesh_AddTexCoords, METH_VARARGS},
  {"add_polygons", PyMesh_AddPolygons, METH_VARARGS},
  {NULL, NULL, 0, NULL}
};

///////////////////////////////////////////////////////////////////////////////

PyTypeObject* PyMesh_GetType() {
  struct Setter {
    Setter() : py_type{PyObject_HEAD_INIT(NULL)} {
      auto& t = py_type;
      t.tp_name = "deepsurface.Mesh";
      t.tp_basicsize = sizeof(PyMesh);
      t.tp_dealloc = PyMesh_Dealloc;
      t.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
      t.tp_methods = pymesh_methods;
      t.tp_new = PyMesh_New;
    }

    PyTypeObject py_type;
  };

  static Setter instance{};
  return &instance.py_type;
}

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

  PyObject *py_obj = PyMesh_GetType()->tp_alloc(type, 0);
  if (py_obj == nullptr)
    return nullptr;

  PyMesh* py_mesh = reinterpret_cast<PyMesh*>(py_obj);
  py_mesh->mesh = mesh.release();
  return py_obj;
}

static void PyMesh_Dealloc(PyObject* py_obj) {
  PyMesh* py_mesh = reinterpret_cast<PyMesh*>(py_obj);
  delete py_mesh->mesh;
  DS_GET_PY_HEAD(py_mesh)->ob_type->tp_free(py_obj);
}

bool ForEachPyObj(PyObject* in, std::function<bool(PyObject*, int)> fn) {
  PyObject* iter = PyObject_GetIter(in);
  bool ok = (iter != nullptr);

  for (int i = 0; ok; i++) {
    PyObject* py_item = PyIter_Next(iter);
    if (py_item == nullptr)
      break;

    ok = fn(py_item, i);
    Py_DECREF(py_item);
  };

  Py_XDECREF(iter);
  if (!ok)
    PyErr_Clear();
  return ok;
}

static bool
SetScalarFromPy(float* out, PyObject* in) {
  if (in == nullptr)
    return false;
  else if (PyFloat_Check(in))
    *out = static_cast<float>(PyFloat_AsDouble(in));
  else if (DS_PyLong_Check(in))
    *out = static_cast<float>(DS_PyLong_AsLong(in));
  else
    return false;
  return true;
}

static bool
SetAffine3FromPy(deepsurface::Affine3<float>& matrix, PyObject* py_matrix) {
  return ForEachPyObj(py_matrix, [&matrix](PyObject* py_row, int i)->bool {
    auto& row = matrix[i];
    return ForEachPyObj(py_row, [&row](PyObject* py_entry, int j)->bool {
      return SetScalarFromPy(&row[j], py_entry);
    });
  });
}

static PyObject* PyMesh_Draw(PyObject* mesh, PyObject* args) {
  PyObject* py_image;
  PyObject* py_depth;
  PyObject* py_matrix;
  if (!PyArg_ParseTuple(args, "OOO:Mesh.draw",
                        &py_depth, &py_image, &py_matrix))
    return nullptr;

  if (!(PyObject_TypeCheck(py_depth, PyDepthBuffer_GetType()) &&
        PyObject_TypeCheck(py_image, PyImageBuffer_GetType()))) {
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

#include <iostream>

using Point2 = deepsurface::Mesh::Point2;
using Point3 = deepsurface::Mesh::Point3;

template <typename T, int N>
static bool PointFromPy(deepsurface::Point<T, N>* out, PyObject* in) {
  PyObject* iter = PyObject_GetIter(in);
  bool ok = (iter != nullptr);
  for (int i = 0; ok && i < N; i++) {
    PyObject* py_coord = PyIter_Next(iter);
    ok = SetScalarFromPy(&(*out)[i], py_coord);
    Py_XDECREF(py_coord);
  }
  Py_XDECREF(iter);
  return ok;
}

static bool
AddVerticesFromPy(deepsurface::Mesh* mesh, PyObject* py_vertices) {
  return ForEachPyObj(py_vertices, [mesh](PyObject* py_obj, int i)->bool {
    Point3 vertex;
    if (!PointFromPy(&vertex, py_obj))
      return false;
    mesh->AddVertex(vertex);
    return true;
  });
}

static PyObject*
PyMesh_AddVertices(PyObject* mesh, PyObject* args) {
  PyObject* py_vertices;
  if (!PyArg_ParseTuple(args, "O:Mesh.add_vertices", &py_vertices))
    return nullptr;

  PyMesh* py_mesh = reinterpret_cast<PyMesh*>(mesh);
  if (py_mesh->mesh != nullptr &&
      AddVerticesFromPy(py_mesh->mesh, py_vertices))
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static PyObject*
PyMesh_AddTexCoords(PyObject* mesh, PyObject* args) {
  PyObject* py_tex_coords;
  if (!PyArg_ParseTuple(args, "O:Mesh.add_tex_coords", &py_tex_coords))
    return nullptr;

  PyMesh* py_mesh = reinterpret_cast<PyMesh*>(mesh);
  bool success = (py_mesh->mesh != nullptr);
  if (success) {
    deepsurface::Mesh* mesh = py_mesh->mesh;
    success = ForEachPyObj(py_tex_coords,
      [mesh](PyObject* py_obj, int i)->bool {
        Point2 tex_coord;
        if (!PointFromPy(&tex_coord, py_obj))
          return false;
        mesh->AddTexCoords(tex_coord);
        return true;
      });
  }

  if (success)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static bool
AddPolygonsFromPy(deepsurface::Mesh* mesh, PyObject* py_polygons) {
  return ForEachPyObj(py_polygons, [mesh](PyObject* py_polygon, int)->bool {
    deepsurface::Face* face = mesh->CreateFace();
    return ForEachPyObj(py_polygon, [face](PyObject* py_indices, int)->bool {
      std::array<int, 3> indices;
      bool indices_ok =
        ForEachPyObj(py_indices, [&indices](PyObject* py_index, int i)->bool {
          if (i >= 0 && static_cast<size_t>(i) < indices.size()) {
            auto index_as_long = DS_PyLong_AsLong(py_index);
            if (index_as_long != -1 || !PyErr_Occurred()) {
              indices[i] = static_cast<int>(index_as_long);
              return true;
            }
          }
          return false;
        });
      if (!indices_ok)
        return false;
      deepsurface::Face::Indices idxs{indices[0], indices[1], indices[2]};
      face->Append(idxs);
      return true;
    });
  });
}

static bool ARGBColorFromRGBAPyTuple(PyObject* tp, uint32_t* argb_out) {
  std::array<int, 4> rgba;
  auto setter = [&rgba](PyObject* py_color_component, int i)->bool {
    if (i < 0 || i > 3 || !PyFloat_Check(py_color_component)) {
      return false;
    }
    double cc = PyFloat_AsDouble(py_color_component);
    rgba[i] = static_cast<int>(255.0 * std::min(1.0, std::max(0.0, cc)));
    return true;
  };
  if (!ForEachPyObj(tp, setter)) {
    return false;
  }
  *argb_out = GetARGB(rgba[3], rgba[0], rgba[1], rgba[2]);
  return true;
}

// PyImageTexture is an ImageTexture which holds a Python reference to the
// original PyImageBuffer which contains the ImageBuffer. Instances of
// PyImageBuffer will be held as unique_ptr inside the Mesh and will thus be
// alive for the time they are needed.
class PyImageTexture : public deepsurface::ImageTexture {
 public:
  PyImageTexture(PyImageBuffer* py_ib)
    : deepsurface::ImageTexture(py_ib->image_buffer),
      ref_to_py_material_(reinterpret_cast<PyObject*>(py_ib)) { }

 private:
  deepsurface::PyRef ref_to_py_material_;
};

static std::unique_ptr<deepsurface::Texture>
NewMaterialFromPy(PyObject* py_material) {
  if (PyObject_TypeCheck(py_material, PyImageBuffer_GetType())) {
    // Create an ImageTexture holding a Python reference to the PyImageBuffer.
    auto py_ib = reinterpret_cast<PyImageBuffer*>(py_material);
    return std::unique_ptr<deepsurface::Texture>{new PyImageTexture(py_ib)};
  } else {
    // Try creating a UniformTexture.
    uint32_t argb;
    if (!ARGBColorFromRGBAPyTuple(py_material, &argb))
      return std::unique_ptr<deepsurface::Texture>();
    return std::unique_ptr<deepsurface::Texture>{
      new deepsurface::UniformTexture(argb)};
  }
}

static PyObject*
PyMesh_AddPolygons(PyObject* mesh, PyObject* args) {
  PyObject* py_polygons;
  PyObject* py_material;
  if (!PyArg_ParseTuple(args, "OO:Mesh.add_polygons",
                        &py_polygons, &py_material))
    return nullptr;

  PyMesh* py_mesh = reinterpret_cast<PyMesh*>(mesh);
  auto tex = NewMaterialFromPy(py_material);

  if (py_mesh->mesh == nullptr || !tex)
    Py_RETURN_FALSE;

  py_mesh->mesh->SetTexture(std::move(tex));
  if (!AddPolygonsFromPy(py_mesh->mesh, py_polygons))
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}
