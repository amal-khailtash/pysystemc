"""Check resolved-vector assignment return identity against native signatures."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


cppyy.cppdef(
    """
    #include <systemc>
    #include <type_traits>
    #include <utility>

    using f9_signal_rv = sc_core::sc_signal_rv<16>;
    using f9_signal_value = f9_signal_rv::value_type;
    using f9_signal_interface = sc_core::sc_signal_in_if<f9_signal_value>;
    static_assert(
        std::is_same_v<decltype(std::declval<f9_signal_rv &>() =
                                    std::declval<const f9_signal_value &>()),
                       f9_signal_rv &>);
    static_assert(
        std::is_same_v<decltype(std::declval<f9_signal_rv &>() =
                                    std::declval<const f9_signal_interface &>()),
                       f9_signal_rv &>);
    static_assert(std::is_same_v<decltype(std::declval<f9_signal_rv &>() =
                                              std::declval<const f9_signal_rv &>()),
                                 f9_signal_rv &>);
    """
)

signal_type = sc.sc_signal_rv[16]
value_type = sc_dt.sc_lv[16]
target = signal_type("assignment_target", value_type("0000"))
interface_source = sc.sc_signal[value_type]("assignment_interface_source", value_type("0011"))
same_type_source = signal_type("assignment_same_type_source", value_type("1100"))

results = (
    target.__assign__(value_type("1010")),
    target.__assign__(interface_source),
    target.__assign__(same_type_source),
)
assert all(cppyy.addressof(result) == cppyy.addressof(target) for result in results)
print("F9_SEVENTH_SIGNAL_ASSIGNMENT_RETURNS value=1|interface=1|same_type=1")
