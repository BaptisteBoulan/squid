#include <squid/Core/Device.hpp>
#include <squid/Pipeline/RenderPass.hpp>

#include "../VulkanUtils.hpp"

#include <utility>
#include <vector>

namespace sqd {

RenderPass::RenderPass(const Device& device, const RenderPassConfig& config) : m_device(device.handle()) {
    std::vector<VkAttachmentDescription> attachments;
    VkAttachmentDescription color{};
    color.format = static_cast<VkFormat>(config.colorFormat);
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = static_cast<VkAttachmentLoadOp>(config.loadOp);
    color.storeOp = static_cast<VkAttachmentStoreOp>(config.storeOp);
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = static_cast<VkImageLayout>(config.initialLayout);
    color.finalLayout = static_cast<VkImageLayout>(config.finalLayout);
    attachments.push_back(color);

    VkAttachmentDescription depth{};
    if (config.enableDepth) {
        depth.format = static_cast<VkFormat>(config.depthFormat);
        depth.samples = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depth);
    }

    VkAttachmentReference colorReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthReference{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    if (config.enableDepth) subpass.pDepthStencilAttachment = &depthReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (config.enableDepth) {
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    detail::check(vkCreateRenderPass(static_cast<VkDevice>(m_device), &createInfo, nullptr, &renderPass),
                  "Create render pass");
    m_renderPass = vk::RenderPass(renderPass);
}

RenderPass::~RenderPass() {
    if (m_renderPass) vkDestroyRenderPass(static_cast<VkDevice>(m_device), static_cast<VkRenderPass>(m_renderPass), nullptr);
}

RenderPass::RenderPass(RenderPass&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_renderPass(std::exchange(other.m_renderPass, vk::RenderPass{})) {}

RenderPass& RenderPass::operator=(RenderPass&& other) noexcept {
    if (this != &other) {
        if (m_renderPass) vkDestroyRenderPass(static_cast<VkDevice>(m_device), static_cast<VkRenderPass>(m_renderPass), nullptr);
        m_device = std::exchange(other.m_device, vk::Device{});
        m_renderPass = std::exchange(other.m_renderPass, vk::RenderPass{});
    }
    return *this;
}

} // namespace sqd
