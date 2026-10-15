/**
 * @file sc_main.cpp
 * @brief Invoke Python sc_main with native arguments and fmt diagnostics.
 */
#include <Python.h>

#include <limits>

#include "binding_log.h"

/**
 * @brief Run __main__.sc_main(argv) while owning the GIL.
 * @param argc Number of native arguments.
 * @param argv Null-terminated native argument array, borrowed for this call.
 * @return Python's integer status, or -1 after reporting a Python failure.
 * @details All owned references and acquired GIL state are released on every exit path.
 */
extern "C" int sc_main(int argc, char* argv[])
{
  pysystemc::log(false, fmt::format("Entering sc_main with {} arguments", argc));
  PyGILState_STATE state = PyGILState_Ensure();
  PyObject* module = PyImport_AddModule("__main__");  // Borrowed reference.
  PyObject* callable = module ? PyObject_GetAttrString(module, "sc_main") : nullptr;
  PyObject* args = nullptr;
  PyObject* result = nullptr;
  int status = -1;
  if (callable && PyCallable_Check(callable))
  {
    args = PyList_New(argc);
    if (args)
    {
      bool valid = true;
      for (int i = 0; i < argc; ++i)
      {
        PyObject* item = PyUnicode_DecodeFSDefault(argv[i]);
        if (!item)
        {
          valid = false;
          break;
        }
        PyList_SET_ITEM(args, i, item);  // Steals the reference.
      }
      if (valid) result = PyObject_CallFunctionObjArgs(callable, args, nullptr);
    }
    if (result)
    {
      long code = PyLong_AsLong(result);
      if (!PyErr_Occurred())
      {
        if (code < std::numeric_limits<int>::min() || code > std::numeric_limits<int>::max())
          PyErr_SetString(PyExc_OverflowError, "sc_main return status must fit a native int");
        else
          status = static_cast<int>(code);
      }
    }
  }
  else if (!PyErr_Occurred())
  {
    PyErr_SetString(PyExc_TypeError, "__main__.sc_main must be callable");
  }
  if (PyErr_Occurred())
  {
    PyErr_Print();
    pysystemc::log(true, "Python sc_main failed");
  }
  Py_XDECREF(result);
  Py_XDECREF(args);
  Py_XDECREF(callable);
  PyGILState_Release(state);
  pysystemc::log(false, fmt::format("Leaving sc_main with status {}", status));
  return status;
}
