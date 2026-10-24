"""Interactive Python playground backed by the native SystemC kernel."""

import builtins
import code
import os
from pathlib import Path
import rlcompleter
import re
from collections.abc import Callable, Iterator
from typing import Any

from prompt_toolkit import PromptSession
from prompt_toolkit.auto_suggest import AutoSuggestFromHistory
from prompt_toolkit.completion import Completer, Completion, CompleteEvent
from prompt_toolkit.document import Document
from prompt_toolkit.history import FileHistory, InMemoryHistory
from prompt_toolkit.lexers import PygmentsLexer
from pygments.lexers.python import PythonLexer

HELP = """pysystemc Python playground

Python names: sc_core, sc_dt, tlm, tlm_utils, cppyy, and every public sc_core name.
Example:
    sig = sc_core.sc_signal[int]("sig")
    sig.write(7)
    sc_core.sc_start(sc_core.SC_ZERO_TIME)
    print("signal:", sig.read())
    sc_core.sc_start(sc_core.sc_time(1, sc_core.SC_NS))
    print("time:", sc_core.sc_time_stamp())

This interpreter owns one SystemC kernel for its lifetime. Continue a design
with further Python commands and sc_start calls. Use a fresh pysystemc process
for an independent simulation; the REPL does not reset native kernel state.
Press Ctrl+C to clear the current input and continue, or Ctrl+D to exit.
"""

_KERNEL_OWNERS: list[tuple[object, ...]] = []


def default_history_path() -> Path:
    """Return the platform state-directory path for persistent REPL history.

    Returns:
        ``$XDG_STATE_HOME/pysystemc/history`` when configured, otherwise
        ``~/.local/state/pysystemc/history``.
    """
    state_home = os.environ.get("XDG_STATE_HOME")
    root = Path(state_home).expanduser() if state_home else Path.home() / ".local" / "state"
    return root / "pysystemc" / "history"


class PythonNamespaceCompleter(Completer):
    """Complete Python identifiers and attributes against a live namespace."""

    def __init__(self, namespace: dict[str, Any]) -> None:
        """Bind completion lookup to the persistent interactive namespace.

        Args:
            namespace: Mutable globals used by the interactive console.
        """
        self.namespace = namespace

    def get_completions(self, document: Document, complete_event: CompleteEvent) -> Iterator[Completion]:
        """Complete a Python identifier or attribute immediately before the cursor.

        Args:
            document: Current prompt buffer and cursor position.
            complete_event: Prompt Toolkit completion request metadata.

        Yields:
            Prompt Toolkit completions from Python's namespace-aware rlcompleter.
        """
        del complete_event
        match = re.search(r"[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*\.?$", document.text_before_cursor)
        if match is None:
            return
        word = match.group()
        completer = rlcompleter.Completer(self.namespace)
        index = 0
        while (candidate := completer.complete(word, index)) is not None:
            index += 1
            if candidate.startswith(word):
                yield Completion(candidate, start_position=-len(word), display=candidate)


def _seed_namespace() -> tuple[dict[str, Any], tuple[object, ...]]:
    """Load and retain the public Python/native namespace for one REPL.

    Returns:
        A dictionary for ``InteractiveConsole`` and strong owners for modules
        whose native objects or callbacks may remain registered in the kernel.
    """
    import amal.eda.systemc as systemc
    import cppyy
    from amal.eda.systemc import sc_core, sc_dt, tlm
    import amal.eda.systemc.tlm_utils as tlm_utils

    namespace: dict[str, Any] = {
        "sc_core": sc_core,
        "sc_dt": sc_dt,
        "tlm": tlm,
        "tlm_utils": tlm_utils,
        "cppyy": cppyy,
    }
    for name in sc_core.__all__:
        # Preserve Python's useful builtins if a native export happens to share
        # a spelling such as ``id`` or ``print``.
        if name not in vars(builtins):
            namespace[name] = getattr(sc_core, name)
    owners = (systemc, sc_core, sc_dt, tlm, tlm_utils, cppyy)
    return namespace, owners


class Repl:
    """Run a persistent Python console against one native SystemC kernel."""

    def __init__(
        self,
        history_path: Path | str | None = None,
        no_history: bool = False,
        prompt_session_factory: Callable[..., PromptSession] | None = None,
    ) -> None:
        """Create the console, history backend, completer, and line editor.

        Args:
            history_path: Persistent history file, or ``None`` to use the XDG
                state path. Ignored when ``no_history`` is true.
            no_history: Keep history in memory for this session only.
            prompt_session_factory: Optional Prompt Toolkit session factory,
                useful for embedding or deterministic input sources.

        The native namespace and console locals are retained at module scope for
        the interpreter lifetime because SystemC may keep callbacks registered
        after the prompt exits.
        """
        self.namespace, module_owners = _seed_namespace()
        self.console = code.InteractiveConsole(self.namespace)
        self.completer = PythonNamespaceCompleter(self.console.locals)

        if no_history:
            self.history = InMemoryHistory()
            self.history_path = None
        else:
            self.history_path = Path(history_path).expanduser() if history_path is not None else default_history_path()
            self.history_path.parent.mkdir(parents=True, exist_ok=True)
            self.history = FileHistory(str(self.history_path))

        session_factory = prompt_session_factory or PromptSession
        self.session = session_factory(
            history=self.history,
            auto_suggest=AutoSuggestFromHistory(),
            lexer=PygmentsLexer(PythonLexer),
            completer=self.completer,
            complete_while_typing=True,
        )
        _KERNEL_OWNERS.append((self.console.locals, self.console, *module_owners))

    def run(self) -> None:
        """Read and execute Python statements until EOF or an exit exception.

        Incomplete statements use the standard console continuation buffer.
        Complete pasted blocks compile together, including dedented statements.
        Exceptions are rendered by ``InteractiveConsole`` and leave the local
        namespace available for later commands.
        """
        print(HELP)
        more = False
        while True:
            prompt = "... " if more else "pysystemc >>> "
            try:
                source = self.session.prompt(prompt)
            except EOFError:
                print()
                return
            except KeyboardInterrupt:
                self.console.resetbuffer()
                more = False
                print("KeyboardInterrupt")
                continue

            try:
                if "\n" in source:
                    combined = "\n".join((*self.console.buffer, source))
                    try:
                        compiled = self.console.compile(combined, self.console.filename, "exec")
                    except SyntaxError, OverflowError, ValueError:
                        self.console.showsyntaxerror(self.console.filename)
                        self.console.resetbuffer()
                        more = False
                    else:
                        more = compiled is None
                        if more:
                            self.console.buffer = combined.splitlines()
                        else:
                            self.console.resetbuffer()
                            self.console.runcode(compiled)
                else:
                    more = self.console.push(source)
            except KeyboardInterrupt:
                self.console.resetbuffer()
                more = False
                print("KeyboardInterrupt")
            except SystemExit:
                return


def main() -> None:
    """Start the default interactive SystemC Python playground."""
    Repl().run()
