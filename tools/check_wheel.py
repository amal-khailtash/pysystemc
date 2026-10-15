"""Check installed native resources and compile a user model through cppyy.

Run outside the checkout in an environment containing the built wheel. Fail if
resource discovery, runtime compilation, process execution or timing is wrong.
"""

from pathlib import Path
from amal.eda.systemc import __version__, sc_core
import amal.eda.systemc as systemc
import cppyy

native = Path(systemc.__file__).parent / "_native"
assert native.is_dir(), native
for header in ("fmt/format.h", "spdlog/spdlog.h", "systemc"):
    assert (native / "include" / header).is_file(), header
cppyy.cppdef("""


struct WheelModel : sc_core::sc_module
{
  SC_HAS_PROCESS(WheelModel);
  int calls = 0;

  explicit WheelModel(sc_core::sc_module_name name) : sc_module(name) { SC_THREAD(run); }

  void run()
  {
    ++calls;
    wait(2, sc_core::SC_NS);
    ++calls;
  }
};
""")
model = cppyy.gbl.WheelModel("wheel_model")
sc_core.sc_start(3, sc_core.SC_NS)
assert model.calls == 2, model.calls
assert sc_core.sc_time_stamp() == sc_core.sc_time(3, sc_core.SC_NS)
assert __version__ == "0.2.3", __version__
print("Installed wheel: native resources, runtime C++ compilation and pthread wait passed")
