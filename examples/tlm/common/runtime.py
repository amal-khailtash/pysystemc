"""Shared native-time scheduling for Accellera TLM Python example ports.

Apache-2.0; see upstream SystemC LICENSE/NOTICE. Payload owners must outlive all
scheduled callbacks. Independent simulations require separate interpreters.
"""

import heapq
import itertools
from amal.eda.systemc import sc_core as sc


class Queue:
    """Retain objects ordered by native simulation time and insertion order."""

    def __init__(self, name):
        """Create an event and empty queue before elaboration."""
        self.event = sc.sc_event(name)
        self.items = []
        self.sequence = itertools.count()

    def notify(self, item, delay):
        """Retain item until delay expires and notify at the earliest due time."""
        due = int((sc.sc_time_stamp() + delay).value())
        heapq.heappush(self.items, (due, next(self.sequence), item))
        self.event.notify(sc.sc_time.from_value(self.items[0][0] - int(sc.sc_time_stamp().value())))

    def pop(self):
        """Return one due object, or None; schedule the next future arrival."""
        if not self.items:
            return None
        now = int(sc.sc_time_stamp().value())
        if self.items[0][0] > now:
            self.event.notify(sc.sc_time.from_value(self.items[0][0] - now))
            return None
        return heapq.heappop(self.items)[2]


def report(filename, function, message):
    """Print the source report fields with the native current time."""
    print(f"\nInfo: {filename}: {sc.sc_time_stamp()} - {function}\n      {message}", flush=True)
