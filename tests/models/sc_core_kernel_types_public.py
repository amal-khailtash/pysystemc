"""Verify borrowed native reset views and typed report-handler token transport."""

from amal.eda.systemc import sc_core as sc
import cppyy
from pathlib import Path

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_kernel_types_probe.h"))
source = sc.sc_signal[bool]("reset_view_source")
view = cppyy.gbl.small_types_probe.ResetView(source)
borrowed = view.borrowed()
assert isinstance(borrowed, sc.sc_reset)
assert cppyy.addressof(borrowed) == cppyy.addressof(view)
assert cppyy.addressof(view.borrowed()) == cppyy.addressof(borrowed)
handler = sc.sc_report_handler.get_handler()
previous = sc.sc_report_handler.set_handler(handler)
assert cppyy.gbl.small_types_probe.same_handler_token(handler, previous)
assert cppyy.gbl.small_types_probe.handler_roundtrip()
print("SMALL_TYPES_OK", 1, 1)
