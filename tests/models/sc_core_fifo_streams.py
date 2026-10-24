"""Exercise FIFO insertion through native string, file and derived streams."""

from pathlib import Path
from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy

cppyy.cppdef(r"""
#include <sstream>
#include <cstdint>
namespace fifo_stream_test {
/** Load a derived stream after stream pythonizations have been installed. */
struct Derived : std::ostringstream {};
/** Native sample with no precompiled Python datatype specialization. */
struct Sample {
  int value;
  Sample(int value = 0) : value(value) {}
};
/** Print a borrowed sample without callbacks, mutation or GIL release. */
std::ostream& operator<<(std::ostream& stream, const Sample& sample) {
  return stream << "sample(" << sample.value << ")";
}
/** Return the adjusted borrowed ostream base address. */
std::uintptr_t address(std::ostream& stream) {return reinterpret_cast<std::uintptr_t>(&stream);}
/** Trigger stream failures to check insertion error propagation. */
void fail(std::ostream& stream) {
  stream.exceptions(std::ios::badbit);
  stream.setstate(std::ios::badbit);
}
}
""")

fifo = sc.sc_fifo[int]("fifo", 2)
assert fifo.nb_write(17) and fifo.nb_write(23)
wide = sc.sc_fifo[sc_dt.sc_biguint[137]]("wide", 1)
assert wide.nb_write(sc_dt.sc_biguint[137]("87112285931760246646623899502532662132737"))
sample = sc.sc_fifo[cppyy.gbl.fifo_stream_test.Sample]("sample", 1)
assert sample.nb_write(cppyy.gbl.fifo_stream_test.Sample(9))
sc.sc_start(sc.SC_ZERO_TIME)
for label, stream in (
    ("string", cppyy.gbl.std.ostringstream()),
    ("combined", cppyy.gbl.std.stringstream()),
    ("derived", cppyy.gbl.fifo_stream_test.Derived()),
):
    result = stream << "begin:" << fifo << "end:" << 41
    assert cppyy.addressof(result) == cppyy.gbl.fifo_stream_test.address(stream)
    assert str(stream.str()) == "begin:17\n23\nend:41"
    assert fifo.num_available() == 2
    print("FIFO_STREAM", label, str(stream.str()).replace("\n", "|"), sep="\t")
file = cppyy.gbl.std.ofstream("fifo.txt")
file << fifo
file.close()
assert Path("fifo.txt").read_text() == "17\n23\n"
print("FIFO_STREAM", "file", Path("fifo.txt").read_text().replace("\n", "|"), sep="\t")
stream = cppyy.gbl.std.ostringstream()
stream << wide << sample
assert str(stream.str()) == "87112285931760246646623899502532662132737\nsample(9)\n"
print("FIFO_STREAM", "generic", str(stream.str()).replace("\n", "|"), sep="\t")
failed = cppyy.gbl.std.ostringstream()
try:
    cppyy.gbl.fifo_stream_test.fail(failed)
except Exception:
    pass
else:
    raise AssertionError("badbit did not throw")
try:
    failed << fifo
except OSError as error:
    assert "iostream error" in str(error), error
else:
    raise AssertionError("failed stream accepted insertion")
assert fifo.num_available() == 2
try:
    failed << "text"
except OSError as error:
    assert "iostream error" in str(error), error
else:
    raise AssertionError("failed stream accepted text")
print("FIFO_STREAM_ERROR", "iostream error", sep="\t")
sc.sc_stop()
