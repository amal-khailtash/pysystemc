"""Compare native free bitwise, complement, negation and compound operations."""

from pathlib import Path
import gc
from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_free_operations_probe.h"))
probe = cppyy.gbl.datatype_free_probe


def emit(label, value):
    """Emit one native value record without retaining borrowed references."""
    if isinstance(value, dt.sc_bit):
        text = value.to_char()
    elif isinstance(value, (dt.sc_lv_base, dt.sc_bv_base)):
        text = str(value.to_string())
    else:
        text = str(value.to_string(dt.SC_BIN))
    print("DT_FREE", label, text)


output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = dt.sc_bit(0)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = dt.sc_bit(1)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = dt.sc_bit(0)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = dt.sc_bit(1)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|11", output)
emit("b_and|const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", dt.b_and(dt.sc_bit(0), dt.sc_bit(0)))
emit("b_and|const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", dt.b_and(dt.sc_bit(0), dt.sc_bit(1)))
emit("b_and|const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", dt.b_and(dt.sc_bit(1), dt.sc_bit(0)))
emit("b_and|const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", dt.b_and(dt.sc_bit(1), dt.sc_bit(1)))
emit("b_and|bool,const sc_dt::sc_bit &|00", dt.b_and(False, dt.sc_bit(0)))
emit("b_and|bool,const sc_dt::sc_bit &|01", dt.b_and(False, dt.sc_bit(1)))
emit("b_and|bool,const sc_dt::sc_bit &|10", dt.b_and(True, dt.sc_bit(0)))
emit("b_and|bool,const sc_dt::sc_bit &|11", dt.b_and(True, dt.sc_bit(1)))
output = dt.sc_bit(1)
left = 0
right = dt.sc_bit(0)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = 0
right = dt.sc_bit(1)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = 1
right = dt.sc_bit(0)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = 1
right = dt.sc_bit(1)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|11", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = "0"
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = "1"
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = "0"
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = "1"
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|11", output)
emit("b_and|const sc_dt::sc_bit &,int|00", dt.b_and(dt.sc_bit(0), 0))
emit("b_and|const sc_dt::sc_bit &,int|01", dt.b_and(dt.sc_bit(0), 1))
emit("b_and|const sc_dt::sc_bit &,int|10", dt.b_and(dt.sc_bit(1), 0))
emit("b_and|const sc_dt::sc_bit &,int|11", dt.b_and(dt.sc_bit(1), 1))
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|11", output)
output = dt.sc_bit(1)
left = False
right = dt.sc_bit(0)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = False
right = dt.sc_bit(1)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = True
right = dt.sc_bit(0)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = True
right = dt.sc_bit(1)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|11", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|00", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|01", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|10", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|11", output)
emit("b_and|int,const sc_dt::sc_bit &|00", dt.b_and(0, dt.sc_bit(0)))
emit("b_and|int,const sc_dt::sc_bit &|01", dt.b_and(0, dt.sc_bit(1)))
emit("b_and|int,const sc_dt::sc_bit &|10", dt.b_and(1, dt.sc_bit(0)))
emit("b_and|int,const sc_dt::sc_bit &|11", dt.b_and(1, dt.sc_bit(1)))
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = 0
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = 1
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = 0
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = 1
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|11", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|11", output)
emit("b_and|const sc_dt::sc_bit &,bool|00", dt.b_and(dt.sc_bit(0), False))
emit("b_and|const sc_dt::sc_bit &,bool|01", dt.b_and(dt.sc_bit(0), True))
emit("b_and|const sc_dt::sc_bit &,bool|10", dt.b_and(dt.sc_bit(1), False))
emit("b_and|const sc_dt::sc_bit &,bool|11", dt.b_and(dt.sc_bit(1), True))
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|11", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = False
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = True
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = False
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = True
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|11", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|11", output)
emit("b_and|const sc_dt::sc_bit &,char|00", dt.b_and(dt.sc_bit(0), "0"))
emit("b_and|const sc_dt::sc_bit &,char|01", dt.b_and(dt.sc_bit(0), "1"))
emit("b_and|const sc_dt::sc_bit &,char|10", dt.b_and(dt.sc_bit(1), "0"))
emit("b_and|const sc_dt::sc_bit &,char|11", dt.b_and(dt.sc_bit(1), "1"))
emit("b_and|char,const sc_dt::sc_bit &|00", dt.b_and("0", dt.sc_bit(0)))
emit("b_and|char,const sc_dt::sc_bit &|01", dt.b_and("0", dt.sc_bit(1)))
emit("b_and|char,const sc_dt::sc_bit &|10", dt.b_and("1", dt.sc_bit(0)))
emit("b_and|char,const sc_dt::sc_bit &|11", dt.b_and("1", dt.sc_bit(1)))
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|00", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|01", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|10", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|11", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|11", output)
output = dt.sc_bit(1)
left = "0"
right = dt.sc_bit(0)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = "0"
right = dt.sc_bit(1)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = "1"
right = dt.sc_bit(0)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = "1"
right = dt.sc_bit(1)
assert dt.b_and(output, left, right) is None
emit("b_and|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|11", output)
output = dt.sc_bit(1)
left = "0"
right = dt.sc_bit(0)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = "0"
right = dt.sc_bit(1)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = "1"
right = dt.sc_bit(0)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = "1"
right = dt.sc_bit(1)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|11", output)
emit("b_or|const sc_dt::sc_bit &,bool|00", dt.b_or(dt.sc_bit(0), False))
emit("b_or|const sc_dt::sc_bit &,bool|01", dt.b_or(dt.sc_bit(0), True))
emit("b_or|const sc_dt::sc_bit &,bool|10", dt.b_or(dt.sc_bit(1), False))
emit("b_or|const sc_dt::sc_bit &,bool|11", dt.b_or(dt.sc_bit(1), True))
emit("b_or|const sc_dt::sc_bit &,char|00", dt.b_or(dt.sc_bit(0), "0"))
emit("b_or|const sc_dt::sc_bit &,char|01", dt.b_or(dt.sc_bit(0), "1"))
emit("b_or|const sc_dt::sc_bit &,char|10", dt.b_or(dt.sc_bit(1), "0"))
emit("b_or|const sc_dt::sc_bit &,char|11", dt.b_or(dt.sc_bit(1), "1"))
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|11", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = "0"
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = "1"
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = "0"
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = "1"
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|11", output)
emit("b_or|int,const sc_dt::sc_bit &|00", dt.b_or(0, dt.sc_bit(0)))
emit("b_or|int,const sc_dt::sc_bit &|01", dt.b_or(0, dt.sc_bit(1)))
emit("b_or|int,const sc_dt::sc_bit &|10", dt.b_or(1, dt.sc_bit(0)))
emit("b_or|int,const sc_dt::sc_bit &|11", dt.b_or(1, dt.sc_bit(1)))
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = dt.sc_bit(0)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = dt.sc_bit(1)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = dt.sc_bit(0)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = dt.sc_bit(1)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|00", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|01", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|10", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|11", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|11", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|11", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = False
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = True
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = False
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = True
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|11", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|11", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|00", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|01", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|10", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|11", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|11", output)
emit("b_or|bool,const sc_dt::sc_bit &|00", dt.b_or(False, dt.sc_bit(0)))
emit("b_or|bool,const sc_dt::sc_bit &|01", dt.b_or(False, dt.sc_bit(1)))
emit("b_or|bool,const sc_dt::sc_bit &|10", dt.b_or(True, dt.sc_bit(0)))
emit("b_or|bool,const sc_dt::sc_bit &|11", dt.b_or(True, dt.sc_bit(1)))
emit("b_or|const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", dt.b_or(dt.sc_bit(0), dt.sc_bit(0)))
emit("b_or|const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", dt.b_or(dt.sc_bit(0), dt.sc_bit(1)))
emit("b_or|const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", dt.b_or(dt.sc_bit(1), dt.sc_bit(0)))
emit("b_or|const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", dt.b_or(dt.sc_bit(1), dt.sc_bit(1)))
output = dt.sc_bit(1)
left = False
right = dt.sc_bit(0)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = False
right = dt.sc_bit(1)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = True
right = dt.sc_bit(0)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = True
right = dt.sc_bit(1)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|11", output)
emit("b_or|char,const sc_dt::sc_bit &|00", dt.b_or("0", dt.sc_bit(0)))
emit("b_or|char,const sc_dt::sc_bit &|01", dt.b_or("0", dt.sc_bit(1)))
emit("b_or|char,const sc_dt::sc_bit &|10", dt.b_or("1", dt.sc_bit(0)))
emit("b_or|char,const sc_dt::sc_bit &|11", dt.b_or("1", dt.sc_bit(1)))
emit("b_or|const sc_dt::sc_bit &,int|00", dt.b_or(dt.sc_bit(0), 0))
emit("b_or|const sc_dt::sc_bit &,int|01", dt.b_or(dt.sc_bit(0), 1))
emit("b_or|const sc_dt::sc_bit &,int|10", dt.b_or(dt.sc_bit(1), 0))
emit("b_or|const sc_dt::sc_bit &,int|11", dt.b_or(dt.sc_bit(1), 1))
output = dt.sc_bit(1)
left = 0
right = dt.sc_bit(0)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = 0
right = dt.sc_bit(1)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = 1
right = dt.sc_bit(0)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = 1
right = dt.sc_bit(1)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|11", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|11", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = 0
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = 1
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = 0
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = 1
assert dt.b_or(output, left, right) is None
emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|11", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = False
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = True
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = False
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = True
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|11", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|11", output)
emit("b_xor|char,const sc_dt::sc_bit &|00", dt.b_xor("0", dt.sc_bit(0)))
emit("b_xor|char,const sc_dt::sc_bit &|01", dt.b_xor("0", dt.sc_bit(1)))
emit("b_xor|char,const sc_dt::sc_bit &|10", dt.b_xor("1", dt.sc_bit(0)))
emit("b_xor|char,const sc_dt::sc_bit &|11", dt.b_xor("1", dt.sc_bit(1)))
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|11", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|00", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|01", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|10", output)
output = probe.fixed_value[dt.sc_ufix_fast](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|11", output)
emit("b_xor|int,const sc_dt::sc_bit &|00", dt.b_xor(0, dt.sc_bit(0)))
emit("b_xor|int,const sc_dt::sc_bit &|01", dt.b_xor(0, dt.sc_bit(1)))
emit("b_xor|int,const sc_dt::sc_bit &|10", dt.b_xor(1, dt.sc_bit(0)))
emit("b_xor|int,const sc_dt::sc_bit &|11", dt.b_xor(1, dt.sc_bit(1)))
output = dt.sc_bit(1)
left = 0
right = dt.sc_bit(0)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = 0
right = dt.sc_bit(1)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = 1
right = dt.sc_bit(0)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = 1
right = dt.sc_bit(1)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|11", output)
emit("b_xor|const sc_dt::sc_bit &,bool|00", dt.b_xor(dt.sc_bit(0), False))
emit("b_xor|const sc_dt::sc_bit &,bool|01", dt.b_xor(dt.sc_bit(0), True))
emit("b_xor|const sc_dt::sc_bit &,bool|10", dt.b_xor(dt.sc_bit(1), False))
emit("b_xor|const sc_dt::sc_bit &,bool|11", dt.b_xor(dt.sc_bit(1), True))
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](5.75)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix_fast](-3.25)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|11", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|00", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](5.75)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|01", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|10", output)
output = probe.fixed_value[dt.sc_fix_fast](-3.25)
left = probe.fixed_value[dt.sc_fix_fast](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|11", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|11", output)
emit("b_xor|const sc_dt::sc_bit &,int|00", dt.b_xor(dt.sc_bit(0), 0))
emit("b_xor|const sc_dt::sc_bit &,int|01", dt.b_xor(dt.sc_bit(0), 1))
emit("b_xor|const sc_dt::sc_bit &,int|10", dt.b_xor(dt.sc_bit(1), 0))
emit("b_xor|const sc_dt::sc_bit &,int|11", dt.b_xor(dt.sc_bit(1), 1))
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = 0
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = 1
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = 0
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = 1
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|11", output)
emit("b_xor|bool,const sc_dt::sc_bit &|00", dt.b_xor(False, dt.sc_bit(0)))
emit("b_xor|bool,const sc_dt::sc_bit &|01", dt.b_xor(False, dt.sc_bit(1)))
emit("b_xor|bool,const sc_dt::sc_bit &|10", dt.b_xor(True, dt.sc_bit(0)))
emit("b_xor|bool,const sc_dt::sc_bit &|11", dt.b_xor(True, dt.sc_bit(1)))
emit("b_xor|const sc_dt::sc_bit &,char|00", dt.b_xor(dt.sc_bit(0), "0"))
emit("b_xor|const sc_dt::sc_bit &,char|01", dt.b_xor(dt.sc_bit(0), "1"))
emit("b_xor|const sc_dt::sc_bit &,char|10", dt.b_xor(dt.sc_bit(1), "0"))
emit("b_xor|const sc_dt::sc_bit &,char|11", dt.b_xor(dt.sc_bit(1), "1"))
output = dt.sc_bit(1)
left = "0"
right = dt.sc_bit(0)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = "0"
right = dt.sc_bit(1)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = "1"
right = dt.sc_bit(0)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = "1"
right = dt.sc_bit(1)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|11", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = dt.sc_bit(0)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = dt.sc_bit(1)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = dt.sc_bit(0)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = dt.sc_bit(1)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", output)
emit("b_xor|const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", dt.b_xor(dt.sc_bit(0), dt.sc_bit(0)))
emit("b_xor|const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", dt.b_xor(dt.sc_bit(0), dt.sc_bit(1)))
emit("b_xor|const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", dt.b_xor(dt.sc_bit(1), dt.sc_bit(0)))
emit("b_xor|const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", dt.b_xor(dt.sc_bit(1), dt.sc_bit(1)))
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|00", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](5.75)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|01", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|10", output)
output = probe.fixed_value[dt.sc_ufix](-3.25)
left = probe.fixed_value[dt.sc_ufix](-3.25)
right = probe.fixed_value[dt.sc_ufix_fast](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|11", output)
output = dt.sc_bit(1)
left = False
right = dt.sc_bit(0)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|00", output)
output = dt.sc_bit(1)
left = False
right = dt.sc_bit(1)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|01", output)
output = dt.sc_bit(1)
left = True
right = dt.sc_bit(0)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|10", output)
output = dt.sc_bit(1)
left = True
right = dt.sc_bit(1)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|11", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|00", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](5.75)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|01", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](5.75)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|10", output)
output = probe.fixed_value[dt.sc_fix](-3.25)
left = probe.fixed_value[dt.sc_fix](-3.25)
right = probe.fixed_value[dt.sc_fix_fast](-3.25)
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|11", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = "0"
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|00", output)
output = dt.sc_bit(1)
left = dt.sc_bit(0)
right = "1"
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|01", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = "0"
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|10", output)
output = dt.sc_bit(1)
left = dt.sc_bit(1)
right = "1"
assert dt.b_xor(output, left, right) is None
emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|11", output)
output = dt.sc_fxval(7.5)
source = probe.fixed_value[dt.sc_fix](1.25)
assert dt.neg(output, source) is None
assert output.to_double() == -1.25
emit("neg|sc_fxval|sc_fix|1.25", output)
output = dt.sc_fxval(7.5)
source = probe.fixed_value[dt.sc_fix](-3.75)
assert dt.neg(output, source) is None
assert output.to_double() == 3.75
emit("neg|sc_fxval|sc_fix|-3.75", output)
output = dt.sc_fxval_fast(7.5)
source = probe.fixed_value[dt.sc_fix_fast](1.25)
assert dt.neg(output, source) is None
assert output.to_double() == -1.25
emit("neg|sc_fxval_fast|sc_fix_fast|1.25", output)
output = dt.sc_fxval_fast(7.5)
source = probe.fixed_value[dt.sc_fix_fast](-3.75)
assert dt.neg(output, source) is None
assert output.to_double() == 3.75
emit("neg|sc_fxval_fast|sc_fix_fast|-3.75", output)
output = probe.fixed_value[dt.sc_fix](7.5)
source = probe.fixed_value[dt.sc_fix](1.25)
assert dt.neg(output, source) is None
assert output.to_double() == -1.25
emit("neg|sc_fix|sc_fix|1.25", output)
output = probe.fixed_value[dt.sc_fix](7.5)
source = probe.fixed_value[dt.sc_fix](-3.75)
assert dt.neg(output, source) is None
assert output.to_double() == 3.75
emit("neg|sc_fix|sc_fix|-3.75", output)
output = probe.fixed_value[dt.sc_fix_fast](7.5)
source = probe.fixed_value[dt.sc_fix_fast](1.25)
assert dt.neg(output, source) is None
assert output.to_double() == -1.25
emit("neg|sc_fix_fast|sc_fix_fast|1.25", output)
output = probe.fixed_value[dt.sc_fix_fast](7.5)
source = probe.fixed_value[dt.sc_fix_fast](-3.75)
assert dt.neg(output, source) is None
assert output.to_double() == 3.75
emit("neg|sc_fix_fast|sc_fix_fast|-3.75", output)
output = dt.sc_fxval(7.5)
source = dt.sc_fxval(1.25)
assert dt.neg(output, source) is None
assert output.to_double() == -1.25
emit("neg|sc_fxval|sc_fxval|1.25", output)
output = dt.sc_fxval(7.5)
source = dt.sc_fxval(-3.75)
assert dt.neg(output, source) is None
assert output.to_double() == 3.75
emit("neg|sc_fxval|sc_fxval|-3.75", output)
output = dt.sc_fxval_fast(7.5)
source = dt.sc_fxval_fast(1.25)
assert dt.neg(output, source) is None
assert output.to_double() == -1.25
emit("neg|sc_fxval_fast|sc_fxval_fast|1.25", output)
output = dt.sc_fxval_fast(7.5)
source = dt.sc_fxval_fast(-3.75)
assert dt.neg(output, source) is None
assert output.to_double() == 3.75
emit("neg|sc_fxval_fast|sc_fxval_fast|-3.75", output)
source = dt.sc_bit(False)
result = ~source
del source
gc.collect()
assert result.to_bool()
emit("complement|sc_bit", result)
source = dt.sc_signed(191)
source.__assign__("0x100000000000000000000000000000000000000000005a")
result = ~source
del source
gc.collect()
assert result.length() == 191
emit("complement|sc_signed", result)
source = dt.sc_unsigned(191)
source.__assign__("0x100000000000000000000000000000000000000000005a")
result = ~source
del source
gc.collect()
assert result.length() == 192
emit("complement|sc_unsigned", result)
left = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
right = dt.sc_lv_base(
    "10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX0"
)
assert probe.Compound[dt.sc_lv_base, dt.sc_lv_base].apply(0, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
returned = ordinary.__iand__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_lv_base|sc_lv_base|0", left)
left = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
right = dt.sc_lv_base(
    "10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX0"
)
assert probe.Compound[dt.sc_lv_base, dt.sc_lv_base].apply(1, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
returned = ordinary.__ior__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_lv_base|sc_lv_base|1", left)
left = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
right = dt.sc_lv_base(
    "10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX0"
)
assert probe.Compound[dt.sc_lv_base, dt.sc_lv_base].apply(2, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
returned = ordinary.__ixor__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_lv_base|sc_lv_base|2", left)
left = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
right = dt.sc_bv_base(
    "10101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010100"
)
assert probe.Compound[dt.sc_lv_base, dt.sc_bv_base].apply(0, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
returned = ordinary.__iand__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_lv_base|sc_bv_base|0", left)
left = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
right = dt.sc_bv_base(
    "10101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010100"
)
assert probe.Compound[dt.sc_lv_base, dt.sc_bv_base].apply(1, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
returned = ordinary.__ior__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_lv_base|sc_bv_base|1", left)
left = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
right = dt.sc_bv_base(
    "10101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010100"
)
assert probe.Compound[dt.sc_lv_base, dt.sc_bv_base].apply(2, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_lv_base(
    "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ1"
)
returned = ordinary.__ixor__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_lv_base|sc_bv_base|2", left)
left = dt.sc_bv_base(
    "01010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101011"
)
right = dt.sc_bv_base(
    "10101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010100"
)
assert probe.Compound[dt.sc_bv_base, dt.sc_bv_base].apply(0, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_bv_base(
    "01010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101011"
)
returned = ordinary.__iand__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_bv_base|sc_bv_base|0", left)
left = dt.sc_bv_base(
    "01010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101011"
)
right = dt.sc_bv_base(
    "10101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010100"
)
assert probe.Compound[dt.sc_bv_base, dt.sc_bv_base].apply(1, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_bv_base(
    "01010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101011"
)
returned = ordinary.__ior__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_bv_base|sc_bv_base|1", left)
left = dt.sc_bv_base(
    "01010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101011"
)
right = dt.sc_bv_base(
    "10101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010100"
)
assert probe.Compound[dt.sc_bv_base, dt.sc_bv_base].apply(2, left, right) == cppyy.addressof(left)
expected = str(left.to_string())
ordinary = dt.sc_bv_base(
    "01010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101011"
)
returned = ordinary.__ixor__(right)
assert cppyy.addressof(returned) == cppyy.addressof(ordinary)
assert str(ordinary.to_string()) == expected
emit("compound|sc_bv_base|sc_bv_base|2", left)
print("SC_DT_FREE_OPERATIONS_PUBLIC_OK")
