"""Check indexed hierarchy traversal, child ownership and filtering behavior."""

from contextlib import redirect_stdout
from io import StringIO
from amal.eda.systemc import sc_core as sc, print_hierarchy, print_module_hierarchy


class Child(sc.sc_module):
    """Own a named signal within native hierarchy scope."""

    def __init__(self, name):
        """Keep elaboration scope alive until child objects are created."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.signal = sc.sc_signal[bool]("signal")


class Parent(sc.sc_module):
    """Own an ordinary child and a filtered internal module."""

    def __init__(self, name):
        """Create child modules under the parent's live module-name scope."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.child = Child("child")


model = Parent("parent")
assert model.child.name() == "parent.child"
assert model.child.signal.name() == "parent.child.signal"
output = StringIO()
with redirect_stdout(output):
    print_hierarchy(all_modules=True)
text = output.getvalue()
assert text.count("parent.child ") == 1, text
assert "parent " in text
output = StringIO()
with redirect_stdout(output):
    print_module_hierarchy(model.child.signal)
assert output.getvalue() == ""
sc.sc_start(sc.SC_ZERO_TIME)
assert sc.sc_status() == sc.ScStatus.SC_PAUSED
sc.sc_stop()
print("HIERARCHY_CHECKS_OK")
