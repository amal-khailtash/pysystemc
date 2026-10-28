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
    assert inventory["schema_version"] == 4
    assert set(inventory["utility_headers"]) == {
        path.name for path in (ROOT / "submodules/systemc/src/tlm_utils").glob("*.h")
    }
    assert any(
        row["definition"].startswith("#define SC_METHOD(") and row["source"].endswith("sc_module.h") and row["line"] > 0
        for row in inventory["macro_declarations"]
    )
    headers = inventory["headers"]
    assert len({row["source"] for row in headers}) == len(headers)
    expected_headers = {path.relative_to(ROOT).as_posix() for path in (ROOT / "submodules/systemc/src").rglob("*.h")}
    assert expected_headers <= {row["source"] for row in headers}
    assert all(row["reached"] for row in headers if row["distribution"] == "installed")
    assert not any(row["distribution"] == "unclassified" for row in headers)
    assert any(row["source"].endswith("sc_host_mutex.h") for row in headers)
    fragments = [row for row in headers if row["source"].endswith(".inc")]
    assert len(fragments) == 6
    assert all(row["distribution"] == "implementation" for row in fragments)
    assert "SC_DT_MIXED_COMMA_OPERATORS" in inventory["effective_defines"]
    assert any(row["definition"].startswith("#define DECL_TRACE_METHOD_A(") for row in inventory["macro_declarations"])
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
    assert named("sc_core::sc_name_gen::gen_unique_name")[0]["access"] == "public"
    assert named("sc_core::sc_name_gen::gen_unique_name")[0]["signature"] == "const char *(const char *, bool)"
    assert not named("tlm::tlm_command::TLM_READ_COMMAND")
    assert named("sc_core::sc_event::DELTA")[0]["access"] == "private"
    assert named("sc_core::sc_sensitive::SC_METHOD_")[0]["access"] == "private"
    assert named("sc_core::sc_process_b::SPAWN_SIM")[0]["access"] == "protected"
    assert named("sc_core::sc_simcontext::phase_notify")[0]["access"] == "private"
    assert named("sc_dt::scfx_rep::not_a_number")[0]["access"] == "private"
    context_allocators = named("sc_dt::sc_context::operator new")
    assert context_allocators
    assert all(row["access"] == "private" and row["classification"] == "internal" for row in context_allocators)
    for type_name in ("sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast"):
        disabled = [
            row for row in named(type_name + "::" + type_name.rsplit("::", 1)[-1]) if row["access"] == "private"
        ]
        assert len(disabled) == 2
        local_type_name = type_name.rsplit("::", 1)[-1]
        qualified_copy = f"void (const {type_name} &)"
        local_copy = f"void (const {local_type_name} &)"
        # Clang versions vary in qualifying injected-class-name constructor arguments.
        assert {row["signature"] for row in disabled} in (
            {"void ()", qualified_copy},
            {"void ()", local_copy},
        )
        assert all(row["classification"] == "internal" for row in disabled)
    assert named("sc_core::sc_time::from_value")[0]["static"] is True
    assert named("sc_core::sc_time::value")[0]["static"] is False
    assert any(entry["kind"] == "ClassTemplateSpecializationDecl" for entry in entries)
    for entry in entries:
        assert not Path(entry["source"]).is_absolute()
        assert entry["line"] and entry["line"] > 0
        assert entry["status"] == "untested"
        assert entry["classification"] in {"internal", "public_candidate"}
        assert entry["rationale"]
        assert entry["mapping_status"] == "unreviewed"
    assert not any(entry["kind"] == "CXXMethodDecl" and len(entry["name"].split("::")) < 3 for entry in entries)
    assert named("sc_core::sc_in::add_trace")
    for alias in ("base_type", "this_type"):
        port_aliases = named(f"sc_core::sc_port::{alias}")
        assert port_aliases
        assert all(row["access"] == "private" and row["classification"] == "internal" for row in port_aliases)
    assert not named("sc_core::add_trace")
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


def test_inventory_friend_specialization_and_atomic_reviews():
    """Keep namespace friends and specialization IDs distinct; reject partial reviews."""
    import runpy

    import pytest

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    source = str(ROOT / "submodules/systemc/src/tlm")
    friend = {
        "kind": "FunctionDecl",
        "name": "visit",
        "loc": {"file": source, "line": 1},
        "type": {"qualType": "void ()"},
    }
    node = {
        "kind": "NamespaceDecl",
        "name": "outer",
        "inner": [
            {
                "kind": "NamespaceDecl",
                "name": "inner",
                "inner": [
                    {
                        "kind": "CXXRecordDecl",
                        "name": "Host",
                        "loc": {"file": source, "line": 1},
                        "inner": [{"kind": "FriendDecl", "inner": [friend]}],
                    }
                ],
            }
        ],
    }
    entries = list(api["declarations"](node))
    assert any(entry["name"] == "outer::inner::visit" for entry in entries)
    pattern = {
        "kind": "ClassTemplateSpecializationDecl",
        "name": "Box",
        "loc": {"file": source, "line": 1},
        "inner": [{"kind": "TemplateArgument", "type": {"qualType": "int"}}],
    }
    first = next(api["declarations"](pattern))
    pattern["inner"][0]["type"]["qualType"] = "double"
    second = next(api["declarations"](pattern))
    assert api["declaration_id"](first) != api["declaration_id"](second)
    for entry in entries:
        entry["id"] = api["declaration_id"](entry)
    before = json.dumps(entries, sort_keys=True)
    with pytest.raises(ValueError):
        api["apply_reviews"](
            entries, {entries[0]["id"]: {"classification": "public", "rationale": "header"}, "stale": {}}
        )
    assert json.dumps(entries, sort_keys=True) == before
    for review in ({"evidence": "file.py"}, {"ownership": 7}, {"gil": ""}):
        with pytest.raises(ValueError):
            api["apply_reviews"](entries, {entries[0]["id"]: review})


def test_specialized_member_identity_retains_enclosing_arguments():
    """Separate identical member signatures in different template specializations."""
    import runpy

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    source = str(ROOT / "submodules/systemc/src/tlm")

    def member(value):
        """Build an AST specialization with an otherwise identical member."""
        return {
            "kind": "ClassTemplateSpecializationDecl",
            "name": "Box",
            "loc": {"file": source, "line": 1},
            "inner": [
                {"kind": "TemplateArgument", "type": {"qualType": value}},
                {
                    "kind": "CXXMethodDecl",
                    "name": "size",
                    "loc": {"file": source, "line": 2},
                    "type": {"qualType": "int () const"},
                },
            ],
        }

    first = list(api["declarations"](member("int")))[1]
    second = list(api["declarations"](member("double")))[1]
    assert first["name"] == second["name"]
    assert api["declaration_id"](first) != api["declaration_id"](second)


def test_out_of_class_owner_and_stream_file_locations():
    """Preserve owner/access and file changes across sibling AST declarations."""
    import runpy

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    first = str(ROOT / "submodules/systemc/src/sysc/kernel/sc_module.h")
    second = str(ROOT / "submodules/systemc/src/sysc/communication/sc_signal_ports.h")
    root = {
        "kind": "NamespaceDecl",
        "name": "sc_core",
        "loc": {"file": first, "line": 1},
        "inner": [
            {
                "kind": "CXXRecordDecl",
                "id": "owner",
                "name": "Port",
                "loc": {"file": second, "line": 2},
                "inner": [
                    {"kind": "AccessSpecDecl", "access": "protected"},
                    {
                        "kind": "CXXMethodDecl",
                        "id": "decl",
                        "name": "hook",
                        "loc": {"line": 3},
                        "type": {"qualType": "void ()"},
                    },
                ],
            },
            {
                "kind": "CXXMethodDecl",
                "id": "definition",
                "previousDecl": "decl",
                "parentDeclContextId": "owner",
                "name": "hook",
                "loc": {"line": 5},
                "type": {"qualType": "void ()"},
            },
        ],
    }
    api["restore_source_files"]([root])
    entries = api["reconcile_declarations"](list(api["declarations"](root)))
    methods = [entry for entry in entries if entry["kind"] == "CXXMethodDecl"]
    assert len(methods) == 2
    assert {entry["name"] for entry in methods} == {"sc_core::Port::hook"}
    assert {entry["access"] for entry in methods} == {"protected"}
    assert {entry["source"] for entry in methods} == {Path(second).relative_to(ROOT).as_posix()}
    assert len({api["declaration_id"](entry) for entry in methods}) == 1
    assert all("ast_id" not in entry for entry in entries)


def test_reconcile_preserves_configuration_and_source_occurrences():
    """Union redeclarations without losing conditional APIs or source evidence."""
    import runpy

    import pytest

    api = runpy.run_path(str(ROOT / "tools/reconcile_api_inventory.py"))
    entry = {
        "id": "one",
        "name": "sc_core::Port::hook",
        "source": "header.h",
        "line": 2,
        "access": "protected",
    }
    first = {
        "schema_version": 4,
        "systemc_revision": "pinned",
        "configuration": ["modeling"],
        "headers": [],
        "macro_declarations": [],
        "declarations": [entry, {**entry, "line": 5}],
    }
    second = {**first, "configuration": ["compatibility"], "declarations": [entry, {**entry, "id": "extra"}]}
    union = api["reconcile"]([first, second])
    assert len(union["declarations"]) == 2
    row = next(row for row in union["declarations"] if row["id"] == "one")
    assert {item["line"] for item in row["occurrences"]} == {2, 5}
    assert row["configurations"] == [["modeling"], ["compatibility"]]
    assert union["families"] == {"sc_core::Port": ["one", "extra"]}
    with pytest.raises(ValueError, match="revisions"):
        api["reconcile"]([first, {**second, "systemc_revision": "different"}])


def test_example_inventory_preserves_fir_variants_and_shared_sources():
    """Retain separate native FIR targets and TLM common-source dependencies."""
    import runpy

    api = runpy.run_path(str(ROOT / "tools/inventory_examples.py"))
    inventory = api["inventory_examples"](
        {"systemc_revision": "pinned", "families": {"sc_core::sc_module": [], "absent::not_a_native_name": []}}
    )
    fir = [row for row in inventory["build_targets"] if row["group"] == "sysc/fir"]
    assert {row["target"] for row in fir} == {"fir", "fir_rtl"}
    lt = next(row for row in inventory["build_targets"] if row["target"] == "lt")
    assert "../common/src/lt_initiator.cpp" in lt["arguments"]
    assert "submodules/systemc/examples/tlm/common/src/lt_initiator.cpp" in lt["resolved_source_files"]
    cpu = next(row for row in inventory["build_targets"] if row["target"] == "risc_cpu")
    assert "submodules/systemc/examples/sysc/risc_cpu/decode.h" in cpu["local_dependency_files"]
    assert cpu["runtime_review"]["required_input_files"] == ["bios.img", "dcache.img", "icache.img", "register.img"]
    mixed = next(row for row in inventory["build_targets"] if row["target"] == "lt_mixed_endian")
    assert mixed["runtime_review"]["configured_stdin"] == "results/input.txt"
    assert mixed["runtime_review"]["output_filter"] == "::"
    assert all(row["runtime_review"]["registered_test"] for row in inventory["build_targets"])
    assert all(not row["runtime_review"]["missing_input_files"] for row in inventory["build_targets"])
    assert all(not row["missing_source_arguments"] for row in inventory["build_targets"])
    assert all(not row["unresolved_quoted_includes"] for row in inventory["build_targets"])
    assert inventory["family_references"]["sc_core::sc_module"]["candidate_sources"]
    assert not inventory["family_references"]["absent::not_a_native_name"]["candidate_sources"]
    assert all(row["native_status"] == "untested" for row in inventory["build_targets"])
    assert all(not Path(path).is_absolute() for path in inventory["source_files"])


def test_remaining_report_distinguishes_mapping_from_semantic_validation(tmp_path):
    """Keep recorded contracts visible while preserving untested semantic scope."""
    import runpy

    api = runpy.run_path(str(ROOT / "tools/reconcile_api_inventory.py"))
    entry = {
        "id": "reviewed",
        "classification": "public",
        "source": "header.h",
        "python": "namespace.Type.method",
        "ownership": "borrowed",
        "gil": "held",
        "errors": "native preconditions",
        "status": "untested",
        "evidence": ["header.h:3"],
    }
    inventory = {
        "systemc_revision": "pinned",
        "fingerprint": "scope",
        "scope": "source review",
        "declarations": [entry],
        "families": {"namespace::Type": ["reviewed"]},
    }
    output = tmp_path / "remaining.md"
    api["write_remaining"](inventory, output)
    report = output.read_text()
    assert "Complete semantic evidence for public overload scope" in report
    assert "Map Python/ownership/GIL/errors" not in report
    assert "namespace.Type.method" in report
    assert "header.h:3" in report


def test_partial_mapping_remains_in_remaining_work(tmp_path):
    """Do not close mapping review merely because placeholder contract fields exist."""
    import runpy

    api = runpy.run_path(str(ROOT / "tools/reconcile_api_inventory.py"))
    entry = {
        "id": "partial",
        "classification": "public",
        "source": "header.h",
        "python": "operator mapping unverified",
        "ownership": "borrowed",
        "gil": "held",
        "errors": "native preconditions",
        "status": "untested",
        "mapping_status": "partial",
        "evidence": [],
    }
    inventory = {
        "systemc_revision": "pinned",
        "fingerprint": "scope",
        "scope": "source review",
        "declarations": [entry],
        "families": {"namespace::Type": ["partial"]},
    }
    output = tmp_path / "remaining.md"
    api["write_remaining"](inventory, output)
    report = output.read_text()
    assert "Resolve explicitly partial binding mappings" in report
    assert "Fill missing Python/ownership/GIL/error contracts" not in report


def test_reviewed_mapping_rejects_incomplete_or_contradictory_records():
    """Require semantic fields for reviewed mappings and reject partial support."""
    import runpy

    import pytest

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    entries = [{"id": "entry"}]
    for review in (
        {"classification": "public", "rationale": "source", "mapping_status": "reviewed"},
        {"classification": "public", "rationale": "source", "mapping_status": "not_applicable"},
        {"classification": "internal", "rationale": "source", "mapping_status": "reviewed"},
        {"classification": "public", "status": "supported", "mapping_status": "partial"},
    ):
        with pytest.raises(ValueError):
            api["apply_reviews"](entries, {"entry": review})
    assert entries == [{"id": "entry"}]


def test_evidence_locations_reject_missing_and_nonportable_sources(tmp_path):
    """Verify source references exist without confusing integrity with support."""
    import runpy

    import pytest

    api = runpy.run_path(str(ROOT / "tools/reconcile_api_inventory.py"))
    (tmp_path / "source.h").write_text("first\nsecond\n")
    api["validate_evidence_locations"]({"one": {"evidence": ["source.h:2"]}}, tmp_path)
    for evidence in ("missing.h", "source.h:0", "source.h:3", "/absolute.h", "../source.h"):
        with pytest.raises(ValueError):
            api["validate_evidence_locations"]({"one": {"evidence": [evidence]}}, tmp_path)


def test_macro_inventory_retains_nonprefixed_declaration_generators():
    """Keep source macros regardless of name while excluding compiler builtins."""
    import runpy

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    source = str(ROOT / "submodules/systemc/src/sysc/tracing/sc_trace.h")
    preprocessed = (
        '# 1 "<built-in>"\n#define INTERNAL_COMPILER 1\n'
        f'# 10 "{source}"\n#define DECL_TRACE_METHOD_A(tp) virtual void trace(tp);\n'
        "#define SC_TRACE_TEST 1\n"
    )
    macros = list(api["macro_declarations"](preprocessed))
    assert len(macros) == 2
    assert macros[0]["definition"].startswith("#define DECL_TRACE_METHOD_A(")
    assert macros[0]["line"] == 10
    assert all(row["source"].endswith("sc_trace.h") for row in macros)


def test_source_directives_preserve_continued_conditions():
    """Keep all conditional operands and physical locations across continuations."""
    import runpy

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    source = "// header\n#if defined(A) || \\\n  defined(B)\n#define VALUE(x) \\\n  ((x) + 1)\n#endif\n"
    assert list(api["source_directives"](source)) == [
        (2, "#if defined(A) || defined(B)"),
        (4, "#define VALUE(x) ((x) + 1)"),
        (6, "#endif"),
    ]


def test_macro_context_preserves_nested_branch_alternatives():
    """Keep prior alternatives for elif/else and reject unbalanced source."""
    import runpy

    import pytest

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    source = "#if A\n#if B\n#define X 1\n#elif C\n#define X 2\n#else\n#define X 3\n#endif\n#endif\n"
    rows = list(api["conditional_macro_sites"](source))
    assert rows[0]["conditional_context"] == [["#if A"], ["#if B"]]
    assert rows[1]["conditional_context"] == [["#if A"], ["#if B", "#elif C"]]
    assert rows[2]["conditional_context"] == [["#if A"], ["#if B", "#elif C", "#else"]]
    for invalid in ("#endif", "#else", "#if A\n#define X 1"):
        with pytest.raises(ValueError):
            list(api["conditional_macro_sites"](invalid))


def test_nested_enum_preserves_private_access():
    """Exclude private kernel enum constants from public access candidates."""
    import runpy

    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    source = str(ROOT / "submodules/systemc/src/tlm")
    node = {
        "kind": "CXXRecordDecl",
        "name": "Host",
        "tagUsed": "class",
        "loc": {"file": source, "line": 1},
        "inner": [
            {
                "kind": "EnumDecl",
                "name": "State",
                "loc": {"file": source, "line": 2},
                "inner": [{"kind": "EnumConstantDecl", "name": "IDLE", "loc": {"file": source, "line": 2}}],
            }
        ],
    }
    rows = list(api["declarations"](node))
    assert next(row for row in rows if row["kind"] == "EnumConstantDecl")["access"] == "private"
