Binding adapters
================

Templates remain native cppyy templates, instantiated on demand. Native
pass-through availability does not by itself establish tested support.

``sc_module.sc_method(name, callback)`` / ``sc_thread(name, callback)``
   Register a no-argument Python callable, retained on the module proxy.

``sc_module.sensitive(interface_or_port_or_event)``
   Add static native sensitivity, including edge finders.
   ``sensitive_pos`` and ``sensitive_neg`` expose the module's borrowed native
   edge builders and accept ``<<`` registration before process execution.

``sc_module.wait(*args)`` / ``sc_prim_channel.wait(*args)``
   Invoke the owner's native scheduling member, using its simulation context.
   Cycle, time, event and OR/AND list forms release the GIL during suspension.
   Reset and kill run Python cleanup before restoring native scheduler unwind.
   Keep the owner and events alive; do not wait during unwind cleanup.

``wait(*args)`` / ``cppyy.gbl.sc_core.wait(*args)``
   After importing the bindings, Python lookups through either namespace use
   the guarded wait adapter. Cycle, time, event and event-list overloads release
   the GIL and preserve Python cleanup during reset and kill. The original
   native scheduler exception is then restored to the kernel. Event and context
   arguments remain borrowed throughout suspension. This routing applies to
   Python calls; blocking methods inside third-party C++ channels require their
   own native unwind adapters.

``native_blocking_call(operation)``
   Invoke a zero-argument native C++ functor with the GIL released. Use this
   adapter for blocking methods in third-party C++ channels, including inherited
   methods, or for original native function pointers retained before importing
   the bindings. The functor's const call operator must execute the operation
   entirely in C++. It may return void or a copyable native value; the returned
   value has independent storage; borrowed storage inside that value retains
   its normal native lifetime. Pointer, reference and move-only returns are
   rejected before execution. Capture arguments in the functor and retain
   borrowed channels, events, and output buffers throughout the call. Copy Python
   scalar inputs into the functor; a native reference to a cppyy conversion
   temporary does not retain its storage after the factory returns.

   Reset and kill run Python ``finally`` blocks before restoring native scheduler
   unwind. Ordinary native exceptions propagate and subsequent guarded calls
   remain usable. Call from a SystemC thread process; never block in unwind
   cleanup or suppress ``BaseException``.

   For example, wrap an arbitrary native acquisition method in a native functor::

      cppyy.cppdef("""
      struct NativeAcquire {
          sc_core::sc_mutex_if& mutex;
          explicit NativeAcquire(sc_core::sc_mutex_if& value) : mutex(value) {}
          int operator()() const { return mutex.lock(); }
      };
      """)
      operation = cppyy.gbl.NativeAcquire(mutex)
      # Within a registered SystemC thread:
      status = sc_core.native_blocking_call(operation)

   Python callables and raw cppyy function/overload objects are rejected before
   execution. Importing the bindings replaces subsequent Python lookups of
   ``wait``; it cannot replace previously retained raw objects. Calling those
   objects directly during reset or kill bypasses the native guard and can
   cause SystemC's fatal F539 unwind report. A Python lambda around such a call
   does not solve the boundary: use the C++ functor adapter above. Python
   overrides inside the functor must themselves use guarded scheduling calls.

``sc_module.next_trigger(*args)`` / ``sc_prim_channel.next_trigger(*args)``
   Configure dynamic method sensitivity through the native member without
   suspending. Event expressions are consumed once; reuse explicit native event
   lists when registering the same sensitivity repeatedly. ``timed_out()``
   reports whether the current process resumed because its timeout expired;
   querying it outside a process raises native ``std::runtime_error`` before
   pointer access, matching the free timeout helper.

``sc_module.at_posedge(interface)`` / ``at_negedge(interface)`` / ``halt()``
   Preserve native sampled bool/logic edge waits and clocked-thread halt.
   These helpers require a native clocked-thread process context.
   Blocking calls release the GIL and retain reset/kill/halt cleanup behavior.
   Keep module and interfaces alive throughout suspension. Configure native
   stack size and synchronous/asynchronous reset before process execution.

``sc_prim_channel(name)`` / ``sc_prim_channel()``
   Register a named or native automatically named primitive channel. Python
   subclasses can override update and elaboration/simulation lifecycle hooks;
   cppyy acquires the GIL for callbacks. ``request_update()`` coalesces requests
   for the next update phase. Update callbacks must not wait. Retain the channel
   while the kernel or any external requester can use it.

``sc_prim_channel.async_request_update()``
   Request an update from outside the simulator. Attach/detach suspending support
   retains the native kernel's asynchronous wake behavior. Foreign native
   threads may request an update; Python callbacks run on the simulation thread.
   Join external requesters before destroying their borrowed channel.

``sc_spawn(callback, name='', options=None)``
   Spawn a retained Python callable or a native C++ functor and return a native
   process handle. Callback return values are ignored. Native functors execute
   directly in the kernel, including native waits.

``sc_spawn(result, callback, name='', options=None)``
   Assign the callback return value into caller-owned native C++ value storage
   or a ctypes numeric scalar. Templates accept arbitrary native result types,
   including wide integers and custom classes. Assignment happens only after
   successful return; reset, kill and callback errors leave the result unchanged.
   Callbacks and result proxies are retained for the single kernel lifetime.

``sc_spawn_options``
   Configure method/thread execution, initialization, stack size, static
   sensitivity and synchronous/asynchronous reset. Sensitivity accepts events,
   interfaces, ports, exports and event finders; reset accepts boolean signal
   interfaces and input/inout/output ports. Keep borrowed sources alive through
   simulation. Configure options before spawning.

``sc_process_handle.sync_reset_on()`` / ``sync_reset_off()``
   Enable or disable sticky synchronous reset, optionally including descendants.
   Reset takes effect when the process resumes; Python cleanup preserves native
   reset unwind. These controls release the GIL during native execution.

``sc_inout_resolved`` / ``sc_out_resolved`` / ``sc_inout_rv[width]`` / ``sc_out_rv[width]``
   Construct and bind resolved scalar/vector ports, initialize values and assign
   from values, interfaces or compatible ports using ``__assign__``. Vector
   widths instantiate on demand. Native resolution, elaboration diagnostics and
   trace transitions are preserved; bound interfaces must outlive their ports.

``wait(*args)``
   Suspend the native thread with GIL release. No arguments selects a cycle wait;
   arguments select guarded native timed/event overloads, including OR/AND
   expressions. Reset/kill unwind executes Python cleanup before the original
   scheduler exception is restored. Do not suppress unwind or wait in cleanup.
   Nonpositive cycle counts emit native E525; empty OR/AND event lists emit E565.
   With ``SC_THROW`` selected, the original ``sc_report`` propagates to the caller.

``set_report_handler(callback=None)``
   Install a retained callback receiving a native report and integer action mask;
   no callback restores the native default handler.

``next_trigger(*args)``
   Select exact native time, event or event-list overloads, with an optional
   simulation context. Event expressions are consumed once. Calling from a
   thread raises the native E520 ``sc_report`` without changing time or sensitivity.
   Empty OR/AND event lists emit native E565, preserving report metadata and counts.

``sc_report_handler.report(severity, message_type, message, ...)``
   Emit a message report with file/line and optional verbosity, or a legacy
   integer-ID report with file/line. Each call executes native policy counters,
   cache updates and the configured handler once, including when it throws.
   Incompatible Python arguments raise ``TypeError``; integers outside the native
   parameter range raise ``OverflowError`` before report counters, cache or handler
   state changes. Verbosity accepts the full native signed-int range. Only info
   reports are filtered by the current verbosity level; other severities retain
   their native actions even above that level.

``sc_report``
   Copy a borrowed callback report before retaining it. Copies own their message,
   file, process name and timestamp. ``__assign__(source)`` copies fields into
   an existing report; ``swap(other)`` exchanges them. ``valid()`` reflects native
   process-name context and can be false for populated reports outside a process.
   Legacy static methods ``register_id``, ``get_message``, ``is_suppressed``,
   ``suppress_id``, ``suppress_infos``, ``suppress_warnings`` and
   ``make_warnings_errors`` preserve native diagnostics and policy.
   ``register_id`` accepts ``cppyy.nullptr`` to select the native null-message
   error. Invalid, conflicting and null-message registrations raise E800 under
   the default handler without changing registered messages. An empty message
   uses the predefined native message and does not register the requested new ID.
   Policy settings persist in the singleton kernel; restore them when finished. Destruction
   releases copy-owned fields. Borrowed views must not outlive their report.

``sc_report_handler.stop_after(subject, ...)``
   Select severity-wide, message-wide or message/severity limits using native
   severity enums. An omitted limit is -1 and removes that selected limit.
   This adapter avoids treating a severity enum as an integer message limit.

``sc_report_handler.set_actions(...)`` / ``suppress(...)`` / ``force(...)`` / ``set_catch_actions(mask)``
   ``set_actions`` accepts severity, message and message/severity scopes. Omitting
   its mask installs ``SC_UNSPECIFIED`` and returns the previous scoped actions,
   matching the native default argument and fallback policy.
   Action masks must be integer-like and fit native unsigned int. Negative or
   oversized masks raise ``OverflowError`` before changing policy; incompatible
   types raise ``TypeError``. Native custom action bits remain available. Calling
   ``suppress()`` or ``force()`` without a mask resets it to zero and returns its
   previous value. Settings persist for the kernel lifetime; restore them after use.

``sc_report_handler.set_log_file_name(filename)``
   Set a copied native log filename. ``None`` and ``cppyy.nullptr`` clear the
   name through the native null setter, which returns ``False``. A new name
   returns ``True``; a second non-null name is refused while one is configured.
   Close any active default log stream before changing its filename.

``sc_report_handler.get_handler()`` / ``set_handler(token_or_none)``
   Save and restore borrowed native handler pointer tokens. ``None`` restores
   the native default. Tokens do not retain Python callbacks; use
   ``set_report_handler`` for Python callables. A saved Python dispatcher token
   requires its callback to remain retained and active before reinstalling it.
   Report caches are borrowed and invalidate on replacement, clear or release;
   copy reports first. Destroy copies before handler ``release()`` because message
   definitions remain owned by native handler storage.

``sc_attr_base`` / ``sc_attribute[T]`` / ``sc_attr_cltn``
   Attribute templates remain native and accept any native-valid default/copy
   constructible type. Attributes own their names and values. Collections borrow
   pointers; collection copies share those pointers. Keep attributes alive while
   either collection uses them. Removing an attribute or clearing/destroying a
   collection does not delete attributes. Native removal swaps the final pointer
   into the removed slot, so iteration order can change. Lookup returns a borrowed
   pointer or a null proxy; iterators invalidate when the collection is modified.

``sc_trace(file, signal, name)``
   Record stable native signal storage. Keep signals alive until file closure.

``sc_status()``
   Query the native current status and convert it to ``ScStatus``.

``sc_status_type``
   The native ``sc_core::sc_status`` enum type is exported under this distinct
   name so the existing ``sc_status()`` query keeps its callable behavior.
   Native enum members and conversions retain cppyy semantics.

``sc_argc()`` / ``sc_argv()``
   During ``sc_main``, read the native argument count and an immutable tuple of
   original argument strings, including argument zero. The tuple copies native
   storage and survives entry return. Mutating the Python ``sc_main`` argument
   list does not change these original values. ``sc_argv()`` raises
   ``RuntimeError`` outside active entry because native argument storage is
   borrowed only during entry. Native C++ retains its pointer-array signature.

``sc_elab_and_sim(argc=None, argv=None)``
   Enter ``sc_main`` with ``sys.argv`` when both arguments are omitted, or pass
   an explicit nonnegative native count and a list/tuple of strings. Only the
   first ``argc`` entries are selected. Invalid counts, incomplete arguments,
   nonstring selected entries and embedded NULs fail before native entry.
   The native integer status is returned, including nonzero results. Use a
   fresh process for each independent simulation; entry does not reset the kernel.
   Copied native arguments are released and the active argument snapshot is
   restored on every exit. Python report callback failures preserve their original
   exception and traceback, including during native exception translation and
   the final deprecation notice. Retaining a caught exception also retains its
   callback traceback until that exception is released.

``amal.eda.systemc.sc_main(argc=None, argv=None)``
   Call the global native callback bridge directly with the same argument
   validation as ``sc_elab_and_sim``. Define ``__main__.sc_main(argv)`` first.
   The bridge preserves native integer returns and returns -1 for missing,
   noncallable or invalid callbacks, invalid return values, and ``SystemExit``.
   It releases the GIL during native execution and reacquires it for callbacks.
   Direct calls do not activate the native ``sc_argc``/``sc_argv`` snapshot;
   ``sc_elab_and_sim`` manages that entry lifecycle and native exception handling.

``sc_start(duration, unit, p)``
   Select the native time, integer or double overload once, preserving the
   starvation policy and avoiding overload retries after simulation errors.
   Omit all arguments for unbounded simulation. Events exactly at a bounded
   run's end execute on the next simulation call, matching native SystemC.
   Starting after the kernel has stopped emits native E546.

``sc_stop()``
   Request native stop once. Python lifecycle callback errors propagate; if an
   end-of-simulation hook fails, the kernel retains its native terminal phase.
   Further stop calls and interpreter cleanup leave that phase unchanged and
   do not repeat the callback or emit a duplicate-stop warning.

``at_posedge(signal, simc=None)`` / ``at_negedge(signal, simc=None)``
   Use native sampled-edge waits for bool or logic input interfaces inside a
   SystemC thread. Waits release the GIL and preserve Python cleanup during
   native reset/kill. ``timed_out(simc=None)`` reads the current process timeout
   flag; calling it outside a running process raises ``RuntimeError``.

``sc_ref(value)`` / ``sc_cref(value)``
   Infer unrestricted native reference-wrapper templates and retain the Python
   referent until wrapper destruction. Native consumers preserve mutable/const
   contracts. Keep the wrapper alive while using borrowed ``get()`` results.

Native callback typedefs
------------------------

``sc_core.CFT``, ``sc_plist_map_fn``, ``sc_report_handler_proc`` and
``sc_ppq_base.compare_fn_t`` expose native pointer-value adapters. They store
the exact native function-pointer typedef, rather than converting it to
``std::function``. cppyy does not reflect these typedefs as standalone type
objects; the Python aliases name their value adapters. ``CFT`` and
``compare_fn_t`` share a type because their native signatures are identical.

Default construction creates a null value. Copy construction and equality
preserve native pointer identity. Select a qualified native function with
``CFT.from_function("model::compare")`` or ``CFT.from_function("::compare")``.
Overloaded names select the exact native callback signature; malformed names
and incompatible signatures raise ``TypeError``. Keep the selected native code
loaded through every consumer. Adapters do not retain Python callables; existing native
consumer APIs continue accepting Python callbacks with their documented
retention requirements.

Pass a ``CFT`` value to ``sc_pvector.sort`` or a ``sc_plist_map_fn`` value to
``sc_plist.mapcar``. Vector comparators receive pointer-slot addresses; list
mappers receive borrowed payload/context pointers. A comparator's
``create_queue(sc_ppq[T], size)`` method returns a Python-owned native queue
that borrows pointees and the selected native comparator code. Null values
raise ``ValueError`` in these adapter consumers. Report-handler values work
with ``sc_report_handler.get_handler`` and ``set_handler``; use
``set_report_handler`` to install retained Python callbacks.

TLM base socket interface
-------------------------

``tlm_base_socket_if.get_base_interface()`` is conditional on
``SC_ENABLE_COVARIANT_VIRTUAL_BASE`` and is absent from the default runtime.
Both const and nonconst base-interface overloads are supported with a matching
native build and Cling configuration. Select that installation with
``SYSTEMC_INSTALL_DIR`` and add ``-DSC_ENABLE_COVARIANT_VIRTUAL_BASE`` to
``EXTRA_CLING_ARGS`` before importing the package; retain the C++17 compiler
flag and any existing compiler options. All translation units must agree with
the linked library. Changing this macro after import is not supported.

Invoke the virtual getter on a bound socket/base proxy. Its returned native
``sc_interface`` reference borrows the bound endpoint and its module owner;
keep both alive. Default and custom protocols are tested before and after
elaboration using exact const/nonconst base-member calls. Run
``make test-tlm-covariant`` for the separate compatible-build validation.

Time arithmetic and temporary storage
-------------------------------------

Native ``sc_time`` arithmetic supports addition, subtraction, modulo, scalar
multiplication in either order, scalar division and time/time ratios. Results
preserve native tick rounding and leave their operands unchanged. A product of
two times is not defined; divisors must satisfy the native nonzero contract.

Time constructors accept all six native enum units and string symbols, including
``"SC_NS"`` and explicit live simulation-context pointers. String-unit failures
propagate native ``sc_report`` exceptions. Legacy ``sc_time(integer, False)``
preserves the full unsigned 64-bit tick value; floating legacy inputs retain
native rounding and default-unit scaling. Prefer ``from_value``, ``from_seconds``
and ``from_string`` for explicit conversions. In-place arithmetic mutates the
owned duration and returns the same native object. Configuration and resolution
remain shared by the native kernel; contexts are borrowed during construction.


``sc_byte_heap.allocate(bytes_n)`` returns a mutable borrowed byte view instead
of a copied string. The view has exactly the requested length, and allocations
retain native eight-byte alignment and wrapping reuse. Keep the heap alive while
accessing a view. Reinitializing or destroying the heap invalidates existing
views; wrapping allocations can overwrite their contents. An uninitialized heap,
a negative request or an aligned request larger than capacity raises
``ValueError``. Noninteger sizes raise ``TypeError``. An explicitly initialized
zero-capacity heap supports ``allocate(0)`` and returns an empty borrowed view;
a default, uninitialized heap still raises ``ValueError``. Serialize access to a
shared heap from native threads; holding the GIL does not protect its native
allocation cursor.

For signed native byte trace storage, initialize and update
``sc_trace_value["char"]`` with a one-byte ``bytes`` object. This preserves the
native bit pattern. cppyy's Unicode conversion cannot read negative native
``char`` values through the holder's field getter; trace registration reads the
native storage directly.

For borrowed native ``char`` storage, pass a typed
``ctypes.POINTER(ctypes.c_char)`` or a nonempty ``ctypes.c_char`` array to
``sc_trace(file, value, name, width)``. Omitting the width uses the native
8-bit default. The exact ``const char*`` overload selector accepts the same
typed forms. Registration preserves the original address and samples live
mutations without interpreting the byte as text. For Python-managed trace files, the wrapper retains the
ctypes pointer or array until matching closure. With an externally created
native trace file, retain the ctypes object yourself. A pointer created
from a native address still borrows its independent owner. Keep that owner
alive until closure. Null pointers with a live trace file raise ``ValueError`` and empty arrays raise
``TypeError``. Raw ``const char*`` results are converted to Python strings by
cppyy and do not provide a stable scalar trace address.

Trace comments and file ownership
---------------------------------

``tprintf(file, format, *args)`` writes a C printf formatted trace comment.
Numeric length modifiers, dynamic width/precision, positional arguments, string,
character, hexadecimal-float and pointer conversions follow native formatting.
For ``%n``, supply a compatible writable native pointer or contiguous signed
integer buffer matching the required native element type and width. Native typedef
aliases and ctypes integral aliases follow the host ABI. All count lengths,
including ``%hhn``, borrow aligned storage and write counts only during the output
pass. Count conversions cannot use flags, width or precision. Null, read-only, empty,
noncontiguous and incompatible Python buffers raise ``TypeError``. Arguments
remain alive during the call; invalid formats or argument conversions raise
``ValueError`` or ``TypeError``. Comment buffers grow to the required size.
``sc_write_comment(file, text)`` writes unformatted text.

``sc_trace_delta_cycles(file, on=True)`` selects native delta-cycle tracing.
Native signal tracing does not add simulation processes or change delta timing.
Structural reference interfaces retain their value-refresh behavior.

Files returned by ``sc_create_vcd_trace_file`` and ``sc_create_wif_trace_file``
remain open until their matching close helper is called. Close each non-null
file exactly once after tracing ends, keeping traced values alive until then.
Closing invalidates all file aliases and flushes output. Both close helpers
accept ``None`` as a null no-op; native file opening can be deferred until the
first traced cycle.

Stable trace values and native trace-file methods
-------------------------------------------------

``sc_trace_value[Type](initial)`` owns mutable native storage with its exact
C++ type. Use it when tracing scalar values that cppyy otherwise converts to
Python numbers::

    value = sc_core.sc_trace_value["float"](1.5)
    file.trace(value, "sample")
    value.value = 2.75

Both ``file.trace`` and ``sc_trace(file, value, name)`` accept these holders,
with an optional native integer width. Enumeration tracing accepts an
``unsigned int`` holder and a sequence of string labels, optionally terminated
by ``None``. The file retains holders and copied enumeration labels until its
matching close helper succeeds. Types and template widths are instantiated on
demand. Ordinary native reference arguments remain caller-owned; Python scalar
temporaries do not provide persistent waveform storage.

``sc_trace_file`` is abstract, with a public constructor and protected virtual
destructor. Native-derived files expose all native trace overloads, comment,
space, delta-cycle and time-unit methods; native-derived bridges can implement
the protected cycle hook. This does not provide Python virtual subclassing.
Direct native destruction requires caller-managed holder lifetimes.

Free tracing supports native reference and pointer overloads. Explicit native
selection remains available through ``sc_trace.__overload__``; pointed-to values
must remain alive until file closure. Integer holders use native default widths
when the width is omitted. Signal interfaces accept explicit widths for the
native ``char``, ``short``, ``int`` and ``long`` overloads; unsupported signal
width arguments raise ``TypeError``.

``sc_trace(file, port, name)`` supports input and inout/output ports before
binding or elaboration. Registration is deferred through the native port until
elaboration resolves its interface. Bind every required port before starting
simulation and keep ports and interfaces alive through tracing. Once a file
has begun recording, adding a new waveform preserves the native trace-file
report; create another file if additional traces are needed.

Signals and ordinary signal ports
---------------------------------

``sc_signal[Type, Policy]`` supports native construction, assignment, value and
event access for generic datatypes and the bool/logic specializations. Values
written to a signal become current during the kernel update phase. Native writer
policies, edge detection and value-change notifications remain in force.
Datatype widths are instantiated on demand, including widths above 128 bits.

``sc_signal_value(signal)`` invokes the signal's native conversion to a
``const Type&``. Fundamental values become Python scalar copies; native datatype
results borrow the signal's current storage and reflect subsequent updates.
Keep the signal alive and treat these references as read-only. Use ``read()``
or this explicit helper for signal values: ``bool(signal)`` checks cppyy proxy
validity, and arbitrary native signals do not acquire Python ``int()`` coercion.
The helper accepts native signal subclasses and their actual writer policy;
conversion does not advance simulation.

``sc_in[Type]``, ``sc_inout[Type]`` and ``sc_out[Type]`` preserve native interface
and parent-port constructor/binding forms. Input ports provide value/event
access; inout/output ports provide native value, interface and port assignment
and initialization. Initialization can be supplied before binding and is applied
when elaboration resolves the interface. Bool and logic ports expose native
positive/negative edges and cached sensitivity finders. Keep ports, bound
interfaces, parent ports and borrowed finder/value views alive for their native
consumers. Required ports must be bound before simulation starts.

``sc_port_value(port)`` invokes the native value conversion for bound input,
inout and output ports. Fundamental values become Python copies; native
datatype results borrow the bound interface's current storage. Keep both the
port and interface alive, treat the result as read-only and bind before calling.
The helper holds the GIL without advancing simulation or changing proxy
truthiness. Native interface access errors propagate.

Objects, port bases and temporary pools
---------------------------------------

``sc_object`` exposes hierarchy, attributes, printing, context and phase-mask
operations. Its native assignment deliberately returns the destination without
changing its name, attributes or children. Hierarchy and attribute views borrow
their native owners. Protected construction and phase hooks require a native
subclass; keep objects alive while any borrowed views are used.

``sc_port_base`` exposes interface access, type information, binding counts and
native binding operations. Construct ports inside an active module hierarchy,
respect their binding policy and keep bound interfaces and parent ports alive.
Protected constructors, sensitivity helpers and lifecycle hooks require native
derived access and preserve native report behavior.

``sc_port[Interface, N, SC_ALL_BOUND]`` requires all ``N`` slots to be bound
before elaboration completes. With ``N=0``, the native unlimited port still
requires at least one binding. Missing bindings produce native report 109
(``complete binding failed``); ignoring that report does not supply an interface.

``sc_vpool[Type]`` allocates native temporary-array slots, wraps at its capacity
and resets the allocation index without destroying values. Its native destructor
does not free default-allocated storage. An external array must contain live
values and outlive the pool and its borrowed slot views. Reused slots can
replace prior temporary values; reference-proxy slots require native setup
before dereferencing. Allocation, reset and size operations support arbitrary
native types, including the native datatype-proxy specializations.

``sc_pvector[PointerType]`` indexing grows native slots and initializes new
pointer slots to null. Pointees remain borrowed. Negative indices raise
``IndexError``; indices whose count would overflow native ``unsigned int``
raise ``OverflowError`` before accessing or changing storage. ``fetch()`` reads
an existing slot without growing the vector and checks its bounds.

``sc_phash[Key, Contents]`` and its iterators preserve native typed output slots,
copy and removal overloads. Tables borrow pointer contents; duplicated-key
callbacks control key copying and release. Keep contents and callback state
alive for every native use, and discard iterators after invalidating mutations.

Owner lifetimes and shutdown
----------------------------

Native pointer and reference APIs preserve their C++ borrowing rules. Keep the
referent alive for every native borrower; deleting a Python variable does not
end a process, close a trace, or detach a port. ``sc_stop()`` ends simulation
without resetting the singleton kernel or clearing process callback owners.

.. list-table:: Python adapter ownership boundaries
   :header-rows: 1
   :widths: 28 38 34

   * - Adapter
     - Retained storage
     - Release boundary
   * - Module process registration
     - Registered Python callable on its module
     - Native module lifetime; destroy only after its processes and borrowers finish
   * - ``sc_spawn`` and ``sc_bind``
     - Registered callback, typed result storage, and bound native input proxies
     - Interpreter's single kernel lifetime, including after stop or callback error
   * - Structural interfaces
     - Implementation and returned reference storage
     - ``release_interface_callbacks()`` after stop and final reads; native adapter deletion requires ``teardown_interfaces()``
   * - Blocking call frames
     - Channel, input, tag, and output proxies passed to the call
     - Return or unwind; retaining an exception traceback can retain its call frame
   * - Callable TLM sockets
     - Accepted socket and callback registrations; ignored duplicate candidates are dropped
     - Interpreter's single kernel lifetime; stopping simulation does not release active callbacks
   * - Callback PEQ
     - Queue and callback; payload addresses remain borrowed while phases and scheduling times are copied
     - Queue and callback stay retained for the kernel lifetime; retain payloads until final delivery or cancellation
   * - Callback quantumkeeper
     - Callback and copied native time slot
     - Last keeper proxy; a synchronization call retains its keeper through suspension
   * - ``sc_scoped_lock``
     - Its mutex proxy
     - Lock destruction; ``release()`` unlocks without destroying the lock
   * - ``sc_ref`` and ``sc_cref``
     - Referent proxy, with no native value copy
     - Last wrapper reference; borrowed ``get()`` results still require a live referent
   * - Vector creator
     - Returned native element proxies after ownership transfers to the vector
     - Vector destruction; the creator callable is used synchronously during initialization
   * - Trace value holders
     - Holder storage and copied enumeration labels
     - Matching trace-file close; ordinary traced sources remain caller-owned
   * - Hash table callbacks
     - Python constructor and setter callbacks on the table proxy
     - Table proxy destruction; native-created table views need externally retained callbacks

A Python callable converted directly to a raw cppyy ``std::function`` still
needs an external Python owner throughout every possible invocation, including
when that native function proxy is passed to ``sc_bind``. Retaining the native
proxy does not acquire ownership of its original Python callable.

Finish end-of-simulation hooks and final reads before releasing structural
callbacks. Close trace files before deleting traced storage. Destroy native
ports and exports and terminate thread processes before deleting their
structural adapters. ``teardown_interfaces()`` checks those visible borrowers
before mutation; close traces created directly in C++ separately. Borrowed
adapter, event, and result aliases become invalid after native deletion.

Process joins
-------------

``sc_join.wait()`` waits for its registered threads to terminate.
``wait_clocked()`` uses static sensitivity and always waits at least one cycle.
Both waits release the GIL and run Python cleanup when their waiter is reset or
killed. Keep the join alive until all registered threads terminate, even if its
waiter has exited. Register threads before they terminate: adding an already
terminated handle leaves its join count pending. Method handles are rejected by
the native join report.

Diagnostics
-----------

``PYSYSTEMC_LOG_LEVEL=DEBUG`` enables binding diagnostics. Color follows
``PYSYSTEMC_COLOR=auto/always/never``. Explicit ``always`` overrides ``NO_COLOR``;
automatic mode honors it. Native messages use fmt; Python messages use Loguru
and Rich without changing application logging sinks.

The package exposes ``__version__`` from installed metadata and prints it before
the native kernel banner. ``PYSC_COPYRIGHT_MESSAGE=DISABLE`` suppresses only the
binding banner.

Native namespaces and enums
---------------------------

``sc_core`` exposes native time/status, port/writer policies, events, channels,
exports and process classes alongside the Python callback adapters. ``sc_dt``
exposes integer, logic/vector and fixed-point templates and native policy enums.
``dir(namespace)`` and wildcard imports include resolvable native names. Templates
remain parameterized at runtime; there is no registry of permitted widths.

The package initializer selects C++17 before importing cppyy to match the native
SystemC build. Submodules inherit this setting. Existing ``EXTRA_CLING_ARGS``
values are preserved.

Fixed-point headers are enabled before SystemC is parsed. For example::

   from amal.eda.systemc import sc_dt
   value = sc_dt.sc_fixed[8, 4, sc_dt.SC_TRN, sc_dt.SC_SAT](20)
   assert value.to_double() == 7.9375

The ``tlm`` module exposes native TLM protocol enums, generic payloads, phases,
interfaces and socket templates::

   from amal.eda.systemc import tlm
   payload = tlm.tlm_generic_payload()
   payload.set_command(tlm.TLM_WRITE_COMMAND)
   payload.set_address(0x1000)
   payload.set_response_status(tlm.TLM_OK_RESPONSE)

Generic payloads do not own their data or byte-enable buffers. Retain native
buffer storage throughout the transaction. Payload fields and fixed-point
policies are tested. Callable socket factories and their verification limits are
described in :doc:`tlm_sockets`. Full transport and custom-interface coverage
remains incomplete.

Interactive help
----------------

Use standard Python introspection to explore the included native API::

   from amal.eda.systemc import sc_core, sc_dt, tlm
   help(sc_dt)
   help(sc_dt.sc_q_mode)
   help(sc_dt.sc_uint)
   help(sc_dt.sc_uint[17])
   help(tlm.tlm_generic_payload.set_address)
   dir(sc_core)

Namespace help groups classes, templates, enums, functions and constants, and
lists enum values. Native class help retains cppyy's method and constructor
signatures; templates describe parameterized instantiation. Enum type metadata
is read-only in cppyy, so enum help uses its native members while namespace help
provides the formatted values. Newly resolved class/template proxies receive
missing documentation without replacing existing native overload help.

Trace lifecycle
---------------

``sc_core.vcd_trace(filename, time_unit=None)`` closes a native VCD trace when
its context exits, including Python exceptions. Keep all traced objects alive
until that point::

   with sc_core.vcd_trace("waves", time_unit=(1, sc_core.SC_NS)) as trace:
       sc_core.sc_trace(trace, signal, "signal")
       sc_core.sc_start(10, sc_core.SC_NS)

SystemC appends ``.vcd`` to the filename. Exceptions propagate after closure;
native aborts cannot run Python cleanup. See ``examples/toy/trace_cleanup.py``.

Hierarchical interfaces
-----------------------

``examples/sysc/2.1/sc_export/main.py`` binds native custom interfaces through
Python module exports and ports. Keep ``sc_module_name`` alive throughout each
module constructor. Use ``export.get_interface()`` when binding a port or another
export: cppyy does not provide SystemC's implicit export/interface conversion.

Native ``sc_export[IF]`` supports default/named construction, ``bind(interface)``
and call-style binding. Interface access borrows the bound channel, which must
survive the export and its consumers. Const and mutable access preserve the same
native interface and type identity. Duplicate binding, unbound arrow access and
unbound elaboration retain native reports. Protected export-base lifecycle hooks
and reporting are available to native derived modeling classes.

Scoped locks
------------

``sc_core.sc_scoped_lock(mutex)`` constructs an owning native lock through a
GIL-released scheduler guard. It retains the mutex until lock destruction.
``lock.release()`` unlocks once and returns ``True``; subsequent calls return
``False``. Native destruction also releases an active lock. Retain the lock in a
local variable for the critical section and release it in ``finally`` when prompt
cleanup matters. Construction can block and must then run in a SystemC thread;
reset/kill during acquisition unwinds Python frames before the kernel resumes.
The factory returns a native ``sc_scoped_lock`` proxy, usable by native consumers,
and exposes the native ``lockable_type`` alias.

Runtime initialization
----------------------

Native library loading and CPyCppyy header repair are centralized in the private
runtime module. The package ships its licensed native entry adapter and requires
no external kernel source file at startup. Existing environment overrides remain
available; ordinary installed wheels discover bundled dependencies automatically.

Structural reference results
----------------------------

Explicit ``sc.interface(methods=...)`` declarations support lvalue reference
results such as ``const sc_dt::sc_biguint<137>&`` and ``sc_core::sc_event&``.
The ordinary Python method returns a cppyy native object. A native pointer
trampoline preserves its address rather than copying the value. The structural
adapter retains each distinct returned object through interpreter shutdown, so
previously borrowed references survive replacement of the owner's storage.
Return long-lived storage: creating a fresh object on every call retains all
those objects. Python scalars are rejected for reference results.

Mutable results preserve writable native identity; event results preserve
sensitivity to the original event. Named scalar arguments are supported.
Defaults, template-valued argument declarations, rvalue-reference results and
repeated-name overloads require a dedicated native adapter. Native signal and FIFO structural adapters are described below.
Callbacks use cppyy's GIL acquisition; blocking callers must release the GIL and
callbacks must use guarded waits. Owner teardown before shutdown remains open.


Native structural signal and FIFO interfaces
--------------------------------------------

``sc_port[sc_signal_in_if[T]]`` and ``sc_port[sc_signal_inout_if[T]]`` accept
ordinary Python owners implementing ``read``, ``get_data_ref``,
``value_changed_event`` and ``event`` (plus ``write`` for inout). Native datatype
objects returned by read/trace callbacks preserve borrowed address identity.
Fundamental C++ scalar types use stable adapter-owned read/trace slots; logic and bool owners also supply
``posedge``, ``negedge`` and their event callbacks. ``default_event`` delegates
to the owner's value-changed event. Owners must implement native update/delta
semantics; the adapter does not schedule signal writes for them.

``sc_port[sc_fifo_in_if[T]]`` accepts ``read()``, ``nb_read()`` returning a
``(bool, value)`` pair, ``num_available`` and ``data_written_event``.
``sc_fifo_out_if[T]`` accepts ``write(value)``, ``nb_write(value)``, ``num_free``
and ``data_read_event``. Native read overloads return a value or assign an output;
a failed nonblocking read preserves the output. Write callbacks receive copies.
Blocking calls release the GIL and callbacks must use guarded waits.

Native ``sc_fifo[T]`` blocking reads, writes, assignment and conversion use the
same guarded scheduler path. ``fifo.__assign__(value)`` writes and returns the
FIFO; fundamental numeric conversions such as ``int(fifo)`` consume one sample.
Reset/kill during a blocked operation runs Python cleanup before native unwind.
FIFO ``trace`` retains the header's ``DEBUG_SYSTEMC`` behavior: without that
compile-time flag, it emits no buffer declarations. Trace signals or retained
sample values separately when a waveform is needed in that configuration.
FIFO insertion with ``stream << fifo`` selects native sample printing for
arbitrary element types and future native stream subclasses. It returns the
original stream for chaining and leaves the queue unchanged. Native stream I/O
failures raise ``OSError`` with the native diagnostic.

Events returned by different adapter specializations retain the owner's event
identity. Each specialization remains a distinct native interface/module.
Returned native objects and owners remain retained until their documented
callback-release or interface-teardown operation.

The separate ``sc_fifo_blocking_in_if[T]``, ``sc_fifo_blocking_out_if[T]``,
``sc_fifo_nonblocking_in_if[T]`` and ``sc_fifo_nonblocking_out_if[T]`` templates
also accept structural owners. Blocking input requires only ``read``; blocking
output requires only ``write``. Nonblocking input requires ``nb_read`` and
``data_written_event``; nonblocking output requires ``nb_write`` and
``data_read_event``. Counts belong to combined interfaces. Borrowed events,
failed-output preservation and blocking GIL/wait rules match the combined adapters.

Native FIFO ``nb_write`` callbacks must return an actual Python bool; integers,
strings and None raise ``TypeError``. Nonblocking reads require a two-item tuple
with a bool success flag and a convertible native value. Invalid results and
callback exceptions preserve the caller's read output. Blocking callback errors
propagate to the caller; after correcting owner state, later calls remain usable.


Scalar owners return Python values from ``read``; ``get_data_ref`` uses the same
native slot and does not require another owner callback. Successful reads update
that slot immediately; owners are responsible for native update/delta semantics.
Different adapter specializations have separate scalar slots even when they borrow
the same event. Invalid conversions propagate; later valid reads remain usable.

Structural synchronization and write interfaces
------------------------------------------------

``sc_port[sc_mutex_if]`` accepts owners with ``lock``, ``trylock`` and ``unlock``.
``sc_port[sc_semaphore_if]`` requires ``wait``, ``trywait``, ``post`` and
``get_value``. Blocking adapter calls release the GIL. Python callbacks use guarded
waits; callbacks delegating to a blocking native channel must also enable GIL
release on that native call. Retain channel state through simulation.

``sc_port[sc_event_queue_if]`` requires ``notify(time)``, ``cancel_all`` and
``default_event``. Both native notify overloads forward a native sc_time value.
The default event callback returns retained native event storage.

``sc_port[sc_signal_write_if[T]]`` requires ``write(value)`` and
``get_writer_policy``. Write callbacks receive a value copy. The owner provides
native update scheduling and writer policy; the adapter does not enforce a policy
independently.

Synchronization callbacks return signed native 32-bit integers; Python bools,
non-integers and out-of-range values raise ``TypeError``. Signal event/edge flags
must return bool. Writer-policy callbacks return one of the native one/many/unchecked
policies. Borrowed event callbacks require a live native sc_event object; wrong
types and null event proxies raise ``TypeError`` before native pointer dispatch.
These checks do not prevent callbacks from mutating owner state before raising.

Structural TLM ``nb_put``, ``nb_can_put``, ``nb_can_get``, ``nb_can_peek`` and
``get_direct_mem_ptr`` callbacks must return Python bool values. None, integers,
strings and other truthy objects raise ``TypeError`` rather than being converted
implicitly. Later valid callbacks remain usable. Callback mutations of payloads,
DMI descriptors or owner state are not rolled back when result validation fails.

Blocking TLM ``peek()`` and ``peek(tag)`` select the native value overload
directly, including ``cppyy.nullptr`` tags. Callback exceptions
propagate without being replaced by an output-overload conversion error.
Calls after structural callback release raise ``RuntimeError``; failed output
calls preserve caller storage.

Structural TLM FIFO ``nb_expand`` and ``nb_unbound`` callbacks return None, matching
the native void interface. ``nb_reduce``, ``nb_bound`` and debug ``nb_poke`` return
Python bool. Invalid results raise ``TypeError``; default arguments remain native
(expand/reduce 1, unbound 16, poke index 0). Owner mutations are not rolled back.

Signal inout owners may implement ``get_writer_policy`` to report their native
policy. Without this callback, the adapter returns ``SC_DEFAULT_WRITER_POLICY``.
Explicit callbacks use the same policy validation as write-only interfaces.
Policy reporting does not independently enforce owner write scheduling or contention.

Signal inout ``write(value)`` callbacks receive a native value copy, like write-only
signal and FIFO adapters. Retaining or mutating that callback argument does not
alter the native caller's storage. The owner still controls its signal state and
native update/notification scheduling.

Write-only signal and native FIFO blocking/nonblocking output callbacks also
receive Python-owned native copies. Owners can retain the callback object after
the call; caller reassignment or deletion does not invalidate that object.
Write and put callbacks use the native value's copy constructor, including
copy-only types without a default constructor or assignment operator. FIFO
write-only interfaces instantiate only the write guard, leaving read-type
requirements to interfaces that expose reads.

Guarded FIFO ``read()`` uses the native value-returning overload and returns an
independently owned copy without requiring default construction. A supplied
``read(output)`` selects the native output-parameter overload using a copy of
the output's initial value. Reset or kill leaves the caller's output unchanged;
the completed value is assigned only after the read succeeds. An incompatible
initial output is rejected before consuming a sample.

Structural TLM analysis ``write``, blocking/nonblocking ``put`` and debug
``nb_poke`` callbacks receive Python-owned value copies. Retained arguments remain
valid after return and do not alias caller storage. Debug index arguments are
preserved. Transaction transport callbacks continue to borrow native transactions.

FIFO debug ``used`` and ``size`` callbacks return signed native 32-bit integers.
Bool, noninteger and out-of-range results raise ``TypeError``. Negative values
remain representable for native sentinel semantics; the owner determines whether
its counts are consistent with its FIFO state.

TLM write and debug callback contracts
--------------------------------------

Structural owners may bind ``tlm_write_if[T]``, ``tlm_analysis_if[T]``,
``tlm_delayed_write_if[T]`` and ``tlm_delayed_analysis_if[T]``. Implement
``write(value)`` or ``write(value, delay)`` respectively and return ``None``.
Values are copied before Python callback entry; delay is borrowed native time
storage and must be copied if retained beyond the callback. Compatible
write/analysis views share the same adapter. Keep owners and borrowers alive
until stop and follow the structural teardown contract above.

``transport_dbg(payload)`` must return a Python integer in the unsigned native
range and no greater than ``payload.get_data_length()``. Invalid counts raise
before native return conversion. Debug callbacks must not wait or notify events.
Native enum metadata may be lost when cppyy converts a return to Python int;
structural writer-policy callbacks must explicitly return ``sc_writer_policy``
values, including when delegating to a native channel.

Event-list expression construction
----------------------------------

``sc_event_or_list(expression)`` and ``sc_event_and_list(expression)`` consume
a matching native expression once, as native C++ list construction does. The
first-party adapter resolves cppyy's missing expression conversion. The list
owns its storage and borrows its events; keep events alive through list use and
all waiting processes. Construction runs with the GIL held on the kernel thread.
Native errors propagate. Do not reuse a consumed expression or modify a list
while a process waits on it; native assignment and swap report errors while busy, preserve waiters when the
error is thrown, and permit mutation after all waiters resume. Event union
operations retain native behavior, which does not enforce the same busy check.

Calling an event expression's ``release()`` transfers its automatic native list
to the caller as a borrowed reference. Consume it immediately by copying into
the matching owning OR/AND list. That copy deletes the automatic list; discard
the borrowed proxy and never access it afterward. Release each expression once.

The three native ``sc_event.notify_delayed`` signatures use explicit bridge calls
so duplicate pending notifications raise native ``sc_report`` rather than a
cppyy overload-resolution ``TypeError``. Deprecation diagnostics, scheduling and
event lifetime requirements retain native semantics.

Event destruction with active timed waiters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Retain event storage until every dynamic waiter has finished. Deleting an event
while native timed thread/method waits still reference it crashes in both the
Python model and independent native C++ reference on the pinned runtime.
The isolated manual reproducers are ``tests/models/sc_core_event_destruction.py``
and ``tests/native/sc_core_event_destruction.cpp``. This unsupported native
lifetime pattern is excluded from the passing regression suite. Pending events
without active waiters can be destroyed: cancellation and registry removal have
separate passing native comparisons.

Datatype arithmetic and contexts
--------------------------------

Signed ``sc_signed % sc_signed`` and unsigned value shifts by Python integers
use explicit native overload adapters. Results own independent native storage;
widths and remainder signs follow SystemC. Other operand types retain cppyy's
existing overload handling. Native zero-divisor behavior remains terminal.

``sc_dt.sc_context[T]`` owns a native context as a member, since native contexts
prohibit ordinary heap allocation. ``sc_fxtype_context`` and ``sc_fxcast_context``
select the corresponding parameter types. The constructor copies its parameters;
``SC_NOW`` activates immediately and ``SC_LATER`` activates on ``begin()``.
``end()`` or destruction restores the preceding default. End or destroy nested
contexts in reverse order, in the originating native process. References returned
by ``value()`` and ``default_value()`` require their owning context to remain alive.
Copying contexts is disabled by native SystemC.

Public event finders and process suspension
--------------------------------------------

``sc_event_finder_t[IF](port, IF.event_method)`` constructs an owning native finder
for arbitrary native interface types. Reflected event methods avoid cppyy's
member-function-pointer conversion limitation. Overloaded and inherited methods
select the exact zero-argument const event signature. For example::

   IF = sc_core.sc_signal_in_if[sc_dt.sc_biguint[137]]
   finder = sc_core.sc_event_finder_t[IF](port, IF.value_changed_event)
   event = finder.find_event()

The finder borrows its port, interface and returned event. Keep those objects
alive through all sensitivity/wait consumers. Static sensitivity borrows the
finder itself until elaboration completes. Retain a direct finder, or its cached
finder and cache owner, through that boundary; store it on the module rather
than passing a temporary finder. Once elaboration resolves sensitivity to the
native event, the event and its channel must still outlive the process.
Explicit interface lookup retains native dynamic-cast checks; an incompatible or unbound interface reports E118.
If the report handler continues, the result is ``sc_event.none``.

``sc_event_finder_cache()`` supplies an owning pointer slot for
``sc_event_finder.cached_create[IF](cache, port, IF.event_method)``. Calls allocate
once and reuse the same finder, retaining the first event method. All uses of a
cache must reference the same port, as required by the native assertion. Returned
finder proxies borrow and retain the cache; destroy consumers before releasing
its native storage.

A running Python thread may suspend itself with
``sc_get_current_process_handle().suspend()``. The bridge releases the GIL while
the native kernel switches pthreads, then preserves reset/kill unwind and Python
cleanup. Resume, reset and kill during suspension match native timing, including
reset and termination event delivery. Suspension before simulation or while
paused retains native behavior. Descendant inclusion policies preserve native
control propagation. Sticky synchronous reset controls release the GIL and
preserve Python thread cleanup.

``handle.throw_it(native_exception, descendants=SC_NO_DESCENDANTS)`` injects a
copyable native C++ exception into a thread process. Python threads can catch
the native exception class around a guarded wait, inspect its values and wait
again. Injection releases the GIL while target callbacks acquire it; descendant
injection preserves native ordering. Python exception instances raise
``TypeError``. Native process restrictions and uncaught-exception reports apply.

The base interface and mutex/semaphore families now have complete declaration
checks, including protected construction/state/report members exercised through
native-derived models. Ports borrow interfaces and channels; retain their Python
callback and event owners until consumers stop and release their views.


Native vectors and member assemblies
------------------------------------

``sc_vector[Element](prefix, count, creator)`` and ``vector.init(count, creator)``
accept native creator functors or synchronous Python callables taking an element
name and index. Element types are instantiated on demand. Python creators must
return newly allocated native elements: the vector owns their allocations and
retains their Python callback hosts. Do not delete those elements separately.
Creator failures propagate and native partial initialization deletes elements
already created; initialization can then be retried under native elaboration rules.

``vector.assemble("port")`` and ``sc_assemble_vector(vector, "port")`` construct
native member pointers for public data members. Native member-pointer arguments
also remain accepted. Assemblies retain their known Python vector owner across
copy construction, assignment, self-assignment and swap. Views created entirely
in native code remain borrowed unless a Python owner is attached. Keep bound
channels alive through port destruction, and retain owners while using iterators.

Assembly ``bind(first, last, destination)`` and ``assembly(first, last, destination)``
consume the source range starting at the supplied destination iterator. Binding
stops when either range ends and returns the next destination iterator. A short
source leaves the remaining destinations unbound; a short destination leaves
unused source entries untouched. Source subranges and nonzero destination
positions preserve these rules. Bind any required remaining ports before
elaboration; the range operation does not relax their binding policy.

``sc_member_access[Element, Member]("member")`` creates a native access policy;
policy copies and mutable-to-const conversions preserve native type rules.
``sc_direct_access`` remains a native template. Iterator ``__follow__()`` invokes
native ``operator->`` and returns a borrowed element or scalar member pointer.
For a scalar pointer, index zero accesses the member. Range checks and diagnostics
remain native; dereferencing end or invalid iterators has native undefined behavior.

Native callable binding
-----------------------

``sc_bind(native_callable, *arguments)`` supports native functions, functors and
typed ``std::function`` objects. ``sc_placeholder[index]()`` supplies arbitrary
positive, one-based native placeholders, including reordered, repeated and nested
bindings. Native values are copied; ``sc_ref`` and ``sc_cref`` borrow their targets.
Reference results remain borrowed and void results become ``None``. Value results
support copy-only and move-only types; they must be copyable or movable through
the adapter's temporary storage. Bound proxies and input owners are retained
for the kernel lifetime.

Raw Python callables must first be wrapped in an explicitly typed native
``std::function``. Keep the original Python callback alive through all uses of
that wrapper and any spawned process. Guarded waits inside bound Python thread
callbacks preserve reset/kill unwinding and cleanup. Direct binding calls hold
the GIL; simulation callbacks acquire it at Python entry.


Hash containers and callback ownership
--------------------------------------

Native hash tables retain Python hash and comparison callbacks supplied to
constructors or ``set_hash_fn`` / ``set_cmpr_fn`` until their Python table proxy
is released. Calls hold the GIL and callbacks execute synchronously. Views of
tables created in native code require an external callback owner. Iterators,
returned key pointers and content pointers borrow their table and referents.
Retain those owners while using the views; removal and erasure invalidate nodes.

``sc_phash_base`` borrows keys and contents. Its explicit duplication/free
overloads manage key allocations only when requested. ``sc_pdhash[Key, Content]``
uses configured duplicate/free functions; ``sc_strhash[Content]`` owns copied
string keys. Content pointees remain borrowed in all three families. Use the
explicit ``copy`` methods between distinct tables to preserve each family's
key-copy semantics. Template types are instantiated on demand. Lookup and
removal output parameters retain their native pointer-slot signatures.


Static sensitivity builders
---------------------------

Module-owned ``sc_sensitive`` builders accept events, interfaces, ports and
event finders through both ``builder(source)`` and ``builder << source``.
Process-handle insertion selects the process; ``reset()`` clears that selection.
``make_static_sensitivity(process_pointer, source)`` attaches sensitivity directly
to a native process. Keep the process and every event source alive while the
simulation uses them.

The legacy positive and negative builders accept bool/logic input interfaces,
input ports and inout ports through either operator. Their native process-handle,
method-handle and thread-handle selectors retain native type semantics. Clocked
``sc_sensitive`` helpers take a native clocked-thread handle plus an event finder
or bool/logic interface or port. Builders borrow their module and sources;
Python process registration retains callbacks for the native process lifetime.
