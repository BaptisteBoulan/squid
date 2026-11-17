#pragma once

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <functional>

namespace squid {

class Device;

struct CommandPoolConfig {
    vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    uint32_t queueFamilyIndex = UINT32_MAX;
};

class CommandBuffer {
public:
    CommandBuffer() = default;
    ~CommandBuffer();
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&& other) noexcept;
    CommandBuffer& operator=(CommandBuffer&& other) noexcept;

    vk::CommandBuffer handle() const noexcept { return m_buffer; }
    operator vk::CommandBuffer() const noexcept { return m_buffer; }
    void begin(vk::CommandBufferUsageFlags flags = {});
    void end();

private:
    friend class CommandPool;
    CommandBuffer(vk::Device device, vk::CommandPool pool, vk::CommandBuffer buffer) noexcept;
    void destroy() noexcept;
    vk::Device m_device{};
    vk::CommandPool m_pool{};
    vk::CommandBuffer m_buffer{};
};

class CommandPool {
public:
    explicit CommandPool(const Device& device, const CommandPoolConfig& config = {});
    ~CommandPool();

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool(CommandPool&& other) noexcept;
    CommandPool& operator=(CommandPool&& other) noexcept;

    vk::CommandPool handle() const noexcept { return m_pool; }
    operator vk::CommandPool() const noexcept { return m_pool; }
    CommandBuffer allocateBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
    void immediateSubmit(const std::function<void(vk::CommandBuffer)>& recordFunc) const;

private:
    vk::Device m_device{};
    vk::Queue m_queue{};
    vk::CommandPool m_pool{};
};

} // namespace squid
