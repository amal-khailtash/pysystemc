"""Compatibility entry for the packaged native SystemC source runner.

Use pysystemc-cpp with source files, optional -I directories, and '--' before
native arguments. Each invocation runs one independent native kernel.
"""

import sys
from amal.eda.systemc._runner import main


if __name__ == "__main__":
    sys.exit(main())
