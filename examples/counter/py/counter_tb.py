from amal.eda.systemc import main, sc_core, sc_dt

from counter import Counter


def sc_main(args: list) -> int:
    """Main function for the testbench
    """

    global wf, cnt, clock, reset, enable, counter_out

    clock       = sc_core.sc_signal[bool]("clock")
    reset       = sc_core.sc_signal[bool]("reset")
    enable      = sc_core.sc_signal[bool]("enable")
    counter_out = sc_core.sc_signal[sc_dt.sc_uint[4]]("counter_out")

    # Connect the DUT
    counter = Counter("COUNTER")
    cnt = counter
    counter.clock       (clock      )
    counter.reset       (reset      )
    counter.enable      (enable     )
    counter.counter_out (counter_out)

    sc_core.sc_start(1, sc_core.SC_NS)

    # Open VCD file
    wf = sc_core.sc_create_vcd_trace_file("counter")

    import cppyy
    cppyy.set_debug()

    print(type(clock))
    # <class cppyy.gbl.sc_core.sc_signal<bool,sc_core::SC_ONE_WRITER>>

    # Dump the desired signals
    clock.trace(wf)
    reset.trace(wf)
    enable.trace(wf)
    counter_out.trace(wf)
    # sc_core.sc_trace(wf, clock.read(), clock.name())
    # sc_core.sc_trace[bool](wf, clock.read(), clock.name())
    # sc_core.sc_trace[type(clock)](wf, clock, "clock")
    # sc_core.sc_trace[cppyy.gbl.sc_core.sc_signal_in_if[bool]](wf, clock, "clock")
    # sc_core.sc_trace[cppyy.gbl.sc_core.sc_signal[bool]](wf, clock, "clock")
    # sc_core.sc_trace[type(reset)](wf, reset, "reset")
    # sc_core.sc_trace[type(enable)](wf, enable, "enable")
    # sc_core.sc_trace[type(counter_out)](wf, counter_out, "count")

    # Initialize all variables
    reset.write(False)  # initial value of reset
    enable.write(False)  # initial value of enable
    for i in range(5):
        clock.write(False)
        sc_core.sc_start(1, sc_core.SC_NS)
        clock.write(True)
        sc_core.sc_start(1, sc_core.SC_NS)

    reset.write(True)  # Assert the reset
    print(f"@{sc_core.sc_time_stamp()} Asserting reset")
    for i in range(10):
        clock.write(False)
        sc_core.sc_start(1, sc_core.SC_NS)
        clock.write(True)
        sc_core.sc_start(1, sc_core.SC_NS)
    reset.write(False)  # De-assert the reset

    print(f"@{sc_core.sc_time_stamp()} De-Asserting reset")
    for i in range(5):
        clock.write(False)
        sc_core.sc_start(1, sc_core.SC_NS)
        clock.write(True)
        sc_core.sc_start(1, sc_core.SC_NS)

    print(f"@{sc_core.sc_time_stamp()} Asserting Enable")
    enable.write(True)  # Assert enable
    for i in range(20):
        clock.write(False)
        sc_core.sc_start(1, sc_core.SC_NS)
        clock.write(True)
        sc_core.sc_start(1, sc_core.SC_NS)

    print(f"@{sc_core.sc_time_stamp()} De-Asserting Enable")
    enable.write(False)  # De-assert enable

    print(f"@{sc_core.sc_time_stamp()} Terminating simulation")
    sc_core.sc_close_vcd_trace_file(wf)

    sc_core.sc_stop()

    return 0


if __name__ == "__main__":
    main()
