#include <squid/Core/Instance.hpp>
#include <squid/Core/Surface.hpp>
#include <squid/Core/Window.hpp>

#include <mutex>
#include <stdexcept>
#include <utility>

namespace sqd {
namespace {
std::mutex glfwMutex;
unsigned int windowCount = 0;

void release(GLFWwindow* window) noexcept {
    if (!window) {
        return;
    }
    glfwDestroyWindow(window);
    std::lock_guard<std::mutex> lock(glfwMutex);
    if (--windowCount == 0) {
        glfwTerminate();
    }
}
} // namespace

Window::Window(uint32_t width, uint32_t height, std::string title) {
    std::lock_guard<std::mutex> lock(glfwMutex);
    if (windowCount == 0 && glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("GLFW initialization failed");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
    if (!m_window) {
        if (windowCount == 0) {
            glfwTerminate();
        }
        throw std::runtime_error("GLFW window creation failed");
    }
    ++windowCount;
}

Window::~Window() {
    release(m_window);
}

Window::Window(Window&& other) noexcept : m_window(std::exchange(other.m_window, nullptr)) {}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        release(m_window);
        m_window = std::exchange(other.m_window, nullptr);
    }
    return *this;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window) == GLFW_TRUE;
}

void Window::pollEvents() const {
    glfwPollEvents();
}

vk::Extent2D Window::framebufferSize() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

Surface Window::createSurface(const Instance& instance) const {
    return Surface(*this, instance);
}

std::vector<const char*> Window::requiredInstanceExtensions() {
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (!extensions || count == 0) {
        throw std::runtime_error("GLFW did not provide Vulkan instance extensions");
    }
    return {extensions, extensions + count};
}

} // namespace sqd
