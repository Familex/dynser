#pragma once

#include "dynser.h"
#include "printer.hpp"
#include "util_common.hpp"
#include <catch2/catch_test_macros.hpp>

namespace dynser_test
{
struct Bar
{
    bool is_left;

    auto operator<=>(const Bar&) const = default;
};

struct Foo
{
    Bar bar;
    std::int32_t dot;
    std::int32_t dyn;

    auto operator<=>(const Foo&) const = default;
};

struct Baz
{
    std::string letter;

    auto operator<=>(const Baz&) const = default;
};

struct Pos
{
    std::int32_t x;
    std::int32_t y;

    auto operator<=>(const Pos&) const = default;
};

struct Input
{
    Pos from;
    Pos to;

    auto operator<=>(const Input&) const = default;
};

struct Quux
{
    std::int32_t value;
};

struct Quuux
{
    Quux quux;
    std::int32_t value;
};

struct VariantStruct
{
    struct A
    {
        std::int32_t value;
    };
    struct B
    {
        std::uint32_t value;
        bool some_bool;
    };

    std::variant<A, B> value;
};

auto bar_to_prop(dynser::Context&, const Bar& target) noexcept
{
    return dynser::Properties{ { "is-left", dynser::PropertyValue{ target.is_left } } };
}

auto pos_to_prop(dynser::Context&, const Pos& target) noexcept
{
    return dynser::Properties{ { "x", dynser::PropertyValue{ target.x } }, { "y", dynser::PropertyValue{ target.y } } };
}

auto quux_to_prop(dynser::Context&, const Quux& target) -> dynser::Properties
{
    return { { "value", dynser::PropertyValue{ target.value } } };
}

auto prop_to_bar(dynser::Context&, dynser::Properties const& props, Bar& out) noexcept
{
    out = { props.at("is-left").as_const_bool() };
}

auto prop_to_pos(dynser::Context&, dynser::Properties const& props, Pos& out) noexcept
{
    out = { props.at("x").as_const_i32(), props.at("y").as_const_i32() };
}

auto get_dynser_instance() noexcept
{
    using namespace dynser::util;

    static dynser::DynSer result{
        // deserialization
        dynser::generate_property_to_target_mapper(
            &prop_to_bar,
            [&](dynser::Context& ctx, dynser::Properties const& props, Foo& out) {
                Bar bar;
                prop_to_bar(ctx, dynser::Properties{ props }, bar);
                out = { bar, props.at("dot-stopped").as_const_i32(), props.at("len-stopped").as_const_i32() };
            },
            [&](dynser::Context&, dynser::Properties const& props, Baz& out) {
                out = { props.at("letter").as_const_string() };
            },
            &prop_to_pos,
            [&](dynser::Context& ctx, dynser::Properties const& props, Input& out) {
                Pos from, to;
                prop_to_pos(ctx, remove_prefix(props, "from"), from);
                prop_to_pos(ctx, remove_prefix(props, "to"), to);
                out = { from, to };
            },
            [&](dynser::Context& ctx, dynser::Properties const& props, std::vector<int>& out) {
                auto props_tmp = props;
                auto last_element = props_tmp.at("last-element").as_const_i32();
                while (props_tmp.contains("element")) {
                    out.push_back(props_tmp.at("element").as_const_i32());
                    props_tmp = remove_prefix(props_tmp, "next");
                }
                out.push_back(last_element);
            },
            [&](dynser::Context&, dynser::Properties const& props, std::vector<Pos>& out) {
                std::unreachable();    // not implemented
            },
            [&](dynser::Context&, dynser::Properties const& props, Quux& out) {
                std::unreachable();    // not implemented
            },
            [&](dynser::Context&, dynser::Properties const& props, Quuux& out) {
                std::unreachable();    // not implemented
            },
            [&](dynser::Context&, dynser::Properties const& props, VariantStruct& out) {
                std::unreachable();    // not implemented
            }
        ),
        // serialization
        dynser::generate_target_to_property_mapper(
            &bar_to_prop,
            [&](dynser::Context& ctx, const Foo& target) {
                return add_prefix(bar_to_prop(ctx, target.bar), "bar") << dynser::Properties{
                    { "dot-stopped", dynser::PropertyValue{ target.dot } },
                    { "len-stopped", dynser::PropertyValue{ target.dyn } },
                };
            },
            [&](dynser::Context&, const Baz& target) {
                return dynser::Properties{ { "letter", dynser::PropertyValue{ target.letter } } };
            },
            &pos_to_prop,
            [&](dynser::Context& ctx, const Input& target) {
                return add_prefix(pos_to_prop(ctx, target.from), "pos", "from")
                       << add_prefix(pos_to_prop(ctx, target.to), "pos", "to");
            },
            [&](dynser::Context& ctx, const std::vector<int>& target) {
                dynser::Properties result;

                if (target.empty()) {
                    return result;
                }

                for (std::size_t cnt{}, ind{}; ind < target.size() - 1; ++ind) {
                    auto val = dynser::PropertyValue{ target[ind] };
                    dynser::Properties val_prop{ { "element", val } };
                    for (std::size_t i{}; i < cnt; ++i) {
                        val_prop = add_prefix(val_prop, "next");
                    }
                    result.merge(val_prop);
                    ++cnt;
                }

                result["last-element"] = dynser::PropertyValue{ target.back() };

                return result;
            },
            [&](dynser::Context&, const std::vector<Pos>& target) -> dynser::Properties {
                using List = dynser::PropertyValue::ListType<dynser::PropertyValue>;

                dynser::Properties result;

                result["x"] = dynser::PropertyValue{ List{} };
                result["y"] = dynser::PropertyValue{ List{} };
                for (const auto& el : target) {
                    result["x"].as_list().push_back(dynser::PropertyValue{ el.x });
                    result["y"].as_list().push_back(dynser::PropertyValue{ el.y });
                }

                return result;
            },
            quux_to_prop,
            [&](dynser::Context& ctx, const Quuux& target) -> dynser::Properties {
                return map_to_props("value", target.value) << add_prefix(quux_to_prop(ctx, target.quux), "quux");
            },
            [&](dynser::Context&, const VariantStruct& target) -> dynser::Properties {
                return dynser::util::visit_one(
                    target.value,
                    [](VariantStruct::A const& a) {    //
                        return map_to_props("is_a", true, "value", a.value);
                    },
                    [](VariantStruct::B const& b) {
                        return map_to_props("is_a", false, "value", b.value, "some_bool", b.some_bool);
                    }
                );
            }
        )
    };

    return result;
}

#define DYNSER_TEST_SERIALIZE(target, tag, expected)                                                                   \
    do {                                                                                                               \
        using namespace dynser_test;                                                                                   \
        const auto serialized = ser.serialize(target, tag);                                                            \
        REQUIRE(serialized);                                                                                           \
        CHECK(*serialized == expected);                                                                                \
    } while (false);

#define DYNSER_TEST_SERIALIZE_CTX(target, tag, expected, context)                                                      \
    do {                                                                                                               \
        using namespace dynser_test;                                                                                   \
        ser.context = context;                                                                                         \
        const auto serialized = ser.serialize(target, tag);                                                            \
        REQUIRE(serialized);                                                                                           \
        CHECK(*serialized == expected);                                                                                \
        ser.context.clear();                                                                                           \
    } while (false);

}    // namespace dynser_test
