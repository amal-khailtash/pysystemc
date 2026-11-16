"""Drive defined upstream RISC ALU operations through native and Python processes."""

import importlib.util
import json
from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc
import cppyy

ROOT = Path(__file__).resolve().parents[2]
CASES = [
    (0, 0, 0),
    (1, 123, 456),
    (2, -12, 7),
    (3, -123, 123),
    (4, 9, 12),
    (5, 32767, 65535),
    (6, -9, 2),
    (6, 9, 0),
    (7, 12, 10),
    (8, 12, 10),
    (9, 12, 10),
    (10, 12, 10),
    (11, 12, 0),
    (12, 1, 30),
    (13, 1024, 4),
    (14, -9, 2),
    (31, 1, 2),
]


class Stimulus(sc.sc_module):
    """Own ports/signals and observe every result at its native valid transition."""

    def __init__(self, native):
        """Bind either reference ALU and retain all objects until terminal stop.

        Args:
            native: Select the original C++ module rather than the Python port.
        """
        module_name = sc.sc_module_name("stimulus")
        super().__init__(module_name)
        if native:
            cppyy.add_include_path(str(ROOT / "submodules/systemc/examples/sysc/risc_cpu"))
            cppyy.include("exec.cpp")
            self.alu = getattr(cppyy.gbl, "exec")("alu")
        else:
            spec = importlib.util.spec_from_file_location("risc_alu_example", ROOT / "examples/sysc/risc_cpu/exec.py")
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            self.alu = module.Execute("alu")
        self.clock = sc.sc_clock("clock", 2, sc.SC_NS)
        self.alu.CLK.bind(self.clock)
        self.signals = {}
        for name in (
            "opcode",
            "add1",
            "dina",
            "dinb",
            "dest",
            "reset",
            "in_valid",
            "negate",
            "shift_sel",
            "forward_A",
            "forward_B",
            "C",
            "V",
            "Z",
            "out_valid",
            "dout",
            "destout",
        ):
            datatype = (
                "unsigned"
                if name in ("dest", "destout")
                else int
                if name in ("opcode", "add1", "dina", "dinb", "dout")
                else bool
            )
            signal = sc.sc_signal[datatype](name)
            getattr(self.alu, name).bind(signal)
            self.signals[name] = signal
        self.records = []
        self.sc_thread("drive", self.drive)

    def drive(self):
        """Synchronize each operation with valid edges and stop after the matrix."""
        sc.wait(8, sc.SC_NS)
        for index, (opcode, a, b) in enumerate(CASES):
            for key, value in (("opcode", opcode), ("dina", a), ("dinb", b), ("dest", index), ("in_valid", True)):
                self.signals[key].write(value)
            sc.wait(self.signals["out_valid"].posedge_event())
            self.signals["in_valid"].write(False)
            self.records.append(
                {
                    "opcode": opcode,
                    "result": int(self.signals["dout"].read()),
                    "dest": int(self.signals["destout"].read()),
                    "Z": bool(self.signals["Z"].read()),
                    "C": bool(self.signals["C"].read()),
                    "V": bool(self.signals["V"].read()),
                    "ns": round(sc.sc_time_stamp().to_seconds() * 1e9),
                }
            )
            sc.wait(self.signals["out_valid"].negedge_event())
            sc.wait(self.clock.posedge_event())
        sc.sc_stop()


model = Stimulus(sys.argv[1] == "native")
sc.sc_start(300, sc.SC_NS)
assert len(model.records) == len(CASES)
assert sc.sc_end_of_simulation_invoked()
print("ALU_EDGES=" + json.dumps(model.records, sort_keys=True))
