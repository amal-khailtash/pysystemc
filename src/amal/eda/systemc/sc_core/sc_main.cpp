#include <iostream>

#include <Python.h>

extern "C" int sc_main(int argc, char* argv[])
{
    std::cout << "\n"
              << "[C++    ] - sc_main(argc = " << argc << ", argv[0] = [" << argv[0] << ", ...])"
              << std::endl;

    // Acquire GIL and call all registered Python methods
    PyGILState_STATE gstate = PyGILState_Ensure();
    std::cout << "[C++    ] - GIL acquired" << std::endl;

    // std::cout << "[C++    ] - Importing __main__ module ..." << std::endl;
    PyObject* module_init = PyImport_AddModule("__main__");
    // if ( !module_init )
    // {
    //     PyErr_Print();
    //     return -1;
    // }

    // Get the Python `sc_main` function from globals
    std::cout << "[C++    ] - Getting Python sc_main function ..." << std::endl;
    // PyObject* py_sc_main = PyDict_GetItemString(globals, "sc_main");
    PyObject* py_sc_main = PyObject_GetAttrString(module_init, "sc_main");

    // Retrieve the registered Python sc_main function
    // PyObject *py_sc_main = PyObject_GetAttrString(systemc_module, "_python_sc_main_func");
    // Py_DECREF(systemc_module);

    if ( !py_sc_main || !PyCallable_Check(py_sc_main) )
    {
        std::cerr << "Error: Python sc_main function not found or is not callable!" << std::endl;
        Py_XDECREF(py_sc_main);
        return -1;
    }

    // Convert C++ argv to a Python list of strings
    std::cout << "[C++    ] - Converting C++ argv to Python list ..." << std::endl;
    PyObject* py_args = PyList_New(argc);
    for ( int i = 0; i < argc; ++i )
    {
        // PyList_SetItem steals reference to py_arg
        // PyObject* py_arg = PyUnicode_FromString(argv[i]);
        // PyList_SetItem(py_args, i, py_arg);
        PyList_SetItem(py_args, i, PyUnicode_FromString(argv[i]));
    }

    // Call the Python sc_main function with args
    std::cout << "[C++    ] - Calling Python sc_main function ...\n"
              << std::string(80, '-')
              << std::endl;

    PyObject* py_result = PyObject_CallFunctionObjArgs(py_sc_main, py_args, NULL);
    // Decrease reference to args list after call
    Py_DECREF(py_args);
    // Py_DECREF(py_sc_main);

    // Check for errors in Python function call
    if ( !py_result )
    {
        PyErr_Print();
        std::cerr << "Error: Python sc_main execution failed." << std::endl;
        return -1;
    }

    // Cleanup result object
    int result = PyLong_AsLong(py_result);
    Py_DECREF(py_result);

    std::cout << std::string(80, '-')
              << "\n[C++    ] - Python sc_main function executed successfully." << std::endl;

    // Release the GIL
    PyGILState_Release(gstate);
    std::cout << "[C++    ] - GIL released" << std::endl;

    return result;
}
