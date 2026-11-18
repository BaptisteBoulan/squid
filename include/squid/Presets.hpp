#pragma once

#include <vulkan/vulkan.hpp>

#include "Resources/Image.hpp"
#include "Pipeline/Pipeline.hpp"
#include "Resources/Descriptor.hpp"

namespace sqd {

class Device;
class CommandPool;
struct ImageConfig;

// ============================================================================
// RASTER STATE PRESETS
// ============================================================================

namespace RasterState {
    struct Config {
        vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
        vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
        vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
        float lineWidth = 1.0f;
    };

    inline Config solid() {
        return {
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .lineWidth = 1.0f,
        };
    }

    inline Config wireframe() {
        return {
            .polygonMode = vk::PolygonMode::eLine,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .lineWidth = 1.0f,
        };
    }

    inline Config billboard() {
        return {
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eNone,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .lineWidth = 1.0f,
        };
    }

    inline Config doubleSided() {
        return billboard();
    }

    inline Config points() {
        return {
            .polygonMode = vk::PolygonMode::ePoint,
            .cullMode = vk::CullModeFlagBits::eNone,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .lineWidth = 1.0f,
        };
    }
}

// ============================================================================
// BLEND STATE PRESETS
// ============================================================================

namespace BlendState {
    struct Config {
        bool blendEnable = false;
        vk::BlendFactor srcColorBlendFactor = vk::BlendFactor::eOne;
        vk::BlendFactor dstColorBlendFactor = vk::BlendFactor::eZero;
        vk::BlendOp colorBlendOp = vk::BlendOp::eAdd;
        vk::BlendFactor srcAlphaBlendFactor = vk::BlendFactor::eOne;
        vk::BlendFactor dstAlphaBlendFactor = vk::BlendFactor::eZero;
        vk::BlendOp alphaBlendOp = vk::BlendOp::eAdd;
        vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    };

    inline Config opaque() {
        return {
            .blendEnable = false,
            .srcColorBlendFactor = vk::BlendFactor::eOne,
            .dstColorBlendFactor = vk::BlendFactor::eZero,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
            .dstAlphaBlendFactor = vk::BlendFactor::eZero,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
    }

    inline Config transparent() {
        return {
            .blendEnable = true,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
            .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
    }

    inline Config additive() {
        return {
            .blendEnable = true,
            .srcColorBlendFactor = vk::BlendFactor::eOne,
            .dstColorBlendFactor = vk::BlendFactor::eOne,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
            .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
    }

    inline Config premultiplied() {
        return {
            .blendEnable = true,
            .srcColorBlendFactor = vk::BlendFactor::eOne,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
            .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
    }
}

// ============================================================================
// DEPTH/STENCIL STATE PRESETS
// ============================================================================

namespace DepthState {
    struct Config {
        bool depthTest = true;
        bool depthWrite = true;
        vk::CompareOp depthCompareOp = vk::CompareOp::eLess;
    };

    inline Config enabled() {
        return {
            .depthTest = true,
            .depthWrite = true,
            .depthCompareOp = vk::CompareOp::eLess,
        };
    }

    inline Config readOnly() {
        return {
            .depthTest = true,
            .depthWrite = false,
            .depthCompareOp = vk::CompareOp::eLess,
        };
    }

    inline Config disabled() {
        return {
            .depthTest = false,
            .depthWrite = false,
            .depthCompareOp = vk::CompareOp::eLess,
        };
    }

    inline Config billboard() {
        return disabled();
    }
}

// ============================================================================
// SAMPLER PRESETS
// ============================================================================

namespace SamplerPresets {
    struct Config {
        vk::Filter filter = vk::Filter::eLinear;
        vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat;
    };

    inline Config linear() {
        return {
            .filter = vk::Filter::eLinear,
            .addressMode = vk::SamplerAddressMode::eRepeat,
        };
    }

    inline Config nearest() {
        return {
            .filter = vk::Filter::eNearest,
            .addressMode = vk::SamplerAddressMode::eRepeat,
        };
    }

    inline Config clampedLinear() {
        return {
            .filter = vk::Filter::eLinear,
            .addressMode = vk::SamplerAddressMode::eClampToEdge,
        };
    }

    inline Config clampedNearest() {
        return {
            .filter = vk::Filter::eNearest,
            .addressMode = vk::SamplerAddressMode::eClampToEdge,
        };
    }
}

// ============================================================================
// IMAGE CREATION HELPERS
// ============================================================================

inline ImageConfig createDepthImageConfig(vk::Extent3D extent, vk::Format depthFormat = vk::Format::eD32Sfloat) {
    return {
        .extent = extent,
        .format = depthFormat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect = vk::ImageAspectFlagBits::eDepth,
        .createSampler = false,
    };
}

inline ImageConfig createTextureImageConfig(vk::Extent3D extent,
                                            const SamplerPresets::Config& sampler = SamplerPresets::linear()) {
    return {
        .extent = extent,
        .format = vk::Format::eR8G8B8A8Srgb,
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        .aspect = vk::ImageAspectFlagBits::eColor,
        .createSampler = true,
        .filter = sampler.filter,
        .addressMode = sampler.addressMode,
    };
}

// ============================================================================
// DESCRIPTOR LAYOUT HELPERS
// ============================================================================

namespace DescriptorPresets {
    // Standard texture binding (binding 0, sampled image in fragment shader)
    inline std::vector<vk::DescriptorSetLayoutBinding> textureLayout() {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        vk::DescriptorSetLayoutBinding binding;
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        bindings.push_back(binding);
        return bindings;
    }

    // Standard buffer binding (binding 0, uniform buffer in vertex/fragment)
    inline std::vector<vk::DescriptorSetLayoutBinding> bufferLayout(
        vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment) {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        vk::DescriptorSetLayoutBinding binding;
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eUniformBuffer;
        binding.descriptorCount = 1;
        binding.stageFlags = stages;
        bindings.push_back(binding);
        return bindings;
    }

    // Texture + Buffer (common for materials)
    inline std::vector<vk::DescriptorSetLayoutBinding> textureAndBufferLayout() {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        
        vk::DescriptorSetLayoutBinding textureBinding;
        textureBinding.binding = 0;
        textureBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        textureBinding.descriptorCount = 1;
        textureBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        bindings.push_back(textureBinding);
        
        vk::DescriptorSetLayoutBinding bufferBinding;
        bufferBinding.binding = 1;
        bufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        bufferBinding.descriptorCount = 1;
        bufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        bindings.push_back(bufferBinding);
        
        return bindings;
    }
}

// ============================================================================
// SWAPCHAIN PRESETS
// ============================================================================

namespace SwapchainPresets {
    // Standard V-sync mode (FIFO - guaranteed to be available)
    struct Config {
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    };

    inline Config vSync() {
        return {.presentMode = vk::PresentModeKHR::eFifo};
    }

    inline Config noVSync() {
        return {.presentMode = vk::PresentModeKHR::eMailbox};
    }

    inline Config immediate() {
        return {.presentMode = vk::PresentModeKHR::eImmediate};
    }
}

} // namespace sqd
