#pragma once

#include "dynser.h"

namespace dynser::util
{

inline Properties map_to_props() noexcept { return {}; }

template <typename T>
inline Properties map_to_props(T&&) noexcept = delete;

template <typename Key, typename Val, typename... Other>
inline Properties map_to_props(Key&& key, Val&& val, Other&&... other) noexcept
{
    return Properties{ { std::forward<Key>(key), PropertyValue{ std::forward<Val>(val) } } }
           << map_to_props(std::forward<Other>(other)...);
}

inline Context map_to_context() noexcept { return {}; }

template <typename T>
inline Context map_to_context(T&&) noexcept = delete;

template <typename Key, typename Val, typename... Other>
inline Context map_to_context(Key&& key, Val&& val, Other&&... other) noexcept
{
    return Context{ { std::forward<Key>(key), PropertyValue{ std::forward<Val>(val) } } }
           << map_to_context(std::forward<Other>(other)...);
}

}    // namespace dynser::util
