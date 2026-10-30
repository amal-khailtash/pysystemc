"""Regression checks for Clang's elided source filenames in AST locations."""

from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[1]


def _location(path, contents, line, *, include_from=None, include_file=True):
    """Build a Clang-style ordinary source location at the start of a line."""
    lines = contents.splitlines(keepends=True)
    offset = sum(len(value) for value in lines[: line - 1])
    result = {"offset": offset, "line": line, "col": 1, "tokLen": 1}
    if include_file:
        result["file"] = str(path)
    if include_from is not None:
        result["includedFrom"] = {"file": str(include_from)}
    return result


def test_restore_source_locations_in_clang_serialization_order(tmp_path):
    """Restore elided locations through siblings, macro locations, ranges and locationless nodes."""
    api = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))
    spelling_path = tmp_path / "spelling.hpp"
    expansion_path = tmp_path / "expansion.hpp"
    main_path = tmp_path / "main.cpp"
    spelling_text = "first\nmacro spelling\n"
    expansion_text = "a substantially longer first line\nmacro expansion\n"
    main_path.write_text('#include "spelling.hpp"\n#include "expansion.hpp"\n')
    spelling_path.write_text(spelling_text)
    expansion_path.write_text(expansion_text)
    spelling_bytes = spelling_path.read_bytes()
    expansion_bytes = expansion_path.read_bytes()

    def spelling(line, *, include_file=True):
        """Build a location in the macro-definition header."""
        return _location(
            spelling_path,
            spelling_text,
            line,
            include_from=main_path,
            include_file=include_file,
        )

    def expansion(line, *, include_file=True):
        """Build a location in the macro-invocation header."""
        return _location(
            expansion_path,
            expansion_text,
            line,
            include_from=main_path,
            include_file=include_file,
        )

    def omitted(line):
        """Build an expansion point whose path must be restored from the stream."""
        return spelling(line, include_file=False)

    root = {
        "kind": "TranslationUnitDecl",
        "inner": [
            {
                "kind": "NamespaceDecl",
                "name": "ns",
                "loc": spelling(1),
                "inner": [
                    {
                        "kind": "CXXRecordDecl",
                        "name": "OrdinarySibling",
                        "loc": spelling(2, include_file=False),
                        "inner": [],
                    },
                    {
                        "kind": "ClassTemplateDecl",
                        "name": "IncludedType",
                        "loc": expansion(1),
                        "inner": [],
                    },
                    {
                        "kind": "FunctionTemplateDecl",
                        "name": "different_files",
                        "loc": {
                            "spellingLoc": spelling(2),
                            "expansionLoc": expansion(2),
                        },
                        "inner": [],
                    },
                    {
                        "kind": "CXXRecordDecl",
                        "name": "RangeOnly",
                        # A range can move the stream cursor even when the node
                        # has no ordinary `loc` field.
                        "range": {"begin": spelling(1), "end": spelling(2)},
                        "inner": [],
                    },
                    {
                        "kind": "CXXRecordDecl",
                        "name": "LocationlessOwner",
                        "inner": [
                            {
                                "kind": "FunctionDecl",
                                "name": "after_range",
                                "loc": {
                                    "spellingLoc": omitted(2),
                                    "expansionLoc": omitted(2),
                                },
                                "type": {"qualType": "void ()"},
                            }
                        ],
                    },
                    {
                        "kind": "ClassTemplateDecl",
                        "name": "AfterRange",
                        "loc": expansion(1),
                        "inner": [],
                    },
                    {
                        "kind": "FunctionTemplateDecl",
                        "name": "elided_expansion",
                        "loc": {
                            # A later explicit sibling moves the cursor to B;
                            # serialized spelling then restores the elided site.
                            "spellingLoc": spelling(2),
                            "expansionLoc": omitted(2),
                        },
                        "inner": [],
                    },
                ],
            }
        ],
    }

    api["restore_source_files"]([root])
    namespace = root["inner"][0]
    ordinary_sibling = namespace["inner"][0]
    different_files = namespace["inner"][2]
    range_only = namespace["inner"][3]
    locationless_owner = namespace["inner"][4]
    after_range = locationless_owner["inner"][0]
    elided_expansion = namespace["inner"][6]

    assert ordinary_sibling["loc"]["file"] == str(spelling_path)
    assert different_files["loc"]["spellingLoc"]["file"] == str(spelling_path)
    assert different_files["loc"]["expansionLoc"]["file"] == str(expansion_path)
    assert different_files["loc"]["expansionLoc"]["file"] != main_path.as_posix()
    assert range_only["range"]["end"]["file"] == str(spelling_path)
    assert after_range["loc"]["expansionLoc"]["file"] == str(spelling_path)
    assert after_range["loc"]["spellingLoc"]["file"] == str(spelling_path)
    assert elided_expansion["loc"]["spellingLoc"]["file"] == str(spelling_path)
    assert elided_expansion["loc"]["expansionLoc"]["file"] == str(spelling_path)

    # Check offset/line pairs against the resolved file and show why the stale
    # previously visited header could not be accepted for the elided expansion.
    expansion_offset = elided_expansion["loc"]["expansionLoc"]["offset"]
    expansion_line = elided_expansion["loc"]["expansionLoc"]["line"]
    assert spelling_bytes[:expansion_offset].count(b"\n") + 1 == expansion_line
    assert expansion_bytes[:expansion_offset].count(b"\n") + 1 != expansion_line

    identity = {
        "name": "ns::f",
        "kind": "FunctionDecl",
        "signature": "void ()",
        "template_parameters": [],
        "template_arguments": [],
        "template_context": [],
        "source": str(spelling_path),
    }
    wrong_path_identity = {**identity, "source": str(expansion_path)}
    assert api["declaration_id"](identity) != api["declaration_id"](wrong_path_identity)
