#include "Python.h"

#include "py_deep_buffer.h"
#include "py_deep_surface.h"

static PyMethodDef deepsurface_methods[] = {
  {nullptr, nullptr, 0, nullptr}
};

#define PYTYPE_READY(t) \
  if (PyType_Ready(&(t)) < 0) return

#define PYMODULE_ADDOBJECT(m, n, t) \
  do { Py_INCREF(&(t)); \
       PyModule_AddObject(m, n, reinterpret_cast<PyObject*>(&(t))); } while (0)

extern "C" {

  PyMODINIT_FUNC initdeepsurface() {
    PYTYPE_READY(PyDeepSurface_Type);
    PYTYPE_READY(PyDeepBuffer_Type);

    PyObject* m = Py_InitModule("deepsurface", deepsurface_methods);

    PYMODULE_ADDOBJECT(m, "DeepSurface", PyDeepSurface_Type);
    PYMODULE_ADDOBJECT(m, "DeepBuffer", PyDeepBuffer_Type);
  }

}  // extern "C"
