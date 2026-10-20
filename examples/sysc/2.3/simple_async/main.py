"""Notify a SystemC event asynchronously from a separate Python host thread.

Original sources: examples/sysc/2.3/simple_async/main.cpp and async_event.h.
Copyright (C) 2017, GreenSocs Ltd.; developed by Mark Burton.
Apache-2.0; see the upstream LICENSE and NOTICE.
"""

import threading
import time

from amal.eda.systemc import sc_core as sc


class AsyncEvent(sc.sc_prim_channel):
    """Retain native event state and deliver updates on the kernel thread."""

    def __init__(self, name):
        """Attach a suspending asynchronous primitive channel."""
        super().__init__(name)
        self.m_delay = sc.SC_ZERO_TIME
        self.m_event = sc.sc_event(name + "_event")
        self.async_attach_suspending()

    def notify(self, delay):
        """Request a native update from a separate host thread."""
        self.m_delay = delay
        self.async_request_update()

    def update(self):
        """Notify the retained event during the kernel update phase."""
        self.m_event.notify(self.m_delay)


class WatchDog(sc.sc_module):
    """Start host activity and stop simulation when its event reaches10ns."""

    def __init__(self, name):
        """Register noninitializing event sensitivity and retain the worker thread."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.when = AsyncEvent("when")
        self.barked = False
        self.m_thread = None
        self.sc_method("call_stop", self.call_stop)
        self.sensitive(self.when.m_event)
        self.dont_initialize()

    def start_of_simulation(self):
        """Launch one separate host thread when native simulation begins."""
        self.m_thread = threading.Thread(target=self.process)
        self.m_thread.start()

    def process(self):
        """Wait one wall-clock second before requesting native timed notification."""
        time.sleep(1)
        self.when.notify(sc.sc_time(10, sc.SC_NS))

    def call_stop(self):
        """Record event delivery and request native simulation stop."""
        print(f"Asked to stop at time {sc.sc_time_stamp()}")
        self.barked = True
        sc.sc_stop()

    def end_of_simulation(self):
        """Verify event dispatch preceded native end-of-simulation callbacks."""
        assert self.barked
        print("The dog barks before the end of simulation")


class Activity(sc.sc_module):
    """Run the upstream initial busy thread once."""

    def __init__(self, name):
        """Register the initializing native thread callback."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.sc_thread("busy", self.busy)

    def busy(self):
        """Emit the initial activity record."""
        print("I'm busy!")


watchdog = WatchDog("WatchDog")
activity = Activity("Activity")
print("Start SystemC ")
sc.sc_start()
watchdog.m_thread.join(timeout=5)
assert not watchdog.m_thread.is_alive()
assert sc.sc_time_stamp() == sc.sc_time(10, sc.SC_NS)
print("Program completed")
