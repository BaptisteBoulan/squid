#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace sqd {

class Device;
class Shader;

struct PipelineConfig {
    vk::RenderPass renderPass{};
    vk::Extent2D extent{};
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
    vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    bool depthTest = false;
    bool depthWrite = false;
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
};

struct ComputePipelineConfig {
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::PushConstantRange> pushConstantRanges;
};

class Pipeline {
public:
    Pipeline(const Device& device, const Shader& vertexShader, const Shader& fragmentShader,
             const PipelineConfig& config);
    Pipeline(const Device& device, const Shader& computeShader, const ComputePipelineConfig& config = {});
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&& other) noexcept;
    Pipeline& operator=(Pipeline&& other) noexcept;

    vk::Pipeline handle() const noexcept { return m_pipeline; }
    operator vk::Pipeline() const noexcept { return m_pipeline; }
    vk::PipelineLayout layout() const noexcept { return m_layout; }

private:
    void destroy() noexcept;
    vk::Device m_device{};
    vk::Pipeline m_pipeline{};
    vk::PipelineLayout m_layout{};
};

} // namespace sqd
