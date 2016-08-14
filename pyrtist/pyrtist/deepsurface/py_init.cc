#include "Python.h"

#include "py_deep_surface.h"

static PyMethodDef deepsurface_methods[] = {
  {nullptr, nullptr, 0, nullptr}
};

extern "C" {

  PyMODINIT_FUNC initdeepsurface() {
    if (PyType_Ready(&PyDeepSurface_Type) < 0)
      return;

    PyObject* m = Py_InitModule("deepsurface", deepsurface_methods);

    Py_INCREF(&PyDeepSurface_Type);
    PyModule_AddObject(m, "DeepSurface",
                       reinterpret_cast<PyObject*>(&PyDeepSurface_Type));
  }

}  // extern "C"
