#pragma once

#include <vulkan/vulkan.hpp>

#include <string>
#include <vector>

namespace sqd {

class Instance;

struct DeviceConfig {
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char*> requiredExtensions;
    bool requireGraphicsQueue = true;
    bool requirePresentQueue = true;
};

class Device {
public:
    Device(const Instance& instance, vk::SurfaceKHR surface, const DeviceConfig& config = {});
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    vk::Device handle() const noexcept { return m_device; }
    operator vk::Device() const noexcept { return m_device; }
    vk::PhysicalDevice physicalDevice() const noexcept { return m_physicalDevice; }
    vk::Queue graphicsQueue() const noexcept { return m_graphicsQueue; }
    vk::Queue presentQueue() const noexcept { return m_presentQueue; }
    uint32_t graphicsQueueFamily() const noexcept { return m_graphicsQueueFamily; }
    uint32_t presentQueueFamily() const noexcept { return m_presentQueueFamily; }
    vk::PhysicalDeviceMemoryProperties memoryProperties() const;

private:
    vk::PhysicalDevice m_physicalDevice{};
    vk::Device m_device{};
    vk::Queue m_graphicsQueue{};
    vk::Queue m_presentQueue{};
    uint32_t m_graphicsQueueFamily = 0;
    uint32_t m_presentQueueFamily = 0;
};

} // namespace sqd
