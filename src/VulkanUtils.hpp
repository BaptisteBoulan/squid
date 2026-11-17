#pragma once

#include <vulkan/vulkan.hpp>

#include <stdexcept>
#include <string>

namespace squid::detail {

inline void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(operation) + " failed with VkResult " +
                                 std::to_string(static_cast<int>(result)));
    }
}

} // namespace squid::detail
