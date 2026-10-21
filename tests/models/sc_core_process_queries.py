"""Compare public process queries at construction, callback and pause boundaries."""

from amal.eda.systemc import sc_core as sc

records = []
entries = 0
cleanups = []


def observe(label, expected):
    """Verify current raw pointer/handle identity, kind and non-unwind state."""
    kind = sc.sc_get_curr_process_kind()
    raw = sc.sc_get_curr_process_handle()
    handle = sc.sc_get_current_process_handle()
    assert kind == expected and not sc.sc_is_unwinding()
    if expected == sc.SC_NO_PROC_:
        assert not raw
    else:
        assert raw and handle.valid()
        assert raw.name() == handle.name()
        assert raw.proc_kind() == kind
    records.append(
        f"PROCESS_QUERY_REFERENCE {label} {int(kind)} {int(bool(raw))} {int(handle.valid())} {int(sc.sc_is_unwinding())}"
    )


observe("before", sc.SC_NO_PROC_)
name = sc.sc_module_name("queries")
model = sc.sc_module(name)
model.sc_method("method", lambda: observe("method", sc.SC_METHOD_PROC_))
method = sc.sc_get_last_created_process_handle()
assert sc.sc_get_last_created_process_handle().name() == "queries.method"
model.sc_thread("thread", lambda: observe("thread", sc.SC_THREAD_PROC_))
thread = sc.sc_get_last_created_process_handle()
assert sc.sc_get_last_created_process_handle().name() == "queries.thread"
assert sc.sc_get_current_process_handle().name() == "queries.thread"
del name
observe("registered", sc.SC_NO_PROC_)


def unwind_worker():
    """Observe native unwind state during guarded reset and kill cleanup."""
    global entries
    entries += 1
    try:
        sc.wait(100, sc.SC_NS)
    finally:
        state = sc.sc_is_unwinding()
        assert state and sc.sc_get_current_process_handle().is_unwinding()
        cleanups.append(state)
        records.append(f"PROCESS_UNWIND_REFERENCE {entries} {int(state)}")


def controller():
    """Reset then kill the worker at precise native timestamps."""
    sc.wait(1, sc.SC_NS)
    worker.reset()
    sc.wait(1, sc.SC_NS)
    worker.kill()


worker = sc.sc_spawn(unwind_worker, "unwind_worker")
control = sc.sc_spawn(controller, "controller")
sc.sc_start(3, sc.SC_NS)
assert cleanups == [True, True] and worker.terminated() and control.terminated()
observe("paused", sc.SC_NO_PROC_)
assert thread.terminated() and not method.terminated()
sc.sc_stop()
observe("stopped", sc.SC_NO_PROC_)
for record in sorted(records):
    print(record)
