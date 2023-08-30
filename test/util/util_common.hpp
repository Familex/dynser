#pragma once

#include <source_location>

#include <filesystem>

namespace dynser_test
{

namespace fs = std::filesystem;

inline fs::path
from_current_path(std::string postfix, std::source_location location = std::source_location::current()) noexcept
{
    return fs::path{ location.file_name() }.parent_path() / fs::path{ postfix };
}

inline auto const configs_path = from_current_path("../configs");

inline std::string path_to_config(std::string_view config_name) noexcept
{
    return (configs_path / config_name).string();
}

#define DYNSER_LOAD_CONFIG(ser, config)                                                                                \
    do {                                                                                                               \
        const auto load_result = ser.load_config(config);                                                              \
        REQUIRE(load_result);                                                                                          \
    } while (false)

#define DYNSER_LOAD_CONFIG_FILE(ser, config_file_name)                                                                 \
    DYNSER_LOAD_CONFIG(                                                                                                \
        ser, ::dynser::config::FileName{ (configs_path / ::std::filesystem::path{ config_file_name }).string() }       \
    )

}    // namespace dynser_test
