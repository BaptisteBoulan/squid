#include <squid/Core/Instance.hpp>
#include <squid/Core/Surface.hpp>
#include <squid/Core/Window.hpp>

#include "../VulkanUtils.hpp"

#include <utility>

namespace sqd {

Surface::Surface(const Window& window, const Instance& instance) : m_instance(instance.handle()) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    detail::check(glfwCreateWindowSurface(static_cast<VkInstance>(m_instance), window.nativeHandle(), nullptr, &surface),
                  "Create GLFW Vulkan surface");
    m_surface = vk::SurfaceKHR(surface);
}

Surface::~Surface() {
    if (m_surface) {
        vkDestroySurfaceKHR(static_cast<VkInstance>(m_instance), static_cast<VkSurfaceKHR>(m_surface), nullptr);
    }
}

Surface::Surface(Surface&& other) noexcept
    : m_instance(std::exchange(other.m_instance, vk::Instance{})),
      m_surface(std::exchange(other.m_surface, vk::SurfaceKHR{})) {}

Surface& Surface::operator=(Surface&& other) noexcept {
    if (this != &other) {
        if (m_surface) {
            vkDestroySurfaceKHR(static_cast<VkInstance>(m_instance), static_cast<VkSurfaceKHR>(m_surface), nullptr);
        }
        m_instance = std::exchange(other.m_instance, vk::Instance{});
        m_surface = std::exchange(other.m_surface, vk::SurfaceKHR{});
    }
    return *this;
}

} // namespace sqd
