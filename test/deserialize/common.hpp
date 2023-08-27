#pragma once

#include "dynser.h"
#include <catch2/catch_test_macros.hpp>

#include "printer.hpp"

namespace dynser_test
{
#define DYNSER_LOAD_CONFIG(ser, config)                                                                                \
    do {                                                                                                               \
        const auto load_result = ser.load_config(config);                                                              \
        REQUIRE(load_result);                                                                                          \
    } while (false);
}    // namespace dynser_test
