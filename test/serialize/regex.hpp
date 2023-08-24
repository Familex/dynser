#include "common.hpp"

auto quantifier_to_props(dynser::regex::Quantifier const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Empty const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::WildCard const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Group const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::NonCapturingGroup const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Backreference const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Lookup const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::CharacterClass const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Disjunction const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Regex const& value) -> dynser::Properties;

TEST_CASE("Regex")
{
    using namespace dynser_test;
    using namespace dynser::regex;

    dynser::DynSer ser{
        dynser::generate_property_to_target_mapper(),
        dynser::generate_target_to_property_mapper([&](dynser::Context&, Regex const& target) -> dynser::Properties {
            return regex_to_props_helper(target);
        })
    };

    const auto config =
#include "../configs/regex.yaml.raw"
        ;

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

    char const* const regexes[]{
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
        R"()",
        // it works because CharacterClass::characters contains raw "a" and "\a"
        R"(abc\a\b\c\..+\z*?\[\]\(\)\?\+\!\=\"\'\@\#\$\%\\[a-z][a-zA-Z])",
    };

    for (std::size_t ind{}; auto const regex_str : regexes) {
        DYNAMIC_SECTION("Regex #" << ind << " (" << regex_str << ")")
        {
            auto const regex = from_string(regex_str);
            REQUIRE(regex);
            dynser::Context dummy_context{};
            INFO("Regex props: " << Printer::properties_to_string(ser.ttpm(dummy_context, *regex)));
            DYNSER_TEST_SERIALIZE(*regex, "regex", regex_str);
        }
        ++ind;
    }
}

auto quantifier_to_props(dynser::regex::Quantifier const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;

    if (value.to) {
        return map_to_props(
            "from",
            static_cast<std::uint64_t>(value.from),
            "to",
            static_cast<std::uint64_t>(*value.to),
            "is-lazy",
            value.is_lazy
        );
    }
    else {
        return map_to_props("from", static_cast<std::uint64_t>(value.from), "is-lazy", value.is_lazy);
    }
}

auto regex_to_props_helper(dynser::regex::Empty const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;

    return map_to_props("type", "empty");
}

auto regex_to_props_helper(dynser::regex::WildCard const& value) -> dynser::Properties
{
    return regex_to_props_helper(dynser::regex::CharacterClass{
        .characters = ".", .is_negative = false, .quantifier = value.quantifier });
}

auto regex_to_props_helper(dynser::regex::Group const& value) -> dynser::Properties
{
    using dynser::util::add_prefix;
    using dynser::util::map_to_props;

    return map_to_props("type", "group") << add_prefix(regex_to_props_helper(*value.value.get()), "inner")
                                         << quantifier_to_props(value.quantifier);
}

auto regex_to_props_helper(dynser::regex::NonCapturingGroup const& value) -> dynser::Properties
{
    using dynser::util::add_prefix;
    using dynser::util::map_to_props;

    return map_to_props("type", "non-capturing-group")
           << add_prefix(regex_to_props_helper(*value.value.get()), "inner") << quantifier_to_props(value.quantifier);
}

auto regex_to_props_helper(dynser::regex::Backreference const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;

    return map_to_props("type", "backreference", "group-number", static_cast<std::uint64_t>(value.group_number))
           << quantifier_to_props(value.quantifier);
}

auto regex_to_props_helper(dynser::regex::Lookup const& value) -> dynser::Properties
{
    using dynser::util::add_prefix;
    using dynser::util::map_to_props;

    return map_to_props("type", "lookup", "is-negative", value.is_negative, "is-forward", value.is_forward)
           << add_prefix(regex_to_props_helper(*value.value.get()), "inner");
}

auto regex_to_props_helper(dynser::regex::CharacterClass const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;

    return map_to_props("type", "character-class", "characters", value.characters, "is-negative", value.is_negative)
           << quantifier_to_props(value.quantifier);
}

auto regex_to_props_helper(dynser::regex::Disjunction const& value) -> dynser::Properties
{
    using dynser::util::add_prefix;
    using dynser::util::map_to_props;
    using dynser::util::visit_one;

    return map_to_props("type", "disjunction")
           << add_prefix(
                  visit_one(*value.left.get(), [](auto&& tok) { return regex_to_props_helper(std::move(tok)); }), "left"
              )
           << add_prefix(
                  visit_one(*value.right.get(), [](auto&& tok) { return regex_to_props_helper(std::move(tok)); }),
                  "right"
              );
}

auto regex_to_props_helper(dynser::regex::Regex const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;
    using dynser::util::visit_one;

    using List = dynser::PropertyValue::ListType<dynser::PropertyValue>;

    List result;

    for (auto const& tok : value.value) {
        result.emplace_back(visit_one(tok, [](auto&& tok) { return regex_to_props_helper(std::move(tok)); }));
    }

    return map_to_props("value", result);
}
