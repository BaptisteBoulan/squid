#include <squid/Core/Device.hpp>
#include <squid/Resources/Image.hpp>

#include "../VulkanUtils.hpp"
#include "Memory.hpp"

#include <stdexcept>
#include <utility>

namespace sqd {

Image::Image(const Device& device, const ImageConfig& config) : m_device(device.handle()) {
    if (!config.extent.width || !config.extent.height || !config.extent.depth) {
        throw std::invalid_argument("Image extent dimensions must be greater than zero");
    }
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = static_cast<VkImageType>(config.type);
    imageInfo.format = static_cast<VkFormat>(config.format);
    imageInfo.extent = {config.extent.width, config.extent.height, config.extent.depth};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = static_cast<VkImageTiling>(config.tiling);
    imageInfo.usage = static_cast<VkImageUsageFlags>(config.usage);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage image = VK_NULL_HANDLE;
    detail::check(vkCreateImage(static_cast<VkDevice>(m_device), &imageInfo, nullptr, &image), "Create image");
    m_image = vk::Image(image);

    try {
        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(static_cast<VkDevice>(m_device), image, &requirements);
        const uint32_t memoryType = detail::findMemoryType(
            static_cast<VkPhysicalDevice>(device.physicalDevice()), requirements.memoryTypeBits,
            static_cast<VkMemoryPropertyFlags>(config.memoryProperties));
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        detail::check(vkAllocateMemory(static_cast<VkDevice>(m_device), &allocation, nullptr, &memory),
                      "Allocate image memory");
        m_memory = vk::DeviceMemory(memory);
        detail::check(vkBindImageMemory(static_cast<VkDevice>(m_device), image, memory, 0), "Bind image memory");

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = image;
        viewInfo.viewType = static_cast<VkImageViewType>(config.type == vk::ImageType::e3D
                                                             ? vk::ImageViewType::e3D
                                                             : config.type == vk::ImageType::e1D
                                                                   ? vk::ImageViewType::e1D
                                                                   : vk::ImageViewType::e2D);
        viewInfo.format = static_cast<VkFormat>(config.format);
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                              VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange = {static_cast<VkImageAspectFlags>(config.aspect), 0, 1, 0, 1};
        VkImageView view = VK_NULL_HANDLE;
        detail::check(vkCreateImageView(static_cast<VkDevice>(m_device), &viewInfo, nullptr, &view),
                      "Create image view");
        m_view = vk::ImageView(view);

        if (config.createSampler) {
            VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            samplerInfo.magFilter = static_cast<VkFilter>(config.filter);
            samplerInfo.minFilter = static_cast<VkFilter>(config.filter);
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(config.addressMode);
            samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(config.addressMode);
            samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(config.addressMode);
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.maxLod = 0.0f;
            VkSampler sampler = VK_NULL_HANDLE;
            detail::check(vkCreateSampler(static_cast<VkDevice>(m_device), &samplerInfo, nullptr, &sampler),
                          "Create image sampler");
            m_sampler = vk::Sampler(sampler);
        }
    } catch (...) {
        destroy();
        throw;
    }
}

Image::~Image() {
    destroy();
}

void Image::destroy() noexcept {
    if (m_device) {
        if (m_sampler) vkDestroySampler(static_cast<VkDevice>(m_device), static_cast<VkSampler>(m_sampler), nullptr);
        if (m_view) vkDestroyImageView(static_cast<VkDevice>(m_device), static_cast<VkImageView>(m_view), nullptr);
        if (m_image) vkDestroyImage(static_cast<VkDevice>(m_device), static_cast<VkImage>(m_image), nullptr);
        if (m_memory) vkFreeMemory(static_cast<VkDevice>(m_device), static_cast<VkDeviceMemory>(m_memory), nullptr);
    }
    m_sampler = vk::Sampler{};
    m_view = vk::ImageView{};
    m_image = vk::Image{};
    m_memory = vk::DeviceMemory{};
}

Image::Image(Image&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})), m_image(std::exchange(other.m_image, vk::Image{})),
      m_memory(std::exchange(other.m_memory, vk::DeviceMemory{})), m_view(std::exchange(other.m_view, vk::ImageView{})),
      m_sampler(std::exchange(other.m_sampler, vk::Sampler{})) {}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, vk::Device{});
        m_image = std::exchange(other.m_image, vk::Image{});
        m_memory = std::exchange(other.m_memory, vk::DeviceMemory{});
        m_view = std::exchange(other.m_view, vk::ImageView{});
        m_sampler = std::exchange(other.m_sampler, vk::Sampler{});
    }
    return *this;
}

} // namespace sqd
