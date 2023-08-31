#pragma once

#include "dynser.h"
#include "printer.hpp"
#include "util_common.hpp"
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <tuple>

namespace dynser_test
{

struct Pos
{
    std::int32_t x{};
    std::int32_t y{};

    Pos static from_props(dynser::Properties const& props) noexcept
    {
        assert(props.contains("x"));
        assert(props.contains("y"));
        return {
            props.at("x").as_const_i32(),
            props.at("y").as_const_i32(),
        };
    }

    constexpr auto operator<=>(Pos const&) const = default;
};

struct Input
{
    Pos from{};
    Pos to{};

    Input static from_props(dynser::Properties const& props) noexcept
    {
        assert(props.size() >= 2);
        return { Pos::from_props(dynser::util::remove_prefix(props, "from")),
                 Pos::from_props(dynser::util::remove_prefix(props, "to")) };
    }

    constexpr auto operator<=>(Input const&) const = default;
};

struct Bar
{
    bool is_left{};

    Bar static from_props(dynser::Properties const& props) noexcept
    {
        assert(props.contains("is-left"));
        return {
            props.at("is-left").as_const_bool(),
        };
    }

    constexpr auto operator<=>(Bar const&) const = default;
};

struct Foo
{
    std::int32_t dot_stopped{};
    std::int32_t len_stopped{};
    Bar bar{};

    Foo static from_props(dynser::Properties const& props) noexcept
    {
        assert(props.contains("dot-stopped"));
        assert(props.contains("len-stopped"));
        return {
            props.at("dot-stopped").as_const_i32(),
            props.at("len-stopped").as_const_i32(),
            Bar::from_props(props),
        };
    }

    constexpr auto operator<=>(Foo const&) const = default;
};

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
