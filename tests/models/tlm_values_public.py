"""Check TLM DMI, phase, global quantum and quantumkeeper public values."""

from pathlib import Path
import gc

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils

import cppyy

cppyy.include(str(Path(__file__).parents[1] / "native" / "tlm_values_probe.h"))


def time(value):
    """Construct a nanosecond value for compact, readable assertions."""
    return sc.sc_time(value, sc.SC_NS)


def phase_name(phase):
    """Convert the native phase name returned as a borrowed C string."""
    return str(phase.get_name())


def check_dmi():
    """Exercise every public DMI initialization, pointer, range and access call."""
    access_type = tlm.tlm_dmi.dmi_access_e
    assert int(access_type(tlm.tlm_dmi.DMI_ACCESS_NONE)) == 0
    assert int(access_type(tlm.tlm_dmi.DMI_ACCESS_READ)) == 1
    assert int(access_type(tlm.tlm_dmi.DMI_ACCESS_WRITE)) == 2
    assert int(access_type(tlm.tlm_dmi.DMI_ACCESS_READ_WRITE)) == 3

    dmi = tlm.tlm_dmi()
    assert not dmi.get_dmi_ptr()
    assert int(dmi.get_start_address()) == 0
    assert int(dmi.get_end_address()) == 2**64 - 1
    assert dmi.get_read_latency() == sc.SC_ZERO_TIME
    assert dmi.get_write_latency() == sc.SC_ZERO_TIME
    assert int(dmi.get_granted_access()) == int(tlm.tlm_dmi.DMI_ACCESS_NONE)
    assert dmi.is_none_allowed()
    assert not dmi.is_read_allowed() and not dmi.is_write_allowed()
    assert not dmi.is_read_write_allowed()

    dmi.allow_read()
    assert int(dmi.get_granted_access()) == int(tlm.tlm_dmi.DMI_ACCESS_READ)
    assert dmi.is_read_allowed() and not dmi.is_write_allowed()
    assert not dmi.is_none_allowed() and not dmi.is_read_write_allowed()
    dmi.allow_write()
    assert int(dmi.get_granted_access()) == int(tlm.tlm_dmi.DMI_ACCESS_WRITE)
    assert not dmi.is_read_allowed() and dmi.is_write_allowed()
    assert not dmi.is_read_write_allowed()
    dmi.allow_read_write()
    assert int(dmi.get_granted_access()) == int(tlm.tlm_dmi.DMI_ACCESS_READ_WRITE)
    assert dmi.is_read_allowed() and dmi.is_write_allowed() and dmi.is_read_write_allowed()
    dmi.allow_none()
    assert dmi.is_none_allowed() and not dmi.is_read_allowed() and not dmi.is_write_allowed()

    memory = cppyy.gbl.std.vector["unsigned char"](8, 0)
    memory[3] = 29
    dmi.set_dmi_ptr(memory.data())
    pointer = dmi.get_dmi_ptr()
    pointer.reshape((8,))
    assert pointer[3] == 29
    pointer[4] = 47
    assert ord(memory[4]) == 47

    start = 2**63 + 29
    end = 2**64 - 1
    dmi.set_start_address(start)
    dmi.set_end_address(end)
    dmi.set_read_latency(time(3))
    dmi.set_write_latency(time(7))
    dmi.set_granted_access(tlm.tlm_dmi.DMI_ACCESS_READ_WRITE)
    assert int(dmi.get_start_address()) == start
    assert int(dmi.get_end_address()) == end
    assert dmi.get_read_latency() == time(3)
    assert dmi.get_write_latency() == time(7)
    assert int(dmi.get_granted_access()) == int(tlm.tlm_dmi.DMI_ACCESS_READ_WRITE)
    assert dmi.is_read_write_allowed()

    dmi.init()
    assert not dmi.get_dmi_ptr()
    assert int(dmi.get_start_address()) == 0 and int(dmi.get_end_address()) == end
    assert dmi.get_read_latency() == sc.SC_ZERO_TIME
    assert dmi.get_write_latency() == sc.SC_ZERO_TIME
    assert dmi.is_none_allowed()
    print(f"TLM_VALUES_REFERENCE DMI {start} {end} 3 ns 7 ns 29 47")


def check_phases():
    """Check standard enum conversion, assignment and registered phase streams."""
    expected = (
        (tlm.UNINITIALIZED_PHASE, "UNINITIALIZED_PHASE"),
        (tlm.BEGIN_REQ, "BEGIN_REQ"),
        (tlm.END_REQ, "END_REQ"),
        (tlm.BEGIN_RESP, "BEGIN_RESP"),
        (tlm.END_RESP, "END_RESP"),
    )
    for enum_value, name in expected:
        phase = tlm.tlm_phase(enum_value)
        assert int(phase) == int(enum_value)
        assert phase_name(phase) == name
        assert cppyy.gbl.tlm_values_probe.stream_phase(phase) == name

    phase = tlm.tlm_phase()
    assert int(phase) == int(tlm.UNINITIALIZED_PHASE)
    phase.__assign__(tlm.BEGIN_RESP)
    assert int(phase) == int(tlm.BEGIN_RESP) and phase_name(phase) == "BEGIN_RESP"
    phase = tlm.tlm_phase(19)
    assert int(phase) == 19

    custom = cppyy.gbl.tlm_values_probe.custom_phase()
    custom_again = cppyy.gbl.tlm_values_probe.custom_phase()
    assert int(custom) == int(custom_again) == 5
    assert phase_name(custom) == "PYTHON_AUDIT_PHASE"
    assert cppyy.gbl.tlm_values_probe.stream_phase(custom) == "PYTHON_AUDIT_PHASE"
    print("TLM_VALUES_REFERENCE PHASE 0,1,2,3,4 custom=5:PYTHON_AUDIT_PHASE")


def check_quantum_processes():
    """Assert quantum values and lifecycle order in live Python SystemC threads."""
    global_quantum = tlm.tlm_global_quantum.instance()
    global_quantum.set(time(10))
    assert tlm.tlm_global_quantum.instance().get() == time(10)
    assert tlm.tlm_global_quantum.instance().compute_local_quantum() == time(10)
    tlm.tlm_global_quantum.instance().set(time(8))
    assert global_quantum.get() == time(8)
    global_quantum.set(time(10))
    assert tlm_utils.tlm_quantumkeeper.get_global_quantum() == time(10)
    protected_keeper = cppyy.gbl.tlm_values_probe.QuantumkeeperProbe()
    assert protected_keeper.compute_local_quantum_probe() == time(10)
    del protected_keeper
    gc.collect()
    assert cppyy.gbl.tlm_values_probe.QuantumkeeperProbe.destructions() == 1
    tlm_utils.tlm_quantumkeeper.set_global_quantum(time(12))
    assert global_quantum.get() == time(12)
    tlm_utils.tlm_quantumkeeper.set_global_quantum(time(10))
    independent = cppyy.gbl.tlm_values_probe.GlobalQuantumProbe()
    assert independent.get() == sc.SC_ZERO_TIME
    independent.set(time(6))
    assert independent.get() == time(6) and independent.compute_local_quantum() == time(6)
    del independent

    main_keeper = tlm_utils.tlm_quantumkeeper()
    reset_keeper, kill_keeper = tlm_utils.tlm_quantumkeeper(), tlm_utils.tlm_quantumkeeper()
    keepers = (reset_keeper, kill_keeper)
    records = []

    def worker(index):
        """Block in the selected sync method so controller reset/kill unwinds it."""
        keeper = keepers[index]
        records.append(("RESET" if index == 0 else "KILL", "start", str(sc.sc_time_stamp())))
        keeper.reset()
        try:
            if index == 0:
                keeper.set(time(100))
                keeper.sync()
            else:
                keeper.set_and_sync(time(100))
            raise AssertionError("interrupted quantum sync returned")
        finally:
            records.append(("RESET" if index == 0 else "KILL", "cleanup", str(sc.sc_time_stamp())))

    def main_thread():
        """Check local increments and wait boundaries against the global quantum."""
        main_keeper.reset()
        assert main_keeper.get_local_time() == sc.SC_ZERO_TIME
        assert main_keeper.get_current_time() == sc.SC_ZERO_TIME
        records.append(("MAIN", "start", str(sc.sc_time_stamp())))
        main_keeper.inc(time(4))
        assert not main_keeper.need_sync()
        assert main_keeper.get_local_time() == time(4)
        assert main_keeper.get_current_time() == time(4)
        records.append(("MAIN", "inc4", str(main_keeper.get_current_time())))
        main_keeper.inc(time(6))
        assert main_keeper.need_sync()
        assert main_keeper.get_local_time() == time(10)
        assert main_keeper.get_current_time() == time(10)
        records.append(("MAIN", "inc10", str(main_keeper.get_current_time())))
        main_keeper.sync()
        assert sc.sc_time_stamp() == time(10)
        assert main_keeper.get_local_time() == sc.SC_ZERO_TIME
        assert main_keeper.get_current_time() == time(10)
        records.append(("MAIN", "sync", str(sc.sc_time_stamp())))
        main_keeper.set_and_sync(time(10))
        assert sc.sc_time_stamp() == time(20)
        assert main_keeper.get_local_time() == sc.SC_ZERO_TIME
        assert main_keeper.get_current_time() == time(20)
        records.append(("MAIN", "set_sync", str(sc.sc_time_stamp())))

    handles = [sc.sc_spawn(main_thread, "quantum_main")]
    handles.extend(sc.sc_spawn(lambda index=index: worker(index), f"quantum_worker_{index}") for index in range(2))

    def controller():
        """Reset suspended Python workers at 2 ns and kill them at 3 ns."""
        sc.wait(2, sc.SC_NS)
        handles[1].reset()
        handles[2].reset()
        sc.wait(1, sc.SC_NS)
        handles[1].kill()
        handles[2].kill()

    control = sc.sc_spawn(controller, "quantum_controller")
    sc.sc_start(time(21))
    expected_records = [
        ("MAIN", "start", "0 s"),
        ("MAIN", "inc4", "4 ns"),
        ("MAIN", "inc10", "10 ns"),
        ("RESET", "start", "0 s"),
        ("KILL", "start", "0 s"),
        ("RESET", "cleanup", "2 ns"),
        ("RESET", "start", "2 ns"),
        ("KILL", "cleanup", "2 ns"),
        ("KILL", "start", "2 ns"),
        ("RESET", "cleanup", "3 ns"),
        ("KILL", "cleanup", "3 ns"),
        ("MAIN", "sync", "10 ns"),
        ("MAIN", "set_sync", "20 ns"),
    ]
    assert records == expected_records, records
    assert all(handle.terminated() for handle in handles + [control])
    for owner, action, timestamp in records:
        print(f"TLM_VALUES_REFERENCE {owner} {action} {timestamp}")
    sc.sc_stop()


check_dmi()
check_phases()
check_quantum_processes()
print("TLM_VALUES_PUBLIC_OK")
