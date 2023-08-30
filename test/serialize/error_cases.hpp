#include "common.hpp"

TEST_CASE("Error cases")
{
    using namespace dynser;
    using namespace dynser_test;

    const auto dummy_props{ dynser::Properties{} };

    auto ser = get_dynser_instance();

    SECTION("invalid regex in config")
    {
        DYNSER_LOAD_CONFIG_FILE(ser, "invalid_regex.yaml");

        for (auto const tag : { "a", "b", "c", "d" }) {
            DYNAMIC_SECTION("Tag: " << tag)
            {
                const auto serialize_result{ ser.serialize_props(dummy_props, tag) };

                INFO(
                    "Serialize result is: "
                    << (serialize_result ? *serialize_result
                                         : Printer{}.serialize_err_to_string(serialize_result.error()))
                );
                REQUIRE_FALSE(serialize_result);
                const auto error = serialize_result.error().error;
                REQUIRE(std::holds_alternative<serialize_err::ResolveRegexError>(error));
                const auto resolve_regex_err = std::get<serialize_err::ResolveRegexError>(error).error.error;
                REQUIRE(std::holds_alternative<regex::to_string_err::RegexSyntaxError>(resolve_regex_err));
            }
        }
    }

    SECTION("no value")
    {
        DYNSER_LOAD_CONFIG_FILE(ser, "no_value_error_case.yaml");

        for (auto const tag : { "a", "b", "c", "d", "e", "f", "g" }) {

            DYNAMIC_SECTION("Tag: " << tag)
            {
                using List = PropertyValue::ListType<PropertyValue>;

                const auto props_for_9_tag = util::map_to_props(
                    "value-7", List{ PropertyValue{ "1, " }, PropertyValue{ "2, " }, PropertyValue{ "3, " } }
                );
                const auto serialize_result{ ser.serialize_props(props_for_9_tag, tag) };

                INFO(
                    "Serialize result is: "
                    << (serialize_result ? *serialize_result
                                         : Printer{}.serialize_err_to_string(serialize_result.error()))
                );
                REQUIRE_FALSE(serialize_result);
                const auto error = serialize_result.error().error;
                REQUIRE(std::holds_alternative<serialize_err::ScriptVariableNotFound>(error));
            }
        }
    }

    SECTION("invalid tags")
    {
        DYNSER_LOAD_CONFIG_FILE(ser, "empty.yaml");

        for (auto const tag : { "0", "7", "tag", "unnamed", "test", "foo" }) {
            DYNAMIC_SECTION("Tag: " << tag)
            {
                // try to serialize with invalid tag
                const auto serialize_result{ ser.serialize_props(dummy_props, tag) };

                INFO(
                    "Serialize result is: "
                    << (serialize_result ? *serialize_result
                                         : Printer{}.serialize_err_to_string(serialize_result.error()))
                );
                REQUIRE_FALSE(serialize_result);
                const auto error = serialize_result.error().error;
                REQUIRE(std::holds_alternative<serialize_err::ConfigTagNotFound>(error));
            }
        }
    }
}
