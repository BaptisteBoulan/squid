#include <squid/Core/Device.hpp>
#include <squid/Pipeline/Pipeline.hpp>
#include <squid/Pipeline/Shader.hpp>

#include "../VulkanUtils.hpp"

#include <stdexcept>
#include <utility>

namespace squid {

Pipeline::Pipeline(const Device& device, const Shader& vertexShader, const Shader& fragmentShader,
                   const PipelineConfig& config)
    : m_device(device.handle()) {
    if (!config.renderPass) throw std::invalid_argument("Graphics pipeline requires a render pass");
    if (!config.extent.width || !config.extent.height) throw std::invalid_argument("Pipeline extent must be non-zero");

    std::vector<VkDescriptorSetLayout> setLayouts;
    setLayouts.reserve(config.descriptorSetLayouts.size());
    for (vk::DescriptorSetLayout layout : config.descriptorSetLayouts) {
        setLayouts.push_back(static_cast<VkDescriptorSetLayout>(layout));
    }
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    VkPipelineLayout layout = VK_NULL_HANDLE;
    detail::check(vkCreatePipelineLayout(static_cast<VkDevice>(m_device), &layoutInfo, nullptr, &layout),
                  "Create pipeline layout");
    m_layout = vk::PipelineLayout(layout);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = static_cast<VkShaderModule>(vertexShader.handle());
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = static_cast<VkShaderModule>(fragmentShader.handle());
    stages[1].pName = "main";

    std::vector<VkVertexInputBindingDescription> bindings;
    for (const auto& binding : config.vertexBindings) {
        bindings.push_back({binding.binding, binding.stride, static_cast<VkVertexInputRate>(binding.inputRate)});
    }
    std::vector<VkVertexInputAttributeDescription> attributes;
    for (const auto& attribute : config.vertexAttributes) {
        attributes.push_back({attribute.location, attribute.binding, static_cast<VkFormat>(attribute.format),
                              attribute.offset});
    }
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = static_cast<VkPrimitiveTopology>(config.topology);
    assembly.primitiveRestartEnable = VK_FALSE;
    VkViewport viewport{0.0f, 0.0f, static_cast<float>(config.extent.width), static_cast<float>(config.extent.height),
                        0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, {config.extent.width, config.extent.height}};
    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.rasterizerDiscardEnable = VK_FALSE;
    rasterization.polygonMode = static_cast<VkPolygonMode>(config.polygonMode);
    rasterization.cullMode = static_cast<VkCullModeFlags>(config.cullMode);
    rasterization.frontFace = static_cast<VkFrontFace>(config.frontFace);
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = static_cast<VkSampleCountFlagBits>(config.samples);

    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = config.depthTest ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = config.depthWrite ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;
    depth.depthBoundsTestEnable = VK_FALSE;
    depth.stencilTestEnable = VK_FALSE;
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blending.logicOpEnable = VK_FALSE;
    blending.attachmentCount = 1;
    blending.pAttachments = &blendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &assembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterization;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depth;
    pipelineInfo.pColorBlendState = &blending;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = static_cast<VkRenderPass>(config.renderPass);
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    const VkResult result = vkCreateGraphicsPipelines(static_cast<VkDevice>(m_device), VK_NULL_HANDLE, 1,
                                                      &pipelineInfo, nullptr, &pipeline);
    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(static_cast<VkDevice>(m_device), layout, nullptr);
        m_layout = vk::PipelineLayout{};
        detail::check(result, "Create graphics pipeline");
    }
    m_pipeline = vk::Pipeline(pipeline);
}

Pipeline::Pipeline(const Device& device, const Shader& computeShader, const ComputePipelineConfig& config)
    : m_device(device.handle()) {
    std::vector<VkDescriptorSetLayout> setLayouts;
    setLayouts.reserve(config.descriptorSetLayouts.size());
    for (vk::DescriptorSetLayout descriptorLayout : config.descriptorSetLayouts) {
        setLayouts.push_back(static_cast<VkDescriptorSetLayout>(descriptorLayout));
    }
    std::vector<VkPushConstantRange> pushConstantRanges;
    pushConstantRanges.reserve(config.pushConstantRanges.size());
    for (const auto& range : config.pushConstantRanges) {
        pushConstantRanges.push_back({static_cast<VkShaderStageFlags>(range.stageFlags), range.offset, range.size});
    }
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    layoutInfo.pPushConstantRanges = pushConstantRanges.data();
    VkPipelineLayout layout = VK_NULL_HANDLE;
    detail::check(vkCreatePipelineLayout(static_cast<VkDevice>(m_device), &layoutInfo, nullptr, &layout),
                  "Create compute pipeline layout");
    m_layout = vk::PipelineLayout(layout);

    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = static_cast<VkShaderModule>(computeShader.handle());
    stage.pName = "main";
    VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipelineInfo.stage = stage;
    pipelineInfo.layout = layout;
    VkPipeline pipeline = VK_NULL_HANDLE;
    const VkResult result = vkCreateComputePipelines(static_cast<VkDevice>(m_device), VK_NULL_HANDLE, 1,
                                                     &pipelineInfo, nullptr, &pipeline);
    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(static_cast<VkDevice>(m_device), layout, nullptr);
        m_layout = vk::PipelineLayout{};
        detail::check(result, "Create compute pipeline");
    }
    m_pipeline = vk::Pipeline(pipeline);
}

Pipeline::~Pipeline() {
    destroy();
}

void Pipeline::destroy() noexcept {
    if (m_device) {
        if (m_pipeline) vkDestroyPipeline(static_cast<VkDevice>(m_device), static_cast<VkPipeline>(m_pipeline), nullptr);
        if (m_layout) vkDestroyPipelineLayout(static_cast<VkDevice>(m_device),
                                              static_cast<VkPipelineLayout>(m_layout), nullptr);
    }
    m_pipeline = vk::Pipeline{};
    m_layout = vk::PipelineLayout{};
}

Pipeline::Pipeline(Pipeline&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_pipeline(std::exchange(other.m_pipeline, vk::Pipeline{})),
      m_layout(std::exchange(other.m_layout, vk::PipelineLayout{})) {}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, vk::Device{});
        m_pipeline = std::exchange(other.m_pipeline, vk::Pipeline{});
        m_layout = std::exchange(other.m_layout, vk::PipelineLayout{});
    }
    return *this;
}

} // namespace squid
