"""Check pointer-vector index rejection before unsafe native growth."""

from amal.eda.systemc import sc_core as sc
import cppyy


class SlotIndex:
    """Provide a Python integer-index object accepted by slot access."""

    def __index__(self):
        """Select an existing slot without changing the vector."""
        return 0


vector = sc.sc_pvector["sc_core::sc_time*"]()
owner = sc.sc_time(7, sc.SC_NS)
vector[0] = owner
limit = int(cppyy.gbl.std.numeric_limits["unsigned int"].max())
assert vector[SlotIndex()] == owner
for index, expected in (
    (-1, IndexError),
    (1.5, TypeError),
    (limit, OverflowError),
    (limit + 1, OverflowError),
    (1 << 137, OverflowError),
):
    for mode in ("get", "set"):
        try:
            if mode == "get":
                vector[index]
            else:
                vector[index] = owner
        except expected:
            pass
        else:
            raise AssertionError(f"{mode} accepted unsafe index {index}")
        assert vector.size() == 1
        assert vector.fetch(0) == owner
assert not vector[2]
assert vector.size() == 3
assert vector[0] == owner
print("PVECTOR_INDEX_LIMITS_OK rejected=10 unchanged=1 null_growth=1")
