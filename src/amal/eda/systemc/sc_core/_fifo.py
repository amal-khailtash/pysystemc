"""Preserve native FIFO overloads while guarding Python scheduler unwind."""

import cppyy

_bridges = []


class FifoTemplate:
    """Instantiate unrestricted native FIFO types with guarded blocking calls."""

    def __init__(self, template):
        """Retain the native template without precompiling value specializations."""
        self.template = template

    def __getitem__(self, datatype):
        """Return a native class whose blocking calls preserve reset/kill cleanup."""
        native = self.template[datatype]
        if getattr(native, "_guarded_fifo", False):
            return native
        return guard_fifo_class(native, datatype)

    def __call__(self, datatype):
        """Support alternate cppyy template specialization syntax."""
        return self[datatype]


def guard_fifo_class(native, datatype):
    """Guard a native FIFO specialization or borrowed blocking interface class."""
    if getattr(native, "_guarded_fifo", False):
        return native
    value = native.__cpp_name__.split("<", 1)[1].removesuffix(">")
    identifier = f"GuardedFifo{len(_bridges)}"
    cppyy.cppdef(f"""
    /** Guard borrowed native FIFO calls; callers release GIL and retain channel/input.
     * False completion requires Python unwind before original scheduler rethrow.
     */
    namespace pysystemc_interfaces {{ struct {identifier} {{
      static std::pair<{value}, bool> read(sc_core::sc_fifo_blocking_in_if<{value}>& channel) {{
        return sc_core::python_fifo_read(channel);
      }}
      static bool write(sc_core::sc_fifo_blocking_out_if<{value}>& channel, const {value}& value) {{
        return sc_core::python_fifo_write(channel, value);
      }}
    }}; }}
    """)
    bridge = getattr(cppyy.gbl.pysystemc_interfaces, identifier)
    _bridges.append(bridge)
    read_bridge, write_bridge = bridge.read, bridge.write
    read_bridge.__release_gil__ = write_bridge.__release_gil__ = True

    def read(self, output=None):
        """Read by value or assign caller output only after successful completion."""
        from . import _ProcessUnwind

        result = read_bridge(self.get_interface() if hasattr(self, "get_interface") else self)
        if not result.second:
            raise _ProcessUnwind()
        if output is None:
            return result.first
        if hasattr(output, "__assign__"):
            output.__assign__(result.first)
        else:
            output.value = result.first

    def write(self, value):
        """Write a sample, propagating reset/kill through Python cleanup."""
        from . import _ProcessUnwind

        if not write_bridge(self.get_interface() if hasattr(self, "get_interface") else self, value):
            raise _ProcessUnwind()

    if hasattr(native, "read"):
        native.read = read
    if hasattr(native, "write"):
        native.write = write
    if native.__cpp_name__.startswith("sc_core::sc_fifo<"):

        def assign(self, sample):
            """Write through native assignment with guarded reset/kill cleanup."""
            write(self, sample)
            return self

        def convert(self):
            """Consume one sample through the guarded native conversion operator."""
            return read(self)

        native.__assign__ = assign
        setattr(native, "operator " + value, convert)
        if value in (
            "int",
            "unsigned int",
            "long",
            "unsigned long",
            "long long",
            "unsigned long long",
            "short",
            "unsigned short",
            "char",
            "signed char",
            "unsigned char",
        ):

            def integer(self):
                """Consume a native integer sample with guarded scheduler unwind."""
                sample = read(self)
                return ord(sample) if isinstance(sample, str) else int(sample)

            native.__int__ = integer
        elif value in ("float", "double", "long double"):

            def floating(self):
                """Consume a native floating sample with guarded scheduler unwind."""
                return float(read(self))

            native.__float__ = floating
        elif value == "bool":

            def boolean(self):
                """Consume a native bool sample with guarded scheduler unwind."""
                return bool(read(self))

            native.__bool__ = boolean
    native._guarded_fifo = True
    return native
