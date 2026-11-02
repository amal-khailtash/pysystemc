"""Read selected top-level native scalar, string, and enum constants.

The selection excludes mutable bools, the logic table and the bind proxy.
Separate source declarations of one symbol produce separate comparison rows.
"""

import csv
from pathlib import Path
from amal.eda.systemc import sc_core as sc

inventory = Path(__file__).resolve().parents[2] / "docs/_static/public-api-inventory.csv"
count = 0
for row in csv.DictReader(inventory.open()):
    name, signature = row["name"], row["signature"]
    if not name.startswith("sc_core::") or name.count("::") != 1 or row["kind"] not in ("EnumConstantDecl", "VarDecl"):
        continue
    if signature in ("bool", "const sc_dt::sc_logic_value_t[4][4]", "const sc_core::sc_bind_proxy"):
        continue
    value = getattr(sc, name.split("::")[1])
    if row["kind"] == "EnumConstantDecl" or signature in ("const bool", "const char"):
        value = ord(value) if isinstance(value, str) else int(value)
    elif signature == "const double":
        value = format(value, ".17g")
    elif signature == "const sc_core::sc_time":
        value = value.to_string()
    print(f"CORE_CONSTANT {name}={value}")
    count += 1
print("CORE_CONSTANTS_OK", count)

# Named enum members retain native type identity, including the status alias.
ENUM_MEMBERS = {
    "sc_curr_proc_kind": ["SC_CTHREAD_PROC_", "SC_METHOD_PROC_", "SC_NO_PROC_", "SC_THREAD_PROC_"],
    "sc_descendant_inclusion_info": ["SC_INCLUDE_DESCENDANTS", "SC_INVALID_DESCENDANTS", "SC_NO_DESCENDANTS"],
    "sc_port_policy": ["SC_ALL_BOUND", "SC_ONE_OR_MORE_BOUND", "SC_ZERO_OR_MORE_BOUND"],
    "sc_severity": ["SC_ERROR", "SC_FATAL", "SC_INFO", "SC_MAX_SEVERITY", "SC_WARNING"],
    "sc_starvation_policy": ["SC_EXIT_ON_STARVATION", "SC_RUN_TO_TIME"],
    "sc_status": [
        "SC_BEFORE_END_OF_ELABORATION",
        "SC_BEFORE_TIMESTEP",
        "SC_ELABORATION",
        "SC_END_OF_ELABORATION",
        "SC_END_OF_INITIALIZATION",
        "SC_END_OF_SIMULATION",
        "SC_END_OF_UPDATE",
        "SC_PAUSED",
        "SC_RUNNING",
        "SC_START_OF_SIMULATION",
        "SC_STATUS_ANY",
        "SC_STATUS_LAST",
        "SC_STOPPED",
        "SC_UNITIALIZED",
    ],
    "sc_stop_mode": ["SC_STOP_FINISH_DELTA", "SC_STOP_IMMEDIATE"],
    "sc_time_unit": ["SC_FS", "SC_MS", "SC_NS", "SC_PS", "SC_SEC", "SC_US"],
    "sc_verbosity": ["SC_DEBUG", "SC_FULL", "SC_HIGH", "SC_LOW", "SC_MEDIUM", "SC_NONE"],
    "sc_writer_policy": ["SC_MANY_WRITERS", "SC_ONE_WRITER", "SC_UNCHECKED_WRITERS"],
}
for enum_name, members in ENUM_MEMBERS.items():
    enum_type = getattr(sc, "sc_status_type" if enum_name == "sc_status" else enum_name)
    for member in members:
        value = getattr(sc, member)
        assert isinstance(value, enum_type), (enum_name, member)
        constructed = enum_type(int(value))
        assert isinstance(constructed, enum_type)
        assert int(constructed) == int(value)
print("CORE_ENUM_TYPES_OK", len(ENUM_MEMBERS), sum(map(len, ENUM_MEMBERS.values())))
