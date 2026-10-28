/**
 * @file f6_process_ownership.h
 * @brief Native probes for Python entry ownership audits.
 * @details These probes run only in isolated test subprocesses. The partial
 * argument probe invokes the public Python sc_main bridge with a valid first
 * item and an invalid second item. The exception probes retain the native
 * exception carrier through shared exception_ptr copies, then release its last
 * owner either with the caller's GIL held or on a native worker thread without
 * the caller's GIL. Python callbacks reenter the interpreter through cppyy.
 */
#pragma once

#include <Python.h>

#include <cstring>
#include <exception>
#include <stdexcept>
#include <systemc>
#include <thread>
#include <utility>
#include <vector>

/** @brief Fail native entry so a Python report callback raises its own error.
 * @return Never returns; the exception is translated by SystemC entry.
 */
inline int f6_throw_from_native_entry(int, char**)
{
  throw std::runtime_error("f6 native entry probe");
}

/** @brief Emit an ordinary warning through the installed Python report hook. */
inline int f6_report_from_native_entry(int, char**)
{
  SC_REPORT_WARNING("F6_DIRECT_REPORT", "direct native report callback probe");
  return 0;
}

/** @brief Throw a real sc_report so native entry dispatches it to Python. */
inline int f6_native_sc_report_exception(int, char**)
{
  const auto python_handler = sc_core::sc_report_handler::get_handler();
  sc_core::sc_report_handler::set_actions("F6_CAUGHT_REPORT", sc_core::SC_ERROR, sc_core::SC_THROW);
  sc_core::sc_report_handler::set_handler(sc_core::sc_report_handler::default_handler);
  try
  {
    SC_REPORT_ERROR("F6_CAUGHT_REPORT", "caught native report callback probe");
  }
  catch (const sc_core::sc_report&)
  {
    sc_core::sc_report_handler::set_handler(python_handler);
    throw;
  }
  return 1;
}

/** @brief Increment deprecation state for the native post-entry summary hook. */
inline int f6_deprecation_summary_entry(int, char**)
{
  SC_REPORT_INFO(sc_core::SC_ID_IEEE_1666_DEPRECATION_, "deprecation summary callback probe");
  return 0;
}

/** @brief Let native entry capture and release a Python callback exception.
 * @param argc Number of test arguments.
 * @param argv Native argument vector.
 * @param release_on_worker Release the final carrier owner on a new OS thread.
 * @return One if entry threw the Python exception carrier, zero otherwise.
 * @details Copies of the carrier share one owned Python exception reference.
 * The worker case enters the interpreter through the carrier's GIL-aware
 * deleter. The non-worker case destroys it on this caller's current GIL state.
 */
inline int f6_consume_entry_exception_carrier(int argc, char** argv, bool release_on_worker)
{
  std::exception_ptr first;
  std::exception_ptr duplicate;
  try
  {
    sc_core::sc_elab_and_sim(argc, argv);
  }
  catch (const sc_core::python_entry_callback_error& error)
  {
    const auto address = error.address();
    first = std::current_exception();
    sc_core::python_entry_callback_error copy(error);
    if (copy.address() != address) return -3;
    duplicate = std::make_exception_ptr(copy);
    try
    {
      std::rethrow_exception(duplicate);
    }
    catch (const sc_core::python_entry_callback_error& copied_error)
    {
      if (copied_error.address() != address) return -4;
    }
    catch (...)
    {
      return -5;
    }
  }
  catch (...)
  {
    return -2;
  }
  if (!first || !duplicate) return 0;

  first = nullptr;
  if (release_on_worker)
  {
    std::thread worker([owner = std::move(duplicate)]() mutable { owner = nullptr; });
    worker.join();
  }
  else
  {
    duplicate = nullptr;
  }
  return 1;
}

/** @brief Store observer-only Python references around the real sc_main bridge. */
namespace f6_sc_main_probe
{
inline std::vector<PyObject*> references;
inline int fail_list_allocation = 0;
inline int fail_decode_index = 0;
inline int throw_decode_index = 0;
inline int decode_calls = 0;

/** @brief Keep an observer reference so post-return ownership is measurable. */
inline void observe(PyObject* value)
{
  if (value)
  {
    Py_INCREF(value);
    references.push_back(value);
  }
}
}  // namespace f6_sc_main_probe

extern "C" PyObject* f6_probe_list_new(Py_ssize_t size);
extern "C" PyObject* f6_probe_decode(const char* value);
extern "C" PyObject* f6_probe_getattr(PyObject* object, const char* name);
extern "C" PyObject* f6_probe_call(PyObject* callable, PyObject* argument, PyObject* terminator);

// Compile the production bridge itself with test-local C-API call monitors.
#define sc_main f6_sc_main_bridge
#define PythonEntryScope f6_sc_main_PythonEntryScope
#define native_model_entry f6_sc_main_native_model_entry
#define PyList_New f6_probe_list_new
#define PyUnicode_DecodeFSDefault f6_probe_decode
#define PyObject_GetAttrString f6_probe_getattr
#define PyObject_CallFunctionObjArgs f6_probe_call
#include "../../src/amal/eda/systemc/sc_core/sc_main.cpp"
#undef PyObject_CallFunctionObjArgs
#undef PyObject_GetAttrString
#undef PyUnicode_DecodeFSDefault
#undef PyList_New
#undef native_model_entry
#undef PythonEntryScope
#undef sc_main

/** @brief Observe list creation, decoding, callable and result ownership. */
extern "C" PyObject* f6_probe_list_new(Py_ssize_t size)
{
  if (f6_sc_main_probe::fail_list_allocation)
  {
    PyErr_NoMemory();
    return nullptr;
  }
  PyObject* value = PyList_New(size);
  f6_sc_main_probe::observe(value);
  return value;
}

/** @brief Observe a decoded argv item or inject failure on a selected call. */
extern "C" PyObject* f6_probe_decode(const char* value)
{
  ++f6_sc_main_probe::decode_calls;
  if (f6_sc_main_probe::decode_calls == f6_sc_main_probe::throw_decode_index)
    throw std::runtime_error("f6 second decode unwind");
  if (f6_sc_main_probe::decode_calls == f6_sc_main_probe::fail_decode_index)
  {
    PyErr_NoMemory();
    return nullptr;
  }
  PyObject* decoded = PyUnicode_DecodeFSDefault(value);
  f6_sc_main_probe::observe(decoded);
  return decoded;
}

/** @brief Observe the owned reference returned by Python attribute lookup. */
extern "C" PyObject* f6_probe_getattr(PyObject* object, const char* name)
{
  PyObject* value = PyObject_GetAttrString(object, name);
  f6_sc_main_probe::observe(value);
  return value;
}

/** @brief Forward the bridge's one-list call and observe its owned result. */
extern "C" PyObject* f6_probe_call(PyObject* callable, PyObject* value, PyObject* terminator)
{
  if (terminator != nullptr)
  {
    PyErr_SetString(PyExc_TypeError, "F6 sc_main probe accepts exactly one argument");
    return nullptr;
  }
  PyObject* result = PyObject_CallFunctionObjArgs(callable, value, nullptr);
  f6_sc_main_probe::observe(result);
  return result;
}

/** @brief Run the production bridge with optional deterministic C-API failure.
 * @param argc Number of native arguments.
 * @param argv Borrowed native string vector.
 * @param failure 0 succeeds, 1 fails list allocation, 2 fails second decode,
 *        3 passes a null second native argument, or 4 throws on second decode.
 * @return The production bridge status.
 */
inline int f6_run_python_sc_main_bridge(int argc, char** argv, int failure)
{
  (void)argc;
  (void)argv;
  f6_sc_main_probe::fail_list_allocation = failure == 1;
  f6_sc_main_probe::fail_decode_index = failure == 2 ? 2 : 0;
  f6_sc_main_probe::throw_decode_index = failure == 4 ? 2 : 0;
  f6_sc_main_probe::decode_calls = 0;
  f6_sc_main_probe::references.clear();
  if (failure == 3)
  {
    char first[] = "f6-valid-prefix";
    char* partial_arguments[] = {first, nullptr};
    return f6_sc_main_bridge(2, partial_arguments);
  }
  char first[] = "f6-prefix";
  char second[] = "f6-second";
  char* arguments[] = {first, second};
  if (failure == 4)
  {
    try
    {
      return f6_sc_main_bridge(2, arguments);
    }
    catch (const std::runtime_error& error)
    {
      if (std::strcmp(error.what(), "f6 second decode unwind") != 0) throw;
      return -2;
    }
  }
  return f6_sc_main_bridge(2, arguments);
}

/** @brief Return the count of outstanding observer references. */
inline std::size_t f6_sc_main_observer_count()
{
  return f6_sc_main_probe::references.size();
}

/** @brief Return an observed object's reference count, including its monitor. */
inline long f6_sc_main_observer_refcount(std::size_t index)
{
  return static_cast<long>(Py_REFCNT(f6_sc_main_probe::references.at(index)));
}

/** @brief Drop one observer, allowing its owned references to release children. */
inline void f6_release_sc_main_observer(std::size_t index)
{
  PyObject*& value = f6_sc_main_probe::references.at(index);
  if (value)
  {
    Py_DECREF(value);
    value = nullptr;
  }
}

/** @brief Release observer references after their post-bridge counts pass. */
inline void f6_release_sc_main_observers()
{
  for (PyObject* value : f6_sc_main_probe::references)
    if (value) Py_DECREF(value);
  f6_sc_main_probe::references.clear();
}
