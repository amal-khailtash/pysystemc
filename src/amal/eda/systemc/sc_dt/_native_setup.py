"""Compile datatype bridges before loading their Python adapter modules.

The parent package has already prepared SystemC and cppyy. Import this module
before modules that register Pythonizations using these native bridge types.
"""

from pathlib import Path

import cppyy

cppyy.include(str(Path(__file__).with_name("adapters.h")))
