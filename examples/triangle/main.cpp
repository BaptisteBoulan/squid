#include <squid/squid.hpp>

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class Framebuffers {
public:
    Framebuffers(vk::Device device, const sqd::Swapchain& swapchain, vk::RenderPass renderPass)
        : m_device(device) {
        try {
            m_framebuffers.reserve(swapchain.imageViews().size());
            for (vk::ImageView imageView : swapchain.imageViews()) {
                const VkImageView attachment = static_cast<VkImageView>(imageView);
                VkFramebufferCreateInfo createInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
                createInfo.renderPass = static_cast<VkRenderPass>(renderPass);
                createInfo.attachmentCount = 1;
                createInfo.pAttachments = &attachment;
                createInfo.width = swapchain.extent().width;
                createInfo.height = swapchain.extent().height;
                createInfo.layers = 1;
                VkFramebuffer framebuffer = VK_NULL_HANDLE;
                const VkResult result = vkCreateFramebuffer(static_cast<VkDevice>(m_device), &createInfo, nullptr,
                                                            &framebuffer);
                if (result != VK_SUCCESS) throw std::runtime_error("Unable to create swapchain framebuffer");
                m_framebuffers.push_back(framebuffer);
            }
        } catch (...) {
            destroy();
            throw;
        }
    }

    ~Framebuffers() { destroy(); }
    Framebuffers(const Framebuffers&) = delete;
    Framebuffers& operator=(const Framebuffers&) = delete;

    VkFramebuffer operator[](size_t index) const { return m_framebuffers.at(index); }

private:
    void destroy() noexcept {
        for (VkFramebuffer framebuffer : m_framebuffers) {
            vkDestroyFramebuffer(static_cast<VkDevice>(m_device), framebuffer, nullptr);
        }
        m_framebuffers.clear();
    }

    vk::Device m_device{};
    std::vector<VkFramebuffer> m_framebuffers;
};

void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(operation) + " failed with VkResult " +
                                 std::to_string(static_cast<int>(result)));
    }
}

void run() {
    sqd::Window window(1280, 720, "Squid - Vulkan triangle");
    sqd::InstanceConfig instanceConfig;
    instanceConfig.applicationName = "Squid triangle";
    instanceConfig.extensions = sqd::Window::requiredInstanceExtensions();
    sqd::Instance instance(instanceConfig);
    sqd::Surface surface = window.createSurface(instance);
    sqd::Device device(instance, surface.handle());

    sqd::SwapchainConfig swapchainConfig;
    swapchainConfig.extent = window.framebufferSize();
    sqd::Swapchain swapchain(device, surface.handle(), swapchainConfig);
    sqd::RenderPass renderPass(device, {.colorFormat = swapchain.format()});
    sqd::Shader vertexShader(device, SQUID_TRIANGLE_VERTEX_SHADER);
    sqd::Shader fragmentShader(device, SQUID_TRIANGLE_FRAGMENT_SHADER);
    sqd::PipelineConfig pipelineConfig;
    pipelineConfig.renderPass = renderPass.handle();
    pipelineConfig.extent = swapchain.extent();
    pipelineConfig.cullMode = vk::CullModeFlagBits::eNone;
    sqd::Pipeline pipeline(device, vertexShader, fragmentShader, pipelineConfig);
    Framebuffers framebuffers(device.handle(), swapchain, renderPass.handle());
    sqd::CommandPool commandPool(device);
    std::vector<sqd::CommandBuffer> commandBuffers;
    commandBuffers.reserve(swapchain.images().size());

    for (size_t i = 0; i < swapchain.images().size(); ++i) {
        commandBuffers.push_back(commandPool.allocateBuffer());
        auto& commandBuffer = commandBuffers.back();
        commandBuffer.begin();
        VkClearValue clear{};
        clear.color = {{0.02f, 0.03f, 0.06f, 1.0f}};
        VkRenderPassBeginInfo renderBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderBegin.renderPass = static_cast<VkRenderPass>(renderPass.handle());
        renderBegin.framebuffer = framebuffers[i];
        renderBegin.renderArea = {{0, 0}, {swapchain.extent().width, swapchain.extent().height}};
        renderBegin.clearValueCount = 1;
        renderBegin.pClearValues = &clear;
        vkCmdBeginRenderPass(static_cast<VkCommandBuffer>(commandBuffer.handle()), &renderBegin,
                             VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(static_cast<VkCommandBuffer>(commandBuffer.handle()), VK_PIPELINE_BIND_POINT_GRAPHICS,
                          static_cast<VkPipeline>(pipeline.handle()));
        vkCmdDraw(static_cast<VkCommandBuffer>(commandBuffer.handle()), 3, 1, 0, 0);
        vkCmdEndRenderPass(static_cast<VkCommandBuffer>(commandBuffer.handle()));
        commandBuffer.end();
    }

    sqd::Semaphore imageAvailable(device);
    sqd::Semaphore renderFinished(device);
    sqd::Fence inFlight(device, true);
    while (!window.shouldClose()) {
        window.pollEvents();
        inFlight.wait();

        uint32_t imageIndex = 0;
        const VkResult acquire = vkAcquireNextImageKHR(static_cast<VkDevice>(device.handle()),
                                                       static_cast<VkSwapchainKHR>(swapchain.handle()), UINT64_MAX,
                                                       static_cast<VkSemaphore>(imageAvailable.handle()),
                                                       VK_NULL_HANDLE, &imageIndex);
        if (acquire == VK_ERROR_OUT_OF_DATE_KHR) break;
        if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) check(acquire, "Acquire swapchain image");

        inFlight.reset();
        const VkSemaphore waitSemaphore = static_cast<VkSemaphore>(imageAvailable.handle());
        const VkSemaphore signalSemaphore = static_cast<VkSemaphore>(renderFinished.handle());
        const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkCommandBuffer commandBuffer =
            static_cast<VkCommandBuffer>(commandBuffers.at(imageIndex).handle());
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &waitSemaphore;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commandBuffer;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &signalSemaphore;
        check(vkQueueSubmit(static_cast<VkQueue>(device.graphicsQueue()), 1, &submit,
                            static_cast<VkFence>(inFlight.handle())), "Submit triangle");

        const VkSwapchainKHR rawSwapchain = static_cast<VkSwapchainKHR>(swapchain.handle());
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &signalSemaphore;
        present.swapchainCount = 1;
        present.pSwapchains = &rawSwapchain;
        present.pImageIndices = &imageIndex;
        const VkResult presented = vkQueuePresentKHR(static_cast<VkQueue>(device.presentQueue()), &present);
        if (presented == VK_ERROR_OUT_OF_DATE_KHR) break;
        if (presented != VK_SUCCESS && presented != VK_SUBOPTIMAL_KHR) check(presented, "Present triangle");
        check(vkQueueWaitIdle(static_cast<VkQueue>(device.presentQueue())), "Wait for presentation queue");
    }
    check(vkDeviceWaitIdle(static_cast<VkDevice>(device.handle())), "Wait for device");
}

} // namespace

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "Triangle example failed: " << error.what() << '\n';
        return 1;
    }
}
