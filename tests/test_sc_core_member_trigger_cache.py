"""Regress member trigger error recovery, borrowed lifetimes and wait unwind."""

import gc
import importlib.util
from pathlib import Path
import sys
import types
import weakref

ROOT = Path(__file__).resolve().parents[1]


class Unwind(BaseException):
    """Represent a native scheduler cleanup request in this controlled fixture."""


class Argument:
    """Provide a mutable object with an unrestricted native type spelling."""

    __cpp_name__ = "fixture::Argument<257>"

    def __init__(self, value):
        """Store a mutable value without retaining any owner."""
        self.value = value


class Owner:
    """Represent a borrowed native module or channel owner."""


class Operation:
    """Return current argument values, with controllable native exit modes."""

    def __init__(self):
        """Start successful without retaining arguments."""
        self.mode = "success"
        self.current = None
        self.owner_identity = None

    def __call__(self, owner, *values):
        """Read current values, then fail, unwind or complete without retaining them."""
        self.current = tuple(value.value for value in values)
        self.owner_identity = id(owner)
        if self.mode == "error":
            raise ValueError("controlled native failure")
        return self.mode != "unwind"


class Dispatch:
    """Count exact template selection and provide separate native callables."""

    def __init__(self):
        """Start without specializations or selection history."""
        self.selected = []
        self.operations = {}

    def __getitem__(self, key):
        """Resolve a native callable by exact owner and argument types."""
        self.selected.append(key)
        operation = self.operations.setdefault(key, Operation())
        return types.SimpleNamespace(invoke=operation)


def test_member_next_trigger_cache_contract(monkeypatch):
    """Preserve native failures, current values and lifetimes across cache hits."""
    wait, trigger = Dispatch(), Dispatch()
    monkeypatch.setitem(sys.modules, "cppyy", types.SimpleNamespace(
        gbl=types.SimpleNamespace(sc_core=types.SimpleNamespace(
            python_member_wait=wait, python_member_next_trigger=trigger))))
    spec = importlib.util.spec_from_file_location("next_trigger_contract", ROOT / "src/amal/eda/systemc/sc_core/_member_scheduler.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    invoke = module._scheduling_member(Owner, "next_trigger", Unwind)
    owner, argument = Owner(), Argument(1)
    key = (Owner, Argument.__cpp_name__)
    operation = trigger.operations.setdefault(key, Operation())
    operation.mode = "error"
    for _ in range(2):
        try:
            invoke(owner, argument)
        except ValueError:
            pass
        else:
            raise AssertionError("cold native error swallowed")
    assert len(trigger.selected) == 2
    operation.mode = "success"
    assert invoke(owner, argument) is None
    assert len(trigger.selected) == 3 and not operation.__release_gil__
    argument.value = 9
    invoke(owner, argument)
    assert len(trigger.selected) == 3 and operation.current == (9,)
    operation.mode = "error"
    try:
        invoke(owner, argument)
    except ValueError:
        pass
    else:
        raise AssertionError("warm native error swallowed")
    assert len(trigger.selected) == 3
    operation.mode = "success"
    invoke(owner, argument)
    another = module._scheduling_member(Owner, "next_trigger", Unwind)
    another(owner, argument)
    assert len(trigger.selected) == 4

    class OtherArgument(Argument):
        """Provide a distinct unrestricted native signature."""
        __cpp_name__ = "fixture::Argument<513>"

    other = OtherArgument(513)
    invoke(owner, other)
    assert len(trigger.selected) == 5 and trigger.operations[(Owner, other.__cpp_name__)].current == (513,)
    invoke(owner)
    invoke(owner)
    assert len(trigger.selected) == 6
    for value in (object(), "invalid"):
        try:
            invoke(owner, value)
        except TypeError:
            pass
        else:
            raise AssertionError("invalid argument accepted")
    assert len(trigger.selected) == 6
    second_owner = Owner()
    invoke(second_owner, argument)
    assert operation.owner_identity == id(second_owner)
    invoke(owner, argument)
    assert operation.owner_identity == id(owner)
    assert len(trigger.selected) == 6

    class ChannelOwner:
        """Represent a separate native owner type with a distinct closure."""

    channel_owner = ChannelOwner()
    channel_call = module._scheduling_member(ChannelOwner, "next_trigger", Unwind)
    channel_call(channel_owner, argument)
    channel_operation = trigger.operations[(ChannelOwner, Argument.__cpp_name__)]
    assert channel_operation is not operation
    assert channel_operation.owner_identity == id(channel_owner)
    assert len(trigger.selected) == 7
    channel_call(channel_owner, argument)
    assert len(trigger.selected) == 7

    wait_call = module._scheduling_member(Owner, "wait", Unwind)
    wait_call(owner, argument)
    wait_call(owner, argument)
    assert len(wait.selected) == 2 and wait.operations[key].__release_gil__
    wait.operations[key].mode = "unwind"
    try:
        wait_call(owner, argument)
    except Unwind:
        pass
    else:
        raise AssertionError("baseline wait unwind changed")
    assert len(wait.selected) == 3
    refs = [weakref.ref(value) for value in (owner, second_owner, channel_owner, argument, other)]
    del owner, second_owner, channel_owner, argument, other
    gc.collect()
    assert all(ref() is None for ref in refs)
