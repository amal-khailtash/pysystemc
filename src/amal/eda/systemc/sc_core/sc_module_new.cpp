#include <iomanip>
#include <iostream>
#include <string>
#include <mutex>

#include <Python.h>
#include <systemc>

namespace sc_core
{
    class ScModule
        : public sc_module
    {
     public:

        //SC_HAS_PROCESS(ScModule);
        // ScModule
        // ( PyObject*             self
        // // , const std::string&    name
        // , const sc_module_name& nm
        // ) : self      ( self )
        // // , sc_module ( sc_module_name(name.c_str()) )
        // , sc_module ( nm   )
        ScModule(const sc_module_name& nm)
        : sc_module ( nm   )
        {
            std::cout << std::string(80, '-')
            //          << "\nScModule constructor - " << name << std::endl;
                      << "\nScModule constructor - " << nm << std::endl;
            // Py_INCREF(self);
            // processes = PyDict_New();
        }

        virtual ~ScModule() override
        {
            // Py_DECREF(self);
            // Py_DECREF(processess);
        }

        // Virtual functions to be overridden in Python
        virtual void before_end_of_elaboration () override {}
        virtual void end_of_elaboration        () override {}
        virtual void start_of_simulation       () override {}
        virtual void end_of_simulation         () override {}

        // Function to add a Python method as a SystemC sc_method
        void sc_method(const char* name, void(*method)())
        {
            std::cout << "ScModule::sc_method - " << name << std::endl;
            sc_process_handle handle = register_process(name, method, SC_METHOD_PROC_);
        }

        // Function to add a Python method as a SystemC sc_thread
        void sc_thread(const char* name, void(*method)())
        {
            std::cout << "ScModule::sc_thread - " << name << std::endl;
            sc_process_handle handle = register_process(name, method, SC_THREAD_PROC_);
        }

        // Function to add a Python method as a SystemC sc_cthread
        void sc_cthread(const char* name, void(*method)(), sc_event_finder& event_finder)
        {
            std::cout << "ScModule::sc_cthread - " << name << std::endl;
            sc_process_handle handle = register_process(name, method, SC_CTHREAD_PROC_);
            sc_module::sensitive.operator()(handle, event_finder);
        }

        void async_reset_signal_is( const sc_in<bool>&           port,  bool level ) { sc_module::async_reset_signal_is(port,  level); }
        void async_reset_signal_is( const sc_inout<bool>&        port,  bool level ) { sc_module::async_reset_signal_is(port,  level); }
        void async_reset_signal_is( const sc_out<bool>&          port,  bool level ) { sc_module::async_reset_signal_is(port,  level); }
        void async_reset_signal_is( const sc_signal_in_if<bool>& iface, bool level ) { sc_module::async_reset_signal_is(iface, level); }
        void reset_signal_is      ( const sc_in<bool>&           port,  bool level ) { sc_module::reset_signal_is      (port,  level); }
        void reset_signal_is      ( const sc_inout<bool>&        port,  bool level ) { sc_module::reset_signal_is      (port,  level); }
        void reset_signal_is      ( const sc_out<bool>&          port,  bool level ) { sc_module::reset_signal_is      (port,  level); }
        void reset_signal_is      ( const sc_signal_in_if<bool>& iface, bool level ) { sc_module::reset_signal_is      (iface, level); }

        void dont_initialize      (                                                ) { sc_module::dont_initialize();                   }

        // Function to add sensitivity to neg edge
        void sensitive            (const sc_interface&           interface_        ) { sc_module::sensitive << interface_;             }
        void sensitive            (const sc_port_base&           port_             ) { sc_module::sensitive << port_;                  }
        void sensitive            (      sc_event_finder&        event_finder_     ) { sc_module::sensitive << event_finder_;          }

        // sc_sensitive& operator << ( const sc_interface& interface_) { return sc_module::sensitive << interface_; }
        // sc_sensitive& operator << ( const sc_port_base& port_     ) { return sc_module::sensitive << port_;      }

        /* Deprecated
        // Function to add sensitivity to neg edge
        void sensitive_neg(const sc_sensitive_neg::in_if_b_type&      iface     ) { sc_module::sensitive_neg << iface; }
        void sensitive_neg(const sc_sensitive_neg::in_if_l_type&      iface     ) { sc_module::sensitive_neg << iface; }
        void sensitive_neg(const sc_sensitive_neg::in_port_b_type&    port      ) { sc_module::sensitive_neg << port;  }
        void sensitive_neg(const sc_sensitive_neg::in_port_l_type&    port      ) { sc_module::sensitive_neg << port;  }
        void sensitive_neg(const sc_sensitive_neg::inout_port_b_type& port      ) { sc_module::sensitive_neg << port;  }
        void sensitive_neg(const sc_sensitive_neg::inout_port_l_type& port      ) { sc_module::sensitive_neg << port;  }

        // Function to add sensitivity to pos edge
        void sensitive_pos(const sc_sensitive_pos::in_if_b_type&      iface     ) { sc_module::sensitive_pos << iface; }
        void sensitive_pos(const sc_sensitive_pos::in_if_l_type&      iface     ) { sc_module::sensitive_pos << iface; }
        void sensitive_pos(const sc_sensitive_pos::in_port_b_type&    port      ) { sc_module::sensitive_pos << port;  }
        void sensitive_pos(const sc_sensitive_pos::in_port_l_type&    port      ) { sc_module::sensitive_pos << port;  }
        void sensitive_pos(const sc_sensitive_pos::inout_port_b_type& port      ) { sc_module::sensitive_pos << port;  }
        void sensitive_pos(const sc_sensitive_pos::inout_port_l_type& port      ) { sc_module::sensitive_pos << port;  }
        */

        void does_it_have_the_GIL(const char* name)
        {
            if ( PyGILState_Check() ) { printf("[%s] - GIL is held\n", name); }
            else                      { printf("[%s] - GIL is NOT held\n", name); }
        }

        // Function to call the stored function pointer
        void call_process()
        {
            printf("--------------------------------------------------------------------------------\n");
            // does_it_have_the_GIL("call_process");

            const char* method_name = sc_get_current_process_b()->name();
            std::cout << "[" << std::setw(7) << sc_time_stamp() << "]"
                      << " - Current process: " << method_name
                      << " - Kind : " << sc_get_current_process_b()->proc_kind()
                      << " - State: " << sc_get_current_process_b()->current_state()
                    //   << sc_get_current_process_b()->lineno << " - " << sc_get_current_process_b()->file
                      << " " << sc_get_current_process_b()->dump_state()
                      << std::endl;

            // does_it_have_the_GIL("call_process - A");

            // Py_BEGIN_ALLOW_THREADS
            // does_it_have_the_GIL("call_process - B");

            // // Acquire GIL and call all registered Python methods
            // PyGILState_STATE gstate = PyGILState_Ensure();
            // std::cout << "    GIL acquired" << std::endl;

            // Your C code that doesn't need the GIL
            // std::cout << "    Stored functions count: " << this->processes.size() << std::endl;

            // // Release the GIL
            // PyGILState_Release(gstate);
            // std::cout << "    GIL released" << std::endl;

            // Py_END_ALLOW_THREADS



            // auto it = this->processes.find(method_name);
            // if (it != this->processes.end())
            // {
            //     std::cout << "    Calling Method : " << method_name << std::endl;
            //     it->second();
            // }
            // else
            // {
            //     std::cerr << "Error: No function stored for method '" << method_name << "'" << std::endl;
            // }



            // method_name = "dff1";
            // PyObject* py_method_name = PyUnicode_FromString(method_name);
            // PyObject* py_func = PyDict_GetItem(processes, py_method_name);
            // Py_DECREF(py_method_name);

            // if (py_func && PyCallable_Check(py_func))
            // {
            //     PyObject* result = PyObject_CallObject(py_func, nullptr);
            //     if (result)
            //     {
            //         Py_DECREF(result);
            //     }
            //     else
            //     {
            //         PyErr_Print();
            //     }
            // }
            // else
            // {
            //     std::cerr << "Error: No function stored for method '" << method_name << "'" << std::endl;
            // }
        }

        // void call_lambda_function()
        // {
        //     std::cout << "    Current process: " << sc_get_current_process_b()->name()
        //               << " - Kind : " << sc_get_current_process_b()->proc_kind()
        //               << " - State: " << sc_get_current_process_b()->current_state()
        //             //   << sc_get_current_process_b()->lineno << " - " << sc_get_current_process_b()->file
        //               << " " << sc_get_current_process_b()->dump_state()
        //               << std::endl;

        //     // Fetch the method name from the SystemC process context
        //     const char* method_name = sc_get_current_process_b()->name();
        //     std::cout << "    Calling Method : " << method_name << std::endl;

        //     // std::function<void()> func;

        //     // // Lock the mutex and access the map
        //     // {
        //     //     std::cout << "    Accessing lambda map" << std::endl;
        //     //     std::lock_guard<std::mutex> lock(lambda_map_mutex);
        //     //     std::cout << "    Mutex lock guard" << std::endl;

        //     //     auto it = lambda_map.find(method_name);
        //     //     if (it != lambda_map.end())
        //     //     {
        //     //         std::cout << "    Lambda function found" << std::endl;
        //     //         func = it->second;
        //     //     }
        //     // }

        //     // std::cout << "    Calling lambda function" << std::endl;
        //     // if ( func )
        //     // {
        //     //     func();
        //     // }
        //     // else
        //     // {
        //     //     std::cerr << "Error: Lambda function for method '" << method_name << "' not found" << std::endl;
        //     // }

        //     PyObject* py_method = PyObject_GetAttrString(self, "dff1");
        //     PyObject* result = PyObject_CallObject(py_method, nullptr);
        //     if (result)
        //     {
        //         Py_DECREF(result);
        //     }
        //     else
        //     {
        //         PyErr_Print();
        //     }
        // }

     private:

        // Helper to register a Python method as a SystemC process
        sc_process_handle register_process(const char* method_name, void(*method)(), sc_curr_proc_kind proc_kind)
        {
            std::cout << "  " << method_name << " - Registering process" <<  std::endl;

            // Store the function pointer in the member variable
            processes[method_name] = method;

            // PyObject* py_method_name = PyUnicode_FromString(method_name);
            // PyObject* py_func = PyCapsule_New((void*)method, nullptr, nullptr);
            // PyDict_SetItem(processes, py_method_name, py_func);
            // Py_DECREF(py_method_name);
            // Py_DECREF(py_func);

            /*
            if ( !PyObject_HasAttrString(self, method_name) )
            {
                std::cerr << "Error: Method '" << method_name << "' not found in Python module." << std::endl;
                return sc_process_handle();
            }
            std::cout << "  " << method_name << " - Method found" << std::endl;

            PyObject* py_method = PyObject_GetAttrString(self, method_name);
            if ( !py_method || !PyCallable_Check(py_method) )
            {
                std::cerr << "Error: '" << method_name << "' is not callable." << std::endl;
                Py_XDECREF(py_method);
                return sc_process_handle();
            }
            std::cout << "  " << method_name << " - Method callable" << std::endl;

            // Create a lambda function to call the Python method
            auto func = [py_method]() {
                // Acquire GIL and call all registered Python methods
                PyGILState_STATE gstate = PyGILState_Ensure();
                std::cout << "    GIL acquired" << std::endl;

                // Call the Python method
                if ( PyCallable_Check(py_method) )
                {
                    PyObject* result = PyObject_CallObject(py_method, nullptr);
                    if (result)
                    {
                        Py_DECREF(result);
                    }
                    else
                    {
                        PyErr_Print();
                    }
                }
                else
                {
                    PyErr_SetString(PyExc_RuntimeError, "Method is not callable");
                    PyErr_Print();
                }

                // Release the GIL
                PyGILState_Release(gstate);
                std::cout << "    GIL released" << std::endl;
            };

            // Store the lambda function in the static map
            {
                std::lock_guard<std::mutex> lock(lambda_map_mutex);
                lambda_map[method_name] = func;
                std::cout << "  " << method_name << " - Lambda function stored" << std::endl;
                std::cout << "  " << method_name << " - Lambda function count: " << lambda_map.size() << std::endl;
            }
            */

            // Create the process based on the type (method, thread, cthread)
            sc_process_handle process_handle;

            auto simcontext = sc_get_curr_simcontext();

            std::cout << "  " << method_name << " - Creating process" << std::endl;

            switch ( proc_kind )
            {
            case SC_METHOD_PROC_ : process_handle = simcontext->create_method_process (method_name, false, static_cast<SC_ENTRY_FUNC>(&ScModule::call_process), nullptr, nullptr);  break;
            case SC_THREAD_PROC_ : process_handle = simcontext->create_thread_process (method_name, false, static_cast<SC_ENTRY_FUNC>(&ScModule::call_process), nullptr, nullptr);  break;
            case SC_CTHREAD_PROC_: process_handle = simcontext->create_cthread_process(method_name, false, static_cast<SC_ENTRY_FUNC>(&ScModule::call_process), nullptr, nullptr);  break;
            default:                                                                                                                                                                break;
            }

            std::cout << "  " << method_name << " - Process created" << std::endl;

            // Set sensitivities
            sc_module::sensitive     << process_handle;
            sc_module::sensitive_pos << process_handle;
            sc_module::sensitive_neg << process_handle;

            // Decrement ref count for the method object
            // Py_DECREF(py_method);
            std::cout << "  " << method_name << " - Added Python process" << std::endl;

            // // Temporary call to the method
            // method();

            return process_handle;
        }

        std::map<std::string, std::function<void()>> processes;

        // PyObject* processes;

        // PyObject*                                    self {nullptr};  // Store the Python module reference

        // std::map<std::string, std::function<void()>> lambda_map;
        // std::mutex                                   lambda_map_mutex;
    };

}  // namespace sc_core
