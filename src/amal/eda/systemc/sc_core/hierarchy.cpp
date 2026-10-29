/** @file hierarchy.cpp @brief Plain-text native SystemC hierarchy traversal. */

#include "hierarchy.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include <vector>

#if defined(__GNUG__)
#include <cxxabi.h>
#endif

namespace pysystemc_public
{
namespace
{

/** @brief Return a demangled dynamic type name, falling back to type_info::name(). */
std::string dynamic_type_name(const std::type_info& type)
{
#if defined(__GNUG__)
  int status = -1;
  std::unique_ptr<char, decltype(&std::free)> demangled(abi::__cxa_demangle(type.name(), nullptr, nullptr, &status),
                                                        &std::free);
  if (status == 0 && demangled) return demangled.get();
#endif
  return type.name();
}

/** @brief Return whether a module type is hidden by the default native adapter filter. */
bool is_filtered_type(const std::string& type_name)
{
  return type_name.compare(0, 6, "cynw::") == 0;
}

/** @brief Append the printable module rows for one borrowed object subtree. */
void append_module_rows(std::ostream& output, const sc_core::sc_object* object, int level, bool all_modules)
{
  if (!object) return;

  const auto* module = dynamic_cast<const sc_core::sc_module*>(object);
  if (!module) return;

  const std::string type_name = dynamic_type_name(typeid(*module));
  if (!all_modules && is_filtered_type(type_name)) return;

  for (int depth = 0; depth < level; ++depth) output << "  ";
  output << type_name << " (" << object->name() << ")\n";

  const std::vector<sc_core::sc_object*>& children = object->get_child_objects();
  for (const sc_core::sc_object* child : children) append_module_rows(output, child, level + 1, all_modules);
}

/** @brief Validate the caller-provided indentation depth. */
void validate_level(int level)
{
  if (level < 0) throw std::invalid_argument("hierarchy indentation level must be nonnegative");
}

}  // namespace

void print_hierarchy(std::ostream& output, bool all_modules)
{
  output << "Design Hierarchy:\n" << std::string(80, '=') << '\n';
  const std::vector<sc_core::sc_object*>& top_level_objects = sc_core::sc_get_top_level_objects();
  for (const sc_core::sc_object* object : top_level_objects) append_module_rows(output, object, 1, all_modules);
}

void print_hierarchy(bool all_modules)
{
  print_hierarchy(std::cout, all_modules);
}

void print_hierarchy(std::ostream& output, const sc_core::sc_object* root, int level, bool all_modules)
{
  validate_level(level);
  append_module_rows(output, root, level, all_modules);
}

void print_hierarchy(const sc_core::sc_object* root, int level, bool all_modules)
{
  print_hierarchy(std::cout, root, level, all_modules);
}

std::string hierarchy_text(bool all_modules)
{
  std::ostringstream output;
  print_hierarchy(output, all_modules);
  return output.str();
}

std::string module_hierarchy_text(const sc_core::sc_object* root, int level, bool all_modules)
{
  validate_level(level);
  std::ostringstream output;
  append_module_rows(output, root, level, all_modules);
  return output.str();
}

}  // namespace pysystemc_public
