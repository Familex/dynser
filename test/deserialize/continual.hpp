#include "common.hpp"

#include <cassert>

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

TEST_CASE("Continual")
{
    using namespace dynser_test;

    dynser::DynSer ser{
        dynser::generate_property_to_target_mapper(
            [](dynser::Context&, dynser::Properties const& props, Pos& target) { target = Pos::from_props(props); },
            [](dynser::Context&, dynser::Properties const& props, Input& target) { target = Input::from_props(props); },
            [](dynser::Context&, dynser::Properties const& props, Bar& target) { target = Bar::from_props(props); },
            [](dynser::Context&, dynser::Properties const& props, Foo& target) { target = Foo::from_props(props); }
        ),
        dynser::generate_target_to_property_mapper(),
    };

    DYNSER_LOAD_CONFIG_FILE(ser, "continual.yaml");

    SECTION("Pos")
    {
        TestCases<Pos> poss{
            { "1, 2", Pos{ 1, 2 } },
            { "3, 4", Pos{ 3, 4 } },
            { "-3, -5", Pos{ -3, -5 } },
            { "4, -10", Pos{ 4, -10 } },
        };

        DYNSER_TEST_FOREACH_DESERIALIZE(Pos, poss, "pos", "Pos #");
    }

    SECTION("Input")
    {
        TestCases<Input> inputs{
            //
            { "from: (1, 2) to: (3, 4)", Input{ { 1, 2 }, { 3, 4 } } },
            { "from: (-4, -1) to: (-2, -10)", Input{ { -4, -1 }, { -2, -10 } } },
            { "from: (100, 200) to: (999, 024)", Input{ { 100, 200 }, { 999, 24 } } },
            { "from: (123, 321) to: (123123, 321321)", Input{ { 123, 321 }, { 123123, 321321 } } },
            { "from: (0, 0) to: (0, 0)", Input{ { 0, 0 }, { 0, 0 } } },
            //
        };

        DYNSER_TEST_FOREACH_DESERIALIZE(Input, inputs, "input", "Input #");
    }

    SECTION("Bar")
    {
        TestCases<Bar> bars{
            { "left", Bar{ true } },
            { "right", Bar{ false } },
        };

        DYNSER_TEST_FOREACH_DESERIALIZE(Bar, bars, "bar", "Bar #");
    }

    SECTION("Foo")
    {
        using dynser::util::map_to_context;

        TestCasesWithCtx<Foo> foos{
            { "right.0.0", Foo{ 0, 0, Bar{ false } }, map_to_context("val-length", "1") },
            { "left.1.1", Foo{ 1, 1, Bar{ true } }, map_to_context("val-length", "1") },
            { "right.-4.88", Foo{ -4, 88, Bar{ false } }, map_to_context("val-length", "2") },
            { "left.-999.-999", Foo{ -999, -999, Bar{ true } }, map_to_context("val-length", "3") },
            { "right.099.-09", Foo{ 99, -9, Bar{ false } }, map_to_context("val-length", "2") },
        };

        DYNSER_TEST_FOREACH_DESERIALIZE_WITH_CONTEXT(Foo, foos, "foo", "Foo #");
    }
}
