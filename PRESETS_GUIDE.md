# Squid Presets - Configuration Helpers

The Squid library provides preset configurations and helper functions to reduce boilerplate code when setting up common Vulkan patterns. These presets encode best practices and sensible defaults.

## Overview

The `Presets.hpp` header provides:
- **Raster State Presets** - Common polygon and culling modes
- **Blend State Presets** - Alpha blending configurations  
- **Depth/Stencil State Presets** - Depth testing modes
- **Sampler Presets** - Texture sampling configurations
- **Image Helpers** - Simplified depth and texture image creation
- **Descriptor Helpers** - Standard descriptor set layouts
- **Swapchain Presets** - Present mode configurations

## Usage

### 1. Raster State Presets

Control polygon rendering and culling:

```cpp
#include <squid/squid.hpp>

// Solid rendering with back-face culling (default)
auto raster = sqd::RasterState::solid();

// Wireframe rendering
auto wireframe = sqd::RasterState::wireframe();

// Billboard/double-sided rendering (no culling)
auto billboard = sqd::RasterState::billboard();

// Point rendering
auto points = sqd::RasterState::points();

// Apply to pipeline
sqd::PipelineConfig config{
    .polygonMode = raster.polygonMode,
    .cullMode = raster.cullMode,
    .frontFace = raster.frontFace,
    // ... other config ...
};
```

**Available Presets:**
- `solid()` - Standard filled rendering with back-face culling
- `wireframe()` - Edge rendering with back-face culling
- `billboard()` - Two-sided rendering (no culling)
- `doubleSided()` - Alias for billboard()
- `points()` - Point cloud rendering

### 2. Blend State Presets

Configure alpha blending modes:

```cpp
#include <squid/squid.hpp>

// Opaque surfaces (no blending)
auto opaque = sqd::BlendState::opaque();

// Transparent with per-pixel alpha
auto transparent = sqd::BlendState::transparent();

// Additive blending (glow effects)
auto additive = sqd::BlendState::additive();

// Premultiplied alpha blending
auto premult = sqd::BlendState::premultiplied();

// Apply to pipeline (you'll need to construct vk::PipelineColorBlendAttachmentState)
auto blendConfig = sqd::BlendState::transparent();
vk::PipelineColorBlendAttachmentState attachment{
    .blendEnable = blendConfig.blendEnable,
    .srcColorBlendFactor = blendConfig.srcColorBlendFactor,
    .dstColorBlendFactor = blendConfig.dstColorBlendFactor,
    .colorBlendOp = blendConfig.colorBlendOp,
    .srcAlphaBlendFactor = blendConfig.srcAlphaBlendFactor,
    .dstAlphaBlendFactor = blendConfig.dstAlphaBlendFactor,
    .alphaBlendOp = blendConfig.alphaBlendOp,
    .colorWriteMask = blendConfig.colorWriteMask,
};
```

**Available Presets:**
- `opaque()` - No blending (full replacement)
- `transparent()` - Alpha blending with src alpha
- `additive()` - Additive blend (accumulate light)
- `premultiplied()` - Premultiplied alpha blending

### 3. Depth/Stencil State Presets

Configure depth testing:

```cpp
#include <squid/squid.hpp>

// Standard depth testing with write (3D scenes)
auto depth = sqd::DepthState::enabled();

// Read-only depth test (UI overlays)
auto readOnly = sqd::DepthState::readOnly();

// No depth testing (billboards)
auto none = sqd::DepthState::disabled();

// Apply to pipeline
sqd::PipelineConfig config{
    .depthTest = depth.depthTest,
    .depthWrite = depth.depthWrite,
    // ... other config ...
};
```

**Available Presets:**
- `enabled()` - Full depth testing with write
- `readOnly()` - Depth test without writing
- `disabled()` - No depth testing
- `billboard()` - Alias for disabled()

### 4. Sampler Presets

Configure texture sampling:

```cpp
#include <squid/squid.hpp>

// Linear filtering, repeated UV wrapping (default)
auto linear = sqd::SamplerPresets::linear();

// Nearest filtering for pixel-perfect rendering
auto nearest = sqd::SamplerPresets::nearest();

// Linear with clamped edge addressing
auto clamped = sqd::SamplerPresets::clampedLinear();

// Use in texture image creation
auto imageConfig = sqd::createTextureImageConfig(
    vk::Extent3D{512, 512, 1},
    linear
);
sqd::Image texture(device, imageConfig);
```

**Available Presets:**
- `linear()` - Linear filtering, repeated wrapping
- `nearest()` - Nearest filtering, repeated wrapping
- `clampedLinear()` - Linear filtering, clamped to edge
- `clampedNearest()` - Nearest filtering, clamped to edge

### 5. Image Creation Helpers

Simplified image creation for common patterns:

```cpp
#include <squid/squid.hpp>

// Create depth attachment image
auto depthConfig = sqd::createDepthImageConfig(
    vk::Extent3D{1920, 1080, 1},
    vk::Format::eD32Sfloat
);
sqd::Image depthBuffer(device, depthConfig);

// Create texture image with custom sampler
auto texConfig = sqd::createTextureImageConfig(
    vk::Extent3D{512, 512, 1},
    sqd::SamplerPresets::clampedLinear()
);
sqd::Image texture(device, texConfig);
```

**Helper Functions:**
- `createDepthImageConfig()` - Depth/stencil attachment
- `createTextureImageConfig()` - Sampled texture

### 6. Descriptor Layout Helpers

Standard descriptor set layouts:

```cpp
#include <squid/squid.hpp>

// Single texture binding
auto textureBindings = sqd::DescriptorPresets::textureLayout();
sqd::DescriptorSetLayout layout(device, textureBindings);

// Single buffer binding
auto bufferBindings = sqd::DescriptorPresets::bufferLayout();
sqd::DescriptorSetLayout layout(device, bufferBindings);

// Combined texture + buffer (common for materials)
auto materialBindings = sqd::DescriptorPresets::textureAndBufferLayout();
sqd::DescriptorSetLayout layout(device, materialBindings);
```

**Available Layouts:**
- `textureLayout()` - Fragment-visible sampled image at binding 0
- `bufferLayout()` - Vertex/fragment-visible uniform buffer at binding 0
- `textureAndBufferLayout()` - Texture at binding 0, buffer at binding 1

### 7. Swapchain Presets

Configure present modes:

```cpp
#include <squid/squid.hpp>

// V-sync mode (guaranteed available, 60 FPS cap)
auto vsync = sqd::SwapchainPresets::vSync();

// No V-sync (mailbox mode, may tear)
auto noVSync = sqd::SwapchainPresets::noVSync();

// Immediate mode (highest latency)
auto immediate = sqd::SwapchainPresets::immediate();

// Apply to swapchain
sqd::SwapchainConfig config{
    .extent = {1920, 1080},
    .preferredPresentMode = vsync.presentMode,
};
sqd::Swapchain swapchain(device, surface, config);
```

**Available Presets:**
- `vSync()` - FIFO mode (default, recommended)
- `noVSync()` - Mailbox mode (high performance)
- `immediate()` - Immediate mode (highest latency)

## Complete Example

Here's a realistic example combining multiple presets:

```cpp
#include <squid/squid.hpp>
#include <memory>

int main() {
    // Create core objects
    sqd::Window window("My App", 1920, 1080);
    sqd::Instance instance(window);
    sqd::Device device(instance);
    
    // Setup swapchain with V-sync
    auto swapchainPreset = sqd::SwapchainPresets::vSync();
    sqd::Swapchain swapchain(device, instance.surface(), {
        .extent = {1920, 1080},
        .preferredPresentMode = swapchainPreset.presentMode,
    });
    
    // Create depth buffer using helper
    auto depthConfig = sqd::createDepthImageConfig({1920, 1080, 1});
    sqd::Image depthBuffer(device, depthConfig);
    
    // Create texture using helper with clamped sampler
    auto texConfig = sqd::createTextureImageConfig(
        {512, 512, 1},
        sqd::SamplerPresets::clampedLinear()
    );
    sqd::Image texture(device, texConfig);
    
    // Create render pass
    sqd::RenderPass renderPass(device, {
        .colorFormat = swapchain.format(),
        .enableDepth = true,
    });
    
    // Create descriptor layout using preset
    auto bindings = sqd::DescriptorPresets::textureAndBufferLayout();
    sqd::DescriptorSetLayout descriptorLayout(device, bindings);
    
    // Create pipeline with raster and blend presets
    auto rasterConfig = sqd::RasterState::solid();
    auto blendConfig = sqd::BlendState::transparent();
    auto depthConfig = sqd::DepthState::enabled();
    
    sqd::PipelineConfig pipelineConfig{
        .renderPass = renderPass,
        .extent = {1920, 1080},
        .polygonMode = rasterConfig.polygonMode,
        .cullMode = rasterConfig.cullMode,
        .frontFace = rasterConfig.frontFace,
        .depthTest = depthConfig.depthTest,
        .depthWrite = depthConfig.depthWrite,
        .descriptorSetLayouts = {descriptorLayout},
    };
    
    sqd::Shader vertShader(device, "shader.vert.spv");
    sqd::Shader fragShader(device, "shader.frag.spv");
    sqd::Pipeline pipeline(device, vertShader, fragShader, pipelineConfig);
    
    return 0;
}
```

## Best Practices

### Performance
- Presets are **zero-cost abstractions** - they're inline functions that resolve to constants at compile time
- No runtime overhead compared to manual configuration
- Use as much as possible for consistency and clarity

### Customization
If a preset doesn't match your needs, you can override individual fields:

```cpp
auto raster = sqd::RasterState::solid();
raster.lineWidth = 2.0f;  // Override individual field
```

Or create custom presets in your application:

```cpp
namespace MyPresets {
    inline sqd::RasterState::Config customRaster() {
        return {
            .polygonMode = vk::PolygonMode::eLine,
            .cullMode = vk::CullModeFlagBits::eFront,
            .frontFace = vk::FrontFace::eClockwise,
            .lineWidth = 1.5f,
        };
    }
}
```

### Documentation
Always document why you chose a specific preset:

```cpp
// Billboard quads don't have depth, use two-sided rendering
auto raster = sqd::RasterState::billboard();
auto depth = sqd::DepthState::disabled();

// Particle effects need additive blending to accumulate light
auto blend = sqd::BlendState::additive();
```

## Migration Guide

If you're updating existing code:

**Before:**
```cpp
vk::PipelineRasterizationStateCreateInfo raster(
    {},
    false,
    false,
    vk::PolygonMode::eFill,
    vk::CullModeFlagBits::eBack,
    vk::FrontFace::eCounterClockwise,
    false,
    0, 0, 0,
    1.0f);
```

**After:**
```cpp
auto raster = sqd::RasterState::solid();
// Use raster.polygonMode, raster.cullMode, etc.
```

**Before:**
```cpp
sqd::ImageConfig depth{
    .extent = vk::Extent3D(1920, 1080, 1),
    .format = vk::Format::eD32Sfloat,
    .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
    .aspect = vk::ImageAspectFlagBits::eDepth,
    .createSampler = false,
};
```

**After:**
```cpp
auto depth = sqd::createDepthImageConfig({1920, 1080, 1});
```

## See Also

- [Pipeline Documentation](./README.md#pipeline)
- [Image Documentation](./README.md#image)
- [Descriptor Documentation](./README.md#descriptors)
- [PipelineConfig Reference](./QUICK_REFERENCE.md#pipelineconfig)
