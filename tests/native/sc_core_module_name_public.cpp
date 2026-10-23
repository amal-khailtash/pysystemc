/** @file sc_core_module_name_public.cpp
 *  @brief Native reference for public sc_module_name construction and lifetime.
 */
#include <iostream>
#include <string>
#include <systemc>

/** @brief Expose the protected native module constructor for hierarchy checks. */
struct NamedModule : sc_core::sc_module
{
  /** @brief Construct a module within the supplied native name scope. */
  explicit NamedModule(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
  }
};

/** @brief Exercise the native conversion operator while the borrowed string lives. */
static std::string module_name_text(const sc_core::sc_module_name& name)
{
  return static_cast<const char*>(name);
}

/** @brief Compare constructor, copy, conversion, destructor and hierarchy behavior. */
int sc_main(int, char**)
{
  using namespace sc_core;
  std::string source("root");
  auto* root_name = new sc_module_name(source.c_str());
  auto* root = new NamedModule(*root_name);
  const std::string root_path = root->name();
  const std::string root_text = module_name_text(*root_name);
  if (root_path != "root" || root_text != "root") return 1;

  std::string copied_text;
  {
    sc_module_name copied_name(*root_name);
    copied_text = module_name_text(copied_name);
    if (copied_text != "root") return 2;
  }

  std::string child_path;
  std::string grandchild_path;
  std::string marker_path;
  std::string after_grandchild_path;
  std::string after_child_path;
  {
    auto* child_name = new sc_module_name("child");
    auto* child = new NamedModule(*child_name);
    child_path = child->name();
    {
      auto* grandchild_name = new sc_module_name("grandchild");
      auto* grandchild = new NamedModule(*grandchild_name);
      sc_event grandchild_marker("marker");
      grandchild_path = grandchild->name();
      marker_path = grandchild_marker.name();
      if (child_path != "root.child" || grandchild_path != "root.child.grandchild" ||
          marker_path != "root.child.grandchild.marker")
        return 3;
      delete grandchild_name;
      sc_event after_grandchild("after_grandchild");
      after_grandchild_path = after_grandchild.name();
      if (after_grandchild_path != "root.child.after_grandchild") return 4;
      delete grandchild;
    }
    delete child_name;
    sc_event after_child("after_child");
    after_child_path = after_child.name();
    if (after_child_path != "root.after_child") return 5;
    delete child;
  }
  delete root_name;
  sc_event outside("outside");
  const std::string outside_path = outside.name();
  if (outside_path != "outside") return 6;
  delete root;

  std::cout << "MODULE_NAME_REFERENCE " << root_path << ' ' << root_text << ' ' << copied_text << ' ' << child_path
            << ' ' << grandchild_path << ' ' << marker_path << ' ' << after_grandchild_path << ' ' << after_child_path
            << ' ' << outside_path << '\n';
  return 0;
}
