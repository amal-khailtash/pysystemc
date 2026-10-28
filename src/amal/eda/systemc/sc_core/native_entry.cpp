/**
 * @file native_entry.cpp
 * @brief SystemC 2.3.4 entry adapter retained from the upstream kernel.
 * @details Owns copied arguments and restores entry state on every exit,
 * including allocation and report-handler failures. Native exception/status
 * handling is preserved. Pending Python callback errors are fetched under the
 * GIL before another report handler can replace their original diagnostics.
 * The Python caller releases the GIL; sc_main reacquires it for Python callbacks.
 * Original Accellera license and modification notices are retained below.
 */

/*****************************************************************************

  Licensed to Accellera Systems Initiative Inc. (Accellera) under one or
  more contributor license agreements.  See the NOTICE file distributed
  with this work for additional information regarding copyright ownership.
  Accellera licenses this file to you under the Apache License, Version 2.0
  (the "License"); you may not use this file except in compliance with the
  License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  implied.  See the License for the specific language governing
  permissions and limitations under the License.

 *****************************************************************************/

/*****************************************************************************

  sc_main_main.cpp - Wrapper around user's toplevel routine `sc_main'.

  Original Author: Andy Goodrich, Forte Design Systems

 CHANGE LOG APPEARS AT THE END OF THE FILE
 *****************************************************************************/

#define SC_DISABLE_API_VERSION_CHECK  // for in-library sc_ver.h inclusion

#include <Python.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <sstream>
#include <vector>

#include "sysc/kernel/sc_cmnhdr.h"
#include "sysc/kernel/sc_except.h"
#include "sysc/kernel/sc_externs.h"
#include "sysc/kernel/sc_ver.h"
#include "sysc/utils/sc_report.h"
#include "sysc/utils/sc_report_handler.h"
#include "sysc/utils/sc_utils_ids.h"

namespace sc_core
{

extern void pln();

static int argc_orig;     // Original argc value passed to sc_elab_and_sim.
static char** argv_orig;  // Original argv value passed to sc_elab_and_sim.

bool sc_in_action = false;

/** @brief Transport a captured Python callback exception across native entry.
 * @details Owns the fetched exception reference, including during native throw
 * and cppyy exception conversion. Copies share ownership; the last owner
 * acquires the GIL before releasing the Python exception and traceback.
 * Python entry wrappers restore the original exception after releasing this
 * carrier so a hidden C++ ownership cycle cannot retain its traceback.
 */
class python_entry_callback_error : public std::exception
{
  std::shared_ptr<PyObject> error_;

 public:
  /** @brief Adopt one owned exception reference fetched with the GIL held. */
  explicit python_entry_callback_error(PyObject* error)
    : error_(error,
             [](PyObject* value)
             {
               const auto state = PyGILState_Ensure();
               Py_DECREF(value);
               PyGILState_Release(state);
             })
  {
  }

  /** @brief Return a borrowed address valid only while a carrier remains owned. */
  std::uintptr_t address() const
  {
    return reinterpret_cast<std::uintptr_t>(error_.get());
  }

  /** @brief Describe the transport without entering Python or requiring GIL. */
  const char* what() const noexcept override
  {
    return "Python simulation entry callback failed";
  }
};

namespace
{
/** @brief Own mutable argument copies and restore the borrowed entry snapshot.
 * @details Construction performs all allocations before changing entry globals.
 * Members release partially allocated buffers if construction fails. The caller
 * retains the original argv until entry returns; sc_main may replace its copied
 * pointer slots without changing buffer ownership. Destruction restores the
 * previous activity/argument state on normal return and exception unwinding.
 * Use only on the simulation entry thread; no Python references or callbacks
 * are owned here and the caller may release the GIL.
 */
class EntryArguments
{
  int previous_argc_ = argc_orig;
  char** previous_argv_ = argv_orig;
  bool previous_active_ = sc_in_action;
  std::vector<std::unique_ptr<char[]>> buffers_;
  std::vector<char*> arguments_;

 public:
  /** @brief Copy borrowed strings into independently owned null-terminated buffers. */
  EntryArguments(int argc, char* argv[]) : buffers_(argc), arguments_(argc + 1, nullptr)
  {
    for (int index = 0; index < argc; ++index)
    {
      std::size_t size = std::strlen(argv[index]) + 1;
      buffers_[index] = std::make_unique<char[]>(size);
      std::copy(argv[index], argv[index] + size, buffers_[index].get());
      arguments_[index] = buffers_[index].get();
    }
  }

  /** @brief Activate a borrowed original snapshot only after initialization succeeds. */
  void activate(int argc, char* argv[])
  {
    argc_orig = argc;
    argv_orig = argv;
    sc_in_action = true;
  }

  /** @brief Return mutable pointer slots; replacing them does not release owned copies. */
  char** arguments()
  {
    return arguments_.data();
  }

  /** @brief Restore entry state before automatically releasing all copied storage. */
  ~EntryArguments()
  {
    sc_in_action = previous_active_;
    argc_orig = previous_argc_;
    argv_orig = previous_argv_;
  }
};

/** @brief Fetch a pending callback error before returning a native integer.
 * @details cppyy may leave an error pending after a void Python callback. Clear
 * it under the GIL and throw an owning carrier after entry storage/state cleanup;
 * otherwise integer result conversion would replace it with SystemError.
 */
void throw_pending_entry_python_error()
{
  const auto state = PyGILState_Ensure();
  PyObject* error = PyErr_GetRaisedException();
  PyGILState_Release(state);
  if (error) throw python_entry_callback_error(error);
}
}  // namespace

int sc_argc()
{
  return argc_orig;
}

const char* const* sc_argv()
{
  return argv_orig;
}

int sc_elab_and_sim(int argc, char* argv[])
{
  try
  {
    int status = 1;
    {
      EntryArguments entry(argc, argv);
      try
      {
        pln();
        entry.activate(argc, argv);
        status = sc_main(argc, entry.arguments());
      }
      catch (const sc_report& report)
      {
        throw_pending_entry_python_error();
        sc_report_handler::get_handler()(report, sc_report_handler::get_catch_actions());
      }
      catch (...)
      {
        // Translation itself emits a report and can reenter the Python handler.
        throw_pending_entry_python_error();
        std::unique_ptr<sc_report> report(sc_handle_exception());
        if (report)
        {
          sc_report_handler::get_handler()(*report, sc_report_handler::get_catch_actions());
        }
      }
    }

    throw_pending_entry_python_error();

    // IF DEPRECATION WARNINGS WERE ISSUED TELL THE USER HOW TO TURN THEM OFF

    if (sc_report_handler::get_count(SC_ID_IEEE_1666_DEPRECATION_) > 0)
    {
      std::stringstream ss;

      const char MSGNL[] = "\n             ";
      const char CODENL[] = "\n  ";

      ss << "You can turn off warnings about" << MSGNL << "IEEE 1666 deprecated features by placing this method call"
         << MSGNL << "as the first statement in your sc_main() function:\n"
         << CODENL << "sc_core::sc_report_handler::set_actions( "
         << "\"" << SC_ID_IEEE_1666_DEPRECATION_ << "\"," << CODENL
         << "                                         " /* indent param */
         << "sc_core::SC_DO_NOTHING );" << std::endl;

      SC_REPORT_INFO(SC_ID_IEEE_1666_DEPRECATION_, ss.str().c_str());
    }

    throw_pending_entry_python_error();
    return status;
  }
  catch (...)
  {
    // Capture pending callback errors from entry, report translation/dispatch,
    // and the deprecation summary after all owned entry buffers/state unwind.
    // Native callers may consume the C++ exception without entering cppyy.
    throw_pending_entry_python_error();
    throw;
  }
}

}  // namespace sc_core

// $Log: sc_main_main.cpp,v $
// Revision 1.9  2011/08/26 20:46:10  acg
//  Andy Goodrich: moved the modification log to the end of the file to
//  eliminate source line number skew when check-ins are done.
//
// Revision 1.8  2011/05/09 04:07:48  acg
//  Philipp A. Hartmann:
//    (1) Restore hierarchy in all phase callbacks.
//    (2) Ensure calls to before_end_of_elaboration.
//
// Revision 1.7  2011/02/18 20:27:14  acg
//  Andy Goodrich: Updated Copyrights.
//
// Revision 1.6  2011/02/13 21:47:37  acg
//  Andy Goodrich: update copyright notice.
//
// Revision 1.5  2010/03/15 18:29:25  acg
//  Andy Goodrich: Changed the default stack size to 128K from 64K.
//
// Revision 1.4  2009/10/14 19:06:48  acg
//  Andy Goodrich: changed the way the "copy" of argv is handled. It is
//  now passed to sc_main, and the original is referenced via argv_copy.
//
// Revision 1.3  2008/05/22 17:06:25  acg
//  Andy Goodrich: updated copyright notice to include 2008.
//
// Revision 1.2  2008/04/11 20:41:28  acg
//  Andy Goodrich: changed the return value in sc_elab_and_sim() to be 1
//  when an exception occurs in sc_main() rather than 0.
//
// Revision 1.1.1.1  2006/12/15 20:20:05  acg
// SystemC 2.3
//
// Revision 1.4  2006/01/25 00:31:19  acg
//  Andy Goodrich: Changed over to use a standard message id of
//  SC_ID_IEEE_1666_DEPRECATION for all deprecation messages.
//
// Revision 1.3  2006/01/13 18:44:29  acg
// Added $Log to record CVS changes into the source.
//
