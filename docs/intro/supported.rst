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

Structural Python interface implementations
------------------------------------------

``sc_core.sc_port[IF](name).bind(implementation)`` accepts ordinary Python
objects for TLM blocking-put, blocking-get, blocking-peek, transport and analysis interfaces.
Implement ``put(value)``, ``get()``, ``peek()``, ``transport(request)`` or ``write(value)`` using
the familiar SystemC method name. Native value-return and output-reference
calls are forwarded by a package adapter; Python methods return values.
Blocking put/get/peek/transport calls through the adapter release the GIL.

For example::

    class Transport:
        def transport(self, request):
            sc.wait(1, sc.SC_NS)
            return request * 2

    port = sc.sc_port[tlm.tlm_transport_if[int, int]]("port")
    port.bind(Transport())

Ports must be constructed within a module construction scope. Native interface
and hierarchical port bindings continue to use SystemC's native binding rules.
Adapters are child modules and retain Python implementations until interpreter
shutdown. Python ``Protocol`` definitions are optional typing aids; native
interface specializations determine C++ signatures. Missing required methods
raise ``TypeError``. Other structural interface families, specialized TLM ports,
and arbitrary Python reference-return implementations are not yet supported.

Custom interfaces can infer native signatures from annotated Protocol methods::

    from typing import Protocol

    @sc.interface
    class C_if(Protocol):
        def run(self) -> None: ...

    export = sc.sc_export[C_if]("IFP")
    export.bind(implementation)

``None`` results become C++ ``void``, ``bool`` becomes ``bool``, and ``float``
becomes ``double``. Native cppyy classes, including SystemC template
specializations, retain their C++ types. Inherited methods and resolvable string
annotations are supported. Python ``int`` does not specify a native width and
is rejected. Missing annotations, variadic/keyword-only parameters and defaults
also require an explicit declaration. Reference qualifications can use ``@sc.interface(methods={...})`` with trusted native signatures.
Repeated-name overload declarations are not supported by this dictionary.
The two sc_export examples show native and annotated Protocol implementations.

Structural nonblocking TLM put/get/peek and combined get-peek bindings use
``nb_put(value)``, ``nb_can_put()``, ``nb_get()``, ``nb_can_get()``, ``nb_peek()``
and ``nb_can_peek()``. Get and peek callbacks return ``(success, value)`` with a
boolean success flag; the native output argument is assigned only on success.
These callbacks must not wait. Native readiness events belong to the adapter;
``port.get_interface().notify_ready(delay)`` schedules readiness notification.
For the combined get-peek interface, it notifies both separate events.

``tlm_get_if[T]`` structural implementations provide ``get()``, ``nb_get()``
and ``nb_can_get()`` on one object. Native blocking value/output overloads and
nonblocking output assignment use that shared implementation. Blocking callbacks
may use guarded waits; nonblocking callbacks must return immediately.

``tlm_put_if[T]`` structural implementations provide ``put(value)``,
``nb_put(value)`` and ``nb_can_put()``. Both put callbacks receive value copies.
Blocking calls release the GIL, while nonblocking calls must return immediately.
Adapter-owned ``ok_to_put()`` storage is notified through ``notify_ready()``.

``tlm_peek_if[T]`` structural implementations provide ``peek()``, ``nb_peek()``
and ``nb_can_peek()``. Blocking value/output overloads receive copies; callbacks
must preserve samples rather than consume them. Nonblocking callbacks return a
``(bool, value)`` tuple. ``notify_ready(delay)`` schedules native peek readiness.

``tlm_get_peek_if[T]`` structural implementations combine ``get()``, ``peek()``,
``nb_get()``, ``nb_peek()``, ``nb_can_get()`` and ``nb_can_peek()`` on one object.
The native adapter implements the blocking/nonblocking get-peek inheritance
hierarchy, so base-interface ports can bind the resulting native interface.
``notify_ready()`` schedules both separate native readiness events.
