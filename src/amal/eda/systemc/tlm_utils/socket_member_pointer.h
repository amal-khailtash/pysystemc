/** @file Exact native storage for multi-socket callback member pointers. */
#pragma once

#include <stdexcept>

namespace pysystemc_interfaces
{
/** @brief Select a public callback typedef without changing its native type. */
template <class Socket, int Kind>
struct socket_callback_traits;

#define PYSYSTEMC_SOCKET_CALLBACK_TRAIT(KIND, ALIAS) \
  template <class Socket>                            \
  struct socket_callback_traits<Socket, KIND>        \
  {                                                  \
    using type = typename Socket::ALIAS;             \
  }
PYSYSTEMC_SOCKET_CALLBACK_TRAIT(0, nb_cb);
PYSYSTEMC_SOCKET_CALLBACK_TRAIT(1, dmi_cb);
PYSYSTEMC_SOCKET_CALLBACK_TRAIT(2, nb_cb);
PYSYSTEMC_SOCKET_CALLBACK_TRAIT(3, b_cb);
PYSYSTEMC_SOCKET_CALLBACK_TRAIT(4, dbg_cb);
PYSYSTEMC_SOCKET_CALLBACK_TRAIT(5, dmi_cb);
#undef PYSYSTEMC_SOCKET_CALLBACK_TRAIT

/** @brief Recover the native MODULE owner of an exact member-pointer signature. */
template <class Pointer>
struct socket_member_owner;

/** @brief Preserve arbitrary payload and phase argument types in owner selection. */
template <class Result, class Module, class... Arguments>
struct socket_member_owner<Result (Module::*)(Arguments...)>
{
  using type = Module;
};

/** @brief Store an exact native callback typedef that cppyy cannot reflect.
 * @details This value owns only a member pointer, never its MODULE or socket.
 * Retain the native owner through simulation. Registration does not invoke the
 * callback or enter the scheduler; keep the GIL held during registration.
 * Native registration reports and conversion exceptions propagate unchanged.
 */
template <class Socket, int Kind>
struct socket_member_pointer
{
  using pointer_type = typename socket_callback_traits<Socket, Kind>::type;
  using module_type = typename socket_member_owner<pointer_type>::type;
  pointer_type value = nullptr;

  /** @brief Construct a null member-pointer value without acquiring ownership. */
  socket_member_pointer() = default;

  /** @brief Copy an exact native member pointer into owned value storage. */
  explicit socket_member_pointer(pointer_type pointer) : value(pointer)
  {
  }

  /** @brief Report whether this value can identify a callback. */
  bool valid() const
  {
    return value != nullptr;
  }

  /** @brief Register the stored pointer on a borrowed socket and MODULE owner. */
  void register_on(Socket& socket, module_type* owner) const
  {
    if (!valid()) throw std::invalid_argument("cannot register a null socket callback member pointer");
    if constexpr (Kind == 0)
      socket.register_nb_transport_bw(owner, value);
    else if constexpr (Kind == 1)
      socket.register_invalidate_direct_mem_ptr(owner, value);
    else if constexpr (Kind == 2)
      socket.register_nb_transport_fw(owner, value);
    else if constexpr (Kind == 3)
      socket.register_b_transport(owner, value);
    else if constexpr (Kind == 4)
      socket.register_transport_dbg(owner, value);
    else if constexpr (Kind == 5)
      socket.register_get_direct_mem_ptr(owner, value);
  }
};

/** @brief Materialize a compile-time member pointer with exact native type checking.
 * @details No owner is retained and no callback is invoked. Incompatible member
 * signatures fail C++ template instantiation instead of selecting an overload.
 */
template <class Socket, int Kind, auto Member>
socket_member_pointer<Socket, Kind> make_socket_member_pointer()
{
  return socket_member_pointer<Socket, Kind>(Member);
}

/** @brief Resolve hierarchical target binding using the exact native source type.
 * @details cppyy can confuse a registered socket's interface conversion with
 * conversion to its hierarchy base. Native overload resolution preserves the
 * upstream binding operation. Both sockets remain borrowed and must outlive
 * elaboration and simulation. Hold the GIL; reports and conversion errors
 * propagate. Binding must precede elaboration and never invokes transport.
 */
template <class Socket>
struct multi_target_binding
{
  /** @brief Invoke the native bind overload on two borrowed socket objects. */
  template <class Other>
  static void bind(Socket& socket, Other& other)
  {
    socket.bind(other);
  }

  /** @brief Invoke the native function-call hierarchy binding overload. */
  template <class Other>
  static void call(Socket& socket, Other& other)
  {
    socket(other);
  }
};
}  // namespace pysystemc_interfaces
