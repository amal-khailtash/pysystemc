"""Run the counter testbench through the conventional tutorial entrypoint name."""

from amal.eda.systemc import main

from counter_tb import sc_main

__all__ = ["sc_main"]


if __name__ == "__main__":
    main()
