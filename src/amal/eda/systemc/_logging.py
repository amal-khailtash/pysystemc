"""Loguru diagnostics rendered by Rich with native-compatible color controls."""

import os
import sys
from loguru._logger import Core, Logger
from rich.console import Console
from rich.text import Text


def _console() -> Console:
    """Create a stderr console honoring explicit color mode and NO_COLOR."""
    mode = os.environ.get("PYSYSTEMC_COLOR", "auto")
    force = True if mode == "always" else False if mode == "never" else None
    return Console(
        file=sys.stderr,
        force_terminal=force,
        no_color=mode == "never" or (mode != "always" and "NO_COLOR" in os.environ),
        highlight=False,
        markup=False,
    )


def _sink(message) -> None:
    """Render only this binding's Loguru records without changing global sinks."""
    record = message.record
    level = record["level"].name
    text = Text(f"[pysystemc:Python:{level}] {record['message']}", style="red" if level == "ERROR" else "cyan")
    _console().print(text, soft_wrap=True)
    sys.stderr.flush()


# A separate logger core avoids modifying the application's global Loguru sinks.
logger = Logger(Core(), None, 0, False, False, False, False, True, [], {})
logger.add(_sink, level="DEBUG", format="{message}")
_binding_logger = logger.bind(pysystemc_binding=True)


def log(message: str, *, error: bool = False) -> None:
    """Emit a Rich-rendered Loguru binding diagnostic.

    Debug requires PYSYSTEMC_LOG_LEVEL=DEBUG. Errors always appear. Explicit
    PYSYSTEMC_COLOR=always overrides NO_COLOR; never disables ANSI output.
    """
    if not error and os.environ.get("PYSYSTEMC_LOG_LEVEL") != "DEBUG":
        return
    _binding_logger.opt(depth=0).log("ERROR" if error else "DEBUG", message)


def show_banner(version: str) -> None:
    """Print the version before native startup unless PYSC_COPYRIGHT_MESSAGE=DISABLE."""
    if os.environ.get("PYSC_COPYRIGHT_MESSAGE") == "DISABLE":
        return
    _console().print(
        Text(f"pysystemc {version} — SystemC Python bindings\nCopyright (c) 2026 Amal Khailtash", style="cyan"),
        soft_wrap=True,
    )
    sys.stderr.flush()
