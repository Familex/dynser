#include "dynser.h"
#include "printer.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Serialize")
{
    using namespace dynser;

    // mappers not necessary, serialize_props may be used
    DynSer ser{};

    // load config
    {
        // to prevent optimizations
        config::ParseResult config_load_result;

        const auto config =
#include "../configs/benchmark_serialize.yaml.raw"
            ;

        BENCHMARK("load config")
        {
            const auto load_result{ ser.load_config(config::RawContents{ config }) };
            config_load_result = load_result;
            return load_result;
        };

        REQUIRE(config_load_result);
    }

    // serialize tests
    {
#define DYNSER_BENCHMARK_SERIALIZE_PROPS(ser_, props_, str_literal_tag_, context_)                                     \
    BENCHMARK("Tag: " str_literal_tag_)                                                                                \
    {                                                                                                                  \
        const auto context = context_;                                                                                 \
        const auto props = props_;                                                                                     \
        const auto tag = str_literal_tag_;                                                                             \
        ser_.context = context;                                                                                        \
        const auto result = ser.serialize_props(props, tag);                                                           \
        INFO(                                                                                                          \
            "Serialize result is: "                                                                                    \
            << (result ? *result : dynser_test::Printer{}.serialize_err_to_string(result.error()))                     \
        );                                                                                                             \
        REQUIRE(result);                                                                                               \
        ser_.context.clear();                                                                                          \
        return result;                                                                                                 \
    }

        using List = PropertyValue::ListType<PropertyValue>;

        DYNSER_BENCHMARK_SERIALIZE_PROPS(ser, Properties{}, "continual-without-fields", Context{});
        DYNSER_BENCHMARK_SERIALIZE_PROPS(ser, Properties{}, "empty", Context{});
        DYNSER_BENCHMARK_SERIALIZE_PROPS(ser, util::map_to_props("value", "lorem ipsum"), "minimal-lua", Context{});
        DYNSER_BENCHMARK_SERIALIZE_PROPS(
            ser, util::map_to_props("len-expander", List(100ull, PropertyValue{ "" })), "recurrent-empty", Context{}
        );
        DYNSER_BENCHMARK_SERIALIZE_PROPS(
            ser,
            util::map_to_props("len-expander", List(100ull, PropertyValue{ "" })),
            "recurrent-existing-empty",
            Context{}
        );

#undef DYNSER_BANCHMARK_SERIALIZE_PROPS
    }
}
