#pragma once

#include "dynser.h"
#include "printer.hpp"
#include "util_common.hpp"
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <tuple>

namespace dynser_test
{

template <typename Target>
using TestCases = std::pair<std::string, Target>[];

template <typename Target>
using TestCasesWithCtx = std::tuple<std::string, Target, dynser::Context>[];

#define DYNSER_TEST_DESERIALIZE_WITH_CONTEXT(target_type, target, tag, expected, context_)                             \
    ser.context = context_;                                                                                            \
    auto const props = ser.deserialize_to_props(target, tag);                                                          \
    REQUIRE(props);                                                                                                    \
    INFO("raw props: " << Printer::properties_to_string(*props));                                                      \
    auto const parsed = ser.deserialize<target_type>(target, tag);                                                     \
    REQUIRE(parsed);                                                                                                   \
    CHECK(*parsed == expected);                                                                                        \
    ser.context.clear()

#define DYNSER_TEST_DESERIALIZE(target_type, target, tag, expected)                                                    \
    DYNSER_TEST_DESERIALIZE_WITH_CONTEXT(target_type, target, tag, expected, ::dynser::Context{})

#define DYNSER_TEST_FOREACH_DESERIALIZE(target_type, targets, tag, dyn_section_prefix)                                 \
    for (std::size_t test_ind{}; auto const& [target, expected] : targets) {                                           \
        DYNAMIC_SECTION(dyn_section_prefix << test_ind)                                                                \
        {                                                                                                              \
            DYNSER_TEST_DESERIALIZE(target_type, target, tag, expected);                                               \
        }                                                                                                              \
        ++test_ind;                                                                                                    \
    }

#define DYNSER_TEST_FOREACH_DESERIALIZE_WITH_CONTEXT(target_type, targets, tag, dyn_section_prefix)                    \
    for (std::size_t test_ind{}; auto const& [target, expected, context] : targets) {                                  \
        DYNAMIC_SECTION(dyn_section_prefix << test_ind)                                                                \
        {                                                                                                              \
            DYNSER_TEST_DESERIALIZE_WITH_CONTEXT(target_type, target, tag, expected, context);                         \
        }                                                                                                              \
        ++test_ind;                                                                                                    \
    }

}    // namespace dynser_test
