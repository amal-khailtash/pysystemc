from amal.eda.systemc import main, sc_core, sc_dt

from counter import Counter


def sc_main(args: list) -> int:
    """Main function for the testbench
    """

    clock       = sc_core.sc_signal[bool]("clock")
    reset       = sc_core.sc_signal[bool]("reset")
    enable      = sc_core.sc_signal[bool]("enable")
    counter_out = sc_core.sc_signal[sc_dt.sc_uint[4]]("counter_out")

    # Connect the DUT
    counter = Counter("COUNTER")
    counter.clock       (clock      )
    counter.reset       (reset      )
    counter.enable      (enable     )
    counter.counter_out (counter_out)

    sc_core.sc_start(1, sc_core.SC_NS)

    # Open VCD file
    wf = sc_core.sc_create_vcd_trace_file("counter")

    # Dump the desired signals
    sc_core.sc_trace(wf, clock, "clock")
    sc_core.sc_trace(wf, reset, "reset")
    sc_core.sc_trace(wf, enable, "enable")
    sc_core.sc_trace(wf, counter_out, "count")

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

    return 0


if __name__ == "__main__":
    main()
