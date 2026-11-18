#include <squid/Core/Device.hpp>
#include <squid/Execution/Sync.hpp>

#include "../VulkanUtils.hpp"

#include <utility>

namespace sqd {

Semaphore::Semaphore(const Device& device) : m_device(device.handle()) {
    VkSemaphoreCreateInfo createInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphore semaphore = VK_NULL_HANDLE;
    detail::check(vkCreateSemaphore(static_cast<VkDevice>(m_device), &createInfo, nullptr, &semaphore),
                  "Create semaphore");
    m_semaphore = vk::Semaphore(semaphore);
}

Semaphore::~Semaphore() {
    if (m_semaphore) vkDestroySemaphore(static_cast<VkDevice>(m_device), static_cast<VkSemaphore>(m_semaphore), nullptr);
}

Semaphore::Semaphore(Semaphore&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_semaphore(std::exchange(other.m_semaphore, vk::Semaphore{})) {}

Semaphore& Semaphore::operator=(Semaphore&& other) noexcept {
    if (this != &other) {
        if (m_semaphore) vkDestroySemaphore(static_cast<VkDevice>(m_device), static_cast<VkSemaphore>(m_semaphore), nullptr);
        m_device = std::exchange(other.m_device, vk::Device{});
        m_semaphore = std::exchange(other.m_semaphore, vk::Semaphore{});
    }
    return *this;
}

Fence::Fence(const Device& device, bool signaled) : m_device(device.handle()) {
    VkFenceCreateInfo createInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VkFence fence = VK_NULL_HANDLE;
    detail::check(vkCreateFence(static_cast<VkDevice>(m_device), &createInfo, nullptr, &fence), "Create fence");
    m_fence = vk::Fence(fence);
}

Fence::~Fence() {
    if (m_fence) vkDestroyFence(static_cast<VkDevice>(m_device), static_cast<VkFence>(m_fence), nullptr);
}

Fence::Fence(Fence&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})), m_fence(std::exchange(other.m_fence, vk::Fence{})) {}

Fence& Fence::operator=(Fence&& other) noexcept {
    if (this != &other) {
        if (m_fence) vkDestroyFence(static_cast<VkDevice>(m_device), static_cast<VkFence>(m_fence), nullptr);
        m_device = std::exchange(other.m_device, vk::Device{});
        m_fence = std::exchange(other.m_fence, vk::Fence{});
    }
    return *this;
}

void Fence::wait(uint64_t timeout) const {
    const VkFence fence = static_cast<VkFence>(m_fence);
    detail::check(vkWaitForFences(static_cast<VkDevice>(m_device), 1, &fence, VK_TRUE, timeout), "Wait for fence");
}

void Fence::reset() const {
    const VkFence fence = static_cast<VkFence>(m_fence);
    detail::check(vkResetFences(static_cast<VkDevice>(m_device), 1, &fence), "Reset fence");
}

} // namespace sqd
