"""Reproduce raw analysis FIFO instantiation followed by facade preparation."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


cppyy.cppdef("""
namespace tlm_analysis_preparation_edges
{
/** @brief Copyable payload used to trigger late analysis FIFO preparation.
 * @details The FIFO owns value copies; each payload object owns its integer
 * field, with no Python callback or borrowed storage involved.
 */
struct Value
{
  int data;
  /** @brief Construct a payload with an optional initial value. */
  Value(int initial = 0) : data(initial) {}
};
}
""")

late_type = cppyy.gbl.tlm_analysis_preparation_edges.Value
raw_fifo_type = cppyy.gbl.tlm.tlm_analysis_fifo[late_type]

# The raw proxy's constructor compiles its virtual write methods. SystemC's
# unqualified dependent-base calls fail in Cling, but leave the specialization
# far enough instantiated that the later facade declarations are rejected.
try:
    raw_fifo_type("raw_analysis_fifo")
except TypeError as error:
    assert "constructor failed" in str(error)
else:
    raise AssertionError("raw FIFO construction unexpectedly bypassed native write instantiation")

try:
    tlm.tlm_analysis_fifo[late_type]
except RuntimeError as error:
    assert str(error) == (
        "Cannot prepare native tlm_analysis_fifo<tlm_analysis_preparation_edges::Value>; "
        "instantiate through amal.eda.systemc.tlm before using the raw cppyy template."
    )
    assert isinstance(error.__cause__, SyntaxError)
    assert "out-of-line declaration of 'write' does not match" in str(error.__cause__)
    print("TLM_ANALYSIS_LATE_FAILURE", type(error.__cause__).__name__, True)
else:
    raise AssertionError("late facade preparation unexpectedly succeeded")

try:
    tlm.tlm_analysis_fifo[late_type]
except RuntimeError as error:
    assert str(error).startswith("Cannot prepare native tlm_analysis_fifo<")
    assert isinstance(error.__cause__, SyntaxError)
else:
    raise AssertionError("failed late preparation was cached as a usable type")

# A different arbitrary payload remains preparable and usable after the raw
# specialization failure. Its width exceeds cppyy's built-in integer types.
recovery_type = sc_dt.sc_biguint[257]
recovery_fifo_type = tlm.tlm_analysis_fifo[recovery_type]
assert recovery_fifo_type is tlm.tlm_analysis_fifo[recovery_type]
recovery_triple_type = tlm.tlm_analysis_triple[recovery_type]
fifo = recovery_fifo_type("recovery_analysis_fifo")

first = recovery_type(57)
first[256] = True
second = recovery_type(88)
second[256] = True
fifo.write(first)
fifo.write(recovery_triple_type(second))

sc.sc_start(sc.sc_time(6, sc.SC_NS))
assert sc.sc_time_stamp() == sc.sc_time(6, sc.SC_NS)
first_read = recovery_type()
second_read = recovery_type()
assert fifo.nb_get(first_read)
assert fifo.nb_get(second_read)
assert first_read[256].to_bool() and first_read.to_uint64() == 57
assert second_read[256].to_bool() and second_read.to_uint64() == 88
print("TLM_ANALYSIS_RECOVERY", sc.sc_time_stamp().to_string(), 1, first_read.to_uint64(), 1, second_read.to_uint64())
print("TLM_ANALYSIS_PREPARATION_EDGES_OK")
