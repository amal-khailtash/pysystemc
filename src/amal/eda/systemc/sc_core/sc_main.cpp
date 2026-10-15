/**
 * @file sc_main.cpp
 * @brief Invoke Python sc_main with native arguments and fmt diagnostics.
 */
#include <Python.h>

#include <limits>

#include "binding_log.h"

namespace pysystemc
{

/**
 * @brief Optional native model entry, borrowed for a single interpreter run.
 * @details Set before simulation while holding the GIL. Native dispatch does not acquire the GIL;
 * exceptions propagate to the packaged SystemC startup wrapper.
 */
inline int (*native_model_entry)(int, char**) = nullptr;

/**
 * @brief Own entry temporaries and GIL state until the native call leaves scope.
 * @details Construct only in an initialized Python interpreter. Destruction decrefs owned references while the GIL
 * is held, then releases acquired state, including during C++ exception unwinding. The module reference is borrowed.
 */
struct PythonEntryScope
{
  PyGILState_STATE state = PyGILState_Ensure();
  PyObject* callable = nullptr;
  PyObject* args = nullptr;
  PyObject* result = nullptr;

  /** @brief Release owned Python references before releasing the GIL. */
  ~PythonEntryScope()
  {
    Py_XDECREF(result);
    Py_XDECREF(args);
    Py_XDECREF(callable);
    PyGILState_Release(state);
  }
};

}  // namespace pysystemc

/**
 * @brief Run __main__.sc_main(argv) while owning the GIL.
 * @param argc Number of native arguments.
 * @param argv Null-terminated native argument array, borrowed for this call.
 * @return Python's integer status, or -1 after reporting a Python failure.
 * @details All owned references and acquired GIL state are released on every exit path.
 */
extern "C" int sc_main(int argc, char* argv[])
{
  if (pysystemc::native_model_entry) return pysystemc::native_model_entry(argc, argv);
  pysystemc::log(false, fmt::format("Entering sc_main with {} arguments", argc));
  pysystemc::PythonEntryScope scope;
  PyObject* module = PyImport_AddModule("__main__");  // Borrowed reference.
  auto& callable = scope.callable;
  auto& args = scope.args;
  auto& result = scope.result;
  callable = module ? PyObject_GetAttrString(module, "sc_main") : nullptr;
  int status = -1;
  if (argc < 0 || (argc > 0 && !argv))
  {
    PyErr_SetString(PyExc_ValueError, "sc_main requires nonnegative argc and a valid argument array");
  }
  else if (callable && PyCallable_Check(callable))
  {
    args = PyList_New(argc);
    if (args)
    {
      bool valid = true;
      for (int i = 0; i < argc; ++i)
      {
        if (!argv[i])
        {
          PyErr_SetString(PyExc_ValueError, "sc_main argument must not be null");
          valid = false;
          break;
        }
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
    PyObject* error = PyErr_GetRaisedException();  // Owned; also clears the pending error.
    PyErr_DisplayException(error);                 // Unlike PyErr_Print, SystemExit does not terminate the interpreter.
    Py_DECREF(error);
    pysystemc::log(true, "Python sc_main failed");
  }
  pysystemc::log(false, fmt::format("Leaving sc_main with status {}", status));
  return status;
}
