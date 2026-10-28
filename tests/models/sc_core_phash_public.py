"""Compare every generic and inventoried specialized native pointer-hash contract."""

from pathlib import Path
from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_phash_public_probe.h"))
probe = cppyy.gbl.phash_public_probe


def configured(table_type, default):
    """Construct a table borrowing contents and comparing cloned keys by value."""
    return table_type(default, 11, 2, 2.0, False, probe.hash_key, probe.compare_key)


def exercise(value_type, label):
    """Verify full table and iterator overloads for a live native content type."""
    F = probe.fixture[value_type]
    T = sc.sc_phash["void*", f"const {value_type}*"]
    iterator_type = sc.sc_phash_iter["void*", f"const {value_type}*"]
    assert T.iterator.__cpp_name__ == iterator_type.__cpp_name__
    empty = T()
    assert empty.empty()
    del empty
    slots = F()
    table = configured(T, F.value(0))
    insert = [
        table.insert(probe.key(1)),
        table.insert(probe.key(2), F.value(1)),
        table.insert(probe.key(2), F.value(2)),
        table.insert_if_not_exists(probe.key(2), F.value(0)),
        table.insert_if_not_exists(probe.key(3)),
    ]
    assert insert == [0, 0, 1, 1, 0]
    assert table.lookup(probe.key(2), slots.content_slot()) == 1 and slots.output_index() == 2
    assert table.lookup(probe.key(7), slots.content_slot()) == 0 and slots.output_index() == 0
    assert table.lookup(probe.key(2), None) == 1 and table.lookup(probe.key(7), cppyy.nullptr) == 0
    assert table.contains(probe.key(2)) and not table.contains(probe.key(7))
    assert F.value_index(table[probe.key(1)]) == F.value_index(table[probe.key(7)]) == 0
    iterator = iterator_type.__new__(iterator_type)
    iterator_type.__init__.__overload__(T.__cpp_name__ + "*")(iterator, table)
    collected = []
    while not iterator.empty():
        number = probe.key_value(iterator.key())
        collected.append(number)
        if number == 1:
            assert F.value_index(iterator.set_contents(F.value(1))) == 1
        assert F.value_index(iterator.contents()) in (0, 1, 2)
        iterator.step()
    assert sorted(collected) == [1, 2, 3]
    iterator.reset.__overload__(T.__cpp_name__ + "&")(table)
    assert not iterator.empty()
    iterator.reset.__overload__(T.__cpp_name__ + "*")(table)
    assert not iterator.empty()
    reference = iterator_type.__new__(iterator_type)
    iterator_type.__init__.__overload__(T.__cpp_name__ + "&")(reference, table)
    assert not reference.empty()
    del iterator, reference
    removed = table.remove(probe.key(2), slots.key_slot(), slots.content_slot())
    assert removed == 1 and slots.output_key() == 2 and slots.output_index() == 2
    assert table.remove(probe.key(2), slots.key_slot(), slots.content_slot()) == 0
    assert slots.output_key() == slots.output_index() == -1
    assert table.remove(probe.key(3)) == 1 and table.count() == 1
    pointer_copy, reference_copy = T(), T()
    pointer_copy.copy.__overload__("const " + T.__cpp_name__ + "*")(table)
    reference_copy.copy.__overload__("const " + T.__cpp_name__ + "&")(table)
    assert pointer_copy.count() == reference_copy.count() == 1
    assert F.value_index(pointer_copy[probe.key(1)]) == F.value_index(reference_copy[probe.key(1)]) == 1
    print(
        "PHASH_BORROWED",
        label,
        *insert,
        removed,
        len(collected),
        table.count(),
        pointer_copy.count(),
        reference_copy.count(),
    )
    for destination in (table, pointer_copy, reference_copy):
        destination.erase()
    for number, value in [(1, 1), (2, 1), (3, 2)]:
        table.insert(probe.key(number), F.value(value))
    direct_removed = table.remove_by_contents(F.value(1))
    predicate_removed = table.remove_by_contents(probe.same_content, F.argument(2))
    assert direct_removed == 2 and predicate_removed == 1 and table.empty()
    print("PHASH_CONTENTS", label, direct_removed, predicate_removed, table.count())
    probe.reset_counts()
    owned, copied = configured(T, F.value(0)), configured(T, F.value(0))
    assert owned.insert(probe.key(4), F.value(1), probe.duplicate_key) == 0
    assert owned.insert_if_not_exists(probe.key(4), F.value(0), probe.duplicate_key) == 1
    assert owned.insert_if_not_exists(probe.key(5), F.value(2), probe.duplicate_key) == 0
    assert owned.remove(probe.key(4), probe.free_key) == 1
    owned.insert(probe.key(4), F.value(1), probe.duplicate_key)
    copied.insert(probe.key(6), F.value(0), probe.duplicate_key)
    copied.copy(owned, probe.duplicate_key, probe.free_key)
    copied.copy(owned, probe.duplicate_key, probe.free_key)
    assert copied.count() == 2 and F.value_index(copied[probe.key(4)]) == 1
    assert copied.remove_by_contents(F.argument(1), probe.free_key) == 1
    assert copied.remove_by_contents(probe.same_content, F.argument(2), probe.free_key) == 1
    owned.erase(probe.free_key)
    copied.erase(probe.free_key)
    assert probe.duplicates == probe.frees == 8
    print("PHASH_OWNERSHIP", label, int(probe.duplicates), int(probe.frees), owned.count(), copied.count())


for value_type, label in [
    ("phash_public_probe::payload", "custom"),
    ("sc_dt::sc_length_param", "length"),
    ("sc_dt::sc_fxtype_params", "fixed"),
    ("sc_dt::sc_fxcast_switch", "cast"),
]:
    exercise(value_type, label)
