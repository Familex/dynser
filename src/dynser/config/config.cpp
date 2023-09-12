#include "config.h"

#include "defaults.h"
#include "keywords.h"
#include "yaml-cpp/yaml.h"

#include <cassert>
#include <charconv>
#include <expected>
#include <locale>
#include <optional>
#include <regex>

using namespace dynser;

// https://stackoverflow.com/a/37516316
namespace
{
template <class BidirIt, class Traits, class CharT, class UnaryFunction>
std::basic_string<CharT>
regex_replace(BidirIt first, BidirIt last, const std::basic_regex<CharT, Traits>& re, UnaryFunction f)
{
    std::basic_string<CharT> s;

    typename std::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
    auto endOfLastMatch = first;

    auto callback = [&](const std::match_results<BidirIt>& match) {
        auto positionOfThisMatch = match.position(0);
        auto diff = positionOfThisMatch - positionOfLastMatch;

        auto startOfThisMatch = endOfLastMatch;
        std::advance(startOfThisMatch, diff);

        s.append(endOfLastMatch, startOfThisMatch);
        s.append(f(match));

        auto lengthOfMatch = match.length(0);

        positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

        endOfLastMatch = startOfThisMatch;
        std::advance(endOfLastMatch, lengthOfMatch);
    };

    std::regex_iterator<BidirIt> begin(first, last, re), end;
    std::for_each(begin, end, callback);

    s.append(endOfLastMatch, last);

    return s;
}

template <class Traits, class CharT, class UnaryFunction>
std::string regex_replace(const std::string& s, const std::basic_regex<CharT, Traits>& re, UnaryFunction f)
{
    return regex_replace(s.cbegin(), s.cend(), re, f);
}
}    // namespace

config::yaml::Regex
config::details::resolve_dyn_regex(const yaml::DynRegex& dyn_reg, const yaml::DynGroupValues& dyn_gr_vals) noexcept
{
    static const std::regex dyn_gr_pattern{ R"(\\_(\d+))" };
    return { regex_replace(dyn_reg, dyn_gr_pattern, [&dyn_gr_vals](const std::smatch& m) {
        const auto gr_num = std::atoi(m.str(1).c_str());
        if (dyn_gr_vals.contains(gr_num)) {
            return dyn_gr_vals.at(gr_num);
        }
        return std::string{};
    }) };
}

regex::ToStringResult config::details::resolve_regex(const yaml::Regex& reg, const yaml::GroupValues& vals) noexcept
{
    using namespace dynser::regex;

    auto reg_sus = regex::from_string(reg);
    if (!reg_sus) {
        return std::unexpected{ ToStringError{
            to_string_err::RegexSyntaxError{ reg_sus.error() },
            0    // group number
        } };
    }
    const auto regex = !vals.contains(0)
                           ? *reg_sus
                           : Regex{ std::vector<Token>{ Group{ std::make_unique<Regex>(std::move(*reg_sus)),
                                                               Quantifier{ 1, 1, false },
                                                               std::regex{ reg.data(), reg.size() },
                                                               0 } } };
    return to_string(regex, vals);
}

// config::from_string helpers
namespace
{

template <typename Res>
inline std::optional<Res> as_opt(YAML::Node const& node) noexcept(false)    // throws YAML::BadConversion or anything
{
    if (node.IsDefined()) {
        return node.as<Res>();
    }
    return std::nullopt;
};

}    // namespace

config::ParseResult config::from_string(const std::string_view sv) noexcept
{
    try {
        using namespace dynser::config;
        using namespace dynser::config::yaml;

        const auto yaml = YAML::Load(std::string{ sv });
        Config result;

        result.version = yaml[keywords::VERSION].as<std::string>();

        for (const auto tag : yaml[keywords::TAGS]) {
            const auto tag_name = tag[keywords::NAME].as<std::string>();

            result.tags[tag_name] = {
                .name = tag_name,
                .nested = [&]() -> yaml::Nested {    // iife
                    if (const auto continual = tag[keywords::NESTED_CONTINUAL]) {
                        yaml::Continual nested;

                        for (const auto continual_rule_type : continual) {
                            if (const auto rule = continual_rule_type[keywords::CONTINUAL_EXISTING]) {
                                nested.push_back(ConExisting{
                                    .tag = rule[keywords::CONTINUAL_EXISTING_TAG].as<std::string>(),
                                    .prefix = as_opt<std::string>(rule[keywords::CONTINUAL_EXISTING_PREFIX]),
                                    .required =
                                        as_opt<bool>(rule[keywords::CONTINUAL_EXISTING_REQUIRED]).value_or(true) });
                            }
                            else if (const auto rule = continual_rule_type[keywords::CONTINUAL_LINEAR]) {
                                nested.push_back(ConLinear{
                                    .pattern = rule[keywords::CONTINUAL_LINEAR_PATTERN].as<std::string>(),
                                    .dyn_groups = as_opt<DynGroupValues>(rule[keywords::CONTINUAL_LINEAR_DYN_GROUPS]),
                                    .fields = as_opt<GroupValues>(rule[keywords::CONTINUAL_LINEAR_FIELDS]) });
                            }
                        }

                        return nested;
                    }
                    else if (const auto branched = tag[keywords::NESTED_BRANCHED]) {
                        Branched nested;

                        nested.branching_script = branched[keywords::BRANCHED_BRANCHING_SCRIPT].as<std::string>();
                        nested.debranching_script = branched[keywords::BRANCHED_DEBRANCHING_SCRIPT].as<std::string>();

                        for (const auto branched_rule_type : branched[keywords::BRANCHED_RULES]) {
                            if (const auto rule = branched_rule_type[keywords::BRANCHED_EXISTING]) {
                                nested.rules.push_back(BraExisting{
                                    .tag = rule[keywords::BRANCHED_EXISTING_TAG].as<std::string>(),
                                    .prefix = as_opt<std::string>(rule[keywords::BRANCHED_EXISTING_PREFIX]),
                                    .required =
                                        as_opt<bool>(rule[keywords::BRANCHED_EXISTING_REQUIRED]).value_or(true) });
                            }
                            else if (const auto rule = branched_rule_type[keywords::BRANCHED_LINEAR]) {
                                nested.rules.push_back(BraLinear{
                                    .pattern = rule[keywords::BRANCHED_LINEAR_PATTERN].as<std::string>(),
                                    .dyn_groups = as_opt<DynGroupValues>(rule[keywords::BRANCHED_LINEAR_DYN_GROUPS]),
                                    .fields = as_opt<GroupValues>(rule[keywords::BRANCHED_LINEAR_FIELDS]) });
                            }
                        }

                        return nested;
                    }
                    else if (const auto recurrent = tag[keywords::NESTED_RECURRENT]) {
                        Recurrent nested{
                            .mapping_type = DEFLT_REC_MAPPING_TYPE,
                        };

                        if (const auto mapping_settings = recurrent[keywords::RECURRENT_MAPPING_SETTINGS]) {
                            nested.mapping_type =
                                as_opt<std::string>(mapping_settings[keywords::RECURRENT_MAPPING_TYPE])
                                    .transform([](std::string const& type) {
                                        if (type == keywords::RECURRENT_MAPPING_TYPE_DICT_OF_ARRAYS) {
                                            return RecurrentMappingType::DictOfArrays;
                                        }
                                        else if (type == keywords::RECURRENT_MAPPING_TYPE_ARRAY_OF_DICTS) {
                                            return RecurrentMappingType::ArrayOfDicts;
                                        }
                                        else {
                                            // FIXME throw exception or something
                                            assert(false);
                                            return DEFLT_REC_MAPPING_TYPE;
                                        }
                                    })
                                    .value_or(DEFLT_REC_MAPPING_TYPE);
                            nested.root_key =
                                as_opt<std::string>(mapping_settings[keywords::RECURRENT_MAPPING_ROOT_KEY]);
                        }

                        for (const auto recurrent_rule_type : recurrent[keywords::RECURRENT_RULES]) {
                            if (const auto rule = recurrent_rule_type[keywords::RECURRENT_EXISTING]) {
                                nested.rules.push_back(RecExisting{
                                    .tag = rule[keywords::RECURRENT_EXISTING_TAG].as<std::string>(),
                                    .prefix = as_opt<std::string>(rule[keywords::RECURRENT_EXISTING_PREFIX]),
                                    .required =
                                        as_opt<bool>(rule[keywords::RECURRENT_EXISTING_REQUIRED]).value_or(true),
                                    .wrap = as_opt<bool>(rule[keywords::RECURRENT_EXISTING_WRAP]).value_or(false),
                                    .default_value =
                                        as_opt<std::string>(rule[keywords::RECURRENT_EXISTING_DEFAULT_VALUE]),
                                    .priority =
                                        as_opt<PriorityType>(rule[keywords::RECURRENT_EXISTING_PRIORITY]).value_or(0),
                                });
                            }
                            else if (const auto rule = recurrent_rule_type[keywords::RECURRENT_LINEAR]) {
                                nested.rules.push_back(RecLinear{
                                    .pattern = rule[keywords::RECURRENT_LINEAR_PATTERN].as<std::string>(),
                                    .dyn_groups = as_opt<DynGroupValues>(rule[keywords::RECURRENT_LINEAR_DYN_GROUPS]),
                                    .fields = as_opt<GroupValues>(rule[keywords::RECURRENT_LINEAR_FIELDS]),
                                    .wrap = as_opt<bool>(rule[keywords::RECURRENT_LINEAR_WRAP]).value_or(false),
                                    .default_value =
                                        as_opt<std::string>(rule[keywords::RECURRENT_LINEAR_DEFAULT_VALUE]),
                                    .priority =
                                        as_opt<PriorityType>(rule[keywords::RECURRENT_LINEAR_PRIORITY]).value_or(0),
                                });
                            }
                            else if (const auto rule = recurrent_rule_type[keywords::RECURRENT_INFIX]) {
                                nested.rules.push_back(RecInfix{
                                    .pattern = rule[keywords::RECURRENT_INFIX_PATTERN].as<std::string>(),
                                    .dyn_groups = as_opt<DynGroupValues>(rule[keywords::RECURRENT_INFIX_DYN_GROUPS]),
                                    .fields = as_opt<GroupValues>(rule[keywords::RECURRENT_INFIX_FIELDS]),
                                    .wrap = as_opt<bool>(rule[keywords::RECURRENT_INFIX_WRAP]).value_or(false),
                                    .default_value = as_opt<std::string>(rule[keywords::RECURRENT_INFIX_DEFAULT_VALUE]),
                                });
                            }
                        }

                        return nested;
                    }

                    std::unreachable();
                }(),
                .serialization_script = as_opt<Script>(tag[keywords::SERIALIZATION_SCRIPT]),
                .deserialization_script = as_opt<Script>(tag[keywords::DESERIALIZATION_SCRIPT]),
            };
        }

        // FIXME logic validation
        // - claim prefix if several existing of one tag
        // - reccurent with array-of-dicts structure must have root-key
        // etc

        return result;
    }
    catch (YAML::ParserException& ex) {
        return std::unexpected{ ParseError{ ParseError::Type::ParserException, ex.mark, ex.msg } };
    }
    catch (YAML::RepresentationException& ex) {
        return std::unexpected{ ParseError{ ParseError::Type::RepresentationException, ex.mark, ex.msg } };
    }
    catch (YAML::Exception& ex) {
        return std::unexpected{ ParseError{ ParseError::Type::UnknownYamlCppException, ex.mark, ex.msg } };
    }
    catch (...) {
        return std::unexpected{ ParseError{ ParseError::Type::UnknownException } };
    }
}
