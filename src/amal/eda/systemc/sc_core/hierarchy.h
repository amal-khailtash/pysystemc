/**
 * @file hierarchy.h
 * @brief Public SystemC module-hierarchy formatting helpers.
 * @details Calls borrow the object tree only for the duration of the call and retain no native objects.
 */
#ifndef PYSYSTEMC_SC_CORE_HIERARCHY_H
#define PYSYSTEMC_SC_CORE_HIERARCHY_H

#include <ostream>
#include <string>
#include <systemc>

namespace pysystemc_public
{

/**
 * @brief Write the full current design hierarchy to a stream.
 * @param output Borrowed output stream; it remains owned by the caller.
 * @param all_modules Include native modules whose dynamic type begins with `cynw::`.
 * @throws std::ios_base::failure If the output stream is configured to throw on write failure.
 *
 * Output is plain text with one `Design Hierarchy:` header and an 80-character separator. Rows contain the
 * dynamically demangled module type followed by the full SystemC object name in parentheses. The top-level
 * rows are indented by two spaces. A filtered module and its entire subtree are omitted unless `all_modules`
 * is true. The object tree is borrowed and is not retained. Call on the simulation thread while the hierarchy
 * is stable (during elaboration or while simulation is paused). When called from Python, keep the GIL held;
 * this function does not release or otherwise access the GIL and does not invoke Python callbacks.
 */
void print_hierarchy(std::ostream& output, bool all_modules = false);

/**
 * @brief Write the full current design hierarchy to standard output.
 * @param all_modules Include native modules whose dynamic type begins with `cynw::`.
 * @throws std::ios_base::failure If standard output is configured to throw on write failure.
 *
 * Uses the same output and lifecycle rules as the stream overload. The standard output stream is borrowed and
 * no native object or callback is retained.
 */
void print_hierarchy(bool all_modules = false);

/**
 * @brief Write one module subtree to a stream.
 * @param output Borrowed output stream; it remains owned by the caller.
 * @param root Borrowed root object; it must remain alive and the hierarchy stable during this call.
 * @param level Indentation depth, measured in two-space levels; must be nonnegative.
 * @param all_modules Include native modules whose dynamic type begins with `cynw::`.
 * @throws std::invalid_argument If `level` is negative.
 * @throws std::ios_base::failure If the output stream is configured to throw on write failure.
 *
 * A null or non-module root produces no output. A filtered module and its subtree are omitted unless
 * `all_modules` is true. The root and children are borrowed and never retained. Call on the simulation thread
 * while the hierarchy is stable (during elaboration or while simulation is paused). When called from Python,
 * keep the GIL held; this function does not release or access the GIL and invokes no Python callbacks.
 */
void print_hierarchy(std::ostream& output, const sc_core::sc_object* root, int level = 0, bool all_modules = false);

/**
 * @brief Write one module subtree to standard output.
 * @param root Borrowed root object; it must remain alive and the hierarchy stable during this call.
 * @param level Indentation depth, measured in two-space levels; must be nonnegative.
 * @param all_modules Include native modules whose dynamic type begins with `cynw::`.
 * @throws std::invalid_argument If `level` is negative.
 * @throws std::ios_base::failure If standard output is configured to throw on write failure.
 *
 * Uses the same traversal and lifecycle rules as the stream overload. The standard output stream is borrowed;
 * no native object or callback is retained.
 */
void print_hierarchy(const sc_core::sc_object* root, int level = 0, bool all_modules = false);

/**
 * @brief Return owned plain-text output for the full current design hierarchy.
 * @param all_modules Include native modules whose dynamic type begins with `cynw::`.
 * @return An owned string containing the full hierarchy header and rows.
 * @throws std::bad_alloc If the returned string cannot be allocated.
 *
 * Uses the same traversal and lifecycle rules as `print_hierarchy(std::ostream&, bool)`, but does not write
 * to standard output. When called from Python, keep the GIL held during the stable hierarchy read; no callback
 * or native object is retained, and the returned string owns its character storage.
 */
std::string hierarchy_text(bool all_modules = false);

/**
 * @brief Return owned plain-text output for a module subtree.
 * @param root Borrowed root object; it must remain alive and the hierarchy stable during this call.
 * @param level Indentation depth, measured in two-space levels; must be nonnegative.
 * @param all_modules Include native modules whose dynamic type begins with `cynw::`.
 * @return An owned string containing subtree rows without a forest header; empty for a null or non-module root.
 * @throws std::invalid_argument If `level` is negative.
 * @throws std::bad_alloc If the returned string cannot be allocated.
 *
 * A filtered root and its subtree produce an empty string unless `all_modules` is true. Call on the simulation
 * thread while the hierarchy is stable (during elaboration or while simulation is paused). When called from
 * Python, keep the GIL held during the traversal. No callback or native object is retained; the returned string
 * owns its character storage.
 */
std::string module_hierarchy_text(const sc_core::sc_object* root, int level = 0, bool all_modules = false);

}  // namespace pysystemc_public

#endif  // PYSYSTEMC_SC_CORE_HIERARCHY_H
