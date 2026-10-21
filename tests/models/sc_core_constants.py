"""Read all inventoried public native scalar/string/enum constants."""

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
