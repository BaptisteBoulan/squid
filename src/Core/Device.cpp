#include <squid/Core/Device.hpp>
#include <squid/Core/Instance.hpp>

#include "../VulkanUtils.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <stdexcept>
#include <utility>

namespace sqd {
namespace {
struct QueueFamilies {
    uint32_t graphics = UINT32_MAX;
    uint32_t present = UINT32_MAX;
};

QueueFamilies findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, properties.data());

    QueueFamilies result;
    uint32_t firstGraphics = UINT32_MAX;
    uint32_t firstPresent = UINT32_MAX;
    for (uint32_t i = 0; i < count; ++i) {
        const bool graphics = properties[i].queueCount && (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
        VkBool32 supported = VK_FALSE;
        const bool present = surface &&
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supported) == VK_SUCCESS && supported;
        if (graphics && firstGraphics == UINT32_MAX) firstGraphics = i;
        if (present && firstPresent == UINT32_MAX) firstPresent = i;
        if (graphics && present) {
            return {i, i};
        }
    }
    return {firstGraphics, firstPresent};
}

bool hasExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& required) {
    uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }
    std::vector<VkExtensionProperties> properties(count);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, properties.data()) != VK_SUCCESS) {
        return false;
    }
    return std::all_of(required.begin(), required.end(), [&](const char* name) {
        return std::any_of(properties.begin(), properties.end(), [name](const auto& property) {
            return std::string(property.extensionName) == name;
        });
    });
}
} // namespace

Device::Device(const Instance& instance, vk::SurfaceKHR surface, const DeviceConfig& config) {
    std::vector<const char*> requiredExtensions = config.extensions;
    requiredExtensions.insert(requiredExtensions.end(), config.requiredExtensions.begin(), config.requiredExtensions.end());
    std::sort(requiredExtensions.begin(), requiredExtensions.end(), [](const char* lhs, const char* rhs) {
        return std::string(lhs) < std::string(rhs);
    });
    requiredExtensions.erase(std::unique(requiredExtensions.begin(), requiredExtensions.end(),
                                         [](const char* lhs, const char* rhs) {
                                             return std::string(lhs) == rhs;
                                         }),
                             requiredExtensions.end());

    uint32_t count = 0;
    detail::check(vkEnumeratePhysicalDevices(static_cast<VkInstance>(instance.handle()), &count, nullptr),
                  "Enumerate physical devices");
    if (!count) {
        throw std::runtime_error("No Vulkan physical devices are available");
    }
    std::vector<VkPhysicalDevice> devices(count);
    detail::check(vkEnumeratePhysicalDevices(static_cast<VkInstance>(instance.handle()), &count, devices.data()),
                  "Enumerate physical devices");

    int bestScore = -1;
    QueueFamilies bestQueues;
    for (VkPhysicalDevice candidate : devices) {
        const QueueFamilies queues = findQueueFamilies(candidate, static_cast<VkSurfaceKHR>(surface));
        if ((config.requireGraphicsQueue && queues.graphics == UINT32_MAX) ||
            (config.requirePresentQueue && queues.present == UINT32_MAX) ||
            !hasExtensions(candidate, requiredExtensions)) {
            continue;
        }
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 10000 : 1000;
        score += static_cast<int>(properties.limits.maxImageDimension2D / 1024);
        if (score > bestScore) {
            bestScore = score;
            m_physicalDevice = vk::PhysicalDevice(candidate);
            bestQueues = queues;
        }
    }
    if (!m_physicalDevice) {
        throw std::runtime_error("No physical device satisfies the requested queues and extensions");
    }

    m_graphicsQueueFamily = bestQueues.graphics;
    m_presentQueueFamily = bestQueues.present;
    std::set<uint32_t> uniqueFamilies;
    if (m_graphicsQueueFamily != UINT32_MAX) uniqueFamilies.insert(m_graphicsQueueFamily);
    if (m_presentQueueFamily != UINT32_MAX) uniqueFamilies.insert(m_presentQueueFamily);
    const float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueInfos.push_back(queueInfo);
    }

    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    VkDevice device = VK_NULL_HANDLE;
    detail::check(vkCreateDevice(static_cast<VkPhysicalDevice>(m_physicalDevice), &createInfo, nullptr, &device),
                  "Create logical device");
    m_device = vk::Device(device);
    if (m_graphicsQueueFamily != UINT32_MAX) {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, m_graphicsQueueFamily, 0, &queue);
        m_graphicsQueue = vk::Queue(queue);
    }
    if (m_presentQueueFamily != UINT32_MAX) {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, m_presentQueueFamily, 0, &queue);
        m_presentQueue = vk::Queue(queue);
    }
}

Device::~Device() {
    if (m_device) vkDestroyDevice(static_cast<VkDevice>(m_device), nullptr);
}

Device::Device(Device&& other) noexcept
    : m_physicalDevice(std::exchange(other.m_physicalDevice, vk::PhysicalDevice{})),
      m_device(std::exchange(other.m_device, vk::Device{})),
      m_graphicsQueue(std::exchange(other.m_graphicsQueue, vk::Queue{})),
      m_presentQueue(std::exchange(other.m_presentQueue, vk::Queue{})),
      m_graphicsQueueFamily(std::exchange(other.m_graphicsQueueFamily, UINT32_MAX)),
      m_presentQueueFamily(std::exchange(other.m_presentQueueFamily, UINT32_MAX)) {}

Device& Device::operator=(Device&& other) noexcept {
    if (this != &other) {
        if (m_device) vkDestroyDevice(static_cast<VkDevice>(m_device), nullptr);
        m_physicalDevice = std::exchange(other.m_physicalDevice, vk::PhysicalDevice{});
        m_device = std::exchange(other.m_device, vk::Device{});
        m_graphicsQueue = std::exchange(other.m_graphicsQueue, vk::Queue{});
        m_presentQueue = std::exchange(other.m_presentQueue, vk::Queue{});
        m_graphicsQueueFamily = std::exchange(other.m_graphicsQueueFamily, UINT32_MAX);
        m_presentQueueFamily = std::exchange(other.m_presentQueueFamily, UINT32_MAX);
    }
    return *this;
}

vk::PhysicalDeviceMemoryProperties Device::memoryProperties() const {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(static_cast<VkPhysicalDevice>(m_physicalDevice), &properties);
    return vk::PhysicalDeviceMemoryProperties(properties);
}

} // namespace sqd
