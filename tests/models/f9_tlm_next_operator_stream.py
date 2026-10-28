"""Check direct TLM namespace access to the phase stream insertion function."""

from amal.eda.systemc import tlm
import cppyy

cppyy.cppdef(
    r"""
    #include <sstream>
    #include <string>
    #include <tlm>

    namespace f9_tlm_next_probe
    {
    /** Return a phase's spelling using the native stream insertion operator. */
    inline std::string phase_stream(const tlm::tlm_phase& phase)
    {
      std::ostringstream stream;
      stream << phase;
      return stream.str();
    }
    }  // namespace f9_tlm_next_probe
    """
)

phase = tlm.tlm_phase(tlm.BEGIN_REQ)
native_text = str(cppyy.gbl.f9_tlm_next_probe.phase_stream(phase))
stream = cppyy.gbl.std.ostringstream()
formatter = getattr(tlm, "operator<<")
result = formatter(stream, phase)

assert cppyy.addressof(result) == cppyy.addressof(stream)
assert str(stream.str()) == native_text == "BEGIN_REQ"
print("TLM_NEXT_OPERATOR_STREAM|BEGIN_REQ|1")
