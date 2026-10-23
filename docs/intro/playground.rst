Command line and playground
===========================

The ``pysystemc`` command provides installed-version information, command help
and an interactive Python playground:

.. code-block:: console

   uv run pysystemc --version
   uv run pysystemc version
   uv run pysystemc --help
   uv run pysystemc help repl
   uv run pysystemc repl

From a source checkout, ``make cli`` shows help, ``make cli ARGS=version``
prints the version, and ``make repl`` launches the playground. Both targets
forward ``ARGS``; for example, ``make repl ARGS=--no-history``.

Version and help commands do not load the native simulation kernel. The existing
``pysystemc-cpp`` command remains available for compiling and running C++ models.

The playground preloads ``sc_core``, ``sc_dt``, ``tlm``, ``tlm_utils`` and
``cppyy``, together with public kernel names such as ``sc_time``, ``sc_start``
and ``SC_NS``. Python builtins remain available. For example:

.. code-block:: pycon

   >>> signal = sc_core.sc_signal[int]("demo")
   >>> signal.write(17)
   >>> sc_start(SC_ZERO_TIME)
   >>> print(signal.read())
   17
   >>> sc_start(sc_time(3, SC_NS))
   >>> print(sc_core.sc_time_stamp())
   3 ns

Use ordinary Python class and function definitions to prototype modules and
callbacks. Incomplete statements show a continuation prompt; pasted multiline
input is supported. Python and native binding exceptions are displayed while
session variables remain available for inspection.

The prompt provides Python syntax highlighting, tab completion, history
suggestions, arrow-key navigation and line editing. Ctrl+C clears unfinished
input and returns to the prompt. Ctrl+D or ``exit()`` leaves the playground.
History persists at ``$XDG_STATE_HOME/pysystemc/history`` when configured, or
``~/.local/state/pysystemc/history`` otherwise. Choose another location with
``pysystemc repl --history PATH`` or keep history in memory with
``pysystemc repl --no-history``.

Each process has one native SystemC kernel. Further ``sc_start`` calls advance
the same design. Retain modules, channels and callback dependencies while the
kernel uses them. After ``sc_stop`` or a terminal simulation error, start a new
playground process for another simulation.
