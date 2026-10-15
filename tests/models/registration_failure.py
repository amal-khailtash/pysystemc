"""Verify failed native conversions release callbacks without kernel mutation."""

import gc
import weakref
from amal.eda.systemc import sc_core as sc


class Callback:
    """Callable with a measurable Python lifetime."""

    def __call__(self):
        """Reject any accidental execution of a failed registration."""
        raise AssertionError("failed callback executed")


module_name = sc.sc_module_name("registration")
model = sc.sc_module(module_name)
del module_name
seen = []
model.sc_method("valid", lambda: seen.append("valid"))
children = len(model.get_child_objects())
for kind in ("sc_method", "sc_thread", "sc_cthread", "spawn"):
    callback = Callback()
    reference = weakref.ref(callback)
    try:
        if kind == "spawn":
            sc.sc_spawn(callback, "bad", object())
        elif kind == "sc_cthread":
            model.sc_cthread("bad", callback, object())
        else:
            getattr(model, kind)(object(), callback)
    except TypeError:
        pass
    else:
        raise AssertionError("invalid native arguments accepted")
    del callback
    gc.collect()
    assert reference() is None, kind
    assert len(model._process_callbacks) == 1
    assert len(model.get_child_objects()) == children
sc.sc_start(1, sc.SC_NS)
assert seen == ["valid"], seen
sc.sc_stop()
print("REGISTRATION_FAILURE_OK")
