#include "common.hpp"

auto quantifier_to_string(dynser::regex::Quantifier const& value) -> std::string;
auto regex_to_string(dynser::regex::Empty const& value) -> std::string;
auto regex_to_string(dynser::regex::WildCard const& value) -> std::string;
auto regex_to_string(dynser::regex::Group const& value) -> std::string;
auto regex_to_string(dynser::regex::NonCapturingGroup const& value) -> std::string;
auto regex_to_string(dynser::regex::Backreference const& value) -> std::string;
auto regex_to_string(dynser::regex::Lookup const& value) -> std::string;
auto regex_to_string(dynser::regex::CharacterClass const& value) -> std::string;
auto regex_to_string(dynser::regex::Disjunction const& value) -> std::string;
auto regex_to_string(dynser::regex::Token const& value) -> std::string;
auto regex_to_string(dynser::regex::Regex const& value) -> std::string;

auto props_to_quantifier(dynser::Properties const& value) -> dynser::regex::Quantifier;
auto props_to_regex(dynser::Properties const& value) -> dynser::regex::Regex;

TEST_CASE("Regex")
{
    using namespace dynser_test;

    dynser::DynSer ser{
        dynser::generate_property_to_target_mapper(
            [](dynser::Context&, dynser::Properties const& props, dynser::regex::Regex& target) noexcept {
                target = props_to_regex(props);
            }
        ),
        dynser::generate_target_to_property_mapper(),
    };

    DYNSER_LOAD_CONFIG_FILE(ser, "regex.yaml");

    char const* const regexes[]{
        R"()",
        // empty | empty group
        R"(|())",
        R"([abc]([^D])(?:u))",
        // empty | empty | empty | empty
        R"(|||)",
        R"((?=a|b\[)(?<=a\))(?!test)(?<!wha-\!))",
        R"(|[ab]|\d+|[a-zA-Z]|[^a-z]|.|.+)",
        R"((test)\1(\1)\2)",
        // quantifiers
        R"(a*b+c+?d*?ef{10,}g{10,20}h{10,}?j{10,20}?)",
        // it works because CharacterClass::characters contains raw "a" and "\a"
        R"(abc\a\b\c\..+\z*?\[\]\(\)\?\+\!\=\"\'\@\#\$\%\\[a-z][a-zA-Z])",
    };

    SECTION("check test methods")
    {
        for (std::size_t ind{}; auto const& regex : regexes) {
            DYNAMIC_SECTION("regex #" << ind)
            {
                const auto parsed = dynser::regex::from_string(regex);
                REQUIRE(parsed);
                const auto string = regex_to_string(*parsed);
                CHECK(string == regex);
            }
            ++ind;
        }
    }

    SECTION("check deserialization with config")
    {
        for (std::size_t ind{}; auto const& regex : regexes) {
            DYNAMIC_SECTION("regex #" << ind)
            {
                auto const props_sus = ser.deserialize_to_props(regex, "regex");
                REQUIRE(props_sus);
                INFO("raw props: " << Printer::properties_to_string(*props_sus));
                auto const parsed = ser.deserialize<dynser::regex::Regex>(regex, "regex");
                REQUIRE(parsed);
                auto const string = regex_to_string(*parsed);
                CHECK(string == regex);
            }
            ++ind;
        }
    }
}

auto quantifier_to_string(dynser::regex::Quantifier const& value) -> std::string
{
    const auto lazy_str = (value.is_lazy ? "?" : "");
    if (!value.to) {
        switch (value.from) {
            case 0:
                return std::string{ "*" } + lazy_str;
            case 1:
                return std::string{ "+" } + lazy_str;
            default:
                return "{" + std::to_string(value.from) + ",}" + lazy_str;
        }
    }
    else {
        if (value.from == *value.to && value.from == 1) {
            return "";
        }
        else if (value.from == *value.to) {
            return "{" + std::to_string(value.from) + "}" + lazy_str;
        }
        else {
            return "{" + std::to_string(value.from) + "," + std::to_string(*value.to) + "}" + lazy_str;
        }
    }
}

auto regex_to_string(dynser::regex::Empty const& value) -> std::string { return ""; }

auto regex_to_string(dynser::regex::WildCard const& value) -> std::string
{
    return "." + quantifier_to_string(value.quantifier);
}

auto regex_to_string(dynser::regex::Group const& value) -> std::string
{
    return "(" + regex_to_string(*value.value.get()) + ")" + quantifier_to_string(value.quantifier);
}

auto regex_to_string(dynser::regex::NonCapturingGroup const& value) -> std::string
{
    return "(?:" + regex_to_string(*value.value.get()) + ")" + quantifier_to_string(value.quantifier);
}

auto regex_to_string(dynser::regex::Backreference const& value) -> std::string
{
    return "\\" + std::to_string(value.group_number) + quantifier_to_string(value.quantifier);
}

auto regex_to_string(dynser::regex::Lookup const& value) -> std::string
{
    return std::string{ "(?" } + (value.is_forward ? "" : "<") + (value.is_negative ? "!" : "=") +
           regex_to_string(*value.value.get()) + ")";
}

auto regex_to_string(dynser::regex::CharacterClass const& value) -> std::string
{
    if (value.characters.empty()) {
        return "";
    }
    if (value.is_negative) {
        return "[^" + value.characters + "]" + quantifier_to_string(value.quantifier);
    }
    if (value.characters.size() == 1) {
        return value.characters + quantifier_to_string(value.quantifier);
    }
    if (value.characters.size() == 2 && value.characters[0] == '\\') {
        return value.characters + quantifier_to_string(value.quantifier);
    }
    return "[" + value.characters + "]" + quantifier_to_string(value.quantifier);
}

auto regex_to_string(dynser::regex::Disjunction const& value) -> std::string
{
    return regex_to_string(*value.left.get()) + "|" + regex_to_string(*value.right.get());
}

auto regex_to_string(dynser::regex::Token const& value) -> std::string
{
    return std::visit([](auto const& tok) { return regex_to_string(tok); }, value);
}

auto regex_to_string(dynser::regex::Regex const& value) -> std::string
{
    std::string result;

    for (auto const& token : value.value) {
        result += regex_to_string(token);
    }
    return result;
}

auto props_to_quantifier(dynser::Properties const& value) -> dynser::regex::Quantifier
{
    return {
        value.at("from").as_const_u64(),
        value.contains("to") ? std::optional{ value.at("to").as_const_u64() } : std::nullopt,
        value.at("is_lazy").as_const_bool(),
    };
}

auto props_to_regex(dynser::Properties const& value) -> dynser::regex::Regex
{
    // FIXME
    return {};
}
