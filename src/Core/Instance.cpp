#include <squid/Core/Instance.hpp>

#include "../VulkanUtils.hpp"

#include <algorithm>
#include <utility>

namespace sqd {

Instance::Instance(const InstanceConfig& config) {
    std::vector<const char*> layers;
    if (config.enableValidation) {
        uint32_t count = 0;
        detail::check(vkEnumerateInstanceLayerProperties(&count, nullptr), "Enumerate instance layers");
        std::vector<VkLayerProperties> available(count);
        detail::check(vkEnumerateInstanceLayerProperties(&count, available.data()), "Enumerate instance layers");
        for (const char* requested : config.validationLayers) {
            const bool found = std::any_of(available.begin(), available.end(), [requested](const auto& layer) {
                return std::string(layer.layerName) == requested;
            });
            if (!found) {
                throw std::runtime_error(std::string("Requested Vulkan validation layer is unavailable: ") + requested);
            }
            layers.push_back(requested);
        }
    }

    std::vector<const char*> extensions = config.extensions;
    std::sort(extensions.begin(), extensions.end(), [](const char* lhs, const char* rhs) {
        return std::string(lhs) < std::string(rhs);
    });
    extensions.erase(std::unique(extensions.begin(), extensions.end(), [](const char* lhs, const char* rhs) {
        return std::string(lhs) == rhs;
    }), extensions.end());

    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = config.applicationName.c_str();
    applicationInfo.applicationVersion = config.applicationVersion;
    applicationInfo.pEngineName = "Squid";
    applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    applicationInfo.apiVersion = config.apiVersion;

    VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkInstance instance = VK_NULL_HANDLE;
    detail::check(vkCreateInstance(&createInfo, nullptr, &instance), "Create Vulkan instance");
    m_instance = vk::Instance(instance);
}

Instance::~Instance() {
    if (m_instance) {
        vkDestroyInstance(static_cast<VkInstance>(m_instance), nullptr);
    }
}

Instance::Instance(Instance&& other) noexcept : m_instance(std::exchange(other.m_instance, vk::Instance{})) {}

Instance& Instance::operator=(Instance&& other) noexcept {
    if (this != &other) {
        if (m_instance) {
            vkDestroyInstance(static_cast<VkInstance>(m_instance), nullptr);
        }
        m_instance = std::exchange(other.m_instance, vk::Instance{});
    }
    return *this;
}

} // namespace sqd
