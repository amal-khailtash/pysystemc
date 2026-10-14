"""Run a short SystemC startup check against an installed published package.

Pass the expected distribution version as the first argument. Run outside the
checkout so native resource discovery uses the installed wheel.
"""
import sys
from amal.eda.systemc import __version__, main, sc_core


def sc_main(args: list[str]) -> int:
    """Run one method callback and verify the kernel advances to 1 ns.

    Parameters
    ----------
    args : list[str]
        Native entry-point arguments; unused by this check.

    Returns
    -------
    int
        Zero after successful callback and simulation-time assertions.
    """
    calls = []
    model = sc_core.sc_module('published_check')
    model.sc_method('tick', lambda: calls.append(str(sc_core.sc_time_stamp())))
    sc_core.sc_start(1, sc_core.SC_NS)
    assert calls == ['0 s'], calls
    assert sc_core.sc_time_stamp() == sc_core.sc_time(1, sc_core.SC_NS)
    print('SystemC startup, callback and simulation time passed', flush=True)
    return 0


if __name__ == '__main__':
    assert __version__ == sys.argv[1], (__version__, sys.argv[1])
    print(f'Installed pysystemc {__version__}', flush=True)
    main()
