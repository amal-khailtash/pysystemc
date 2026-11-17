/** @file vector_policies.cpp
 * @brief Native access-policy construction avoiding member-pointer conversion.
 * @details Policies borrow element/member pointers and never own elements. GIL
 * stays held for construction and access. Native const-conversion restrictions
 * and copy semantics apply; no callbacks or scheduler entry occur.
 */
#include <stdexcept>
#include <systemc>

namespace pysystemc_vectors
{
template <class Element, class Member>
struct member_pointer_access;

template <class Element, class Member, Member Element::*Pointer>
struct member_pointer_factory;

/** @brief Own one exact native pointer-to-member value without owning its element type.
 * @tparam Element Native vector element type.
 * @tparam Member Exact member value type selected by sc_member_access.
 * @details The token stores Member Element::* directly. It owns no vector, element,
 * or member storage; callers retain those owners while assemblies/views are used.
 * No callbacks or scheduler entry occur, and callers keep the GIL for its Python API.
 */
template <class Element, class Member>
class member_pointer
{
 public:
  using element_type = Element;
  using value_type = Member;
  using pointer_type = Member Element::*;

  /** @brief Construct a null member-pointer token. */
  member_pointer() = default;

  /** @brief Copy one token with the same exact element and member signature. */
  member_pointer(const member_pointer&) = default;

  /** @brief Report whether this token contains a non-null native member pointer. */
  bool valid() const
  {
    return pointer_ != nullptr;
  }

  /** @brief Compare native pointer identity for tokens with this exact signature. */
  bool same_as(const member_pointer& other) const
  {
    return pointer_ == other.pointer_;
  }

  /** @brief Compare tokens with the same exact signature by native pointer identity. */
  bool operator==(const member_pointer& other) const
  {
    return same_as(other);
  }

  /** @brief Compare tokens with the same exact signature by native pointer identity. */
  bool operator!=(const member_pointer& other) const
  {
    return !same_as(other);
  }

 private:
  explicit member_pointer(pointer_type pointer) : pointer_(pointer)
  {
  }

  pointer_type pointer_ = nullptr;

  friend struct member_pointer_access<Element, Member>;
  template <class E, class M, M E::*Pointer>
  friend struct member_pointer_factory;
};

/** @brief Create a typed token from a compile-time native member pointer.
 * @tparam Element Owning native element type.
 * @tparam Member Exact native member value type.
 * @tparam Pointer Genuine Member Element::* non-type template argument.
 * @details The selected pointer is stored by value; no pointer bytes are reinterpreted.
 */
template <class Element, class Member, Member Element::*Pointer>
struct member_pointer_factory
{
  /** @brief Return a value token containing the exact non-type template pointer. */
  static member_pointer<Element, Member> create()
  {
    return member_pointer<Element, Member>(Pointer);
  }
};

/** @brief Route typed token values through native member/free assembly operations.
 * @tparam Element Vector element type.
 * @tparam Member Exact member value type.
 * @details Tokens borrow the vector and member storage. The GIL stays held; no
 * callbacks or scheduler entry occur. Null tokens throw before native assembly.
 */
template <class Element, class Member>
struct member_pointer_access
{
  using token_type = member_pointer<Element, Member>;

  /** @brief Capture a runtime native pointer-to-member in an owned value token.
   * @param pointer Exact or implicitly convertible Member Element::* value.
   * @return A token copying the pointer value, including a null pointer.
   * @details Called by generated C++ symbol adapters with the GIL held. No raw
   * pointer bytes cross Python, no element ownership is acquired, and no
   * callbacks, kernel entry or exceptions occur. Later symbol changes do not
   * change the returned token; callers retain elements used by its assemblies.
   */
  static token_type capture(typename token_type::pointer_type pointer)
  {
    return token_type(pointer);
  }

  /** @brief Assemble via sc_vector::assemble from a validated typed token.
   * @param vector Native vector borrowed by the returned assembly.
   * @param token Typed pointer value, whose member storage remains vector-owned.
   * @return Native assembly borrowing vector elements and their members.
   * @throws std::invalid_argument If the token is null.
   */
  static auto member(sc_core::sc_vector<Element>& vector, const token_type& token)
  {
    if (!token.valid()) throw std::invalid_argument("member pointer token is null");
    return vector.assemble(token.pointer_);
  }

  /** @brief Assemble via sc_assemble_vector from a validated typed token.
   * @param vector Native vector borrowed by the returned assembly.
   * @param token Typed pointer value, whose member storage remains vector-owned.
   * @return Native assembly borrowing vector elements and their members.
   * @throws std::invalid_argument If the token is null.
   */
  static auto free(sc_core::sc_vector<Element>& vector, const token_type& token)
  {
    if (!token.valid()) throw std::invalid_argument("member pointer token is null");
    return sc_core::sc_assemble_vector(vector, token.pointer_);
  }

  /** @brief Construct a native member policy from one validated token.
   * @param token Exact-signature member token storing Member Element::* by value.
   * @return Native policy borrowing the selected member from caller-owned elements.
   * @throws std::invalid_argument If the token is null.
   * @note The policy owns no element; retain every element used with get(). GIL held.
   */
  static auto make_policy(const token_type& token)
  {
    if (!token.valid()) throw std::invalid_argument("member pointer token is null");
    return sc_core::sc_member_access<Element, Member>(token.pointer_);
  }
};

/** @brief Construct access policy copies with their exact native const rules. */
template <class Element, class Member>
struct member_policy
{
  using policy = sc_core::sc_member_access<Element, Member>;
  using non_const_policy = typename policy::non_const_policy;
  /** @brief Copy an identical native policy without pointer-to-member conversion. */
  static policy copy(const policy& source)
  {
    return source;
  }
  /** @brief Convert a mutable native policy using its declared constructor. */
  static policy convert(const non_const_policy& source)
  {
    return policy(source);
  }
};

/** @brief Invoke the native templated direct-policy conversion constructor. */
template <class Element>
struct direct_policy
{
  /** @brief Convert a borrowed policy while preserving native SFINAE const restrictions. */
  template <class Other>
  static sc_core::sc_direct_access<Element> convert(const Other& source)
  {
    return sc_core::sc_direct_access<Element>(source);
  }
};
/** @brief Expose iterator arrow dereference without cppyy attribute forwarding. */
template <class Iterator>
struct iterator_pointer
{
  /** @brief Return a borrowed native element/member pointer from operator->.
   * @details The iterator and vector/assembly owners must remain alive. GIL held;
   * no bounds checks are added, so end iterators must never be dereferenced.
   */
  static auto get(const Iterator& iterator)
  {
    return iterator.operator->();
  }
};
}  // namespace pysystemc_vectors
