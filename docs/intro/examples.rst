Examples
========

Run each example in a fresh interpreter: the native kernel retains simulation
state. The examples below are original small models illustrating topics from
ASIC World and Learn SystemC; they include assertions and bounded runs.

Adder
-----

Inspired by `ASIC World's adder <https://www.asic-world.com/examples/systemc/adder.html#Adder>`_.
The Python model retains native parameterized ports and tests width-limited sums::

   make example-adder-py

.. literalinclude:: ../../examples/toy/adder.py
   :language: python

8-bit up counter
----------------

Inspired by `ASIC World's counter <https://www.asic-world.com/examples/systemc/counters.html#8-Bit_Up_Counter>`_.
Reset is synchronous and enable holds the count; the test includes wraparound::

   make example-up-counter-py

.. literalinclude:: ../../examples/toy/up_counter.py
   :language: python

Event cancellation
------------------

.. literalinclude:: ../../examples/toy/event_cancel.py
   :language: python

Mixed C++/Python gate
---------------------

.. literalinclude:: ../../examples/toy/mixed_gate.py
   :language: python

Trace cleanup
-------------

.. literalinclude:: ../../examples/toy/trace_cleanup.py
   :language: python

Upstream ports
--------------

The sysc/2.1 ports preserve the upstream directory hierarchy. Fork/join,
barrier, report rules and hierarchical exports are compared against native C++.
Run ``make example-export-py``, ``make example-forkjoin-py`` or
``make example-report-py``. See :doc:`features` for tested behavior and limits.

The specialized-signals port preserves deferred signal updates and input/inout
port wiring. It checks writable concatenations and mixed signed/unsigned
arithmetic across native small and big integers. All 13 displayed values match
the native reference, and 26 arithmetic comparisons are asserted. Run
``make example-specialized-signals-py`` or
``uv run pytest tests/test_upstream.py -k specialized_signals``.
