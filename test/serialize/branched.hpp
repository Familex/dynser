#include "common.hpp"

TEST_CASE("Branched rule")
{
    using namespace dynser_test;

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG_FILE(ser, "branched.yaml");

    SECTION("'Bar' rule", "[branched] [linear]")
    {

        std::pair<Bar, std::string> bars[]{
            { { false }, "right" },
            { { true }, "left" },
        };

        for (auto const& [bar, expected] : bars) {
            DYNSER_TEST_SERIALIZE(bar, "bar", expected);
        }
    }

    SECTION("'Baz' rule", "[branched] [linear] [existing] [helper] [context]")
    {
        using Prop = dynser::PropertyValue;

        std::tuple<Baz, dynser::Context, std::string> bazs[]{
            { { "aaaa" }, { { "type", Prop{ "a" } } }, "aaaa" },
            { { "bbbb" }, { { "type", Prop{ "b" } } }, "bbbb" },
            { { "" }, { { "type", Prop{ "a" } } }, "" },
        };

        for (auto const& [baz, context, expected] : bazs) {
            DYNSER_TEST_SERIALIZE_CTX(baz, "baz", expected, context);
        }
    }
}

TEST_CASE("'VariantStruct'")
{
    using namespace dynser_test;
    using A = VariantStruct::A;
    using B = VariantStruct::B;

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG_FILE(ser, "variant_struct.yaml");

    std::pair<VariantStruct, std::string> variant_structs[]{
        { { A{ 42 } }, "42" },
        { { A{ 0xFFFFFFD } }, "268435453" },
        { { B{ 42, true } }, "true;42" },
        { { B{ 42, false } }, "false;42" },
        { { B{ 0xFFFFFFFD, true } }, "true;4294967293" },
    };

    for (auto const& [vs, expected] : variant_structs) {
        DYNSER_TEST_SERIALIZE(vs, "variant-struct", expected)
    }
}
