#pragma once

#include "dynser.h"
#include <catch2/catch_tostring.hpp>

#include <format>

namespace dynser_test
{

struct Printer
{
    [[nodiscard]] static std::string serialize_err_to_string(dynser::SerializeError const& wrapper) noexcept
    {
        using namespace dynser::serialize_err;

        std::string ref_str;

        for (const auto& el : wrapper.ref_seq) {
            ref_str += std::format(" from '{}' tag at '{}' rule", el.tag, el.rule_ind);
        }

        auto error_str = dynser::util::visit_one_terminated(
            wrapper.error,
            [](const Unknown&) -> std::string { return "unknown error"; },
            [](const ConfigNotLoaded&) -> std::string { return "config not loaded"; },
            [](const ConfigTagNotFound& error) -> std::string {
                return std::format("config tag '{}' not found", error.tag);
            },
            [](const BranchNotSet&) -> std::string { return "branch not set"; },
            [](const BranchOutOfBounds& error) -> std::string {
                return std::format(
                    "seleced branch: '{}'; expected between 0 and '{}'", error.selected_branch, error.max_branch
                );
            },
            [](const ScriptError& error) -> std::string {
                return std::format("lua script error: '{}'", error.message);
            },
            [](const ScriptVariableNotFound& error) {
                return std::format("script variable '{}' not set", error.variable_name);
            },
            [](const ResolveRegexError& error) -> std::string {
                using namespace dynser::regex::to_string_err;

                return std::format(
                    "regex error '{}' on group '{}'",
                    dynser::util::visit_one_terminated(
                        error.error.error,
                        [](const RegexSyntaxError& reg_err) -> std::string {
                            return std::format("syntax error at {}", reg_err.position);
                        },
                        [](const MissingValue&) -> std::string { return "missing value"; },
                        [](const InvalidValue& reg_err) -> std::string {
                            return std::format("invalid value: '{}'", reg_err.value);
                        }
                    ),
                    error.error.group_num
                );
            },
            [](const RecurrentDictKeyNotFound& error) -> std::string {
                return std::format("recurrent-dict key '{}' not found", error.key);
            }
        );

        auto scope_props = properties_to_string(wrapper.scope_props);

        return error_str + ref_str + " with props in scope: " + scope_props;
    }

    [[nodiscard]] static std::string config_parse_err_to_string(dynser::config::ParseError const& error) noexcept
    {
        using enum dynser::config::ParseError::Type;

        switch (error.type) {
            case ParserException:
                return std::format(
                    "yaml-cpp parser exception at 'pos: {}, line: {}, col: {}' with msg: {}",
                    error.mark.pos,
                    error.mark.line,
                    error.mark.column,
                    error.msg
                );
            case RepresentationException:
                return std::format(
                    "yaml-cpp representation exception at 'pos: {}, line: {}, col: {}' with msg: {}",
                    error.mark.pos,
                    error.mark.line,
                    error.mark.column,
                    error.msg
                );
            case UnknownYamlCppException:
                return std::format(
                    "yaml-cpp unknown exception at 'pos: {}, line: {}, col: {}' with msg: {}",
                    error.mark.pos,
                    error.mark.line,
                    error.mark.column,
                    error.msg
                );
            case UnknownException:
                return std::format("unknown exception");
        }
        std::unreachable();
    }

    [[nodiscard]] static std::string regex_parse_err_to_string(dynser::regex::ParseError const& error)
    {
        return std::to_string(error);
    }

    [[nodiscard]] static std::string properties_to_string(dynser::Properties const& props) noexcept
    {
        if (props.empty()) {
            return "{}";
        }
        std::string result{ "{" };
        for (auto const& [key, val] : props) {
            result += key;
            result += ": ";
            result += property_value_to_string(val);
            result += ", ";
        }
        result.pop_back();
        result.pop_back();
        result += "}";
        return result;
    }

    [[nodiscard]] static std::string property_value_to_string(dynser::PropertyValue const& value) noexcept
    {
#define DYNSER_USE_STD_TO_STRING(type)                                                                                 \
    if (value.is_##type()) {                                                                                           \
        return std::to_string(value.as_const_##type());                                                                \
    }

        DYNSER_USE_STD_TO_STRING(i32)
        DYNSER_USE_STD_TO_STRING(i64)
        DYNSER_USE_STD_TO_STRING(u32)
        DYNSER_USE_STD_TO_STRING(u64)
        DYNSER_USE_STD_TO_STRING(float)
        if (value.is_string()) {
            return value.as_const_string();
        }
        DYNSER_USE_STD_TO_STRING(bool)
        DYNSER_USE_STD_TO_STRING(char)
        if (value.is_list()) {
            auto const& list = value.as_const_list();
            if (list.empty()) {
                return "[]";
            }
            std::string result{ "[" };
            for (auto const& inner : value.as_const_list()) {
                result += property_value_to_string(inner) + ", ";
            }
            result.pop_back();
            result.pop_back();
            result += "]";

            return result;
        }
        if (value.is_map()) {
            return properties_to_string(value.as_const_map());
        }
        std::unreachable();

#undef DYNSER_USE_STD_TO_STRING
    }

    [[nodiscard]] static std::string deserialize_err_to_string(dynser::DeserializeError const& wrapper) noexcept
    {
        using namespace dynser::deserialize_err;

        std::string ref_str;

        for (const auto& el : wrapper.ref_seq) {
            ref_str += std::format(" from '{}' tag at '{}' rule", el.tag, el.rule_ind);
        }

        auto error_str = dynser::util::visit_one_terminated(
            wrapper.error,
            [](const Unknown&) -> std::string { return "unknown error"; },
            [](const ConfigNotLoaded&) -> std::string { return "config not loaded"; },
            [](const ConfigTagNotFound& error) -> std::string {
                return std::format("config tag '{}' not found", error.tag);
            },
            [](const ScriptError& error) -> std::string { return std::format("script error: '{}'", error.message); },
            [](const PatternNotFound& error) -> std::string {
                return std::format("pattern '{}' not found", error.pattern);
            },
            [](const FieldNotFound& error) -> std::string {
                return std::format("field '{}' not found", error.field_name);
            },
            [](const NoBranchMatched&) -> std::string { return "no branch matched"; },
            [](const DynGroupValueNotFound& error) -> std::string {
                return std::format("value for dyn group '{}' not found", error.key);
            }
        );

        return error_str + ref_str + " in substr: '" + wrapper.scope_string + "'";
    }
};

}    // namespace dynser_test

namespace Catch
{
template <typename T>
struct StringMaker<dynser::SerializeResult<T>>
{
    static std::string convert(dynser::SerializeResult<T> const& value)
    {
        if (value) {
            if constexpr (std::is_same_v<T, std::string>) {
                return *value;
            }
            else {
                return std::format("<{}>", typeid(T).name());
            }
        }
        else {
            return dynser_test::Printer::serialize_err_to_string(value.error());
        }
    }
};

template <typename T>
struct StringMaker<dynser::DeserializeResult<T>>
{
    static std::string convert(dynser::DeserializeResult<T> const& value)
    {
        if (value) {
            if constexpr (std::is_same_v<T, dynser::Properties>) {
                return dynser_test::Printer::properties_to_string(*value);
            }
            else {
                return std::format("<{}>", typeid(T).name());
            }
        }
        else {
            return dynser_test::Printer::deserialize_err_to_string(value.error());
        }
    }
};

template <>
struct StringMaker<dynser::config::ParseResult>
{
    static std::string convert(dynser::config::ParseResult const& value)
    {
        if (value) {
            return "<config content>";
        }
        else {
            return dynser_test::Printer::config_parse_err_to_string(value.error());
        }
    }
};

template <>
struct StringMaker<dynser::regex::ParseResult>
{
    static std::string convert(dynser::regex::ParseResult const& value)
    {
        if (value) {
            return "<regex>";
        }
        else {
            return "Regex parse error at: " + dynser_test::Printer::regex_parse_err_to_string(value.error());
        }
    }
};
}    // namespace Catch
