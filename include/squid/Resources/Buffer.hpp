#pragma once

#include <vulkan/vulkan.hpp>

#include <cstddef>

namespace sqd {

class Device;

struct BufferConfig {
    vk::DeviceSize size = 0;
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eVertexBuffer;
    vk::MemoryPropertyFlags memoryProperties =
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
};

class Buffer {
public:
    Buffer(const Device& device, const BufferConfig& config);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    vk::Buffer handle() const noexcept { return m_buffer; }
    operator vk::Buffer() const noexcept { return m_buffer; }
    vk::DeviceSize size() const noexcept { return m_size; }
    void upload(const void* data, size_t size, size_t offset = 0);

private:
    void destroy() noexcept;
    vk::Device m_device{};
    vk::Buffer m_buffer{};
    vk::DeviceMemory m_memory{};
    vk::DeviceSize m_size = 0;
    vk::MemoryPropertyFlags m_memoryProperties{};
};

} // namespace sqd
