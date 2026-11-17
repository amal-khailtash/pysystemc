"""Check real native policy spelling and the helper's malformed-proxy guard."""

import os
import subprocess
import sys

from .support import ROOT


def test_native_nested_policy_spelling_and_forged_proxy_rejection(tmp_path):
    """Exercise cppyy specializations before isolating the forged spelling case."""
    source = r"""
from amal.eda.systemc import sc_core as sc
import cppyy
from amal.eda.systemc.sc_core._vector_policies import _split_policy

nested_element = cppyy.gbl.std.pair[int, cppyy.gbl.std.vector[int]]
nested_member = cppyy.gbl.std.vector[int]
facade_policy = sc.sc_member_access[nested_element, nested_member]
native_policy = cppyy.gbl.sc_core.sc_member_access[nested_element, nested_member]
assert facade_policy.__cpp_name__ == native_policy.__cpp_name__
assert facade_policy.element_type.__cpp_name__ == native_policy.element_type.__cpp_name__
assert _split_policy(native_policy) == (
    native_policy.element_type.__cpp_name__,
    native_policy.type.__cpp_name__,
)

const_policy = facade_policy.const_policy
native_const_policy = native_policy.const_policy
assert const_policy.__cpp_name__ == native_const_policy.__cpp_name__
assert const_policy.type.__cpp_name__ == native_const_policy.type.__cpp_name__
assert const_policy.non_const_policy.__cpp_name__ == native_const_policy.non_const_policy.__cpp_name__

element = nested_element()
element.first = 7
element.second.push_back(11)
member = facade_policy("second")
selected = member.get(element)
assert cppyy.addressof(selected) == cppyy.addressof(element.second)
assert selected[0] == 11

# This metadata cannot come from cppyy's template specialization path above.
class ForgedNativeType:
    __cpp_name__ = "sc_core::sc_member_access<std::vector<int>>"

try:
    _split_policy(ForgedNativeType)
except TypeError as error:
    assert str(error) == "expected a native member policy specialization"
else:
    raise AssertionError("malformed forged metadata was accepted")

print("VECTOR_POLICY_SPELLING_OK")
"""
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [sys.executable, "-c", source],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "VECTOR_POLICY_SPELLING_OK" in result.stdout
