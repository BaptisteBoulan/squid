#include <squid/Core/Device.hpp>
#include <squid/Execution/CommandPool.hpp>
#include <squid/Resources/Buffer.hpp>
#include <squid/Resources/Image.hpp>

#include "../VulkanUtils.hpp"
#include "Memory.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace sqd {

Image::Image(const Device& device, const ImageConfig& config)
    : m_device(device.handle()), m_extent(config.extent), m_format(config.format), m_type(config.type),
      m_usage(config.usage), m_aspect(config.aspect) {
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
    m_layout = vk::ImageLayout::eUndefined;
}

Image::Image(Image&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})), m_extent(std::exchange(other.m_extent, vk::Extent3D{})),
      m_format(std::exchange(other.m_format, vk::Format{})), m_type(std::exchange(other.m_type, vk::ImageType{})),
      m_usage(std::exchange(other.m_usage, vk::ImageUsageFlags{})),
      m_aspect(std::exchange(other.m_aspect, vk::ImageAspectFlags{})),
      m_layout(std::exchange(other.m_layout, vk::ImageLayout::eUndefined)),
      m_image(std::exchange(other.m_image, vk::Image{})),
      m_memory(std::exchange(other.m_memory, vk::DeviceMemory{})), m_view(std::exchange(other.m_view, vk::ImageView{})),
      m_sampler(std::exchange(other.m_sampler, vk::Sampler{})) {}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, vk::Device{});
        m_extent = std::exchange(other.m_extent, vk::Extent3D{});
        m_format = std::exchange(other.m_format, vk::Format{});
        m_type = std::exchange(other.m_type, vk::ImageType{});
        m_usage = std::exchange(other.m_usage, vk::ImageUsageFlags{});
        m_aspect = std::exchange(other.m_aspect, vk::ImageAspectFlags{});
        m_layout = std::exchange(other.m_layout, vk::ImageLayout::eUndefined);
        m_image = std::exchange(other.m_image, vk::Image{});
        m_memory = std::exchange(other.m_memory, vk::DeviceMemory{});
        m_view = std::exchange(other.m_view, vk::ImageView{});
        m_sampler = std::exchange(other.m_sampler, vk::Sampler{});
    }
    return *this;
}

void Image::uploadRgba8(const Device& device, const CommandPool& commandPool, const uint8_t* pixels, size_t size) {
    if (!pixels) throw std::invalid_argument("Texture pixel data cannot be null");
    if (device.handle() != m_device || commandPool.m_device != m_device) {
        throw std::invalid_argument("Texture, device, and command pool must use the same Vulkan device");
    }
    if (m_type != vk::ImageType::e2D || m_extent.depth != 1 || m_extent.width == 0 || m_extent.height == 0 ||
        m_aspect != vk::ImageAspectFlagBits::eColor) {
        throw std::invalid_argument("RGBA8 upload requires a non-empty 2D image");
    }
    if (m_format != vk::Format::eR8G8B8A8Unorm && m_format != vk::Format::eR8G8B8A8Srgb) {
        throw std::invalid_argument("RGBA8 upload requires an R8G8B8A8 image format");
    }
    if (!(m_usage & vk::ImageUsageFlagBits::eTransferDst) || !(m_usage & vk::ImageUsageFlagBits::eSampled)) {
        throw std::invalid_argument("RGBA8 upload requires transfer-destination and sampled image usage");
    }

    const vk::DeviceSize maxSize = std::numeric_limits<vk::DeviceSize>::max();
    if (m_extent.width > maxSize / 4 / m_extent.height) {
        throw std::overflow_error("Texture dimensions exceed the supported upload size");
    }
    const vk::DeviceSize requiredSize =
        static_cast<vk::DeviceSize>(m_extent.width) * m_extent.height * 4;
    if (requiredSize > std::numeric_limits<size_t>::max() || size != static_cast<size_t>(requiredSize)) {
        throw std::invalid_argument("RGBA8 pixel data size must equal width * height * 4");
    }

    BufferConfig stagingConfig;
    stagingConfig.size = requiredSize;
    stagingConfig.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingConfig.memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible;
    Buffer staging(device, stagingConfig);
    staging.upload(pixels, size);

    commandPool.immediateSubmit([&](vk::CommandBuffer commandBuffer) {
        VkImageMemoryBarrier toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toTransfer.srcAccessMask =
            m_layout == vk::ImageLayout::eUndefined ? 0 : VK_ACCESS_SHADER_READ_BIT;
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransfer.oldLayout = static_cast<VkImageLayout>(m_layout);
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = static_cast<VkImage>(m_image);
        toTransfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(static_cast<VkCommandBuffer>(commandBuffer),
                             m_layout == vk::ImageLayout::eUndefined ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                                      : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {m_extent.width, m_extent.height, 1};
        vkCmdCopyBufferToImage(static_cast<VkCommandBuffer>(commandBuffer),
                               static_cast<VkBuffer>(staging.handle()), static_cast<VkImage>(m_image),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        VkImageMemoryBarrier toShader{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        toShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toShader.image = static_cast<VkImage>(m_image);
        toShader.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(static_cast<VkCommandBuffer>(commandBuffer), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &toShader);
    });
    m_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

} // namespace sqd
