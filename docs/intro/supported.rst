Supported behavior
==================

* Hello: Python method callbacks and elaboration/simulation hooks.
* Counter: sensitivity, four-bit wraparound and native VCD signal transitions.
* FIFO: blocking exchange between Python native pthread processes.
* Fork/join: dynamic spawning, joins, event/time waits and termination events;
  25 records match the upstream C++ reference.
* Barrier: three threads released in one delta cycle at 11 ns; release order
  and timestamps match native C++.
* Report rules: custom Python handlers, native actions, severity/id precedence,
  suppression and forcing; 80 records match the upstream C++ reference.

Run ``make example-forkjoin-py`` and ``make example-report-py`` for the newer ports.
The synchronous reset port matches all 101 native reset/value records. Guarded
waits preserve scheduler unwind across Python frames; repeated asynchronous reset,
explicit reset and kill are checked across cycle, timed, event and event-list waits.
Process reset/kill must be invoked from a running SystemC process. Calls while
paused or stopped raise ``RuntimeError``. Use native waits through the Python
``wait()`` adapter; direct cppyy waits bypass unwind protection. Python cleanup
must propagate the private ``BaseException`` unwind and must not wait again.
Broad datatype/TLM support remains partial.

Only the scenarios listed above are verified. Remaining upstream ports and
feature coverage are still in development.

Additional verified behavior includes native fixed-point policies, TLM payload
metadata, namespace help, hierarchical exports with explicit get_interface(),
and VCD closure after Python exceptions. TLM transport callbacks, Python custom
interfaces and the full upstream feature matrix remain
incomplete. API exposure alone is not a compatibility claim.
