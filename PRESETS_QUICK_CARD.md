# Squid Presets - Quick Reference Card

## 🎨 Raster State Presets

```cpp
sqd::RasterState::solid()       // PolygonMode::eFill, CullMode::eBack
sqd::RasterState::wireframe()   // PolygonMode::eLine, CullMode::eBack
sqd::RasterState::billboard()   // PolygonMode::eFill, CullMode::eNone
sqd::RasterState::points()      // PolygonMode::ePoint, CullMode::eNone
```

**Common patterns:**
```cpp
auto raster = debug ? sqd::RasterState::wireframe() : sqd::RasterState::solid();
pipelineConfig.polygonMode = raster.polygonMode;
pipelineConfig.cullMode = raster.cullMode;
```

---

## 🎨 Blend State Presets

```cpp
sqd::BlendState::opaque()        // No blending
sqd::BlendState::transparent()   // Alpha blending
sqd::BlendState::additive()      // Light accumulation
sqd::BlendState::premultiplied() // Premultiplied alpha
```

---

## 🎨 Depth State Presets

```cpp
sqd::DepthState::enabled()     // Test + Write
sqd::DepthState::readOnly()    // Test only
sqd::DepthState::disabled()    // No depth testing
```

**Use cases:**
- `enabled()` - 3D scenes
- `readOnly()` - UI overlays over scene
- `disabled()` - Billboards, 2D overlays

---

## 🎨 Sampler Presets

```cpp
sqd::SamplerPresets::linear()         // Linear, Repeat
sqd::SamplerPresets::nearest()        // Nearest, Repeat
sqd::SamplerPresets::clampedLinear()  // Linear, ClampToEdge
sqd::SamplerPresets::clampedNearest() // Nearest, ClampToEdge
```

**Use in image creation:**
```cpp
auto texConfig = sqd::createTextureImageConfig(
    {512, 512, 1},
    sqd::SamplerPresets::clampedLinear()
);
```

---

## 🖼️ Image Creation Helpers

### Depth Image
```cpp
auto config = sqd::createDepthImageConfig(
    vk::Extent3D{width, height, 1},
    vk::Format::eD32Sfloat  // Optional, default shown
);
sqd::Image depth(device, config);
```

### Texture Image
```cpp
auto config = sqd::createTextureImageConfig(
    vk::Extent3D{width, height, 1},
    sqd::SamplerPresets::linear()  // Optional sampler
);
sqd::Image texture(device, config);
```

---

## 📌 Descriptor Layout Presets

```cpp
// Single texture
auto bindings = sqd::DescriptorPresets::textureLayout();
// Binding 0: CombinedImageSampler, Fragment

// Single buffer
auto bindings = sqd::DescriptorPresets::bufferLayout();
// Binding 0: UniformBuffer, Vertex+Fragment

// Texture + Buffer (material)
auto bindings = sqd::DescriptorPresets::textureAndBufferLayout();
// Binding 0: CombinedImageSampler, Fragment
// Binding 1: UniformBuffer, Vertex+Fragment
```

**Usage:**
```cpp
sqd::DescriptorSetLayout layout(device, bindings);
```

---

## 🔄 Swapchain Presets

```cpp
// V-sync (60 FPS cap) - RECOMMENDED
auto preset = sqd::SwapchainPresets::vSync();

// No V-sync (high performance)
auto preset = sqd::SwapchainPresets::noVSync();

// Immediate mode (highest latency)
auto preset = sqd::SwapchainPresets::immediate();
```

**Usage:**
```cpp
sqd::SwapchainConfig config{
    .extent = {1920, 1080},
    .preferredPresentMode = preset.presentMode,
};
sqd::Swapchain swapchain(device, surface, config);
```

---

## 🔄 Complete Pipeline Example

```cpp
// 1. Get presets
auto raster = sqd::RasterState::solid();
auto blend = sqd::BlendState::transparent();
auto depth = sqd::DepthState::enabled();

// 2. Create descriptor layout
auto descriptorBindings = sqd::DescriptorPresets::textureAndBufferLayout();
sqd::DescriptorSetLayout descriptorLayout(device, descriptorBindings);

// 3. Create pipeline config
sqd::PipelineConfig pipelineConfig{
    .renderPass = renderPass,
    .extent = {1920, 1080},
    .polygonMode = raster.polygonMode,
    .cullMode = raster.cullMode,
    .frontFace = raster.frontFace,
    .depthTest = depth.depthTest,
    .depthWrite = depth.depthWrite,
    .descriptorSetLayouts = {descriptorLayout},
};

// 4. Create pipeline
sqd::Pipeline pipeline(device, vertShader, fragShader, pipelineConfig);
```

---

## 🌳 Resource Creation Pattern

```cpp
// Depth buffer
auto depthConfig = sqd::createDepthImageConfig({1920, 1080, 1});
sqd::Image depthBuffer(device, depthConfig);

// Textures
auto texConfig = sqd::createTextureImageConfig(
    {512, 512, 1},
    sqd::SamplerPresets::clampedLinear()
);
sqd::Image texture(device, texConfig);

// Descriptors
auto bindings = sqd::DescriptorPresets::textureAndBufferLayout();
sqd::DescriptorSetLayout layout(device, bindings);

// Swapchain with V-sync
auto vsync = sqd::SwapchainPresets::vSync();
sqd::Swapchain swapchain(device, surface, {
    .extent = {1920, 1080},
    .preferredPresentMode = vsync.presentMode,
});
```

---

## 💡 Quick Tips

### Override Individual Fields
```cpp
auto raster = sqd::RasterState::solid();
raster.lineWidth = 2.0f;  // Custom override
```

### Conditional Presets
```cpp
auto raster = debug ? sqd::RasterState::wireframe() : sqd::RasterState::solid();
auto depth = billboard ? sqd::DepthState::disabled() : sqd::DepthState::enabled();
```

### Combining Presets
```cpp
// Billboard with no depth and transparent blending
auto raster = sqd::RasterState::billboard();
auto depth = sqd::DepthState::disabled();
auto blend = sqd::BlendState::transparent();
```

---

## 📊 Code Reduction Cheat Sheet

| Component | Before | After | Reduction |
|-----------|--------|-------|-----------|
| Raster | 11 params | 1 preset | 91% |
| Blend | 8 params | 1 preset | 87% |
| Depth Image | 7 params | 2 params | 71% |
| Texture | 9 params | 2 params | 78% |
| Descriptors | 20 lines | 2 lines | 90% |

---

## 🎯 Common Use Cases

### Opaque 3D Scene
```cpp
sqd::RasterState::solid()
sqd::BlendState::opaque()
sqd::DepthState::enabled()
```

### Transparent Objects
```cpp
sqd::RasterState::solid()
sqd::BlendState::transparent()
sqd::DepthState::readOnly()
```

### Particle Effects
```cpp
sqd::RasterState::billboard()
sqd::BlendState::additive()
sqd::DepthState::disabled()
```

### UI/2D
```cpp
sqd::RasterState::solid()
sqd::BlendState::transparent()
sqd::DepthState::disabled()
```

### Debug Wireframe
```cpp
sqd::RasterState::wireframe()
sqd::BlendState::opaque()
sqd::DepthState::enabled()
```

---

## 📚 Documentation Index

| Document | Purpose |
|----------|---------|
| **PRESETS_GUIDE.md** | Complete reference & examples |
| **PRESETS_BEFORE_AFTER.md** | Visual comparisons |
| **QUICK_REFERENCE.md** | API cheat sheet |
| **PRESETS_SUMMARY.md** | Overview & benefits |

---

## ⚡ Performance Notes

✅ **Zero Runtime Overhead**
- All presets are `inline` functions
- Compile to constants at compile time
- Identical performance to handwritten code

---

## 🔗 Getting Started

1. **Include presets:**
   ```cpp
   #include <squid/squid.hpp>  // Automatically includes Presets.hpp
   ```

2. **Use in your code:**
   ```cpp
   auto config = sqd::RasterState::solid();
   ```

3. **Read full guide:**
   See [PRESETS_GUIDE.md](./PRESETS_GUIDE.md)

---

**Keep this card nearby! Print it out! 🖨️**
