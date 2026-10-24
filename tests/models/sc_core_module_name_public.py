"""Compare sc_module_name construction, conversion and scope lifetime with native SystemC."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(
    """
    #include <string>
    namespace module_name_probe {
    /** @brief Convert a borrowed module name through its public native conversion operator. */
    std::string text(const sc_core::sc_module_name& name)
    {
      return static_cast<const char*>(name);
    }
    }
    """
)

source = cppyy.gbl.std.string("root")
root_name = sc.sc_module_name(source.c_str())
root = sc.sc_module(root_name)
root_text = cppyy.gbl.module_name_probe.text(root_name)
root_path = root.name()
assert root_path == root_text == "root"

copied_name = sc.sc_module_name(root_name)
copied_text = cppyy.gbl.module_name_probe.text(copied_name)
assert copied_text == root_text
copied_name.__destruct__()

child_name = sc.sc_module_name("child")
child = sc.sc_module(child_name)
grandchild_name = sc.sc_module_name("grandchild")
grandchild = sc.sc_module(grandchild_name)
grandchild_marker = sc.sc_event("marker")
assert child.name() == "root.child"
assert grandchild.name() == "root.child.grandchild"
assert grandchild_marker.name() == "root.child.grandchild.marker"
child_path = child.name()
grandchild_path = grandchild.name()
marker_path = grandchild_marker.name()

grandchild_name.__destruct__()
after_grandchild = sc.sc_event("after_grandchild")
assert after_grandchild.name() == "root.child.after_grandchild"
after_grandchild_path = after_grandchild.name()
child_name.__destruct__()
after_child = sc.sc_event("after_child")
assert after_child.name() == "root.after_child"
after_child_path = after_child.name()
root_name.__destruct__()
outside = sc.sc_event("outside")
assert outside.name() == "outside"
outside_path = outside.name()

print(
    "MODULE_NAME_REFERENCE",
    root_path,
    root_text,
    copied_text,
    child_path,
    grandchild_path,
    marker_path,
    after_grandchild_path,
    after_child_path,
    outside_path,
)
