"""Verify plain/delayed TLM write copying, time arguments and error recovery."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


class Owner:
    """Retain copied wide samples and borrowed delay observations."""

    def __init__(self):
        """Initialize error injection and callback records."""
        self.fail = False
        self.records = []

    def write(self, value, delay=None):
        """Observe copied native input and optional time without waiting."""
        if self.fail:
            raise ValueError("write failure")
        self.records.append((value, str(delay) if delay is not None else "none"))


name = sc.sc_module_name("top")
top = sc.sc_module(name)
groups = []
for family in (tlm.tlm_write_if, tlm.tlm_delayed_write_if, tlm.tlm_delayed_analysis_if):
    owner = Owner()
    port = sc.sc_port[family[sc_dt.sc_biguint[137]]]()
    port(owner)
    view = sc.sc_port[
        (tlm.tlm_analysis_if if family is tlm.tlm_write_if else tlm.tlm_delayed_analysis_if)[sc_dt.sc_biguint[137]]
    ]()
    view(owner)
    assert cppyy.addressof(port.get_interface()) == cppyy.addressof(view.get_interface())
    groups.append((owner, port, view))
del name
for owner, port, view in groups:
    sample = sc_dt.sc_biguint[137](17)
    sample[136] = True
    arguments = (
        (sample,)
        if isinstance(port.get_interface(), tlm.tlm_write_if[sc_dt.sc_biguint[137]])
        else (sample, sc.sc_time(2, sc.SC_NS))
    )
    owner.fail = True
    try:
        port.get_interface().write(*arguments)
    except ValueError as error:
        assert "write failure" in str(error)
    else:
        raise AssertionError("write callback exception lost")
    owner.fail = False
    port.get_interface().write(*arguments)
    sample[136] = False
    assert owner.records[0][0][136], "write input retained by reference"
    print(
        "TLM_WRITE_REFERENCE",
        "plain" if len(arguments) == 1 else "delayed",
        owner.records[0][0].to_uint64(),
        int(bool(owner.records[0][0][136])),
        owner.records[0][1],
    )
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
print("TLM_WRITE_CONTRACTS_OK")
