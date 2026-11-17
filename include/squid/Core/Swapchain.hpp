#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace squid {

class Device;

struct SwapchainConfig {
    vk::Extent2D extent{};
    vk::Format preferredFormat = vk::Format::eB8G8R8A8Srgb;
    vk::ColorSpaceKHR preferredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eMailbox;
    vk::ImageUsageFlags imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
};

class Swapchain {
public:
    Swapchain(const Device& device, vk::SurfaceKHR surface, const SwapchainConfig& config = {});
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;

    vk::SwapchainKHR handle() const noexcept { return m_swapchain; }
    operator vk::SwapchainKHR() const noexcept { return m_swapchain; }
    const std::vector<vk::Image>& images() const noexcept { return m_images; }
    const std::vector<vk::ImageView>& imageViews() const noexcept { return m_imageViews; }
    vk::Format format() const noexcept { return m_format; }
    vk::Extent2D extent() const noexcept { return m_extent; }

private:
    void destroy() noexcept;
    vk::Device m_device{};
    vk::SwapchainKHR m_swapchain{};
    std::vector<vk::Image> m_images;
    std::vector<vk::ImageView> m_imageViews;
    vk::Format m_format = vk::Format::eUndefined;
    vk::Extent2D m_extent{};
};

} // namespace squid
