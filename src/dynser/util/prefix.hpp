#pragma once

#include "structs/properties.h"

#include <string>

namespace dynser::util
{

constexpr auto infix{ "@" };

template <typename Map>
inline Map add_prefix(const Map& map, const std::string_view prefix) noexcept
{
    Map result{};
    for (auto& [key, value] : map) {
        result[prefix.data() + (infix + key)] = value;
    }
    return result;
}

template <typename Map, typename... Prefixes>
inline Map add_prefix(const Map& map, const std::string_view prefix, Prefixes&&... prefixes) noexcept
{
    return add_prefix(add_prefix(map, prefix), std::forward<Prefixes>(prefixes)...);
}

/**
 * \brief filters keys what starts with prefix and remove this prefix from it.
 */
template <typename Map>
inline Map remove_prefix(const Map& props, const std::string_view prefix) noexcept
{
    constexpr auto infix_size = std::string{ "@" }.size();

    Map result{};
    for (auto& [key, value] : props) {
        if (key.starts_with(prefix)) {
            result.insert({ key.substr(prefix.size() + infix_size), value });
        }
    }
    return result;
}

}    // namespace dynser::util
