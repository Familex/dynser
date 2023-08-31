#include "common.hpp"

TEST_CASE("Recurrent")
{
    using namespace dynser_test;

    dynser::DynSer ser{
        dynser::generate_property_to_target_mapper(
            [](dynser::Context, dynser::Properties const& props, std::vector<Pos>& target) {
                if (props.empty()) {
                    target = {};
                    return;
                }
                assert(props.contains("x"));
                assert(props.contains("y"));
                auto const x_list = props.at("x").as_const_list();
                auto const y_list = props.at("y").as_const_list();
                for (std::size_t ind{}; ind < x_list.size(); ++ind) {
                    target.emplace_back(x_list[ind].as_const_i32(), y_list[ind].as_const_i32());
                }
            }
        ),
        dynser::generate_target_to_property_mapper(),
    };

    DYNSER_LOAD_CONFIG_FILE(ser, "recurrent.yaml");

    SECTION("'Pos-list' rule", "[recurrent]")
    {
        TestCases<std::vector<Pos>> lists{
            { "[ ( 1, 2 ), ( 3, 4 ) ]", { { 1, 2 }, { 3, 4 } } },
            { "[ ( -1, -254 ), ( 123, 0 ), ( 0, 2 ) ]", { { -1, -254 }, { 123, 0 }, { 00, 2 } } },
            { "[  ]", {} },
            { "[ ( 0, 0 ) ]", { {} } },
        };

        DYNSER_TEST_FOREACH_DESERIALIZE(std::vector<Pos>, lists, "pos-list", "Pos list #");
    }
}
