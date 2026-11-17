"""Check native hierarchy formatting, filtering, validation and capture."""

from contextlib import redirect_stdout
from io import StringIO

from amal.eda.systemc import cppyy, print_hierarchy, print_module_hierarchy, sc_core as sc


cppyy.cppdef(
    """
namespace cynw {
class NestedModule : public sc_core::sc_module {
public:
  explicit NestedModule(sc_core::sc_module_name name) : sc_core::sc_module(name) {}
};
class FilteredModule : public sc_core::sc_module {
public:
  NestedModule nested;
  explicit FilteredModule(sc_core::sc_module_name name)
      : sc_core::sc_module(name), nested("nested") {}
};
}
namespace pysystemc_hierarchy_test {
class KindOverrideModule : public sc_core::sc_module {
public:
  explicit KindOverrideModule(sc_core::sc_module_name name) : sc_core::sc_module(name) {}
  const char* kind() const override { return "fixture_module"; }
};
}
"""
)


class Child(sc.sc_module):
    """Own a channel under a native module to check type-based filtering."""

    def __init__(self, name):
        """Create the module and one child signal under native hierarchy scope."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.signal = sc.sc_signal[bool]("signal")


class SetterShadow(sc.sc_module):
    """Ensure hierarchy labels bypass any Python method with the native setter name."""

    def __init__(self, name):
        """Construct a module while its Python setter name is shadowed."""
        super().__init__(sc.sc_module_name(name))

    def set_hierarchy_type_name(self, name):
        """Fail if the constructor hook dispatches through a Python override."""
        raise AssertionError(f"Python setter override unexpectedly called with {name!r}")


class Parent(sc.sc_module):
    """Own Python and native C++ modules alongside native channels."""

    def __init__(self, name):
        """Build the nested hierarchy while the parent's naming scope is active."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.fifo = sc.sc_fifo[int]("fifo", 2)
        self.child = Child("child")
        self.unicode_child = Child("café")
        self.setter_shadow = SetterShadow("setter_shadow")
        self.filtered = cppyy.gbl.cynw.FilteredModule(sc.sc_module_name("internal"))
        self.kind_override = cppyy.gbl.pysystemc_hierarchy_test.KindOverrideModule(sc.sc_module_name("custom_kind"))


empty_output = StringIO()
with redirect_stdout(empty_output):
    print_hierarchy()
assert empty_output.getvalue() == f"\nDesign Hierarchy:\n{'=' * 80}\n\n"

model = Parent("parent")
assert model.child.name() == "parent.child"
assert model.child.signal.name() == "parent.child.signal"

default_output = StringIO()
with redirect_stdout(default_output):
    print_hierarchy()
text = default_output.getvalue()
assert text.startswith(f"\nDesign Hierarchy:\n{'=' * 80}\n")
assert text.endswith("\n\n")
for module_path in ("parent", "parent.child", "parent.café", "parent.setter_shadow", "parent.custom_kind"):
    assert text.count(f"({module_path})") == 1, text
for type_label, module_path in (
    ("Parent", "parent"),
    ("Child", "parent.child"),
    ("Child", "parent.café"),
    ("SetterShadow", "parent.setter_shadow"),
    ("pysystemc_hierarchy_test::KindOverrideModule", "parent.custom_kind"),
):
    assert f"{type_label} ({module_path})" in text
assert "KindOverrideModule" in text
assert "__cppyy_internal" not in text
assert "(anonymous namespace)" not in text
assert model.kind_override.kind() == "fixture_module"
assert "cynw::FilteredModule" not in text
assert "cynw::NestedModule" not in text
assert "(parent.internal)" not in text
assert "(parent.internal.nested)" not in text
assert "(parent.fifo)" not in text
assert "(parent.child.signal)" not in text

all_output = StringIO()
with redirect_stdout(all_output):
    print_hierarchy(all_modules=True)
all_text = all_output.getvalue()
assert all_text.startswith(f"\nDesign Hierarchy:\n{'=' * 80}\n")
assert all_text.endswith("\n\n")
for module_path in (
    "parent",
    "parent.child",
    "parent.café",
    "parent.setter_shadow",
    "parent.custom_kind",
    "parent.internal",
    "parent.internal.nested",
):
    assert all_text.count(f"({module_path})") == 1, all_text
assert "cynw::FilteredModule" in all_text
assert "cynw::NestedModule" in all_text
assert "__cppyy_internal" not in all_text
assert "(anonymous namespace)" not in all_text
assert "(parent.fifo)" not in all_text
assert "(parent.child.signal)" not in all_text

subtree_output = StringIO()
with redirect_stdout(subtree_output):
    print_module_hierarchy(model.child, level=2)
assert subtree_output.getvalue().startswith("    ")
assert subtree_output.getvalue().count("(parent.child)") == 1

nonmodule_output = StringIO()
with redirect_stdout(nonmodule_output):
    print_module_hierarchy(model.child.signal)
    print_module_hierarchy(model.fifo)
    print_module_hierarchy(model.filtered)
assert nonmodule_output.getvalue().count("(parent.internal)") == 0
assert "(parent.child.signal)" not in nonmodule_output.getvalue()
assert "(parent.fifo)" not in nonmodule_output.getvalue()

kind_override_output = StringIO()
with redirect_stdout(kind_override_output):
    print_module_hierarchy(model.kind_override)
assert "(parent.custom_kind)" in kind_override_output.getvalue()

for invalid_object in (None, object()):
    try:
        print_module_hierarchy(invalid_object)
    except TypeError:
        pass
    else:
        raise AssertionError("non-SystemC root accepted")

for invalid_level in (True, 1.5, "2"):
    try:
        print_module_hierarchy(model.child, level=invalid_level)
    except TypeError:
        pass
    else:
        raise AssertionError(f"invalid hierarchy level accepted: {invalid_level!r}")

try:
    print_module_hierarchy(model.child, level=-1)
except ValueError:
    pass
else:
    raise AssertionError("negative hierarchy level accepted")

try:
    print_module_hierarchy(model.child, all_modules=1)
except TypeError:
    pass
else:
    raise AssertionError("non-boolean all_modules accepted by subtree printer")

try:
    print_hierarchy(all_modules=1)
except TypeError:
    pass
else:
    raise AssertionError("non-boolean all_modules accepted by hierarchy printer")

sc.sc_start(sc.SC_ZERO_TIME)
assert sc.sc_status() == sc.ScStatus.SC_PAUSED
sc.sc_stop()
print("HIERARCHY_CHECKS_OK")
