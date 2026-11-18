#pragma once

#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

namespace sqd {

class Instance;
class Surface;

class Window {
public:
    Window(uint32_t width = 1280, uint32_t height = 720, std::string title = "Squid");
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    bool shouldClose() const;
    void pollEvents() const;
    vk::Extent2D framebufferSize() const;
    GLFWwindow* nativeHandle() const noexcept { return m_window; }
    Surface createSurface(const Instance& instance) const;
    static std::vector<const char*> requiredInstanceExtensions();

private:
    GLFWwindow* m_window = nullptr;
};

} // namespace sqd
