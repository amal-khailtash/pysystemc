"""Check M6A inventory extraction against the pinned native header contract."""

import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def test_source_inventory_overloads_templates_and_locations(tmp_path):
    """Preserve real overloads, template parameters, access and enum spellings."""
    output = tmp_path / "inventory.json"
    result = subprocess.run(
        [sys.executable, str(ROOT / "tools/inventory_public_api.py"), "--output", str(output)],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=120,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    inventory = json.loads(output.read_text())
    entries = inventory["declarations"]

    def named(name):
        """Select a declaration family without using unstable declaration counts."""
        return [entry for entry in entries if entry["name"] == name]

    assert {"T (tlm_tag<T> *)", "void (T &)"} <= {
        entry["signature"] for entry in named("tlm::tlm_blocking_get_if::get")
    }
    assert any(entry["template_parameters"] for entry in named("tlm::tlm_blocking_get_if"))
    assert named("tlm_utils::tlm_quantumkeeper::compute_local_quantum")[0]["access"] == "protected"
    assert len(named("sc_core::sc_module::wait")) == 13
    assert named("tlm::TLM_READ_COMMAND")
    assert not named("tlm::tlm_command::TLM_READ_COMMAND")
    assert any(entry["kind"] == "ClassTemplateSpecializationDecl" for entry in entries)
    for entry in entries:
        assert not Path(entry["source"]).is_absolute()
        assert entry["line"] and entry["line"] > 0
        assert entry["status"] == "untested" and entry["classification"] == "unreviewed"
    extension = named("tlm::tlm_generic_payload::get_extension")
    # Clang versions differ in qualification of types within their own namespace.
    assert any(
        entry["signature"]
        in {
            "tlm::tlm_extension_base *(unsigned int) const",
            "tlm_extension_base *(unsigned int) const",
        }
        for entry in extension
    ), [entry["signature"] for entry in extension]
    assert any(entry["signature"] == "T *() const" for entry in extension)
    assert any(line.startswith("#define SC_METHOD(") for line in inventory["macros"])
