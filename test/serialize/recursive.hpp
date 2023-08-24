#include "common.hpp"

TEST_CASE("Recursive rule")
{
    using namespace dynser_test;

    const auto config =
#include "../configs/recursive.yaml.raw"
        ;

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

    SECTION("'recursive' rule", "[continual] [recursive] [existing] [linear] [required]")
    {
        std::pair<std::vector<int>, std::string> recursives[]{
            { { 1, 2, 3 }, "[ 1, 2, 3 ]" },
            // { {}, "[  ]" }, // not implemented
            { { -1, -3, 10, 0, 9999 }, "[ -1, -3, 10, 0, 9999 ]" },
        };

        for (auto const& [recursive, expected] : recursives) {
            DYNSER_TEST_SERIALIZE(recursive, "recursive", expected);
        }
    }
}
