#include "common.hpp"

TEST_CASE("Recurrent rule")
{
    using namespace dynser_test;

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG_FILE(ser, "recurrent.yaml");

    SECTION("'Pos-list' rule", "[recurrent]")
    {
        std::pair<std::vector<Pos>, std::string> lists[]{
            { { { 1, 2 }, { 3, 4 } }, "[ ( 1, 2 ), ( 3, 4 ) ]" },
            { { { -1, -254 }, { 123, 0 }, { 00, 2 } }, "[ ( -1, -254 ), ( 123, 0 ), ( 0, 2 ) ]" },
            { {}, "[  ]" },
            { { {} }, "[ ( 0, 0 ) ]" },
        };

        for (auto const& [list, expected] : lists) {
            DYNSER_TEST_SERIALIZE(list, "pos-list", expected);
        }
    }
}
