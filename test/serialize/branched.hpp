#include "common.hpp"

TEST_CASE("Branched rule")
{
    using namespace dynser_test;

    const auto config =
#include "../configs/branched.yaml.raw"
        ;

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

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

    const auto config = R"##(---
version: ''
tags:
  - name: "variant-struct"
    branched:
      branching-script: |
        branch = inp['is_a']:as_bool() and 0 or 1
      debranching-script: |
        not implemented
      rules:
        - existing: { tag: "variant-struct-a" }
        - existing: { tag: "variant-struct-b" }
  
  - name: "variant-struct-a"
    continual:
      - linear: { pattern: '-?\d+', fields: { 0: value } }
    serialization-script: |
      out['value'] = tostring(inp['value']:as_i32())
  
  - name: "variant-struct-b"
    continual:
      - linear: { pattern: '((?:true)|(?:false));(\d+)', fields: { 2: value, 1: some_bool } }
    serialization-script: |
      out['value'] = tostring(inp['value']:as_u32())
      out['some_bool'] = inp['some_bool']:as_bool() and 'true' or 'false'
...)##";

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

    {
        using A = VariantStruct::A;
        using B = VariantStruct::B;

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
}
