"""Verify outstanding example imports and asynchronous native default timing."""

from pathlib import Path

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("path", ["examples/sysc/fft/fft_fxpt/main.py", "examples/sysc/fir/main_rtl.py"])
def test_example_entrypoint_ignores_cached_main(tmp_path: Path, path: str) -> None:
    """Load the intended sibling even when another example owns ``main``."""
    script = tmp_path / "import_example.py"
    script.write_text(
        '"""Exercise sibling loading with a conflicting cached module."""\n'
        "import importlib.util, sys, types\n"
        f"source = {str(ROOT / path)!r}\n"
        "sentinel = types.ModuleType('main')\n"
        "sys.modules['main'] = sentinel\n"
        "before = list(sys.path)\n"
        "spec = importlib.util.spec_from_file_location('example_under_test', source)\n"
        "module = importlib.util.module_from_spec(spec)\n"
        "spec.loader.exec_module(module)\n"
        "assert callable(module.run)\n"
        "assert module.run.__module__ in ('pysystemc_example_fft_flpt', 'pysystemc_example_fir')\n"
        "assert sys.modules['main'] is sentinel\n"
        "assert sys.path == before\n"
        "print('SIBLING_IMPORT_OK')\n"
    )
    assert "SIBLING_IMPORT_OK" in run_example(tmp_path, script)


@pytest.mark.parametrize("delay", [0, 7])
def test_async_default_and_explicit_delay_match_native(tmp_path: Path, delay: int) -> None:
    """Compare callback count, time, and lifecycle for native and Python channels."""
    outputs = []
    for native in (False, True):
        script = tmp_path / f"async_{native}_{delay}.py"
        script.write_text(
            '"""Compare a fresh asynchronous event simulation to native SystemC."""\n'
            "import importlib.util, threading, time\n"
            "from amal.eda.systemc import sc_core as sc\n"
            "import cppyy\n"
            f"native = {native!r}\ndelay = {delay}\n"
            f"directory = {str(ROOT / 'submodules/systemc/examples/sysc/2.3/simple_async')!r}\n"
            "if native:\n"
            "    cppyy.add_include_path(directory)\n"
            "    cppyy.cppdef('''\n"
            '#include <systemc>\nusing namespace sc_core;\n#include "async_event.h"\n'
            "/** @brief Own a native channel and count its simulation-thread callback. */\n"
            "struct AsyncDefaultReference : sc_module {\n"
            "  async_event event; int count = 0; double when = -1;\n"
            "  SC_HAS_PROCESS(AsyncDefaultReference);\n"
            '  AsyncDefaultReference(sc_module_name n) : sc_module(n), event("event") {\n'
            "    SC_METHOD(deliver); sensitive << event; dont_initialize();\n"
            "  }\n"
            "  void deliver() { ++count; when = sc_time_stamp().to_seconds(); sc_stop(); }\n"
            "};\n''')\n"
            "    model = cppyy.gbl.AsyncDefaultReference('reference')\n"
            "else:\n"
            f"    spec = importlib.util.spec_from_file_location('async_example', {str(ROOT / 'examples/sysc/2.3/simple_async/main.py')!r})\n"
            "    module = importlib.util.module_from_spec(spec)\n"
            "    spec.loader.exec_module(module)\n"
            "    assert sc.sc_get_top_level_objects().size() == 0\n"
            "    class Model(sc.sc_module):\n"
            "        def __init__(self):\n"
            "            name = sc.sc_module_name('reference')\n"
            "            super().__init__(name)\n"
            "            self.event = module.AsyncEvent('event')\n"
            "            self.count = 0\n"
            "            self.when = -1\n"
            "            self.sc_method('deliver', self.deliver)\n"
            "            self.sensitive(self.event.m_event)\n"
            "            self.dont_initialize()\n"
            "        def deliver(self):\n"
            "            self.count += 1\n"
            "            self.when = sc.sc_time_stamp().to_seconds()\n"
            "            sc.sc_stop()\n"
            "    model = Model()\n"
            "def notify():\n"
            "    time.sleep(0.02)\n"
            "    if delay:\n"
            "        model.event.notify(sc.sc_time(delay, sc.SC_NS))\n"
            "    else:\n"
            "        model.event.notify()\n"
            "worker = threading.Thread(target=notify)\nworker.start()\nsc.sc_start()\nworker.join(5)\n"
            "assert not worker.is_alive()\nassert model.count == 1\n"
            "assert abs(model.when - delay * 1e-9) < 1e-18\n"
            "assert sc.sc_end_of_simulation_invoked()\n"
            "print(f'ASYNC_RESULT count={model.count} ns={round(model.when * 1e9)} stopped=1')\n"
        )
        outputs.append(
            next(line for line in run_example(tmp_path, script).splitlines() if line.startswith("ASYNC_RESULT"))
        )
    assert outputs == [f"ASYNC_RESULT count=1 ns={delay} stopped=1"] * 2


@pytest.mark.parametrize("variant", ["fft_flpt", "fft_fxpt"])
@pytest.mark.parametrize("component", ["real", "imag"])
def test_fft_malformed_input_reports_parse_failure(tmp_path: Path, variant: str, component: str) -> None:
    """Reject malformed tokens before opening output files in either FFT variant."""
    for name in ("real", "imag"):
        (tmp_path / f"in_{name}").write_text("invalid\n" if name == component else "1\n")
    with pytest.raises(AssertionError, match="ValueError"):
        run_example(tmp_path, f"examples/sysc/fft/{variant}/main.py")
    assert not (tmp_path / "out_real").exists()
    assert not (tmp_path / "out_imag").exists()


def test_async_generated_channel_names_match_native(tmp_path: Path) -> None:
    """Generate two default channel names without colliding with an explicit name."""
    observations = []
    for native in (False, True):
        script = tmp_path / f"async_names_{native}.py"
        script.write_text(
            '"""Check native asynchronous channel constructor name defaults."""\n'
            "import importlib.util\nfrom amal.eda.systemc import sc_core as sc\nimport cppyy\n"
            f"native = {native!r}\n"
            "if native:\n"
            f"    cppyy.add_include_path({str(ROOT / 'submodules/systemc/examples/sysc/2.3/simple_async')!r})\n"
            "    cppyy.cppdef('#include <systemc>\\nusing namespace sc_core;\\n#include \"async_event.h\"')\n"
            "    channel_type = cppyy.gbl.async_event\n"
            "else:\n"
            f"    spec = importlib.util.spec_from_file_location('async_names', {str(ROOT / 'examples/sysc/2.3/simple_async/main.py')!r})\n"
            "    module = importlib.util.module_from_spec(spec)\n"
            "    spec.loader.exec_module(module)\n"
            "    channel_type = module.AsyncEvent\n"
            "channels = [channel_type(), channel_type(), channel_type('explicit')]\n"
            "names = [str(channel.basename()) for channel in channels]\n"
            "assert names == ['async_event_0', 'async_event_1', 'explicit'], names\n"
            "if not native:\n"
            "    assert [str(channel.m_event.basename()) for channel in channels] == [name + '_event' for name in names]\n"
            "print('ASYNC_NAMES ' + ','.join(names))\n"
        )
        observations.append(
            next(line for line in run_example(tmp_path, script).splitlines() if line.startswith("ASYNC_NAMES"))
        )
    assert observations == ["ASYNC_NAMES async_event_0,async_event_1,explicit"] * 2
