#pragma once

#include <vulkan/vulkan.hpp>

namespace squid {

class Instance;
class Window;

class Surface {
public:
    Surface(const Window& window, const Instance& instance);
    ~Surface();

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;
    Surface(Surface&& other) noexcept;
    Surface& operator=(Surface&& other) noexcept;

    vk::SurfaceKHR handle() const noexcept { return m_surface; }
    operator vk::SurfaceKHR() const noexcept { return m_surface; }

private:
    vk::Instance m_instance{};
    vk::SurfaceKHR m_surface{};
};

} // namespace squid
