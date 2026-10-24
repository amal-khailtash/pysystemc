"""Preserve native consuming operators for chained event expressions."""

import cppyy


def install():
    """Install OR/AND expression append methods without entering the scheduler."""
    native = cppyy.gbl.sc_core
    for list_type, method in ((native.sc_event_or_list, "__or__"), (native.sc_event_and_list, "__and__")):
        expression_type = native.sc_event_expr[list_type]
        operation = cppyy.gbl.pysystemc_public.event_expression_operations[list_type].append
        setattr(expression_type, method, _append(operation, list_type))


def _append(operation, list_type):
    """Build an operator retaining its native overload and matching list type."""

    def append(left, right):
        """Append an event or matching list and consume the left expression.

        Args:
            left: Single-use native OR or AND expression.
            right: Borrowed event or matching native event list.

        Returns:
            New native expression owning the transferred automatic list.

        Raises:
            TypeError: The right operand is not an event or matching list.

        All events must outlive consumers. This synchronous call holds the GIL;
        consuming an expression does not transfer ownership of referenced events.
        """
        if not isinstance(right, (cppyy.gbl.sc_core.sc_event, list_type)):
            return NotImplemented
        return operation(left, right)

    return append
