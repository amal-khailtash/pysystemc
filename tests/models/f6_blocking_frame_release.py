"""Check local Python wrapper release after interrupted TLM FIFO calls."""

import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc, sc_dt, tlm


VALUE = sc_dt.sc_biguint[137]


def wide(low):
    """Construct a wide sample with a nonzero top bit."""
    sample = VALUE(low)
    sample[136] = True
    return sample


def pair(value):
    """Return the low word and top bit without narrowing the full value."""
    return int(value.to_uint64()), bool(value[136])


def check_blocking_frame(case):
    """Interrupt one direct FIFO call twice and check local-frame release."""
    fifo = tlm.tlm_fifo[VALUE](f"f6_{case}_fifo", 1)
    if case == "put-full":
        seed = wide(11)
        assert fifo.nb_put(seed)
        del seed

    references = {}
    cleanup = []
    starts = []
    controller_checks = []

    def worker():
        """Create all native arguments locally, then block in one FIFO call."""
        round_number = len(starts) + 1
        output = wide(71)
        tag = tlm.tlm_tag[VALUE]()
        input_value = wide(99)
        references[round_number] = {
            "output": weakref.ref(output),
            "tag": weakref.ref(tag),
            "input": weakref.ref(input_value),
        }
        starts.append(round_number)
        try:
            if case == "get-output":
                fifo.get(output)
            elif case == "peek-value":
                fifo.peek(tag)
            elif case == "put-full":
                fifo.put(input_value)
            else:
                raise AssertionError(f"unknown blocked FIFO case: {case}")
            raise AssertionError(f"blocked {case} call returned without progress")
        finally:
            assert pair(output) == (71, True), (case, round_number, "output", pair(output))
            assert pair(input_value) == (99, True), (case, round_number, "input", pair(input_value))
            cleanup.append((round_number, str(sc.sc_time_stamp())))

    options = sc.sc_spawn_options()
    options.set_stack_size(2 * 1024 * 1024)
    handle = sc.sc_spawn(worker, f"f6_{case}_worker", options)

    def controller():
        """Reset at two ns, then kill the restarted call at three ns."""
        sc.wait(2, sc.SC_NS)
        assert starts == [1], (case, starts)
        handle.reset()
        while len(starts) < 2:
            sc.wait(sc.SC_ZERO_TIME)
        gc.collect()
        assert cleanup.count((1, "2 ns")) == 1, (case, cleanup)
        assert all(reference() is None for reference in references[1].values()), (case, references[1])
        controller_checks.append((1, str(sc.sc_time_stamp())))

        sc.wait(1, sc.SC_NS)
        assert str(sc.sc_time_stamp()) == "3 ns"
        handle.kill()
        sc.wait(sc.SC_ZERO_TIME)
        gc.collect()
        assert cleanup.count((2, "3 ns")) == 1, (case, cleanup)
        assert all(reference() is None for reference in references[2].values()), (case, references[2])
        controller_checks.append((2, str(sc.sc_time_stamp())))
        assert handle.terminated(), case
        if case == "put-full":
            retained = wide(71)
            assert fifo.nb_peek(retained)
            assert pair(retained) == (11, True)
        else:
            assert fifo.used() == 0, (case, fifo.used())
        sc.sc_stop()

    controller_handle = sc.sc_spawn(controller, f"f6_{case}_controller", options)
    sc.sc_start(4, sc.SC_NS)
    assert starts == [1, 2], (case, starts)
    assert cleanup == [(1, "2 ns"), (2, "3 ns")], (case, cleanup)
    assert controller_checks == [(1, "2 ns"), (2, "3 ns")], (case, controller_checks)
    assert handle.terminated() and controller_handle.terminated()
    print(f"F6_08_BLOCKING_FRAME_RELEASE_OK case={case} reset=2ns kill=3ns refs=6 cleanup=2")


if __name__ == "__main__":
    """Run one blocking-call interruption case in a fresh kernel process."""
    selected = sys.argv[1]
    if selected not in ("get-output", "peek-value", "put-full"):
        raise SystemExit(f"unknown blocked FIFO case: {selected}")
    check_blocking_frame(selected)
