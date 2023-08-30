#include "properties.h"

void dynser::register_userdata_property_value(luwra::StateWrapper& state) noexcept
{
    state.registerUserType<dynser::PropertyValue>(
        // members
        {
            // as
            LUWRA_MEMBER(dynser::PropertyValue, as_i32),
            LUWRA_MEMBER(dynser::PropertyValue, as_i64),
            LUWRA_MEMBER(dynser::PropertyValue, as_u32),
            LUWRA_MEMBER(dynser::PropertyValue, as_u64),
            LUWRA_MEMBER(dynser::PropertyValue, as_float),
            LUWRA_MEMBER(dynser::PropertyValue, as_string),
            LUWRA_MEMBER(dynser::PropertyValue, as_bool),
            LUWRA_MEMBER(dynser::PropertyValue, as_char),
            LUWRA_MEMBER(dynser::PropertyValue, as_list),
            LUWRA_MEMBER(dynser::PropertyValue, as_map),
            // is
            LUWRA_MEMBER(dynser::PropertyValue, is_i32),
            LUWRA_MEMBER(dynser::PropertyValue, is_i64),
            LUWRA_MEMBER(dynser::PropertyValue, is_u32),
            LUWRA_MEMBER(dynser::PropertyValue, is_u64),
            LUWRA_MEMBER(dynser::PropertyValue, is_float),
            LUWRA_MEMBER(dynser::PropertyValue, is_string),
            LUWRA_MEMBER(dynser::PropertyValue, is_bool),
            LUWRA_MEMBER(dynser::PropertyValue, is_char),
            LUWRA_MEMBER(dynser::PropertyValue, is_list),
            LUWRA_MEMBER(dynser::PropertyValue, is_map),
        },
        // meta-members
        {}
    );

    state["dynser"] = std::map<std::string, luwra::CFunction>{
        { "from_i32", LUWRA_WRAP(dynser::PropertyValue::from_i32) },
        { "from_i64", LUWRA_WRAP(dynser::PropertyValue::from_i64) },
        { "from_u32", LUWRA_WRAP(dynser::PropertyValue::from_u32) },
        { "from_float", LUWRA_WRAP(dynser::PropertyValue::from_float) },
        { "from_string", LUWRA_WRAP(dynser::PropertyValue::from_string) },
        { "from_bool", LUWRA_WRAP(dynser::PropertyValue::from_bool) },
        { "from_char", LUWRA_WRAP(dynser::PropertyValue::from_char) },
        // FIXME 'read': no matching overloaded function found error
        // { "from_list", LUWRA_WRAP(dynser::PropertyValue::from_list) },
        { "from_properties", LUWRA_WRAP(dynser::PropertyValue::from_properties) },
    };
}
