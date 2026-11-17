#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace squid {

class Device;

struct DescriptorPoolConfig {
    std::vector<vk::DescriptorPoolSize> sizes;
    uint32_t maxSets = 1;
    vk::DescriptorPoolCreateFlags flags{};
};

class DescriptorPool {
public:
    DescriptorPool(const Device& device, const DescriptorPoolConfig& config);
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&& other) noexcept;
    DescriptorPool& operator=(DescriptorPool&& other) noexcept;

    vk::DescriptorPool handle() const noexcept { return m_pool; }
    operator vk::DescriptorPool() const noexcept { return m_pool; }

private:
    friend class DescriptorSet;
    vk::Device m_device{};
    vk::DescriptorPool m_pool{};
};

class DescriptorSetLayout {
public:
    DescriptorSetLayout(const Device& device, const std::vector<vk::DescriptorSetLayoutBinding>& bindings);
    ~DescriptorSetLayout();

    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
    DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

    vk::DescriptorSetLayout handle() const noexcept { return m_layout; }
    operator vk::DescriptorSetLayout() const noexcept { return m_layout; }

private:
    vk::Device m_device{};
    vk::DescriptorSetLayout m_layout{};
};

class DescriptorSet {
public:
    DescriptorSet(const DescriptorPool& pool, const DescriptorSetLayout& layout);

    vk::DescriptorSet handle() const noexcept { return m_set; }
    operator vk::DescriptorSet() const noexcept { return m_set; }

private:
    vk::DescriptorSet m_set{};
};

} // namespace squid
