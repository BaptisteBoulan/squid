/**
 * \file presets_example.cpp
 * \brief Example demonstrating Squid presets for reducing boilerplate configuration.
 * 
 * This example shows how to use configuration presets and helper functions
 * to write cleaner, more maintainable Vulkan code with Squid.
 * 
 * Key features demonstrated:
 * - Raster state presets (solid, wireframe, billboard)
 * - Blend state presets (opaque, transparent, additive)
 * - Depth state presets (enabled, readOnly, disabled)
 * - Sampler presets for texture configuration
 * - Image creation helpers for depth and texture images
 * - Descriptor layout presets
 * - Swapchain present mode presets
 */

#include <squid/squid.hpp>
#include <iostream>

int main() {
    try {
        // Create core Vulkan objects
        sqd::Window window("Squid Presets Example", 1920, 1080);
        sqd::Instance instance(window);
        sqd::Device device(instance);

        // === SWAPCHAIN WITH V-SYNC PRESET ===
        // Before: manually set preferredPresentMode = eMailbox
        // After: use vSync() preset which defaults to eFifo
        auto swapchainPreset = sqd::SwapchainPresets::vSync();
        sqd::Swapchain swapchain(device, instance.surface(), {
            .extent = window.framebufferSize(),
            .preferredPresentMode = swapchainPreset.presentMode,
        });

        // === IMAGE HELPERS ===
        // Depth buffer - simplified with helper function
        auto depthConfig = sqd::createDepthImageConfig(
            vk::Extent3D{1920, 1080, 1},
            vk::Format::eD32Sfloat
        );
        sqd::Image depthBuffer(device, depthConfig);

        // Texture image with clamped sampler - simplified with helper
        auto textureConfig = sqd::createTextureImageConfig(
            vk::Extent3D{512, 512, 1},
            sqd::SamplerPresets::clampedLinear()
        );
        sqd::Image texture(device, textureConfig);

        // === DESCRIPTOR LAYOUT PRESETS ===
        // Standard material layout: texture + buffer
        auto materialBindings = sqd::DescriptorPresets::textureAndBufferLayout();
        sqd::DescriptorSetLayout materialLayout(device, materialBindings);

        // === RENDER PASS ===
        sqd::RenderPass renderPass(device, {
            .colorFormat = swapchain.format(),
            .enableDepth = true,
        });

        // === PIPELINE WITH STATE PRESETS ===
        // Raster state - use billboard (no culling) for the example
        auto rasterConfig = sqd::RasterState::billboard();

        // Blend state - transparent for general purpose
        auto blendConfig = sqd::BlendState::transparent();

        // Depth state - full depth testing for 3D
        auto depthStateConfig = sqd::DepthState::enabled();

        sqd::PipelineConfig pipelineConfig{
            .renderPass = renderPass,
            .extent = swapchain.extent(),
            .polygonMode = rasterConfig.polygonMode,
            .cullMode = rasterConfig.cullMode,
            .frontFace = rasterConfig.frontFace,
            .depthTest = depthStateConfig.depthTest,
            .depthWrite = depthStateConfig.depthWrite,
            .descriptorSetLayouts = {materialLayout},
        };

        // Load shaders (paths would be provided by CMake)
        // sqd::Shader vertShader(device, VERTEX_SHADER_PATH);
        // sqd::Shader fragShader(device, FRAGMENT_SHADER_PATH);
        // sqd::Pipeline pipeline(device, vertShader, fragShader, pipelineConfig);

        // === ALTERNATIVE CONFIGURATIONS ===
        // Wire-frame rendering for debugging
        if (false) {  // Set to true to use wireframe
            auto wireframeRaster = sqd::RasterState::wireframe();
            pipelineConfig.polygonMode = wireframeRaster.polygonMode;
            pipelineConfig.cullMode = wireframeRaster.cullMode;
        }

        // Additive blending for particle effects
        if (false) {  // Set to true for particles
            auto additiveBlend = sqd::BlendState::additive();
            // Apply to pipeline blend configuration
        }

        // No depth testing for UI
        if (false) {  // Set to true for UI
            auto noDepth = sqd::DepthState::disabled();
            pipelineConfig.depthTest = noDepth.depthTest;
            pipelineConfig.depthWrite = noDepth.depthWrite;
        }

        // Nearest sampling for pixel-perfect rendering
        if (false) {  // Set to true for pixel art
            auto pixelPerfect = sqd::SamplerPresets::clampedNearest();
            auto pixelArtTexture = sqd::createTextureImageConfig(
                vk::Extent3D{256, 256, 1},
                pixelPerfect
            );
            // sqd::Image uiTexture(device, pixelArtTexture);
        }

        std::cout << "✓ Presets example initialized successfully!" << std::endl;
        std::cout << "✓ Demonstrated:" << std::endl;
        std::cout << "  - Swapchain presets (V-sync)" << std::endl;
        std::cout << "  - Image creation helpers" << std::endl;
        std::cout << "  - Descriptor layout presets" << std::endl;
        std::cout << "  - Raster/Blend/Depth state presets" << std::endl;
        std::cout << "  - Sampler presets" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
