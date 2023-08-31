#pragma once

#include "config/config.h"
#include "config/keywords.h"
#include "luwra.hpp"
#include "structs/context.hpp"
#include "structs/fields.hpp"
#include "util/prefix.hpp"
#include "util/visit.hpp"
#include <unordered_set>

#include <fstream>
#include <ranges>
#include <sstream>
#include <string>

namespace dynser
{

namespace details

{
template <typename... Fs>
struct OverloadedObject : Fs...
{
    using Fs::operator()...;
};
template <typename... Fs>
OverloadedObject(Fs...) -> OverloadedObject<Fs...>;

template <typename Result, typename... Args>
constexpr auto wrap_function_pointer_to_lambda(Result (*f)(Args...)) noexcept
{
    return [=](Args... args) -> Result {
        return f(std::forward<Args>(args)...);
    };
}

template <typename F>
constexpr auto generate_overloaded_object_helper(F&& f) noexcept
{
    if constexpr (std::is_function_v<std::remove_pointer_t<std::decay_t<F>>>) {
        return wrap_function_pointer_to_lambda(std::forward<F>(f));
    }
    else {
        return std::forward<F>(f);
    }
}

template <typename... Fs>
constexpr auto generate_overloaded_object(Fs&&... fs) noexcept
{
    return OverloadedObject{ generate_overloaded_object_helper(std::forward<Fs>(fs))... };
}

}    // namespace details

/**
 * \brief must receive `void(*)(Context&, Properties const&, Target&)` functors.
 * \note generate_* function can be used to constuct this from function pointers (and function objects too).
 */
template <typename... Fs>
using PropertyToTargetMapper = details::OverloadedObject<Fs...>;

template <typename... Fs>
constexpr auto generate_property_to_target_mapper(Fs&&... fs) noexcept
{
    return details::generate_overloaded_object(std::forward<Fs>(fs)...);
}

/**
 * \brief must receive `Properties(*)(Context&, Target const&)` functors.
 * \note generate_* function can be used to constuct this from function pointers (and function objects too).
 */
template <typename... Fs>
using TargetToPropertyMapper = details::OverloadedObject<Fs...>;

template <typename... Fs>
constexpr auto generate_target_to_property_mapper(Fs&&... fs) noexcept
{
    return details::generate_overloaded_object(std::forward<Fs>(fs)...);
}

namespace serialize_err
{

struct Unknown
{ };

struct ConfigNotLoaded
{ };

struct ConfigTagNotFound
{
    std::string tag;
};

struct BranchNotSet
{ };

struct BranchOutOfBounds
{
    std::uint32_t selected_branch;
    std::size_t max_branch;
};

struct ScriptError
{
    std::string message;
};

struct ScriptVariableNotFound
{
    std::string variable_name;
};

struct ResolveRegexError
{
    regex::ToStringError error;
};

struct RecurrentDictKeyNotFound
{
    std::string key;
};

using Error = std::variant<
    Unknown,
    ScriptError,
    ScriptVariableNotFound,
    ResolveRegexError,
    BranchNotSet,
    BranchOutOfBounds,
    ConfigNotLoaded,
    ConfigTagNotFound,
    RecurrentDictKeyNotFound>;

}    // namespace serialize_err

namespace deserialize_err
{
struct Unknown
{ };
struct ConfigNotLoaded
{ };
struct ConfigTagNotFound
{
    std::string tag;
};
struct ScriptError
{
    std::string message;
};
struct PatternNotFound
{
    std::string pattern;
};
struct FieldNotFound
{
    std::string field_name;
};
struct NoBranchMatched
{ };
struct DynGroupValueNotFound
{
    std::string key;
};

using Error = std::variant<
    Unknown,
    ConfigNotLoaded,
    ConfigTagNotFound,
    ScriptError,
    PatternNotFound,
    FieldNotFound,
    NoBranchMatched,
    DynGroupValueNotFound>;

}    // namespace deserialize_err

// FIXME rename
/**
 * \brief Error place context on serialize error to handle nested rules.
 */
struct ErrorRef
{
    std::string tag;
    std::size_t rule_ind{};
};

struct SerializeError
{
    serialize_err::Error error;
    Properties scope_props;
    std::vector<ErrorRef> ref_seq{};
};

struct DeserializeError
{
    deserialize_err::Error error;
    std::string scope_string;
    std::vector<ErrorRef> ref_seq{};
};

using SerializeResult = std::expected<std::string, SerializeError>;

template <typename Result>
using DeserializeResult = std::expected<Result, DeserializeError>;

/**
 * \brief helper.
 */
inline SerializeResult make_serialize_err(serialize_err::Error&& err, Properties scope_props) noexcept
{
    return std::unexpected{ SerializeError{ std::move(err), std::move(scope_props), {} } };
}

/**
 * \brief helper.
 */
template <typename Result>
inline DeserializeResult<Result>
make_deserialize_err(deserialize_err::Error&& err, std::string_view scope_string) noexcept
{
    return std::unexpected{ DeserializeError{ std::move(err), std::string{ scope_string }, {} } };
}

/**
 * \brief helper.
 */
void append_ref_to_err(SerializeError& err, ErrorRef&& error_ref) noexcept
{
    err.ref_seq.push_back(std::move(error_ref));
}

namespace details
{

/**
 * \brief Merge <'a, 'b> map and <'b, 'c> map into <'a, 'c> map.
 * Uses ::key_type and ::mapped_type to get keys and values types.
 */
template <
    typename Lhs,
    typename Rhs,
    typename LhsKey = typename Lhs::key_type,
    typename LhsValue = typename Lhs::mapped_type,
    typename RhsKey = typename Rhs::key_type,
    typename RhsValue = typename Rhs::mapped_type,
    typename Result = std::unordered_map<LhsKey, RhsValue>>
    requires std::same_as<LhsValue, RhsKey>
std::expected<Result, LhsValue> merge_maps(const Lhs& lhs, const Rhs& rhs) noexcept
{
    Result result{};
    for (auto& [key, inter] : lhs) {
        if (!rhs.contains(inter)) {
            return std::unexpected{ inter };
        }
        result[key] = rhs.at(inter);
    }
    return result;
}

/**
 * \brief Filters string properties.
 */
Fields props_to_fields(const Properties& props) noexcept
{
    Fields result;
    for (const auto& [key, val] : props) {
        if (val.is_string()) {
            result[key] = val.as_const_string();
        }
        else if (val.is_i32() && val.as_const_i32() >= 0) {
            result[key] = std::to_string(val.as_const_i32());
        }
        else if (val.is_i64() && val.as_const_i64() >= 0) {
            result[key] = std::to_string(val.as_const_i64());
        }
        else if (val.is_u32()) {
            result[key] = std::to_string(val.as_const_u32());
        }
        else if (val.is_u64()) {
            result[key] = std::to_string(val.as_const_u64());
        }
        else if (val.is_char()) {
            result[key] = std::string(1, val.as_const_char());
        }
    }
    return result;
}

using PrioritizedListLen = std::pair<config::yaml::PriorityType, std::size_t>;

// forward declaration
std::optional<PrioritizedListLen>
calc_max_property_lists_len(config::Config const&, Properties const&, config::yaml::Nested const&) noexcept;

// for existing and linear rules
std::optional<PrioritizedListLen> calc_max_property_lists_len_helper(
    config::Config const& config,
    Properties const& props,
    config::yaml::LikeExisting auto const& rule
) noexcept
{
    using namespace config::yaml;

    if (!config.tags.contains(rule.tag)) {
        return std::nullopt;
    }

    auto result = calc_max_property_lists_len(config, props, config.tags.at(rule.tag).nested);

    if constexpr (HasPriority<decltype(rule)>) {
        if (result) {
            result->first = rule.priority;
        }
    }
    return result;
}

// for existing and linear rules (
std::optional<PrioritizedListLen> calc_max_property_lists_len_helper(
    config::Config const& config,
    Properties const& props,
    config::yaml::LikeLinear auto const& rule
) noexcept
{
    using namespace config::yaml;

    std::optional<std::size_t> result{ std::nullopt };

    if (!rule.fields) {
        return result ? std::optional<PrioritizedListLen>{ { 0, *result } } : std::optional<PrioritizedListLen>{};
    }

    for (auto const& [group_num, field_name] : *rule.fields) {
        if (!props.contains(field_name)) {
            continue;
        }

        const auto& list_prop_sus = props.at(field_name);

        if (!list_prop_sus.is_list()) {
            continue;
        }

        const auto list_len = list_prop_sus.as_const_list().size();

        if (!result || *result < list_len) {
            result = list_len;
        }
    }

    std::optional<PrioritizedListLen> result_prioritized                  //
        { result ? std::optional<PrioritizedListLen>{ { 0, *result } }    //
                 : std::optional<PrioritizedListLen>{} };

    if constexpr (HasPriority<decltype(rule)>) {
        if (result_prioritized) {
            result_prioritized->first = rule.priority;
        }
    }

    return result_prioritized;
}

std::optional<PrioritizedListLen> calc_max_property_lists_len(
    config::Config const& config,
    Properties const& props,
    config::yaml::Nested const& rules
) noexcept
{
    using namespace config::yaml;

    std::optional<PrioritizedListLen> result{ std::nullopt };

    const auto visit_vector_of_existing_or_linear_rules =    //
        [&config, &props, &result](auto const& vector_of_existing_or_linear_rules) {
            for (const auto& rule_v : vector_of_existing_or_linear_rules) {
                const auto rule_result = std::visit(
                    [&config, &props](auto const& rule) {    //
                        return calc_max_property_lists_len_helper(config, props, rule);
                    },
                    rule_v
                );

                if (rule_result) {
                    if (!result ||                               //
                        rule_result->first > result->first ||    //
                        (rule_result->first == result->first && rule_result->second > result->second))
                    {
                        result = *rule_result;
                    }
                }
            }
        };

    // FIXME infix rule must be -1?
    util::visit_one(
        rules,
        [&result, &props](RecurrentDict const& recurrent_dict) {
            if (props.contains(recurrent_dict.key)) {
                result = { 0, props.at(recurrent_dict.key).as_const_list().size() };
            }
        },
        [&](Branched const& branched) {    //
            visit_vector_of_existing_or_linear_rules(branched.rules);
        },
        [&](auto const& vector_of_existing_or_linear_rules) {
            visit_vector_of_existing_or_linear_rules(vector_of_existing_or_linear_rules);
        }
    );

    return result;
}

// forward declaration
DeserializeResult<std::pair<Properties, std::string_view>> deserialize_to_props(
    Context& context,
    config::Config const& config,
    std::string_view const sv,
    std::string_view const tag
) noexcept;

struct DeserializeRuleToPropsResult
{
    Fields fields{};
    Properties properties{};
    std::string_view sv{};
};

// FIXME out-params not required
template <typename Rule>
DeserializeResult<DeserializeRuleToPropsResult> deserialize_rule_to_props(
    Context& context,
    config::Config const& config,
    Rule const& rule,
    std::string_view const sv
) noexcept
{
    using ResultSuccess = DeserializeRuleToPropsResult;
    using Result = DeserializeResult<ResultSuccess>;

    return util::visit_one_terminated(
        rule,
        [&](config::yaml::LikeExisting auto const& rule) -> Result {
            auto const deserialize_to_props_result = deserialize_to_props(context, config, sv, rule.tag);
            if (!deserialize_to_props_result) {
                if (!rule.required &&
                    std::holds_alternative<deserialize_err::PatternNotFound>(deserialize_to_props_result.error().error))
                {
                    // skip rule
                    return ResultSuccess{ .fields = {}, .properties = {}, .sv = sv };
                }
                return std::unexpected{ deserialize_to_props_result.error() };
            }
            auto const& [props, sv_rem] = *deserialize_to_props_result;
            if (rule.prefix) {
                return ResultSuccess{
                    .fields = {},
                    .properties = util::add_prefix(props, *rule.prefix),
                    .sv = sv_rem,
                };
            }
            else {
                return ResultSuccess{
                    .fields = {},
                    .properties = props,
                    .sv = sv_rem,
                };
            }
        },
        [&](config::yaml::LikeLinear auto const& rule) -> Result {
            Fields fields{};
            config::yaml::Regex pattern_str{ rule.pattern };    // mutable, because conditional init below
            if (rule.dyn_groups) {
                auto const dyn_group_vals_sus =
                    dynser::details::merge_maps(*rule.dyn_groups, dynser::details::props_to_fields(context));
                if (!dyn_group_vals_sus) {
                    return make_deserialize_err<ResultSuccess>(
                        deserialize_err::DynGroupValueNotFound{ dyn_group_vals_sus.error() }, sv
                    );
                }
                pattern_str = config::details::resolve_dyn_regex(rule.pattern, *dyn_group_vals_sus);
            }    // default init instead of 'else'
            // FIXME use another regex engine
            std::regex pattern{ pattern_str };
            // check sv slices for pattern match
            for (int len{ static_cast<int>(sv.size()) }; len >= 0; --len) {
                std::smatch match;
                auto const sub_sv{ sv.substr(0, len) };
                std::string const sub_str{ sub_sv };
                if (std::regex_match(sub_str, match, pattern)) {
                    if (rule.fields) {
                        if (rule.fields->size() > match.size()) {
                            return make_deserialize_err<ResultSuccess>(
                                deserialize_err::FieldNotFound{ rule.fields->at(match.size()) }, sv
                            );
                        }
                        for (auto const& [group_ind, group_name] : *rule.fields) {
                            fields[group_name] = match[group_ind].str();
                        }
                    }
                    return ResultSuccess{
                        .fields = fields,
                        .properties = {},
                        .sv = sv.substr(len),
                    };
                }
            }
            return make_deserialize_err<ResultSuccess>(deserialize_err::PatternNotFound{ pattern_str }, sv);
        }
    );
}

DeserializeResult<std::pair<Properties, std::string_view>> deserialize_to_props(
    Context& context,
    config::Config const& config,
    std::string_view const sv,
    std::string_view const tag
) noexcept
{
    using ResultSuccess = std::pair<Properties, std::string_view>;
    using Result = DeserializeResult<ResultSuccess>;
    // process rules to match fields in sv
    // process scripts to convert fields to props
    const auto& curr_tag = config.tags.at(std::string{ tag });

    return util::visit_one_terminated(
        curr_tag.nested,
        [&](config::yaml::Continual const& nested) -> Result {
            std::string_view curr_sv{ sv };
            Properties properties{};
            Fields fields{};
            for (auto const& rule : nested) {
                auto process_result = deserialize_rule_to_props(context, config, rule, curr_sv);
                if (!process_result) {
                    // FIXME add err_ref?
                    return std::unexpected{ process_result.error() };
                }
                fields.merge(process_result->fields);
                properties.merge(process_result->properties);
                curr_sv = process_result->sv;
            }

            // FIXME separate into function
            if (auto const& script = curr_tag.deserialization_script) {
                luwra::StateWrapper state;
                state.loadStandardLibrary();
                register_userdata_property_value(state);
                state[config::keywords::CONTEXT] = context;
                state[config::keywords::INPUT_TABLE] = fields;
                state[config::keywords::OUTPUT_TABLE] = dynser::Properties{};
                auto const script_run_result = state.runString(script->c_str());
                if (script_run_result != LUA_OK) {
                    auto const error = state.read<std::string>(-1);
                    return make_deserialize_err<ResultSuccess>(deserialize_err::ScriptError{ error }, sv);
                }
                auto const table =
                    state[config::keywords::OUTPUT_TABLE].read<std::map<std::string, luwra::Reference>>();
                for (auto const& [key, val] : table) {
                    // FIXME check if type is correct (calls abort() if not)
                    properties[key] = val;
                }
            }

            return std::make_pair(properties, curr_sv);
        },
        [&](config::yaml::Branched const& nested) -> Result {
            // find longest (not first) matching rule
            Properties longest_result{};
            std::optional<std::string_view> sv_rem{};
            std::optional<std::size_t> result_rule_ind{};

            {
                std::optional<std::size_t> result_len{};
                for (std::size_t rule_ind{}; rule_ind < nested.rules.size(); ++rule_ind) {
                    auto process_result = deserialize_rule_to_props(context, config, nested.rules[rule_ind], sv);

                    if (process_result) {
                        auto const rule_len = sv.size() - process_result->sv.size();
                        // result_len == rule_len could be only with overlapping rules (FIXME example)
                        if (!result_len || result_len < rule_len) {
                            result_len = rule_len;
                            result_rule_ind = rule_ind;
                            longest_result = process_result->properties;
                            sv_rem = process_result->sv;
                            if (!process_result->fields.empty()) {
                                // can't be handled by design. FIXME emit warning
                            }
                        }
                    }
                    else {
                        // ignore 'else' branch
                    }
                }
            }

            if (!result_rule_ind) {
                return make_deserialize_err<ResultSuccess>(deserialize_err::NoBranchMatched{}, sv);
            }

            // process debranching script
            {
                luwra::StateWrapper debranching_script_state{};
                debranching_script_state.loadStandardLibrary();
                register_userdata_property_value(debranching_script_state);

                debranching_script_state[config::keywords::CONTEXT] = context;
                debranching_script_state[config::keywords::OUTPUT_TABLE] = Properties{};
                debranching_script_state[config::keywords::BRANCHED_RULE_IND_VARIABLE] = *result_rule_ind;
                auto const debranching_script_run_result =
                    debranching_script_state.runString(nested.debranching_script.c_str());
                if (debranching_script_run_result != LUA_OK) {
                    auto const error = debranching_script_state.read<std::string>(-1);
                    return make_deserialize_err<ResultSuccess>(deserialize_err::ScriptError{ error }, sv);
                }
                auto debranching_script_result_props =
                    debranching_script_state[config::keywords::OUTPUT_TABLE].read<Properties>();

                // merge into result
                longest_result.merge(debranching_script_result_props);
                // FIXME is it necessary? if so, add to other script calls
                context = debranching_script_state[config::keywords::CONTEXT].read<Context>();
            }

            return std::make_pair(longest_result, *sv_rem);
        },
        [&](config::yaml::Recurrent const& nested) -> Result {
            using VecFieldsValue = std::vector<Fields::mapped_type>;
            using VecFields = std::map<Fields::key_type, VecFieldsValue>;
            VecFields vec_fields{};
            Properties properties{};
            std::string_view curr_sv{ sv };
            std::optional<Result> infix_error_sus{};
            bool parsed{ false };

            for (bool is_last_sus{ false }; !curr_sv.empty() && !parsed;) {
                for (bool is_first_rule{ true }; auto const& rule : nested) {
                    // infix rule failed on prefious loop, but first rule parsed successfully
                    if (!is_first_rule && is_last_sus) {
                        assert(infix_error_sus);
                        return *infix_error_sus;
                    }
                    auto process_result = deserialize_rule_to_props(context, config, rule, curr_sv);
                    if (process_result) {
                        for (auto const& [key, val] : process_result->properties) {
                            if (properties.contains(key)) {
                                properties.at(key).as_list().push_back(val);
                            }
                            else {
                                properties[key] = PropertyValue{ PropertyValue::ListType<PropertyValue>{ val } };
                            }
                        }
                        for (auto const& [key, val] : process_result->fields) {
                            if (vec_fields.contains(key)) {
                                vec_fields.at(key).push_back(val);
                            }
                            else {
                                vec_fields[key] = VecFieldsValue{ val };
                            }
                        }
                        curr_sv = process_result->sv;
                    }
                    else {
                        // first rule failed as intended
                        // infix errors ignored as indended
                        if (is_first_rule) {
                            parsed = true;
                            break;
                        }
                        else if (std::holds_alternative<config::yaml::RecInfix>(rule)) {
                            is_last_sus = true;
                            // remember first infix parse error
                            if (!infix_error_sus) {
                                infix_error_sus = std::unexpected{ process_result.error() };
                            }
                        }
                        else {
                            // FIXME add err_ref?
                            return std::unexpected{ process_result.error() };
                        }
                    }
                    is_first_rule = false;
                }
            }

            if (auto const& script = curr_tag.deserialization_script) {
                if (!vec_fields.empty()) {
                    // FIXME is it valid way?
                    std::size_t max_size{ vec_fields.begin()->second.size() };
                    for (auto const& [_, val] : vec_fields) {
                        max_size = std::max(max_size, val.size());
                    }
                    for (std::size_t ind{}; ind < max_size; ++ind) {
                        Fields fields{};
                        for (auto const& [key, val] : vec_fields) {
                            if (val.size() > ind) {
                                fields[key] = val[ind];
                            }    // else FIXME what to do there
                        }
                        // FIXME separate into function
                        {
                            luwra::StateWrapper state;
                            state.loadStandardLibrary();
                            register_userdata_property_value(state);
                            state[config::keywords::CONTEXT] = context;
                            state[config::keywords::INPUT_TABLE] = fields;
                            state[config::keywords::OUTPUT_TABLE] = dynser::Properties{};
                            auto const script_run_result = state.runString(script->c_str());
                            if (script_run_result != LUA_OK) {
                                auto const error = state.read<std::string>(-1);
                                return make_deserialize_err<ResultSuccess>(deserialize_err::ScriptError{ error }, sv);
                            }
                            auto const table =
                                state[config::keywords::OUTPUT_TABLE].read<std::map<std::string, luwra::Reference>>();
                            for (auto const& [key, val] : table) {
                                if (properties.contains(key)) {
                                    // FIXME possible branch?
                                    // FIXME check if type is correct (calls abort() if not)
                                    properties.at(key).as_list().push_back(val);
                                }
                                else {
                                    // FIXME check if type is correct (calls abort() if not)
                                    properties[key] = PropertyValue{ PropertyValue::ListType<PropertyValue>{ val } };
                                }
                            }
                        }
                    }
                }
            }

            return std::make_pair(properties, curr_sv);
        },
        [&](config::yaml::RecurrentDict const& nested) -> Result {
            // FIXME
            return {};
        }
    );
}

}    // namespace details

/**
 * \brief string <=> target convertion based on Mappers and config file.
 * \tparam PropertyToTargetMapper functor what receives properties struct (and context) and returns target.
 * \tparam TargetToPropertyMapper functor what receives target (and context) and returns properties struct.
 */
template <typename PropertyToTargetMapper, typename TargetToPropertyMapper>
class DynSer
{
    std::optional<config::Config> config_{};

    config::ParseResult from_file(const config::RawContents& wrapper) noexcept
    {
        return config::from_string(wrapper.config);
    }

    config::ParseResult from_file(const config::FileName& wrapper) noexcept
    {
        std::ifstream file{ wrapper.config_file_name };
        std::stringstream buffer;
        buffer << file.rdbuf();
        return from_file(config::RawContents(buffer.str()));
    }

    // share 'existing' serialize between continual, branched and recurrent
    template <typename Existing>
    auto gen_existing_process_helper(const auto& props, const auto& after_script_fields) noexcept
    {
        return [&](const Existing& nested) noexcept -> dynser::SerializeResult {
            // remove prefix if exists
            Properties without_prefix = nested.prefix ? util::remove_prefix(props, *nested.prefix) : props;
            // replace parent props with child (existing) props
            // FIXME not obvious behavior, must be documented at least
            const auto inp = util::remove_prefix(without_prefix, nested.tag) << without_prefix;
            const auto serialize_result = this->serialize_props(inp, nested.tag);
            if (!serialize_result &&
                std::holds_alternative<serialize_err::ScriptVariableNotFound>(serialize_result.error().error) &&
                !nested.required)
            {
                return "";    // can be used as recursion exit
            }

            return serialize_result;    // pass through
        };
    }

    // share 'linear' serialize between continual, branched and recurrent
    template <typename Linear>
    auto gen_linear_process_helper(const auto& props, const auto& after_script_fields) noexcept
    {
        return [&](const Linear& nested) noexcept -> dynser::SerializeResult {
            using config::yaml::GroupValues;

            const auto pattern =
                nested.dyn_groups
                    ? config::details::resolve_dyn_regex(
                          nested.pattern,
                          *dynser::details::merge_maps(*nested.dyn_groups, dynser::details::props_to_fields(context))
                      )
                    : nested.pattern;
            const auto regex_fields_sus = nested.fields
                                              ? dynser::details::merge_maps(*nested.fields, after_script_fields)
                                              : std::expected<GroupValues, std::string>{ GroupValues{} };
            if (!regex_fields_sus) {
                // failed to merge script variables (script not set all variables or failed to execute)
                return make_serialize_err(serialize_err::ScriptVariableNotFound{ regex_fields_sus.error() }, props);
            }
            const auto to_string_result = config::details::resolve_regex(pattern, *regex_fields_sus);

            if (!to_string_result) {
                return make_serialize_err(serialize_err::ResolveRegexError{ to_string_result.error() }, props);
            }
            return *to_string_result;
        };
    }

public:
    const PropertyToTargetMapper pttm;
    const TargetToPropertyMapper ttpm;
    Context context;

    DynSer() noexcept
      : pttm{ generate_property_to_target_mapper() }
      , ttpm{ generate_target_to_property_mapper() }
    { }

    DynSer(PropertyToTargetMapper&& pttm, TargetToPropertyMapper&& ttpm) noexcept
      : pttm{ std::move(pttm) }
      , ttpm{ std::move(ttpm) }
    { }

    template <typename ConfigWrapper>
    config::ParseResult load_config(ConfigWrapper&& wrapper) noexcept
    {
        auto config = from_file(std::forward<ConfigWrapper>(wrapper));
        if (config) {
            config_ = std::move(*config);
        }
        return config;
    }

    template <typename ConfigWrapper>
    config::ParseResult merge_config(ConfigWrapper&& wrapper) noexcept
    {
        auto config = from_file(std::forward<ConfigWrapper>(wrapper));
        if (config) {
            config_->merge(std::move(*config));
        }
        return config;
    }

    SerializeResult serialize_props(Properties const& props, std::string_view const tag) noexcept;

    template <typename Target>
        requires requires(Target target) {
            {
                ttpm(context, target)
            } -> std::same_as<dynser::Properties>;
        }
    SerializeResult serialize(Target const& target, std::string_view const tag) noexcept
    {
        if (!config_) {
            return make_serialize_err(serialize_err::ConfigNotLoaded{}, {});
        }

        return serialize_props(ttpm(context, target), tag);
    }

    DeserializeResult<Properties> deserialize_to_props(std::string_view const sv, std::string_view const tag) noexcept;

    template <typename Target>
        requires requires(dynser::Properties props, Target target) {
            {
                pttm(context, props, target)
            } -> std::same_as<void>;
        }
    DeserializeResult<Target> deserialize(std::string_view const sv, std::string_view const tag) noexcept
    {
        auto props_sus = deserialize_to_props(sv, tag);

        if (!props_sus) {
            return std::unexpected{ props_sus.error() };
        }

        Target result{};
        pttm(context, *props_sus, result);
        return result;
    }
};

// Deduction guide for empty constructor
DynSer() -> DynSer<TargetToPropertyMapper<>, PropertyToTargetMapper<>>;

template <typename PTTM, typename TTPM>
SerializeResult DynSer<PTTM, TTPM>::serialize_props(Properties const& props, std::string_view const tag) noexcept
{
    if (!config_) {
        return make_serialize_err(serialize_err::ConfigNotLoaded{}, props);
    }
    if (!config_->tags.contains(std::string{ tag })) {
        return make_serialize_err(serialize_err::ConfigTagNotFound{ std::string{ tag } }, props);
    }

    using namespace config::yaml;
    using namespace config;

    // input: { 'a': 0, 'b': [ 1, 2, 3 ] }
    // output: ( { 'a': 0 }, [ { 'b': 1 }, { 'b': 2 }, { 'b': 3 } ] )
    dynser::Properties non_list_props;
    std::vector<dynser::Properties> unflattened_props_vector;
    std::tie(non_list_props, unflattened_props_vector) =
        [](Properties const& props) -> std::pair<Properties, std::vector<Properties>> {    // iife
        std::vector<Properties> unflattened_props;
        Properties non_list_props;

        for (auto const& [key, prop_val] : props) {
            if (prop_val.is_list()) {
                const auto size = prop_val.as_const_list().size();

                if (unflattened_props.size() < size) {
                    unflattened_props.resize(size);
                }
                for (std::size_t ind{}; auto const& el : prop_val.as_const_list()) {
                    unflattened_props[ind][key] = el;
                    ++ind;
                }
            }
            else {
                non_list_props[key] = prop_val;
            }
        }

        return { non_list_props, unflattened_props };
    }(props);

    luwra::StateWrapper state;
    state.loadStandardLibrary();
    register_userdata_property_value(state);
    state[keywords::CONTEXT] = context;
    const auto& tag_config = config_->tags.at(std::string{ tag });

    const auto props_to_fields =    //
        [](luwra::StateWrapper& state, Properties const& props, Tag const& tag_config
        ) -> std::expected<dynser::Fields, dynser::SerializeError> {
        state[keywords::INPUT_TABLE] = props;
        state[keywords::OUTPUT_TABLE] = Fields{};
        // run script
        if (const auto& script = tag_config.serialization_script) {
            const auto script_run_result = state.runString(script->c_str());
            if (script_run_result != LUA_OK) {
                const auto error = state.read<std::string>(-1);
                return std::unexpected{ SerializeError{ serialize_err::ScriptError{ error }, props } };
            }
        }
        return state[keywords::OUTPUT_TABLE].read<dynser::Fields>();
    };

    Fields fields;
    if (!non_list_props.empty()) {
        auto non_list_fields_sus = props_to_fields(state, non_list_props, tag_config);
        if (!non_list_fields_sus) {
            return make_serialize_err(std::move(non_list_fields_sus.error().error), props);
        }
        fields = *non_list_fields_sus;
    }    // else only list-fields

    // input (unflattened_props_vector): [ { 'b': 1 }, { 'b': 2 }, { 'b': 3 } ]
    // output (unflattened_fields): [ { 'b': '1' }, { 'b': '2' }, { 'b': '3' } ]
    std::vector<Fields> unflattened_fields;    // for recurrent
    // FIXME realization looks bad, especially this check
    if (std::holds_alternative<Recurrent>(tag_config.nested) && !unflattened_props_vector.empty()) {
        for (auto const& unflattened_props : unflattened_props_vector) {
            auto unflattened_fields_sus = props_to_fields(state, unflattened_props, tag_config);
            if (!unflattened_fields_sus) {
                return make_serialize_err(std::move(unflattened_fields_sus.error().error), props);
            }
            unflattened_fields.push_back(*unflattened_fields_sus);
        }
    }

    // nested
    return util::visit_one_terminated(
        tag_config.nested,
        [&](const Continual& continual) -> SerializeResult {
            using namespace config::details;
            std::string result;

            for (std::size_t rule_ind{}; rule_ind < continual.size(); ++rule_ind) {
                const auto& rule = continual[rule_ind];

                auto serialized_continual = util::visit_one_terminated(
                    rule,
                    gen_existing_process_helper<ConExisting>(props, fields),
                    gen_linear_process_helper<ConLinear>(props, fields)
                );
                if (!serialized_continual) {
                    // add ref to outside rule
                    append_ref_to_err(serialized_continual.error(), { std::string{ tag }, rule_ind });
                    return serialized_continual;
                }
                result += *serialized_continual;
            }

            return result;
        },
        [&](const Branched& branched) -> SerializeResult {
            luwra::StateWrapper branched_script_state;
            branched_script_state.loadStandardLibrary();
            register_userdata_property_value(branched_script_state);

            branched_script_state[keywords::CONTEXT] = context;
            branched_script_state[keywords::INPUT_TABLE] = props;
            using keywords::BRANCHED_RULE_IND_ERRVAL;
            branched_script_state[keywords::BRANCHED_RULE_IND_VARIABLE] = BRANCHED_RULE_IND_ERRVAL;
            const auto branched_script_run_result = branched_script_state.runString(branched.branching_script.c_str());
            if (branched_script_run_result != LUA_OK) {
                const auto error = branched_script_state.read<std::string>(-1);
                return make_serialize_err(serialize_err::ScriptError{ error }, props);
            }
            const auto branched_rule_ind = branched_script_state[keywords::BRANCHED_RULE_IND_VARIABLE].read<int>();
            if (branched_rule_ind == BRANCHED_RULE_IND_ERRVAL) {
                return make_serialize_err(serialize_err::BranchNotSet{}, props);
            }
            if (branched_rule_ind >= branched.rules.size()) {
                return make_serialize_err(
                    serialize_err::BranchOutOfBounds{ .selected_branch = static_cast<std::uint32_t>(branched_rule_ind),
                                                      .max_branch = branched.rules.size() - 1 },
                    props
                );
            }

            auto serialized_branched = util::visit_one_terminated(
                branched.rules[branched_rule_ind],
                gen_existing_process_helper<BraExisting>(props, fields),
                gen_linear_process_helper<BraLinear>(props, fields)
            );

            if (!serialized_branched) {
                // add ref to outside rule
                append_ref_to_err(
                    serialized_branched.error(), { std::string{ tag }, static_cast<std::size_t>(branched_rule_ind) }
                );
            }
            return serialized_branched;
        },
        [&](const Recurrent& recurrent) -> SerializeResult {
            std::string result;

            // max length of property lists
            // FIXME is priority unused?
            if (const auto calc_lists_len_result =
                    dynser::details::calc_max_property_lists_len(*config_, props, recurrent)) {
                const auto max_len = calc_lists_len_result->second;

                for (std::size_t ind{}; ind < max_len; ++ind) {
                    for (const auto& recurrent_rule : recurrent) {
                        // split lists in props and fields into current element
                        Fields curr_fields = fields;
                        if (unflattened_fields.size() > ind) {
                            curr_fields.merge(unflattened_fields[ind]);
                        }
                        Properties curr_props = props;
                        if (unflattened_props_vector.size() > ind) {
                            for (auto const& [key, val] : unflattened_props_vector[ind]) {
                                curr_props[key] = val;
                            }
                        }
                        const auto serialized_recurrent = util::visit_one_terminated(
                            recurrent_rule,
                            gen_existing_process_helper<RecExisting>(curr_props, curr_fields),
                            gen_linear_process_helper<RecLinear>(curr_props, curr_fields),
                            [&](const RecInfix& rule) -> SerializeResult {
                                if (ind == max_len - 1) {
                                    return "";    // infix rule -> return empty string on last element
                                }

                                return gen_linear_process_helper<RecInfix>(curr_props, curr_fields)(rule);
                            }
                        );
                        if (!serialized_recurrent) {
                            return serialized_recurrent;
                        }
                        result += *serialized_recurrent;
                    }
                }
            }

            return result;
        },
        [&](const RecurrentDict& recurrent_dict) -> SerializeResult {
            std::string result;

            if (!props.contains(recurrent_dict.key)) {
                return make_serialize_err(serialize_err::RecurrentDictKeyNotFound{ recurrent_dict.key }, props);
            }
            for (std::size_t ind{}; auto const& dict : props.at(recurrent_dict.key).as_const_list()) {
                auto serialize_result = serialize_props(dict.as_const_map(), recurrent_dict.tag);

                if (!serialize_result) {
                    append_ref_to_err(serialize_result.error(), { std::string{ tag }, ind });

                    return serialize_result;
                }

                result += *serialize_result;

                ++ind;
            }
            return result;
        }
    );
}

template <typename PTTM, typename TTPM>
DeserializeResult<Properties>
DynSer<PTTM, TTPM>::deserialize_to_props(std::string_view const sv, std::string_view const tag) noexcept
{
    using Target = Properties;

    if (!config_) {
        return make_deserialize_err<Target>(deserialize_err::ConfigNotLoaded{}, sv);
    }
    if (!config_->tags.contains(std::string{ tag })) {
        return make_deserialize_err<Target>(deserialize_err::ConfigTagNotFound{ std::string{ tag } }, sv);
    }

    auto const deserialize_to_props_result = details::deserialize_to_props(context, *config_, sv, tag);
    if (!deserialize_to_props_result) {
        return std::unexpected{ deserialize_to_props_result.error() };
    }
    // FIMXE sv_rem unused
    auto const& [props, sv_rem] = *deserialize_to_props_result;

    return props;
}

}    // namespace dynser
