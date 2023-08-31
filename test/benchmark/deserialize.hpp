#include "dynser.h"
#include "printer.hpp"
#include "util_common.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Deserialize")
{
    using namespace dynser;

    // mappers not necessary, serialize_props may be used
    DynSer ser{};

    // load config
    {
        // to prevent optimizations
        config::ParseResult config_load_result;

        BENCHMARK("load config")
        {
            const auto load_result{ ser.load_config(config::FileName{ dynser_test::path_to_config("benchmark.yaml") }
            ) };
            config_load_result = load_result;
            return load_result;
        };

        REQUIRE(config_load_result);
    }

    {
        // without result check
#define DYNSER_BENCHMARK_DESERIALIZE_TO_PROPS(ser_, props_, str_literal_tag_, context_)                                \
    /* scope for non-benchmark variables */                                                                            \
    do {                                                                                                               \
        auto const context = context_;                                                                                 \
        auto const props = props_;                                                                                     \
        auto const tag = str_literal_tag_;                                                                             \
        auto const target = ser.serialize_props(props, tag);                                                           \
        REQUIRE(target);                                                                                               \
        BENCHMARK("Tag: " str_literal_tag_)                                                                            \
        {                                                                                                              \
            ser_.context = context;                                                                                    \
            const auto result = ser.deserialize_to_props(*target, tag);                                                \
            REQUIRE(result);                                                                                           \
            ser_.context.clear();                                                                                      \
            return result;                                                                                             \
        };                                                                                                             \
    } while (false)

        using List = PropertyValue::ListType<PropertyValue>;

        DYNSER_BENCHMARK_DESERIALIZE_TO_PROPS(ser, Properties{}, "continual-without-fields", Context{});
        DYNSER_BENCHMARK_DESERIALIZE_TO_PROPS(ser, Properties{}, "empty", Context{});
        DYNSER_BENCHMARK_DESERIALIZE_TO_PROPS(
            ser, util::map_to_props("value", "lorem ipsum"), "minimal-lua", Context{}
        );
        DYNSER_BENCHMARK_DESERIALIZE_TO_PROPS(
            ser, util::map_to_props("len-expander", List(100ull, PropertyValue{ "" })), "recurrent-empty", Context{}
        );
        DYNSER_BENCHMARK_DESERIALIZE_TO_PROPS(
            ser,
            util::map_to_props("len-expander", List(100ull, PropertyValue{ "" })),
            "recurrent-existing-empty",
            Context{}
        );
#undef DYNSER_BANCHMARK_SERIALIZE_PROPS
    }
}
