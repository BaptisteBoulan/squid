#pragma once

#include <vulkan/vulkan.hpp>

#include <stdexcept>

namespace sqd::detail {

inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeBits,
                               VkMemoryPropertyFlags requiredProperties) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((typeBits & (1u << i)) &&
            (properties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties) {
            return i;
        }
    }
    throw std::runtime_error("No Vulkan memory type satisfies the requested properties");
}

} // namespace sqd::detail
