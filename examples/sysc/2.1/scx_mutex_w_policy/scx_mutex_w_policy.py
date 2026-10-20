"""Python port of the upstream FIFO/random mutex arbitration example.

Original source: examples/sysc/2.1/scx_mutex_w_policy/scx_mutex_w_policy.cpp
"""


# Copyright (c) 2002 Thorsten Groetker, Stan Liao, Grant Martin, Stuart Swan
#
# Permission is hereby granted to use, modify, and distribute this source
# code in any way as long as this entire copyright notice is retained
# in unmodified form within the source code.
#
# This software is distributed on an "AS IS" basis, without warranty
# of any kind, either express or implied.
#
# This source code is from the book "System Design with SystemC".
# For detailed discussion on this example, see the relevant section
# within the "System Design with SystemC" book.
#
# To obtain the book and find additional source code downloads, etc., visit
#     www.accellera.org
# Look in the "Products & Solutions" -> "SystemC Books". Then look at the
# entry for "System Design with SystemC".
#

from collections import deque
import ctypes

from amal.eda.systemc import sc_core as sc


class MutexPolicy:
    """Arbitrate native process ownership through per-waiter SystemC events."""

    def __init__(self, policy="FIFO"):
        """Select FIFO or native C rand arbitration and retain pending events."""
        if policy not in ("FIFO", "RANDOM"):
            raise ValueError("policy must be FIFO or RANDOM")
        self.policy = policy
        self.owner = None
        self.events = deque()

    def lock(self):
        """Suspend a contender and acquire ownership when explicitly notified."""
        current = sc.sc_get_current_process_handle()
        if self.owner is not None and self.owner != current:
            event = sc.sc_event()
            self.events.append(event)
            sc.wait(event)
        self.owner = current
        return 0

    def unlock(self):
        """Release only from the owner, waking one selected contender."""
        if self.owner != sc.sc_get_current_process_handle():
            return -1
        self.owner = None
        if self.events:
            index = 0 if self.policy == "FIFO" else ctypes.CDLL(None).rand() % len(self.events)
            event = self.events[index]
            del self.events[index]
            event.notify()
        return 0


class Top(sc.sc_module):
    """Run the upstream three-thread FIFO contention stimulus."""

    def __init__(self, name):
        """Retain mutex and register contenders in native source order."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.mutex = MutexPolicy()
        self.records = []
        for index in range(1, 4):
            self.sc_thread(f"t{index}", lambda index=index: self.contender(index))

    def contender(self, index):
        """Acquire after the source delay and hold ownership for10ns."""
        sc.wait(index, sc.SC_NS)
        assert self.mutex.lock() == 0
        record = f"t{index} got mutex at {sc.sc_time_stamp()}"
        print(record)
        self.records.append(record)
        sc.wait(10, sc.SC_NS)
        assert self.mutex.unlock() == 0


model = Top("Top1")
sc.sc_start(1000, sc.SC_NS)
assert model.records == ["t1 got mutex at 1 ns", "t2 got mutex at 11 ns", "t3 got mutex at 21 ns"]
assert not model.mutex.events and model.mutex.owner is None
sc.sc_stop()
