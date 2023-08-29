#include "dynser.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Regex to string")
{
    using namespace dynser::regex;
    using dynser::config::yaml::GroupValues;

    struct Test
    {
        std::string regex;
        GroupValues values;
        std::optional<std::size_t> resolve_error_group_num{ std::nullopt };
    };

    Test tests[]{
        { R"((?:[)](\w+)){2})", { { 1, "word" } } },                 //
        { R"((?:\)\(\[\][\]]){2,})", {} },                           //
        { "[[\\]]", {} },                                            //
        { "[\\][]", {} },                                            //
        { "(\\d{2})", { { 1, "099" } } },                            //
        { "(\\d{4,})", { { 1, "9" } } },                             //
        { "(\\w{10})!", { { 1, "ha" } } },                           //
        { "(\\d{2})", { { 1, "999" } }, 1 },                         //
        { "(\\d+){2}", { { 1, "10" } } },                            //
        { "(\\d{2})", { { 1, "99" } } },                             //
        { "(.+)[\\\\/](.+)", { { 1, "left" }, { 2, "right" } } },    //
        { "(.+)\\1{4,5}", { { 1, "go!" } } },                        //
        { "", { { 1, "this group not exists" } } },                  //
    };

    for (std::size_t num{}; const auto& test : tests) {
        DYNAMIC_SECTION("Regex#" << num)
        {
            const auto regex = from_string(test.regex);

            if (!regex) {
                UNSCOPED_INFO("error on " << regex.error() << " index");
            }
            REQUIRE(regex);

            const auto string = to_string(*regex, test.values);

            if (!string && !test.resolve_error_group_num) {
                INFO("resolve error on " << string.error().group_num << " group");
                FAIL();
            }
            else if (!string && string.error().group_num != test.resolve_error_group_num) {
                INFO(
                    "expected resolve error group num: "
                    << *test.resolve_error_group_num << " not equal to actual group num: " << string.error().group_num
                );
                FAIL();
            }
            else {
                SUCCEED();
            }
        }
        ++num;
    }
}
