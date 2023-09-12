#pragma once
#include "structures.h"

#define DYNSER_NEW_DEFAULT inline constexpr auto

namespace dynser::config
{

DYNSER_NEW_DEFAULT DEFLT_REC_MAPPING_TYPE = dynser::config::yaml::RecurrentMappingType::DictOfArrays;

}    // namespace dynser::config

#undef DYNSER_NEW_DEFAULT