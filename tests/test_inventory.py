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


def test_inventory_configurations_and_review_contract(tmp_path):
    """Compare real conditional APIs and reject unsupported or stale audit claims."""
    import runpy

    import pytest

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    snapshots = {}
    for configuration in ("baseline", "modeling"):
        output = tmp_path / f"{configuration}.json"
        result = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools/inventory_public_api.py"),
                "--configuration",
                configuration,
                "--output",
                str(output),
            ],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert result.returncode == 0, result.stdout + result.stderr
        snapshots[configuration] = json.loads(output.read_text())
    entries = snapshots["modeling"]["declarations"]
    baseline = snapshots["baseline"]["declarations"]
    assert snapshots["baseline"]["configuration"] == ["C++17"]
    assert any(entry["name"] == "sc_core::sc_spawn" for entry in entries)
    # Convenience socket/PEQ headers enable dynamic processes even in baseline.
    assert any(entry["name"] == "sc_core::sc_spawn" for entry in baseline)
    assert "SC_INCLUDE_DYNAMIC_PROCESSES" in snapshots["baseline"]["effective_defines"]
    assert any(entry["name"] == "sc_dt::sc_fixed" for entry in entries)
    assert not any(entry["name"] == "sc_dt::sc_fixed" for entry in baseline)
    assert any(entry["bases"] for entry in entries if entry["name"] == "tlm::tlm_get_if")
    assert any(parameter["has_default"] for entry in entries for parameter in entry["parameters"])
    common = {entry["id"] for entry in baseline} & {entry["id"] for entry in entries}
    assert common
    entry = next(entry for entry in entries if entry["id"] in common)
    moved = {**entry, "line": entry["line"] + 10, "offset": 0}
    assert api["declaration_id"](moved) == entry["id"]
    for review in (
        {"classification": "internal"},
        {"classification": "public", "status": "supported", "rationale": "Public header"},
        {"status": "invented"},
        {"unexpected": True},
    ):
        with pytest.raises(ValueError):
            api["apply_reviews"](entries, {entry["id"]: review})
    with pytest.raises(ValueError, match="Unknown declaration"):
        api["apply_reviews"](entries, {"stale": {}})
    review = {
        "classification": "public",
        "status": "supported",
        "rationale": "Public modeling declaration",
        "python": "example spelling",
        "ownership": "borrowed",
        "gil": "held",
        "errors": "native exceptions",
        "evidence": ["tests/test_inventory.py"],
    }
    review_file = tmp_path / "reviews.json"
    review_file.write_text(json.dumps({entry["id"]: review}))
    output = tmp_path / "reviewed.json"
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "tools/inventory_public_api.py"),
            "--reviews",
            str(review_file),
            "--output",
            str(output),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    reviewed = next(row for row in json.loads(output.read_text())["declarations"] if row["id"] == entry["id"])
    assert all(reviewed[key] == value for key, value in review.items())
