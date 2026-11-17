#pragma once

#include <vulkan/vulkan.hpp>

namespace squid {

class Device;

struct RenderPassConfig {
    vk::Format colorFormat = vk::Format::eB8G8R8A8Srgb;
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR;
    bool enableDepth = false;
    vk::Format depthFormat = vk::Format::eD32Sfloat;
};

class RenderPass {
public:
    explicit RenderPass(const Device& device, const RenderPassConfig& config = {});
    ~RenderPass();

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&& other) noexcept;
    RenderPass& operator=(RenderPass&& other) noexcept;

    vk::RenderPass handle() const noexcept { return m_renderPass; }
    operator vk::RenderPass() const noexcept { return m_renderPass; }

private:
    vk::Device m_device{};
    vk::RenderPass m_renderPass{};
};

} // namespace squid
