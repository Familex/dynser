#pragma once

#include "luwra.hpp"

#include <any>
#include <cassert>
#include <cstdint>
#include <map>
#include <string>

namespace dynser
{

/**
 * \brief Named parts of type, what can be converted and projected to string.
 * \note luwra's register_user_type can't receive std::unoredered_maps.
 */
using Properties = std::map<std::string, struct PropertyValue>;

/**
 * \brief Property value type, can be used in lua.
 * std::any wrapper with constant type.
 */
struct PropertyValue
{
private:
    std::any data_;

public:
    using FloatType = double;
    using CharType = char;
    using StringType = std::string;
    using MapKeyType = std::string;
    template <typename T>
    using ListType = std::vector<T>;

    // ========================================================================
    // ===                           CONSTRUCTORS                           ===
    // ========================================================================

    inline explicit PropertyValue() noexcept
      : data_{}
    { }

    // trivial one-argument ctor macro
#define DYNSER_POPULATE_PROPERTY_VALUE(type)                                                                           \
    inline explicit PropertyValue(type value) noexcept                                                                 \
      : data_{ value }                                                                                                 \
    { }

    DYNSER_POPULATE_PROPERTY_VALUE(std::int32_t)
    DYNSER_POPULATE_PROPERTY_VALUE(std::int64_t)
    DYNSER_POPULATE_PROPERTY_VALUE(std::uint32_t)
    DYNSER_POPULATE_PROPERTY_VALUE(std::uint64_t)
    DYNSER_POPULATE_PROPERTY_VALUE(FloatType)
    DYNSER_POPULATE_PROPERTY_VALUE(StringType)

    // construct from string literal
    inline explicit PropertyValue(CharType const* value) noexcept
      : data_{ StringType{ value } }
    { }

    DYNSER_POPULATE_PROPERTY_VALUE(bool)
    DYNSER_POPULATE_PROPERTY_VALUE(CharType)
    DYNSER_POPULATE_PROPERTY_VALUE(ListType<PropertyValue>)
    DYNSER_POPULATE_PROPERTY_VALUE(Properties)

#undef DYNSER_POPULATE_PROPERTY_VALUE

    // ========================================================================
    // ===                     STATIC METHOD CONSTRUCTORS                   ===
    // ========================================================================

#define DYNSER_POPULATE_STATIC_METHOD_CTOR(name, type)                                                                 \
    inline static PropertyValue name(type value) /*noexcept*/ { return PropertyValue(std::move(value)); }

    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_i32, std::int32_t)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_i64, std::int64_t)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_u32, std::uint32_t)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_u64, std::uint64_t)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_float, FloatType)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_string, StringType)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_bool, bool)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_char, CharType)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_list, ListType<PropertyValue>)
    DYNSER_POPULATE_STATIC_METHOD_CTOR(from_properties, Properties)

#undef DYNSER_POPULATE_STATIC_METHOD_CTOR

    // ========================================================================
    // ===                           'IS' method                            ===
    // ========================================================================

private:
    // base method
    template <typename T>
    inline decltype(auto) is() const noexcept
    {
        return typeid(T) == data_.type();
    }

public:
#define DYNSER_POPULATE_IS(name, type)                                                                                 \
    inline decltype(auto) name() const noexcept { return is<type>(); }

    DYNSER_POPULATE_IS(is_i32, std::int32_t)
    DYNSER_POPULATE_IS(is_i64, std::int64_t)
    DYNSER_POPULATE_IS(is_u32, std::uint32_t)
    DYNSER_POPULATE_IS(is_u64, std::uint64_t)
    DYNSER_POPULATE_IS(is_float, FloatType)
    DYNSER_POPULATE_IS(is_string, StringType)
    DYNSER_POPULATE_IS(is_bool, bool)
    DYNSER_POPULATE_IS(is_char, CharType)
    DYNSER_POPULATE_IS(is_list, ListType<PropertyValue>)
    DYNSER_POPULATE_IS(is_map, Properties)

#undef DYNSER_POPULATE_IS

    // ========================================================================
    // ===                       'AS_CONST' method                          ===
    // ========================================================================

private:
    // base method
    // get data copy from const object instances
    template <typename T>
    inline decltype(auto) as_const() const
    {
        assert(is<T>());    // FIXME make out parameter optional?
        return std::any_cast<const T&>(data_);
    }

public:
#define DYNSER_POPULATE_AS_CONST(name, raw_type)                                                                       \
    inline decltype(auto) name() const { return as_const<raw_type>(); }

    DYNSER_POPULATE_AS_CONST(as_const_i32, std::int32_t)
    DYNSER_POPULATE_AS_CONST(as_const_i64, std::int64_t)
    DYNSER_POPULATE_AS_CONST(as_const_u32, std::uint32_t)
    DYNSER_POPULATE_AS_CONST(as_const_u64, std::uint64_t)
    DYNSER_POPULATE_AS_CONST(as_const_float, FloatType)
    DYNSER_POPULATE_AS_CONST(as_const_string, StringType)
    DYNSER_POPULATE_AS_CONST(as_const_bool, bool)
    DYNSER_POPULATE_AS_CONST(as_const_char, CharType)
    DYNSER_POPULATE_AS_CONST(as_const_list, ListType<PropertyValue>)
    DYNSER_POPULATE_AS_CONST(as_const_map, Properties)

#undef DYNSER_POPULATE_AS_CONST

    // ========================================================================
    // ===                           'AS' method                            ===
    // ========================================================================

private:
    // base method
    // modify data of object instance
    template <typename T>
    inline decltype(auto) as()
    {
        assert(is<T>());    // FIXME make out parameter optional?
        return std::any_cast<T&>(data_);
    }

public:
#define DYNSER_POPULATE_AS(name, raw_type)                                                                             \
    inline decltype(auto) name() { return as<raw_type>(); }

    DYNSER_POPULATE_AS(as_i32, std::int32_t)
    DYNSER_POPULATE_AS(as_i64, std::int64_t)
    DYNSER_POPULATE_AS(as_u32, std::uint32_t)
    DYNSER_POPULATE_AS(as_u64, std::uint64_t)
    DYNSER_POPULATE_AS(as_float, FloatType)
    DYNSER_POPULATE_AS(as_string, StringType)
    DYNSER_POPULATE_AS(as_bool, bool)
    DYNSER_POPULATE_AS(as_char, CharType)
    DYNSER_POPULATE_AS(as_list, ListType<PropertyValue>)
    DYNSER_POPULATE_AS(as_map, Properties)

#undef DYNSER_POPULATE_AS
};

template <typename Rhs>
inline Properties operator<<(Properties&& lhs, Rhs&& rhs) noexcept {
    lhs.merge(std::forward<Rhs>(rhs));
    return lhs;
}

void register_userdata_property_value(luwra::StateWrapper& state) noexcept;

}    // namespace dynser

// FIXME link errors
// LUWRA_DEF_REGISTRY_NAME(dynser::PropertyValue, "PropertyValue")
