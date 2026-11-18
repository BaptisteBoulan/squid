#include <squid/Core/Device.hpp>
#include <squid/Resources/Buffer.hpp>

#include "../VulkanUtils.hpp"
#include "Memory.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace sqd {

Buffer::Buffer(const Device& device, const BufferConfig& config)
    : m_device(device.handle()), m_size(config.size), m_memoryProperties(config.memoryProperties) {
    if (!config.size) throw std::invalid_argument("Buffer size must be greater than zero");

    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = config.size;
    bufferInfo.usage = static_cast<VkBufferUsageFlags>(config.usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer = VK_NULL_HANDLE;
    detail::check(vkCreateBuffer(static_cast<VkDevice>(m_device), &bufferInfo, nullptr, &buffer), "Create buffer");
    m_buffer = vk::Buffer(buffer);

    try {
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(static_cast<VkDevice>(m_device), buffer, &requirements);
        const uint32_t memoryType = detail::findMemoryType(
            static_cast<VkPhysicalDevice>(device.physicalDevice()), requirements.memoryTypeBits,
            static_cast<VkMemoryPropertyFlags>(config.memoryProperties));
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        detail::check(vkAllocateMemory(static_cast<VkDevice>(m_device), &allocation, nullptr, &memory),
                      "Allocate buffer memory");
        m_memory = vk::DeviceMemory(memory);
        detail::check(vkBindBufferMemory(static_cast<VkDevice>(m_device), buffer, memory, 0), "Bind buffer memory");
    } catch (...) {
        destroy();
        throw;
    }
}

Buffer::~Buffer() {
    destroy();
}

void Buffer::destroy() noexcept {
    if (m_device) {
        if (m_buffer) vkDestroyBuffer(static_cast<VkDevice>(m_device), static_cast<VkBuffer>(m_buffer), nullptr);
        if (m_memory) vkFreeMemory(static_cast<VkDevice>(m_device), static_cast<VkDeviceMemory>(m_memory), nullptr);
    }
    m_buffer = vk::Buffer{};
    m_memory = vk::DeviceMemory{};
}

Buffer::Buffer(Buffer&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_buffer(std::exchange(other.m_buffer, vk::Buffer{})),
      m_memory(std::exchange(other.m_memory, vk::DeviceMemory{})), m_size(std::exchange(other.m_size, 0)),
      m_memoryProperties(std::exchange(other.m_memoryProperties, vk::MemoryPropertyFlags{})) {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, vk::Device{});
        m_buffer = std::exchange(other.m_buffer, vk::Buffer{});
        m_memory = std::exchange(other.m_memory, vk::DeviceMemory{});
        m_size = std::exchange(other.m_size, 0);
        m_memoryProperties = std::exchange(other.m_memoryProperties, vk::MemoryPropertyFlags{});
    }
    return *this;
}

void Buffer::upload(const void* data, size_t size, size_t offset) {
    if (offset > m_size || size > m_size - offset) {
        throw std::out_of_range("Buffer upload exceeds allocated buffer size");
    }
    if (size && !data) throw std::invalid_argument("Buffer upload source cannot be null");
    if (!(m_memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)) {
        throw std::runtime_error("Buffer upload requires host-visible memory");
    }
    if (!size) return;

    void* mapped = nullptr;
    detail::check(vkMapMemory(static_cast<VkDevice>(m_device), static_cast<VkDeviceMemory>(m_memory), 0,
                              VK_WHOLE_SIZE, 0, &mapped), "Map buffer memory");
    std::memcpy(static_cast<std::byte*>(mapped) + offset, data, size);
    if (!(m_memoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent)) {
        VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        range.memory = static_cast<VkDeviceMemory>(m_memory);
        range.offset = 0;
        range.size = VK_WHOLE_SIZE;
        const VkResult flushResult = vkFlushMappedMemoryRanges(static_cast<VkDevice>(m_device), 1, &range);
        vkUnmapMemory(static_cast<VkDevice>(m_device), static_cast<VkDeviceMemory>(m_memory));
        detail::check(flushResult, "Flush buffer memory");
        return;
    }
    vkUnmapMemory(static_cast<VkDevice>(m_device), static_cast<VkDeviceMemory>(m_memory));
}

} // namespace sqd
