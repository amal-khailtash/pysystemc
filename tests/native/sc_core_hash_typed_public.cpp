/** @file sc_core_hash_typed_public.cpp
 * @brief Native reference for typed PD and string hash families.
 */
#include <algorithm>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "sc_core_hash_typed_probe.h"

using namespace hash_typed_probe;

/** @brief Exercise typed table copies, content replacement, ownership and removal. */
int sc_main(int, char**)
{
  std::cout << "HASH_ALIASES|1|1\n";
  {
    pd_hash default_pd;
    str_hash default_str;
    const bool pd_empty = default_pd.empty();
    const bool str_empty = default_str.empty();
    default_pd.erase();
    default_str.erase();
    std::cout << "HASH_CONSTRUCTORS|" << pd_empty << '|' << str_empty << '\n';
  }
  reset_counts();
  {
    owned_pd_hash source;
    std::string alpha("alpha"), beta("beta"), gamma("gamma");
    const int insert_alpha = source.insert(alpha.data(), content_pointer(0x101));
    const int insert_beta = source.insert(beta.data(), content_pointer(0x202));
    const int insert_gamma = source.insert(gamma.data(), content_pointer(0x303));
    const int replace_beta = source.insert(beta.data(), content_pointer(0x204));
    const int keep_beta = source.insert_if_not_exists(beta.data(), content_pointer(0x205));

    owned_pd_hash copied;
    copied.copy(source);
    const auto source_beta = pointer_value(source["beta"]);
    const auto copied_beta = pointer_value(copied["beta"]);
    const unsigned copied_count = copied.count();
    const bool null_lookup = source.lookup(beta.data(), nullptr) && !source.lookup("missing", nullptr);

    pd_iter iterator(copied);
    while (!iterator.empty())
    {
      if (pointer_value(iterator.contents()) == 0x303)
        iterator.remove();
      else
      {
        if (key_text(iterator.key()) == "beta") iterator.set_contents(content_pointer(0x206));
        iterator++;
      }
    }
    pd_iter pointer_constructed(&copied);
    const bool pointer_iterator = !pointer_constructed.empty();
    iterator.reset(copied);
    const bool reference_reset = !iterator.empty();
    iterator.reset(&copied);
    const bool pointer_reset = !iterator.empty();
    const int removed_alpha = copied.remove("alpha");
    std::cout << "HASH_PD_RESULTS|" << insert_alpha << '|' << insert_beta << '|' << insert_gamma << '|' << replace_beta
              << '|' << keep_beta << '|' << source_beta << '|' << copied_beta << '|' << copied_count << '|'
              << null_lookup << '|' << duplicate_count << '\n';
    std::cout << "HASH_PD_MUTATION|" << removed_alpha << '|' << copied.count() << '|' << pointer_value(copied["beta"])
              << '|' << free_count << '\n';
    source.erase();
    std::cout << "HASH_PD_ITERATOR|" << pointer_iterator << '|' << reference_reset << '|' << pointer_reset << '\n';
  }
  std::cout << "HASH_PD_OWNERSHIP|" << duplicate_count << '|' << free_count << '\n';

  reset_counts();
  {
    owned_pd_hash defaults;
    std::string key("default");
    const int inserted = defaults.insert(key.data());
    const int existing = defaults.insert_if_not_exists(key.data());
    const auto value = pointer_value(defaults[key.data()]);
    defaults.erase();
    std::cout << "HASH_PD_DEFAULT|" << inserted << '|' << existing << '|' << value << '|' << duplicate_count << '|'
              << free_count << '\n';
  }
  reset_counts();
  {
    owned_pd_hash outputs;
    std::string key("output");
    outputs.insert(key.data(), content_pointer(0x404));
    pd_slots lookup_output;
    const int found = lookup_pd(outputs, key.data(), lookup_output);
    const auto lookup_value = pointer_value(lookup_output.contents);
    pd_slots remove_output;
    const int removed = remove_pd(outputs, key.data(), remove_output);
    const std::string removed_key = key_text(remove_output.key);
    const auto removed_value = pointer_value(remove_output.contents);
    free_key(remove_output.key);
    outputs.erase();
    std::cout << "HASH_PD_OUT|" << found << '|' << lookup_value << '|' << removed << '|' << removed_key << '|'
              << removed_value << '|' << outputs.count() << '|' << duplicate_count << '|' << free_count << '\n';
  }
  reset_counts();
  {
    owned_pd_hash removals;
    std::string alpha("one"), beta("two"), gamma("three");
    removals.insert(alpha.data(), content_pointer(0x501));
    removals.insert(beta.data(), content_pointer(0x502));
    removals.insert(gamma.data(), content_pointer(0x503));
    const int direct = removals.remove_by_contents(content_pointer(0x502));
    const int predicate = removals.remove_by_contents(content_matches, content_pointer(0x503));
    const unsigned count = removals.count();
    removals.erase();
    std::cout << "HASH_PD_REMOVE_CONTENTS|" << direct << '|' << predicate << '|' << count << '|' << duplicate_count
              << '|' << free_count << '\n';
  }

  {
    str_hash source;
    std::string alpha("alpha"), beta("beta"), gamma("gamma");
    const int insert_alpha = insert_string(source, alpha.data(), content_pointer(0x111));
    const int insert_beta = insert_string(source, beta.data(), content_pointer(0x222));
    const int insert_gamma = insert_string(source, gamma.data(), content_pointer(0x333));
    const int replace_beta = insert_string(source, beta.data(), content_pointer(0x224));
    const int keep_beta = insert_string_if_absent(source, beta.data(), content_pointer(0x225));
    const bool null_lookup = source.lookup("beta", nullptr) && !source.lookup("missing", nullptr);

    str_hash copied;
    copied.copy(source);
    str_hash copied_pointer;
    copied_pointer.copy(&source);
    const auto source_beta = pointer_value(source["beta"]);
    const auto copied_beta = pointer_value(copied["beta"]);
    const auto copied_pointer_beta = pointer_value(copied_pointer["beta"]);
    const unsigned copied_count = copied.count();
    const unsigned copied_pointer_count = copied_pointer.count();

    str_iter iterator(copied);
    while (!iterator.empty())
    {
      if (pointer_value(iterator.contents()) == 0x333)
        iterator.remove();
      else
      {
        if (key_text(iterator.key()) == "beta") iterator.set_contents(content_pointer(0x226));
        iterator++;
      }
    }
    str_iter pointer_constructed(&copied);
    const bool pointer_iterator = !pointer_constructed.empty();
    iterator.reset(copied);
    const bool reference_reset = !iterator.empty();
    iterator.reset(&copied);
    const bool pointer_reset = !iterator.empty();
    const int removed_alpha = copied.remove("alpha");
    std::cout << "HASH_STR_RESULTS|" << insert_alpha << '|' << insert_beta << '|' << insert_gamma << '|' << replace_beta
              << '|' << keep_beta << '|' << source_beta << '|' << copied_beta << '|' << copied_count << '|'
              << copied_pointer_beta << '|' << copied_pointer_count << '|' << null_lookup << '\n';
    std::cout << "HASH_STR_MUTATION|" << removed_alpha << '|' << copied.count() << '|' << pointer_value(copied["beta"])
              << '\n';
    std::cout << "HASH_STR_ITERATOR|" << pointer_iterator << '|' << reference_reset << '|' << pointer_reset << '\n';
    copied.erase();
    copied_pointer.erase();
    source.erase();
  }

  {
    str_hash defaults;
    const int inserted = insert_string(defaults, "default", nullptr);
    const int existing = insert_string_if_absent(defaults, "default", nullptr);
    const auto value = pointer_value(defaults["default"]);
    defaults.erase();
    std::cout << "HASH_STR_DEFAULT|" << inserted << '|' << existing << '|' << value << '\n';
  }
  {
    str_hash outputs;
    insert_string(outputs, "output", content_pointer(0x414));
    str_slots lookup_output;
    const int found = lookup_string(outputs, "output", lookup_output);
    const auto lookup_value = pointer_value(lookup_output.contents);
    str_slots remove_output;
    const int removed = remove_string(outputs, "output", remove_output);
    const std::string removed_key = key_text(remove_output.key);
    const auto removed_value = pointer_value(remove_output.contents);
    sc_core::sc_strhash_kfree(remove_output.key);
    outputs.erase();
    std::cout << "HASH_STR_OUT|" << found << '|' << lookup_value << '|' << removed << '|' << removed_key << '|'
              << removed_value << '|' << outputs.count() << '\n';
  }
  {
    str_hash removals;
    insert_string(removals, "one", content_pointer(0x601));
    insert_string(removals, "two", content_pointer(0x602));
    insert_string(removals, "three", content_pointer(0x603));
    const int direct = removals.remove_by_contents(content_pointer(0x602));
    const int predicate = removals.remove_by_contents(content_matches, content_pointer(0x603));
    const unsigned count = removals.count();
    removals.erase();
    std::cout << "HASH_STR_REMOVE_CONTENTS|" << direct << '|' << predicate << '|' << count << '\n';
  }
  return 0;
}
