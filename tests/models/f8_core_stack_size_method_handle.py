"""Check the native stack-size bridge rejects a live method-process handle."""

from amal.eda.systemc import sc_core as sc
import cppyy


class Model(sc.sc_module):
    """Register a method process for the native bridge precondition probe."""

    def __init__(self):
        """Capture a valid method handle and require native rejection."""
        super().__init__(sc.sc_module_name("f8_method_stack"))
        self.sc_method("method", self.method)
        method_handle = sc.sc_get_last_created_process_handle()
        try:
            cppyy.gbl.pysystemc_public.set_stack_size_handle(method_handle, 4096)
        except cppyy.gbl.std.invalid_argument as error:
            assert "thread" in str(error)
        else:
            raise AssertionError("native stack-size bridge accepted a method handle")

    def method(self):
        """Provide the registered no-op method body."""


Model()
print("F8_CORE_METHOD_STACK_REJECTED")
