"""Fresh-process tests for the Python SystemC playground."""

import os
from pathlib import Path
import subprocess
import sys


def run_script(script: str, *arguments: str, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    """Run an isolated Python script with a bounded timeout.

    Args:
        script: Python source passed to a new interpreter.
        *arguments: Values exposed through ``sys.argv`` in the child.
        env: Optional child environment overlay.

    Returns:
        Captured process output and status.
    """
    child_env = dict(os.environ)
    if env is not None:
        child_env.update(env)
    return subprocess.run(
        [sys.executable, "-c", script, *arguments],
        capture_output=True,
        text=True,
        timeout=60,
        env=child_env,
    )


def test_repl_injected_prompt_execution_completion_history_and_recovery(tmp_path: Path) -> None:
    """Exercise prompt controls, multiline input, completion, and error recovery."""
    script = r"""
import os
import sys
from pathlib import Path
from prompt_toolkit.auto_suggest import AutoSuggestFromHistory
from prompt_toolkit.completion import CompleteEvent
from prompt_toolkit.document import Document
from prompt_toolkit.history import FileHistory, InMemoryHistory
from prompt_toolkit.lexers import PygmentsLexer
from pygments.lexers.python import PythonLexer
from amal.eda.systemc.repl import Repl

history_path = Path(sys.argv[1])
inputs = [
    "sentinel = 40",
    "def add_two(value):",
    "    return value + 2",
    "",
    "print('MULTILINE', add_two(sentinel))",
    "class Box:\n    answer = sentinel + 2\n",
    "print('PASTED', Box.answer)",
    "def pasted_function():\n    return sentinel + 2\nprint('PASTED_DEDENT', pasted_function())",
    "pasted_a = 20\npasted_b = pasted_a + 2",
    "print('PASTED_LINES', pasted_a, pasted_b)",
    "1 / 0",
    "sc_core.sc_time('invalid')",
    "print('SURVIVED', sentinel)",
    "<interrupt>",
    "print('AFTER_INTERRUPT', sentinel)",
    "raise KeyboardInterrupt",
    "print('AFTER_EXEC_INTERRUPT', sentinel)",
]

class InjectedSession:
    def __init__(self, **kwargs):
        self.kwargs = kwargs
        self.prompts = []
        self.values = list(inputs)
        kwargs['history'].append_string('previous command')

    def prompt(self, message):
        self.prompts.append(message)
        if not self.values:
            raise EOFError
        value = self.values.pop(0)
        if value == '<interrupt>':
            raise KeyboardInterrupt
        return value

repl = Repl(history_path=history_path, prompt_session_factory=InjectedSession)
assert isinstance(repl.history, FileHistory)
assert repl.history_path == history_path
assert isinstance(repl.session.kwargs['auto_suggest'], AutoSuggestFromHistory)
assert isinstance(repl.session.kwargs['lexer'], PygmentsLexer)
assert isinstance(repl.session.kwargs['lexer'].pygments_lexer, PythonLexer)
assert repl.session.kwargs['completer'] is repl.completer
repl.run()
assert 'sentinel' in repl.namespace and repl.namespace['sentinel'] == 40
assert any(prompt == '... ' for prompt in repl.session.prompts)
assert history_path.read_text().find('previous command') >= 0
completions = list(repl.completer.get_completions(Document('sent'), CompleteEvent()))
match = next(completion for completion in completions if completion.display_text == 'sentinel')
assert match.text == 'sentinel' and match.start_position == -4
attribute_completions = list(repl.completer.get_completions(Document('sc_core.sc_t'), CompleteEvent()))
assert any(item.display_text.startswith('sc_core.sc_time') and item.text == item.display_text for item in attribute_completions)
inside_call = list(repl.completer.get_completions(Document('print(sc_core.sc_t'), CompleteEvent()))
assert any(item.text.startswith('sc_core.sc_time') and item.start_position == -len('sc_core.sc_t') for item in inside_call)
assert 'previous command' in list(FileHistory(str(history_path)).load_history_strings())

os.environ['XDG_STATE_HOME'] = str(history_path.parent / 'xdg')
second = Repl(no_history=True, prompt_session_factory=lambda **kwargs: InjectedSession(**kwargs))
assert isinstance(second.history, InMemoryHistory) and second.history_path is None
second.session.values = []
second.run()
third = Repl(prompt_session_factory=lambda **kwargs: InjectedSession(**kwargs))
assert third.history_path == Path(os.environ['XDG_STATE_HOME']) / 'pysystemc' / 'history'
third.session.values = []
third.run()
"""
    result = run_script(script, str(tmp_path / "history"))
    assert result.returncode == 0, result.stdout + result.stderr
    assert "MULTILINE 42" in result.stdout
    assert "PASTED 42" in result.stdout
    assert "PASTED_DEDENT 42" in result.stdout
    assert "PASTED_LINES 20 22" in result.stdout
    assert "SURVIVED 40" in result.stdout
    assert "AFTER_INTERRUPT 40" in result.stdout
    assert "AFTER_EXEC_INTERRUPT 40" in result.stdout
    assert "ZeroDivisionError" in result.stderr
    assert "TypeError" in result.stderr
    assert "KeyboardInterrupt" in result.stdout


def test_repl_executes_native_template_and_advances_kernel_time() -> None:
    """Run real cppyy signal-template and SystemC time commands in isolation."""
    script = r"""
from amal.eda.systemc.repl import Repl

class InjectedSession:
    def __init__(self, **kwargs):
        self.values = [
            "signal = sc_core.sc_signal[int]('repl_signal')",
            "signal.write(17)",
            "sc_core.sc_start(sc_core.SC_ZERO_TIME)",
            "print('REPL_VALUE', int(signal.read()))",
            "sc_core.sc_start(sc_core.sc_time(3, sc_core.SC_NS))",
            "print('REPL_TIME', sc_core.sc_time_stamp())",
        ]

    def prompt(self, message):
        if not self.values:
            raise EOFError
        return self.values.pop(0)

Repl(prompt_session_factory=InjectedSession).run()
"""
    result = run_script(script)
    assert result.returncode == 0, result.stdout + result.stderr
    assert "REPL_VALUE 17" in result.stdout
    assert "REPL_TIME 3 ns" in result.stdout
