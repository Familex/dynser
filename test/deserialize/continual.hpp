#include "common.hpp"

struct Pos
{
    std::int32_t x{};
    std::int32_t y{};

    constexpr auto operator<=>(Pos const&) const = default;
};

TEST_CASE("continual")
{
    using namespace dynser_test;

    dynser::DynSer ser{
        dynser::generate_property_to_target_mapper([](dynser::Context&, dynser::Properties const& props, Pos& target) {
            target = { props.at("x").as_const_i32(), props.at("y").as_const_i32() };
        }),
        dynser::generate_target_to_property_mapper(),
    };

    const auto config =
#include "../configs/continual.yaml.raw"
        ;

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

    SECTION("pos")
    {
        std::pair<std::string, Pos> poss[]{
            { "1, 2", Pos{ 1, 2 } },
            { "3, 4", Pos{ 3, 4 } },
            { "-3, -5", Pos{ -3, -5 } },
            { "4, -10", Pos{ 4, -10 } },
        };

        for (std::size_t ind{}; auto const& [pos, expected] : poss) {
            DYNAMIC_SECTION("pos #" << ind)
            {
                auto const props = ser.deserialize_to_props(pos, "pos");
                REQUIRE(props);
                INFO("raw props: " << Printer::properties_to_string(*props));
                auto const parsed = ser.deserialize<Pos>(pos, "pos");
                REQUIRE(parsed);
                CHECK(*parsed == expected);
            }
            ++ind;
        }
    }
}
