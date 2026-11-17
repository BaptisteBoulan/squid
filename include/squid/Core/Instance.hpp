#pragma once

#include <vulkan/vulkan.hpp>

#include <string>
#include <vector>

namespace squid {

struct InstanceConfig {
    std::string applicationName = "Squid application";
    uint32_t applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    uint32_t apiVersion = VK_API_VERSION_1_0;
    bool enableValidation =
#ifndef NDEBUG
        true;
#else
        false;
#endif
    std::vector<const char*> extensions;
    std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
};

class Instance {
public:
    explicit Instance(const InstanceConfig& config = {});
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance(Instance&& other) noexcept;
    Instance& operator=(Instance&& other) noexcept;

    vk::Instance handle() const noexcept { return m_instance; }
    operator vk::Instance() const noexcept { return m_instance; }

private:
    vk::Instance m_instance{};
};

} // namespace squid
