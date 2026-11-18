#pragma once

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>

namespace sqd {

class Device;
class CommandPool;

struct ImageConfig {
    vk::Extent3D extent{1, 1, 1};
    vk::Format format = vk::Format::eR8G8B8A8Srgb;
    vk::ImageType type = vk::ImageType::e2D;
    vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
    bool createSampler = true;
    vk::Filter filter = vk::Filter::eLinear;
    vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat;
};

class Image {
public:
    Image(const Device& device, const ImageConfig& config);
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    vk::Image handle() const noexcept { return m_image; }
    operator vk::Image() const noexcept { return m_image; }
    vk::ImageView view() const noexcept { return m_view; }
    vk::Sampler sampler() const noexcept { return m_sampler; }
    void uploadRgba8(const Device& device, const CommandPool& commandPool, const uint8_t* pixels, size_t size);

private:
    void destroy() noexcept;
    vk::Device m_device{};
    vk::Extent3D m_extent{};
    vk::Format m_format{};
    vk::ImageType m_type{};
    vk::ImageUsageFlags m_usage{};
    vk::ImageAspectFlags m_aspect{};
    vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;
    vk::Image m_image{};
    vk::DeviceMemory m_memory{};
    vk::ImageView m_view{};
    vk::Sampler m_sampler{};
};

} // namespace sqd
