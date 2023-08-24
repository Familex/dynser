#include "common.hpp"

TEST_CASE("Continual rule #0")
{
    using namespace dynser_test;

    const auto config =
#include "../configs/continual.yaml.raw"
        ;

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

    SECTION("'Foo' rule", "[continual] [existing] [linear] [dyn-groups]")
    {
        const std::pair<Foo, std::string> foos[]{
            { { { false }, 1, 2 }, "right.1.02" },
            { { { true }, -1, -3 }, "left.-1.-03" },
            { { { false }, 0, 42 }, "right.0.42" },
            { { { true }, -2, 32 }, "left.-2.32" },
        };

        ser.context["val-length"] = dynser::PropertyValue{ "2" };
        for (const auto& [foo, expected] : foos) {
            DYNSER_TEST_SERIALIZE(foo, "foo", expected);
        }
        ser.context.erase("val-length");
    }

    SECTION("'Pos' rule", "[continual] [linear]")
    {
        const std::pair<Pos, std::string> poss[]{
            { { 1, 2 }, "1, 2" },
            { { -1, -3 }, "-1, -3" },
            { { 0, 4293291 }, "0, 4293291" },
            { { -2, 328238 }, "-2, 328238" },
        };

        for (const auto& [pos, expected] : poss) {
            DYNSER_TEST_SERIALIZE(pos, "pos", expected);
        }
    }

    SECTION("'Input' rule", "[continual] [linear] [existing]")
    {
        const std::pair<Input, std::string> inputs[]    //
            { { {}, "from: (0, 0) to: (0, 0)" },        //
              { { { -1, -1 }, { 2, 2 } }, "from: (-1, -1) to: (2, 2)" } };

        for (const auto& [input, expected] : inputs) {
            DYNSER_TEST_SERIALIZE(input, "input", expected);
        }
    }
}

TEST_CASE("Overlapping properties")
{
    using namespace dynser_test;

    const auto config = R"##(---
version: ''
tags:
  - name: quux
    continual:
      - linear: { pattern: '-?\d+', fields: { 0: value } }
    serialization-script: |
      out['value'] = tostring(inp['value']:as_i32())
  
  - name: quuux
    continual:
      - existing: { tag: "quux" }
      - linear: { pattern: ' ## ' }
      - linear: { pattern: '-?\d+', fields: { 0: value } }
    serialization-script: |
      out['value'] = tostring(inp['value']:as_i32())
...)##";

    auto ser = get_dynser_instance();

    const auto result = ser.load_config(dynser::config::RawContents{ config });

    INFO("Config: " << config);
    REQUIRE(result);

    {
        const std::pair<Quuux, std::string> quuuxs[]{
            { { { 1 }, 2 }, "1 ## 2" },
            { { { -1 }, -3 }, "-1 ## -3" },
            { { { 0 }, 4293291 }, "0 ## 4293291" },
            { { { -2 }, 328238 }, "-2 ## 328238" },
            { { { 10 }, 10 }, "10 ## 10" },
        };

        for (const auto& [quuux, expected] : quuuxs) {
            DYNSER_TEST_SERIALIZE(quuux, "quuux", expected);
        }
    }
}
