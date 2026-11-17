#include <squid/Core/Device.hpp>
#include <squid/Core/Swapchain.hpp>

#include "../VulkanUtils.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace squid {

Swapchain::Swapchain(const Device& device, vk::SurfaceKHR surface, const SwapchainConfig& config)
    : m_device(device.handle()) {
    const VkPhysicalDevice physicalDevice = static_cast<VkPhysicalDevice>(device.physicalDevice());
    const VkDevice logicalDevice = static_cast<VkDevice>(m_device);
    const VkSurfaceKHR rawSurface = static_cast<VkSurfaceKHR>(surface);

    VkSurfaceCapabilitiesKHR capabilities{};
    detail::check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, rawSurface, &capabilities),
                  "Query surface capabilities");

    uint32_t formatCount = 0;
    detail::check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, rawSurface, &formatCount, nullptr),
                  "Query surface formats");
    if (!formatCount) throw std::runtime_error("Surface has no supported formats");
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    detail::check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, rawSurface, &formatCount, formats.data()),
                  "Query surface formats");

    VkSurfaceFormatKHR chosenFormat = formats.front();
    const auto preferredFormat = static_cast<VkFormat>(config.preferredFormat);
    const auto preferredColor = static_cast<VkColorSpaceKHR>(config.preferredColorSpace);
    const auto formatIt = std::find_if(formats.begin(), formats.end(), [&](const auto& value) {
        return value.format == preferredFormat && value.colorSpace == preferredColor;
    });
    if (formatIt != formats.end()) chosenFormat = *formatIt;
    else if (chosenFormat.format == VK_FORMAT_UNDEFINED) chosenFormat.format = preferredFormat;
    m_format = vk::Format(chosenFormat.format);

    uint32_t modeCount = 0;
    detail::check(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, rawSurface, &modeCount, nullptr),
                  "Query present modes");
    if (!modeCount) throw std::runtime_error("Surface has no supported present modes");
    std::vector<VkPresentModeKHR> modes(modeCount);
    detail::check(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, rawSurface, &modeCount, modes.data()),
                  "Query present modes");
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    const auto wantedMode = static_cast<VkPresentModeKHR>(config.preferredPresentMode);
    if (std::find(modes.begin(), modes.end(), wantedMode) != modes.end()) presentMode = wantedMode;

    const VkImageUsageFlags requestedUsage = static_cast<VkImageUsageFlags>(config.imageUsage);
    if ((capabilities.supportedUsageFlags & requestedUsage) != requestedUsage) {
        throw std::runtime_error("Surface does not support the requested swapchain image usage");
    }
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        m_extent = vk::Extent2D(capabilities.currentExtent.width, capabilities.currentExtent.height);
    } else {
        if (!config.extent.width || !config.extent.height) {
            throw std::invalid_argument("Swapchain extent must be non-zero when the surface extent is variable");
        }
        m_extent.width = std::clamp(config.extent.width, capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
        m_extent.height = std::clamp(config.extent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount && imageCount > capabilities.maxImageCount) imageCount = capabilities.maxImageCount;
    uint32_t queueFamilies[] = {device.graphicsQueueFamily(), device.presentQueueFamily()};
    VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.surface = rawSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = {m_extent.width, m_extent.height};
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = requestedUsage;
    if (queueFamilies[0] != queueFamilies[1]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilies;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfo.preTransform = capabilities.currentTransform;
    const VkCompositeAlphaFlagBitsKHR alpha = (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
        : static_cast<VkCompositeAlphaFlagBitsKHR>(capabilities.supportedCompositeAlpha &
                                                    (~capabilities.supportedCompositeAlpha + 1));
    createInfo.compositeAlpha = alpha;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    detail::check(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain), "Create swapchain");
    m_swapchain = vk::SwapchainKHR(swapchain);
    try {
        uint32_t actualCount = 0;
        detail::check(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &actualCount, nullptr), "Get swapchain images");
        std::vector<VkImage> rawImages(actualCount);
        detail::check(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &actualCount, rawImages.data()),
                      "Get swapchain images");
        m_images.reserve(actualCount);
        m_imageViews.reserve(actualCount);
        for (VkImage image : rawImages) {
            m_images.emplace_back(image);
            VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = chosenFormat.format;
            viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            VkImageView imageView = VK_NULL_HANDLE;
            detail::check(vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView), "Create swapchain image view");
            m_imageViews.emplace_back(imageView);
        }
    } catch (...) {
        destroy();
        throw;
    }
}

Swapchain::~Swapchain() {
    destroy();
}

void Swapchain::destroy() noexcept {
    if (m_device) {
        for (vk::ImageView view : m_imageViews) {
            vkDestroyImageView(static_cast<VkDevice>(m_device), static_cast<VkImageView>(view), nullptr);
        }
        m_imageViews.clear();
        if (m_swapchain) {
            vkDestroySwapchainKHR(static_cast<VkDevice>(m_device), static_cast<VkSwapchainKHR>(m_swapchain), nullptr);
        }
    }
    m_swapchain = vk::SwapchainKHR{};
}

Swapchain::Swapchain(Swapchain&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_swapchain(std::exchange(other.m_swapchain, vk::SwapchainKHR{})),
      m_images(std::move(other.m_images)), m_imageViews(std::move(other.m_imageViews)),
      m_format(std::exchange(other.m_format, vk::Format::eUndefined)),
      m_extent(std::exchange(other.m_extent, vk::Extent2D{})) {}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, vk::Device{});
        m_swapchain = std::exchange(other.m_swapchain, vk::SwapchainKHR{});
        m_images = std::move(other.m_images);
        m_imageViews = std::move(other.m_imageViews);
        m_format = std::exchange(other.m_format, vk::Format::eUndefined);
        m_extent = std::exchange(other.m_extent, vk::Extent2D{});
    }
    return *this;
}

} // namespace squid
