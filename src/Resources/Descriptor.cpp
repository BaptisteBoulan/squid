#include <squid/Core/Device.hpp>
#include <squid/Resources/Descriptor.hpp>

#include "../VulkanUtils.hpp"

#include <utility>

namespace squid {

DescriptorPool::DescriptorPool(const Device& device, const DescriptorPoolConfig& config)
    : m_device(device.handle()) {
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(config.sizes.size());
    for (const auto& size : config.sizes) {
        sizes.push_back({static_cast<VkDescriptorType>(size.type), size.descriptorCount});
    }
    VkDescriptorPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    createInfo.flags = static_cast<VkDescriptorPoolCreateFlags>(config.flags);
    createInfo.maxSets = config.maxSets;
    createInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
    createInfo.pPoolSizes = sizes.data();
    VkDescriptorPool pool = VK_NULL_HANDLE;
    detail::check(vkCreateDescriptorPool(static_cast<VkDevice>(m_device), &createInfo, nullptr, &pool),
                  "Create descriptor pool");
    m_pool = vk::DescriptorPool(pool);
}

DescriptorPool::~DescriptorPool() {
    if (m_pool) vkDestroyDescriptorPool(static_cast<VkDevice>(m_device), static_cast<VkDescriptorPool>(m_pool), nullptr);
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_pool(std::exchange(other.m_pool, vk::DescriptorPool{})) {}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept {
    if (this != &other) {
        if (m_pool) vkDestroyDescriptorPool(static_cast<VkDevice>(m_device), static_cast<VkDescriptorPool>(m_pool), nullptr);
        m_device = std::exchange(other.m_device, vk::Device{});
        m_pool = std::exchange(other.m_pool, vk::DescriptorPool{});
    }
    return *this;
}

DescriptorSetLayout::DescriptorSetLayout(const Device& device,
                                         const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
    : m_device(device.handle()) {
    std::vector<VkDescriptorSetLayoutBinding> rawBindings;
    std::vector<std::vector<VkSampler>> immutableSamplers(bindings.size());
    rawBindings.reserve(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        const auto& binding = bindings[i];
        if (binding.pImmutableSamplers) {
            immutableSamplers[i].reserve(binding.descriptorCount);
            for (uint32_t sampler = 0; sampler < binding.descriptorCount; ++sampler) {
                immutableSamplers[i].push_back(static_cast<VkSampler>(binding.pImmutableSamplers[sampler]));
            }
        }
        rawBindings.push_back({binding.binding, static_cast<VkDescriptorType>(binding.descriptorType),
                               binding.descriptorCount, static_cast<VkShaderStageFlags>(binding.stageFlags),
                               immutableSamplers[i].empty() ? nullptr : immutableSamplers[i].data()});
    }
    VkDescriptorSetLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    createInfo.bindingCount = static_cast<uint32_t>(rawBindings.size());
    createInfo.pBindings = rawBindings.data();
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    detail::check(vkCreateDescriptorSetLayout(static_cast<VkDevice>(m_device), &createInfo, nullptr, &layout),
                  "Create descriptor set layout");
    m_layout = vk::DescriptorSetLayout(layout);
}

DescriptorSetLayout::~DescriptorSetLayout() {
    if (m_layout) vkDestroyDescriptorSetLayout(static_cast<VkDevice>(m_device),
                                                static_cast<VkDescriptorSetLayout>(m_layout), nullptr);
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_layout(std::exchange(other.m_layout, vk::DescriptorSetLayout{})) {}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept {
    if (this != &other) {
        if (m_layout) vkDestroyDescriptorSetLayout(static_cast<VkDevice>(m_device),
                                                    static_cast<VkDescriptorSetLayout>(m_layout), nullptr);
        m_device = std::exchange(other.m_device, vk::Device{});
        m_layout = std::exchange(other.m_layout, vk::DescriptorSetLayout{});
    }
    return *this;
}

DescriptorSet::DescriptorSet(const DescriptorPool& pool, const DescriptorSetLayout& layout) {
    VkDescriptorSetLayout rawLayout = static_cast<VkDescriptorSetLayout>(layout.handle());
    VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocateInfo.descriptorPool = static_cast<VkDescriptorPool>(pool.handle());
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &rawLayout;
    VkDescriptorSet set = VK_NULL_HANDLE;
    detail::check(vkAllocateDescriptorSets(static_cast<VkDevice>(pool.m_device), &allocateInfo, &set),
                  "Allocate descriptor set");
    m_set = vk::DescriptorSet(set);
}

} // namespace squid
