"""Compare unique-name scope selection and copied return strings with native SystemC."""

from amal.eda.systemc import sc_core as sc

records = []


def generate(scope):
    """Verify an independent counter, preserve-first behavior and error recovery."""
    values = [
        sc.sc_gen_unique_name("scope_item"),
        sc.sc_gen_unique_name("scope_item", False),
        sc.sc_gen_unique_name("scope_first", True),
        sc.sc_gen_unique_name("scope_first", True),
    ]
    assert values == ["scope_item_0", "scope_item_1", "scope_first", "scope_first_1"], values
    try:
        sc.sc_gen_unique_name("")
    except Exception as error:
        assert "unique name" in str(error)
    else:
        raise AssertionError("empty name accepted")
    next_name = sc.sc_gen_unique_name("scope_item")
    assert next_name == "scope_item_2" and values[0] == "scope_item_0"
    records.append("UNIQUE_NAME_REFERENCE " + scope + " " + " ".join(values + [next_name]))


generate("global")
for module_label in ("first_module", "second_module"):
    name = sc.sc_module_name(module_label)
    model = sc.sc_module(name)
    generate(module_label)
    if module_label == "first_module":
        model.sc_thread("worker_a", lambda: generate("worker_a"))
        model.sc_thread("worker_b", lambda: generate("worker_b"))
        first_model = model
    del name
assert sc.sc_gen_unique_name("scope_item") == "scope_item_3"
sc.sc_start(1, sc.SC_NS)
assert sc.sc_gen_unique_name("scope_item") == "scope_item_4"
sc.sc_stop()
assert len(records) == 5
for record in sorted(records):
    print(record)
