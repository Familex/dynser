#pragma once

#include "yaml-cpp/yaml.h"
#include <unordered_map>

#include <concepts>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <variant>

namespace dynser::config
{

struct FileName
{
    std::string config_file_name;
};

struct RawContents
{
    std::string config;
};

namespace yaml
{
using PriorityType = std::int32_t;

using Regex = std::string;

using DynRegex = std::string;

using GroupValues = std::unordered_map<std::size_t, std::string>;

using DynGroupValues = std::unordered_map<std::size_t, std::string>;

using Script = std::string;

struct ConExisting
{
    std::string tag;
    std::optional<std::string> prefix;
    bool required;
};

struct BraExisting
{
    std::string tag;
    std::optional<std::string> prefix;
    bool required;
};

struct RecExisting
{
    std::string tag;
    std::optional<std::string> prefix;
    bool required;

    bool wrap;
    std::optional<std::string> default_value;
    PriorityType priority;
};

struct ConLinear
{
    yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;
};

struct BraLinear
{
    yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;
};

struct RecLinear
{
    yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;

    bool wrap;
    std::optional<std::string> default_value;
    PriorityType priority;
};

struct RecInfix
{
    yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;

    bool wrap;
    std::optional<std::string> default_value;
};

using Continual = std::vector<std::variant<ConExisting, ConLinear>>;
using Recurrent = std::vector<std::variant<RecExisting, RecLinear, RecInfix>>;
struct Branched
{
    Script branching_script;
    Script debranching_script;

    using Rules = std::variant<BraExisting, BraLinear>;
    std::vector<Rules> rules;
};
struct RecurrentDict
{
    std::string key;
    std::string tag;
};

using Nested = std::variant<Continual, Recurrent, Branched, RecurrentDict>;

struct Tag
{
    std::string name;
    Nested nested;
    std::optional<Script> serialization_script;
    std::optional<Script> deserialization_script;
};

using Tags = std::unordered_map<std::string, Tag>;

template <typename Rule>
concept HasPriority = requires(Rule const rule) {
    {
        rule.priority
    } -> std::same_as<PriorityType const&>;
};

template <typename Rule>
concept LikeExisting = requires(Rule const rule) {
    {
        rule.tag
    } -> std::same_as<std::string const&>;
    {
        rule.prefix
    } -> std::same_as<std::optional<std::string> const&>;
    {
        rule.required
    } -> std::same_as<bool const&>;
};

template <typename Rule>
concept LikeLinear = requires(Rule const rule) {
    {
        rule.pattern
    } -> std::same_as<Regex const&>;
    {
        rule.fields
    } -> std::same_as<std::optional<GroupValues> const&>;
    {
        rule.dyn_groups
    } -> std::same_as<std::optional<DynGroupValues> const&>;
};

}    // namespace yaml

// dynamic exceptions adapter
struct ParseError
{
    enum class Type {
        ParserException,
        RepresentationException,
        UnknownYamlCppException,
        UnknownException,    // mark and msg invalid
    } type;
    YAML::Mark mark;
    std::string msg;
};

struct Config
{
    std::string version;
    yaml::Tags tags;

    // merge tags and version from other config
    void merge(Config&&) noexcept;
};

using ParseResult = std::expected<Config, ParseError>;

}    // namespace dynser::config
