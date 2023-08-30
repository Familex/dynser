#include "common.hpp"

#include <regex>

TEST_CASE("Throw errors from lua scripts")
{
    using namespace dynser_test;

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG_FILE(ser, "throw_lua_errors.yaml");

    const std::pair<Pos, std::string> poss[]{
        { { 1, 2 }, "x < 10" },    //
        { { 40, 1000 }, "x > 20" },       { { 15, -100 }, "y < 0" },
        { { 16, 0xFFFFF }, "y > 10000" }, { { 14, 88 }, "all ok" },
    };

    for (std::size_t ind{}; const auto& [pos, expected_error] : poss) {
        DYNAMIC_SECTION("Pos: #" << ind)
        {
            const auto ser_result = ser.serialize(pos, "pos");
            REQUIRE(!ser_result);
            const auto& error = ser_result.error().error;
            REQUIRE(std::holds_alternative<dynser::serialize_err::ScriptError>(error));
            const auto& script_error = std::get<dynser::serialize_err::ScriptError>(error);
            // match error message without error position etc
            std::regex regex{ ".*" + expected_error + ".*" };
            CHECK(std::regex_match(script_error.message, regex));
        }
        ++ind;
    }
}
