"""Exercise public output-form blocking peek against a native custom interface."""

import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc, sc_dt, tlm  # noqa: F401
import cppyy


cppyy.cppdef(
    """

    namespace exact_tlm_peek_unwind
    {
    using value_type = sc_dt::sc_biguint<137>;
    using peek_interface = tlm::tlm_blocking_peek_if<value_type>;

    /**
     * @brief Native channel whose blocking peek waits for a native event.
     * @details The Python binding owns the module object; SystemC registers it
     * during elaboration without owning the Python proxy. The caller must
     * retain its proxy across peek suspension. No Python
     * callback runs in the native wait or notification path. The Python bridge
     * releases the GIL while scheduling this wait and translates reset or kill
     * back to Python after native stack unwinding.
     */
    class PeekChannel final : public sc_core::sc_module, public peek_interface
    {
     public:
      using peek_interface::peek;

      /** @brief Construct a channel with an empty value until publication. */
      explicit PeekChannel(sc_core::sc_module_name name) : sc_core::sc_module(name), value_(0)
      {
      }

      /**
       * @brief Block until publication and return the stored transaction.
       * @param tag Optional native TLM tag, ignored by this single-value probe.
       * @return A copy of the published value.
       * @throws Any SystemC report raised while waiting or by kernel control.
       * @details The caller owns the returned value and must retain the channel
       * proxy for the full call. The bridge releases the GIL during the native
       * wait; this implementation invokes no Python callbacks.
       */
      value_type peek(tlm::tlm_tag<value_type>* tag = nullptr) const override
      {
        (void)tag;
        sc_core::wait(ready_);
        return value_;
      }

      /**
       * @brief Store a value and notify blocked peek calls.
       * @param value Value copied into native channel storage.
       * @details Notification is native and does not invoke Python. The Python
       * owner must keep this module alive until scheduled processes complete.
       */
      void publish(const value_type& value)
      {
        value_ = value;
        ready_.notify(sc_core::SC_ZERO_TIME);
      }

     private:
      mutable sc_core::sc_event ready_;
      mutable value_type value_;
    };
    }  // namespace exact_tlm_peek_unwind
    """
)


VALUE = sc_dt.sc_biguint[137]
SENTINEL_LOW = 71
PUBLISHED_LOW = 29


def wide(low):
    """Return a 137-bit value with the top bit and selected low word set."""
    value = VALUE(low)
    value[136] = True
    return value


def pair(value):
    """Extract the low word and top bit without narrowing the full value."""
    return int(value.to_uint64()), bool(value[136])


def run_scenario(scenario):
    """Run one exact output-overload scenario in a fresh native kernel."""
    channel = cppyy.gbl.exact_tlm_peek_unwind.PeekChannel("peek_channel")
    interface = channel
    starts = []
    cleanups = []
    normal_returns = []
    outputs = {}
    last_output = []

    def worker():
        """Suspend in exact public peek and preserve output through unwind."""
        invocation = len(starts) + 1
        output = wide(SENTINEL_LOW)
        outputs[invocation] = weakref.ref(output)
        if scenario == "normal":
            last_output.append(output)
        starts.append((invocation, str(sc.sc_time_stamp())))
        try:
            result = interface.peek(output)
            normal_returns.append((invocation, result, str(sc.sc_time_stamp())))
        finally:
            expected = PUBLISHED_LOW if normal_returns else SENTINEL_LOW
            assert pair(output) == (expected, True), (invocation, pair(output))
            cleanups.append((invocation, str(sc.sc_time_stamp())))

    options = sc.sc_spawn_options()
    options.set_stack_size(2 * 1024 * 1024)
    handle = sc.sc_spawn(worker, f"exact_peek_{scenario}_worker", options)

    def controller():
        """Interrupt peek at two ns or publish its value at three ns."""
        if scenario == "normal":
            sc.wait(3, sc.SC_NS)
            channel.publish(wide(PUBLISHED_LOW))
            while not normal_returns:
                sc.wait(sc.SC_ZERO_TIME)
            assert normal_returns == [(1, None, "3 ns")], normal_returns
            assert starts == [(1, "0 s")], starts
            assert cleanups == [(1, "3 ns")], cleanups
            assert pair(last_output[0]) == (PUBLISHED_LOW, True)
            assert handle.terminated()
            sc.sc_stop()
            return

        sc.wait(2, sc.SC_NS)
        assert starts == [(1, "0 s")], starts
        if scenario == "kill":
            handle.kill()
            sc.wait(sc.SC_ZERO_TIME)
            gc.collect()
            assert cleanups == [(1, "2 ns")], cleanups
            assert normal_returns == [], normal_returns
            assert outputs[1]() is None
            assert handle.terminated()
            sc.sc_stop()
            return

        handle.reset()
        while len(starts) < 2:
            sc.wait(sc.SC_ZERO_TIME)
        gc.collect()
        assert starts == [(1, "0 s"), (2, "2 ns")], starts
        assert cleanups == [(1, "2 ns")], cleanups
        assert outputs[1]() is None
        assert normal_returns == [], normal_returns

        sc.wait(1, sc.SC_NS)
        handle.kill()
        sc.wait(sc.SC_ZERO_TIME)
        gc.collect()
        assert cleanups == [(1, "2 ns"), (2, "3 ns")], cleanups
        assert outputs[2]() is None
        assert normal_returns == [], normal_returns
        assert handle.terminated()
        sc.sc_stop()

    # Kept outside the process frame so the borrowed interface remains alive
    # during the native wait and any reset/kill unwind.
    retained_channel = channel
    retained_interface = interface

    controller_handle = sc.sc_spawn(controller, f"exact_peek_{scenario}_controller", options)
    sc.sc_start(5, sc.SC_NS)
    assert handle.terminated() and controller_handle.terminated()
    assert retained_channel is channel and retained_interface is interface
    if scenario == "normal":
        assert normal_returns == [(1, None, "3 ns")], normal_returns
        assert pair(last_output[0]) == (PUBLISHED_LOW, True)
    else:
        assert normal_returns == [], normal_returns
    print(f"EXACT_TLM_PEEK_UNWIND_OK scenario={scenario}")


if __name__ == "__main__":
    """Dispatch one isolated exact-peek scenario."""
    selected = sys.argv[1]
    if selected not in ("kill", "reset", "normal"):
        raise SystemExit(f"unknown exact peek scenario: {selected}")
    run_scenario(selected)
