#include <squid/Core/Device.hpp>
#include <squid/Execution/CommandPool.hpp>

#include "../VulkanUtils.hpp"

#include <stdexcept>
#include <utility>

namespace sqd {

CommandBuffer::CommandBuffer(vk::Device device, vk::CommandPool pool, vk::CommandBuffer buffer) noexcept
    : m_device(device), m_pool(pool), m_buffer(buffer) {}

CommandBuffer::~CommandBuffer() {
    destroy();
}

void CommandBuffer::destroy() noexcept {
    if (m_buffer) {
        const VkCommandBuffer buffer = static_cast<VkCommandBuffer>(m_buffer);
        vkFreeCommandBuffers(static_cast<VkDevice>(m_device), static_cast<VkCommandPool>(m_pool), 1, &buffer);
    }
    m_buffer = vk::CommandBuffer{};
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_pool(std::exchange(other.m_pool, vk::CommandPool{})),
      m_buffer(std::exchange(other.m_buffer, vk::CommandBuffer{})) {}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, vk::Device{});
        m_pool = std::exchange(other.m_pool, vk::CommandPool{});
        m_buffer = std::exchange(other.m_buffer, vk::CommandBuffer{});
    }
    return *this;
}

void CommandBuffer::begin(vk::CommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = static_cast<VkCommandBufferUsageFlags>(flags);
    detail::check(vkBeginCommandBuffer(static_cast<VkCommandBuffer>(m_buffer), &beginInfo), "Begin command buffer");
}

void CommandBuffer::end() {
    detail::check(vkEndCommandBuffer(static_cast<VkCommandBuffer>(m_buffer)), "End command buffer");
}

CommandPool::CommandPool(const Device& device, const CommandPoolConfig& config)
    : m_device(device.handle()), m_queue(device.graphicsQueue()) {
    const uint32_t family = config.queueFamilyIndex == UINT32_MAX ? device.graphicsQueueFamily()
                                                                  : config.queueFamilyIndex;
    if (family == UINT32_MAX) throw std::invalid_argument("Device has no graphics queue family");
    VkCommandPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    createInfo.flags = static_cast<VkCommandPoolCreateFlags>(config.flags);
    createInfo.queueFamilyIndex = family;
    VkCommandPool pool = VK_NULL_HANDLE;
    detail::check(vkCreateCommandPool(static_cast<VkDevice>(m_device), &createInfo, nullptr, &pool),
                  "Create command pool");
    m_pool = vk::CommandPool(pool);
}

CommandPool::~CommandPool() {
    if (m_pool) vkDestroyCommandPool(static_cast<VkDevice>(m_device), static_cast<VkCommandPool>(m_pool), nullptr);
}

CommandPool::CommandPool(CommandPool&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_queue(std::exchange(other.m_queue, vk::Queue{})),
      m_pool(std::exchange(other.m_pool, vk::CommandPool{})) {}

CommandPool& CommandPool::operator=(CommandPool&& other) noexcept {
    if (this != &other) {
        if (m_pool) vkDestroyCommandPool(static_cast<VkDevice>(m_device), static_cast<VkCommandPool>(m_pool), nullptr);
        m_device = std::exchange(other.m_device, vk::Device{});
        m_queue = std::exchange(other.m_queue, vk::Queue{});
        m_pool = std::exchange(other.m_pool, vk::CommandPool{});
    }
    return *this;
}

CommandBuffer CommandPool::allocateBuffer(vk::CommandBufferLevel level) const {
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool = static_cast<VkCommandPool>(m_pool);
    allocateInfo.level = static_cast<VkCommandBufferLevel>(level);
    allocateInfo.commandBufferCount = 1;
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    detail::check(vkAllocateCommandBuffers(static_cast<VkDevice>(m_device), &allocateInfo, &buffer),
                  "Allocate command buffer");
    return CommandBuffer(m_device, m_pool, vk::CommandBuffer(buffer));
}

void CommandPool::immediateSubmit(const std::function<void(vk::CommandBuffer)>& recordFunc) const {
    if (!recordFunc) throw std::invalid_argument("Immediate submit requires a recording callback");
    CommandBuffer buffer = allocateBuffer(vk::CommandBufferLevel::ePrimary);
    buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    recordFunc(buffer.handle());
    buffer.end();
    VkCommandBuffer rawBuffer = static_cast<VkCommandBuffer>(buffer.handle());
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &rawBuffer;
    detail::check(vkQueueSubmit(static_cast<VkQueue>(m_queue), 1, &submit, VK_NULL_HANDLE), "Submit command buffer");
    detail::check(vkQueueWaitIdle(static_cast<VkQueue>(m_queue)), "Wait for graphics queue");
}

} // namespace sqd
