"""Compare every public typed PD and string hash declaration with C++."""

from pathlib import Path

from amal.eda.systemc import sc_core as _sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_hash_typed_probe.h"))
probe = cppyy.gbl.hash_typed_probe
assert _sc.default_str_hash_fn(b"") == 0
assert probe.pd_hash.iterator.__cpp_name__ == probe.pd_iter.__cpp_name__
assert probe.str_hash.iterator.__cpp_name__ == probe.str_iter.__cpp_name__
print("HASH_ALIASES|1|1")
default_pd = probe.pd_hash()
default_str = probe.str_hash()
default_pd_empty = default_pd.empty()
default_str_empty = default_str.empty()
default_pd.erase()
default_str.erase()
print(f"HASH_CONSTRUCTORS|{int(default_pd_empty)}|{int(default_str_empty)}")


def pointer_value(value):
    """Return the address represented by an opaque native pointer."""
    if not value:
        return 0
    return int(probe.content_value(value))


probe.reset_counts()
with_pd = probe.owned_pd_hash()
alpha, beta, gamma = (cppyy.gbl.std.string(text) for text in ("alpha", "beta", "gamma"))
pd_results = [
    with_pd.insert(alpha.data(), probe.content_pointer(0x101)),
    with_pd.insert(beta.data(), probe.content_pointer(0x202)),
    with_pd.insert(gamma.data(), probe.content_pointer(0x303)),
]
pd_results.extend(
    (
        with_pd.insert(beta.data(), probe.content_pointer(0x204)),
        with_pd.insert_if_not_exists(beta.data(), probe.content_pointer(0x205)),
    )
)
copied_pd = probe.owned_pd_hash()
copied_pd.copy(with_pd)
pd_source_beta = pointer_value(with_pd["beta"])
pd_copied_beta = pointer_value(copied_pd["beta"])
pd_copied_count = int(copied_pd.count())
pd_null_lookup = with_pd.lookup("beta", None) and not with_pd.lookup("missing", None)
assert probe.duplicate_count == 6
alpha.assign("changed")
beta.assign("changed")
gamma.assign("changed")
assert with_pd.contains("alpha") and not with_pd.contains("changed")
pd_base = cppyy.bind_object(cppyy.addressof(copied_pd), probe.pd_hash)
pd_iter_type = probe.pd_iter
pd_pointer_iter = pd_iter_type.__new__(pd_iter_type)
pd_iter_type.__init__.__overload__("sc_core::sc_pdhash<char*,int*>*")(pd_pointer_iter, pd_base)
pd_pointer_iterator = not pd_pointer_iter.empty()
pd_iter = pd_iter_type.__new__(pd_iter_type)
pd_iter_type.__init__.__overload__("sc_core::sc_pdhash<char*,int*> &")(pd_iter, pd_base)
while not pd_iter.empty():
    if pointer_value(pd_iter.contents()) == 0x303:
        pd_iter.remove()
    else:
        if str(probe.key_text(pd_iter.key())) == "beta":
            pd_iter.set_contents(probe.content_pointer(0x206))
        pd_iter.step()
pd_iter.reset.__overload__("sc_core::sc_pdhash<char*,int*> &")(pd_base)
pd_reference_reset = not pd_iter.empty()
pd_iter.reset.__overload__("sc_core::sc_pdhash<char*,int*>*")(pd_base)
pd_pointer_reset = not pd_iter.empty()
pd_removed_alpha = copied_pd.remove("alpha")
print(
    "HASH_PD_RESULTS|"
    + "|".join(
        map(
            str,
            (
                *pd_results,
                pd_source_beta,
                pd_copied_beta,
                pd_copied_count,
                int(pd_null_lookup),
                int(probe.duplicate_count),
            ),
        )
    )
)
print(
    "HASH_PD_MUTATION|"
    + "|".join(
        map(
            str,
            (pd_removed_alpha, int(copied_pd.count()), pointer_value(copied_pd["beta"]), int(probe.free_count)),
        )
    )
)
print(f"HASH_PD_ITERATOR|{int(pd_pointer_iterator)}|{int(pd_reference_reset)}|{int(pd_pointer_reset)}")
assert int(probe.free_count) == 2
with_pd.erase()
assert int(probe.free_count) == 5
del copied_pd
assert int(probe.duplicate_count) == int(probe.free_count) == 6
print(f"HASH_PD_OWNERSHIP|{int(probe.duplicate_count)}|{int(probe.free_count)}")

probe.reset_counts()
pd_defaults = probe.owned_pd_hash()
default_key = cppyy.gbl.std.string("default")
pd_default_insert = pd_defaults.insert(default_key.data())
pd_default_existing = pd_defaults.insert_if_not_exists(default_key.data())
pd_default_value = pointer_value(pd_defaults[default_key.data()])
pd_defaults.erase()
print(
    f"HASH_PD_DEFAULT|{pd_default_insert}|{pd_default_existing}|{pd_default_value}|"
    f"{int(probe.duplicate_count)}|{int(probe.free_count)}"
)

probe.reset_counts()
pd_outputs = probe.owned_pd_hash()
output_key = cppyy.gbl.std.string("output")
pd_outputs.insert(output_key.data(), probe.content_pointer(0x404))
pd_lookup_output = probe.pd_slots()
pd_found = probe.lookup_pd(pd_outputs, output_key.data(), pd_lookup_output)
pd_lookup_value = pointer_value(pd_lookup_output.contents)
pd_remove_output = probe.pd_slots()
pd_out_removed = probe.remove_pd(pd_outputs, output_key.data(), pd_remove_output)
pd_out_key = str(probe.key_text(pd_remove_output.key))
pd_out_value = pointer_value(pd_remove_output.contents)
probe.free_pd_output_key(pd_remove_output)
pd_outputs.erase()
print(
    f"HASH_PD_OUT|{pd_found}|{pd_lookup_value}|{pd_out_removed}|{pd_out_key}|{pd_out_value}|"
    f"{int(pd_outputs.count())}|{int(probe.duplicate_count)}|{int(probe.free_count)}"
)

probe.reset_counts()
pd_removals = probe.owned_pd_hash()
for key, value in (("one", 0x501), ("two", 0x502), ("three", 0x503)):
    pd_removals.insert(key, probe.content_pointer(value))
pd_direct_removed = pd_removals.remove_by_contents(probe.content_pointer(0x502))
pd_predicate_removed = pd_removals.remove_by_contents(probe.content_matches, probe.content_pointer(0x503))
pd_remaining = int(pd_removals.count())
pd_removals.erase()
print(
    f"HASH_PD_REMOVE_CONTENTS|{pd_direct_removed}|{pd_predicate_removed}|{pd_remaining}|"
    f"{int(probe.duplicate_count)}|{int(probe.free_count)}"
)

with_str = probe.str_hash()
alpha, beta, gamma = (cppyy.gbl.std.string(text) for text in ("alpha", "beta", "gamma"))
str_results = [
    probe.insert_string(with_str, alpha.data(), probe.content_pointer(0x111)),
    probe.insert_string(with_str, beta.data(), probe.content_pointer(0x222)),
    probe.insert_string(with_str, gamma.data(), probe.content_pointer(0x333)),
    probe.insert_string(with_str, beta.data(), probe.content_pointer(0x224)),
    probe.insert_string_if_absent(with_str, beta.data(), probe.content_pointer(0x225)),
]
copied_str = probe.str_hash()
copied_str.copy(with_str)
copied_str_pointer = probe.str_hash()
source_str_pointer = cppyy.bind_object(cppyy.addressof(with_str), probe.str_hash)
copied_str_pointer.copy.__overload__("const sc_core::sc_strhash<int*>*")(source_str_pointer)
str_source_beta = pointer_value(with_str["beta"])
str_copied_beta = pointer_value(copied_str["beta"])
str_pointer_beta = pointer_value(copied_str_pointer["beta"])
str_copied_count = int(copied_str.count())
str_pointer_count = int(copied_str_pointer.count())
str_null_lookup = with_str.lookup("beta", None) and not with_str.lookup("missing", None)
alpha.assign("changed")
beta.assign("changed")
gamma.assign("changed")
assert with_str.contains("alpha") and not with_str.contains("changed")
str_base = cppyy.bind_object(cppyy.addressof(copied_str), probe.str_hash)
str_iter_type = probe.str_iter
str_pointer_iter = str_iter_type.__new__(str_iter_type)
str_iter_type.__init__.__overload__("sc_core::sc_strhash<int*>*")(str_pointer_iter, str_base)
str_pointer_iterator = not str_pointer_iter.empty()
str_iter = str_iter_type.__new__(str_iter_type)
str_iter_type.__init__.__overload__("sc_core::sc_strhash<int*> &")(str_iter, str_base)
while not str_iter.empty():
    if pointer_value(str_iter.contents()) == 0x333:
        str_iter.remove()
    else:
        if str(probe.key_text(str_iter.key())) == "beta":
            str_iter.set_contents(probe.content_pointer(0x226))
        str_iter.step()
str_iter.reset.__overload__("sc_core::sc_strhash<int*> &")(str_base)
str_reference_reset = not str_iter.empty()
str_iter.reset.__overload__("sc_core::sc_strhash<int*>*")(str_base)
str_pointer_reset = not str_iter.empty()
str_removed_alpha = copied_str.remove("alpha")
print(
    "HASH_STR_RESULTS|"
    + "|".join(
        map(
            str,
            (
                *str_results,
                str_source_beta,
                str_copied_beta,
                str_copied_count,
                str_pointer_beta,
                str_pointer_count,
                int(str_null_lookup),
            ),
        )
    )
)
print(
    "HASH_STR_MUTATION|"
    + "|".join(map(str, (str_removed_alpha, int(copied_str.count()), pointer_value(copied_str["beta"]))))
)
print(f"HASH_STR_ITERATOR|{int(str_pointer_iterator)}|{int(str_reference_reset)}|{int(str_pointer_reset)}")
copied_str.erase()
copied_str_pointer.erase()
with_str.erase()

str_defaults = probe.str_hash()
str_default_insert = probe.insert_string(str_defaults, "default", cppyy.nullptr)
str_default_existing = probe.insert_string_if_absent(str_defaults, "default", cppyy.nullptr)
str_default_value = pointer_value(str_defaults["default"])
str_defaults.erase()
print(f"HASH_STR_DEFAULT|{str_default_insert}|{str_default_existing}|{str_default_value}")

str_outputs = probe.str_hash()
probe.insert_string(str_outputs, "output", probe.content_pointer(0x414))
str_lookup_output = probe.str_slots()
str_found = probe.lookup_string(str_outputs, "output", str_lookup_output)
str_lookup_value = pointer_value(str_lookup_output.contents)
str_remove_output = probe.str_slots()
str_out_removed = probe.remove_string(str_outputs, "output", str_remove_output)
str_out_key = str(probe.key_text(str_remove_output.key))
str_out_value = pointer_value(str_remove_output.contents)
probe.free_string_output_key(str_remove_output)
str_outputs.erase()
print(
    f"HASH_STR_OUT|{str_found}|{str_lookup_value}|{str_out_removed}|{str_out_key}|{str_out_value}|"
    f"{int(str_outputs.count())}"
)

str_removals = probe.str_hash()
for key, value in (("one", 0x601), ("two", 0x602), ("three", 0x603)):
    probe.insert_string(str_removals, key, probe.content_pointer(value))
str_direct_removed = str_removals.remove_by_contents(probe.content_pointer(0x602))
str_predicate_removed = str_removals.remove_by_contents(probe.content_matches, probe.content_pointer(0x603))
str_remaining = int(str_removals.count())
str_removals.erase()
print(f"HASH_STR_REMOVE_CONTENTS|{str_direct_removed}|{str_predicate_removed}|{str_remaining}")
