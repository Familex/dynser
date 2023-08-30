#include "common.hpp"

TEST_CASE("Recursive rule")
{
    using namespace dynser_test;

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG_FILE(ser, "recursive.yaml");

    SECTION("'recursive' rule", "[continual] [recursive] [existing] [linear] [required]")
    {
        std::pair<std::vector<int>, std::string> recursives[]{
            { { 1, 2, 3 }, "[ 1, 2, 3 ]" },
            { {}, "[ ]" },
            { { -1, -3, 10, 0, 9999 }, "[ -1, -3, 10, 0, 9999 ]" },
            { { 0, 12, 12, 12, 12, 12, 12, 13, 0, 0, 0 }, "[ 0, 12, 12, 12, 12, 12, 12, 13, 0, 0, 0 ]" },
            { { 1 }, "[ 1 ]" },
            { { 12 }, "[ 12 ]" },
            { { 13, -21 }, "[ 13, -21 ]" },
            { { -10 }, "[ -10 ]" },
        };

        for (auto const& [recursive, expected] : recursives) {
            DYNSER_TEST_SERIALIZE(recursive, "recursive", expected);
        }
    }
}
