"""Close VCD tracing reliably while preserving a Python exception."""
from pathlib import Path
from amal.eda.systemc import main, sc_core as sc


def sc_main(args: list[str]) -> int:
    """Record transitions and inspect the closed trace after an exception."""
    signal = sc.sc_signal[bool]('pulse')
    try:
        with sc.vcd_trace('cleanup', time_unit=(1, sc.SC_NS)) as trace:
            sc.sc_trace(trace, signal, 'pulse')
            sc.sc_start(1, sc.SC_NS)
            signal.write(True)
            sc.sc_start(1, sc.SC_NS)
            signal.write(False)
            sc.sc_start(1, sc.SC_NS)
            raise ValueError('intentional example failure')
    except ValueError as error:
        assert str(error) == 'intentional example failure'
    text = Path('cleanup.vcd').read_text()
    declaration = next(line for line in text.splitlines() if line.startswith('$var') and 'pulse' in line)
    identifier = declaration.split()[3]
    assert f'1{identifier}' in text and f'0{identifier}' in text
    assert '#1' in text and '#2' in text
    print('TRACE_CLEANUP_OK: transitions flushed after exception')
    return 0


if __name__ == '__main__':
    main()
