#pragma once

#include "dynser.h"
#include "printer.hpp"
#include <catch2/catch_test_macros.hpp>

namespace dynser_test
{

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

#define DYNSER_LOAD_CONFIG(ser, config)                                                                                \
    do {                                                                                                               \
        const auto load_result = ser.load_config(config);                                                              \
        REQUIRE(load_result);                                                                                          \
    } while (false);
}    // namespace dynser_test
