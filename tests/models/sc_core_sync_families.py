"""Compare public mutex/semaphore errors, ownership, return values and timing."""

from amal.eda.systemc import sc_core as sc
import cppyy


def report_reference(form, report):
    """Print the preserved native report fields for one invalid constructor."""
    fields = (
        form,
        str(report.get_id()),
        str(int(report.get_severity())),
        report.get_msg_type(),
        report.get_msg(),
        report.what(),
    )
    print("SYNC_INVALID_SEMAPHORE_REFERENCE\t" + "\t".join(fields))


for form, arguments in (("positional", (-1,)), ("named", ("invalid", -1))):
    try:
        sc.sc_semaphore(*arguments)
    except sc.sc_report as error:
        expected_message = error.get_msg()
        if form == "positional":
            prefix = "semaphore 'semaphore_"
            assert expected_message.startswith(prefix) and expected_message.endswith("'")
            assert expected_message[len(prefix) : -1].isdigit()
        else:
            assert expected_message == "semaphore 'invalid'"
        assert error.get_id() == 119
        assert error.get_severity() == sc.SC_ERROR
        assert error.get_msg_type() == "sc_semaphore requires an initial value >= 0"
        assert error.get_msg() == expected_message
        assert "(E119)" in error.what() and expected_message in error.what()
        report_reference(form, error)
    else:
        raise AssertionError(f"negative semaphore constructor accepted for {form} form")
generated_mutex = sc.sc_mutex()
generated_semaphore = sc.sc_semaphore(2)
assert generated_mutex.basename().startswith("mutex_")
assert generated_semaphore.basename().startswith("semaphore_")
assert generated_semaphore.get_value() == 2
name = sc.sc_module_name("sync")
model = sc.sc_module(name)
mutex = sc.sc_mutex("mutex")
semaphore = sc.sc_semaphore("semaphore", 0)
mutex_if = cppyy.bind_object(cppyy.addressof(mutex), sc.sc_mutex_if)
sem_if = cppyy.bind_object(cppyy.addressof(semaphore), sc.sc_semaphore_if)
records = []
assert mutex.kind() == "sc_mutex" and semaphore.kind() == "sc_semaphore"
assert semaphore.get_value() == 0


def holder():
    """Repeated owner locking is idempotent and unlock wakes the contender."""
    assert mutex.lock() == 0 and mutex.trylock() == 0 and mutex.lock() == 0
    sc.wait(2, sc.SC_NS)
    assert mutex.unlock() == 0
    sc.wait(1, sc.SC_NS)
    assert semaphore.post() == 0 and semaphore.get_value() == 1


def contender():
    """Verify non-owner failures, blocking acquisition and semaphore consumption."""
    sc.wait(1, sc.SC_NS)
    assert mutex_if.trylock() == -1 and mutex_if.unlock() == -1
    assert sem_if.trywait() == -1 and sem_if.get_value() == 0
    assert mutex_if.lock() == 0
    records.append(("mutex", sc.sc_time_stamp().value()))
    assert mutex_if.unlock() == 0
    assert sem_if.wait() == 0 and sem_if.get_value() == 0
    records.append(("semaphore", sc.sc_time_stamp().value()))
    assert sem_if.post() == 0 and sem_if.trywait() == 0
    assert sem_if.get_value() == 0
    sc.sc_stop()


model.sc_thread("holder", holder)
model.sc_thread("contender", contender)
del name
sc.sc_start()
assert records == [("mutex", 2000), ("semaphore", 3000)]
for kind, time in records:
    print("SYNC_FAMILY_REFERENCE", kind, time)
