#pragma once

#include <vulkan/vulkan.hpp>

namespace squid {

class Device;

class Semaphore {
public:
    explicit Semaphore(const Device& device);
    ~Semaphore();
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore(Semaphore&& other) noexcept;
    Semaphore& operator=(Semaphore&& other) noexcept;

    vk::Semaphore handle() const noexcept { return m_semaphore; }
    operator vk::Semaphore() const noexcept { return m_semaphore; }

private:
    vk::Device m_device{};
    vk::Semaphore m_semaphore{};
};

class Fence {
public:
    explicit Fence(const Device& device, bool signaled = false);
    ~Fence();
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
    Fence(Fence&& other) noexcept;
    Fence& operator=(Fence&& other) noexcept;

    vk::Fence handle() const noexcept { return m_fence; }
    operator vk::Fence() const noexcept { return m_fence; }
    void wait(uint64_t timeout = UINT64_MAX) const;
    void reset() const;

private:
    vk::Device m_device{};
    vk::Fence m_fence{};
};

} // namespace squid
