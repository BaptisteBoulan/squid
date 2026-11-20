# Squid

Squid is a lightweight C++20 helper library for Vulkan on Windows. It wraps
Vulkan-Hpp and GLFW objects in small RAII classes so you can get a window, a
device and a swapchain on screen with a few lines, while keeping full access to
the raw Vulkan API whenever you need it.

- **RAII everywhere**: every wrapper owns its Vulkan object and destroys it
  automatically. Wrappers are movable, not copyable.
- **No lock-in**: every wrapper exposes `handle()` and converts implicitly to
  its `vk::` type, so you can mix Squid with plain Vulkan-Hpp calls.
- **Sensible defaults**: each class takes a `...Config` struct whose defaults
  cover the common case. Override only the fields you care about.
- **Batteries included**: texture upload, shader compilation from CMake,
  header-only math, and presets for common pipeline states.

Everything lives in the `sqd` namespace. Include `<squid/squid.hpp>` to get all
of it.

---

## Table of contents

- [Requirements](#requirements)
- [Setup](#setup)
- [Quick start](#quick-start)
- [Features](#features)
  - [Window, Instance and Surface](#window-instance-and-surface)
  - [Device](#device)
  - [Swapchain](#swapchain)
  - [Shaders](#shaders)
  - [Render pass and pipeline](#render-pass-and-pipeline)
  - [Buffers](#buffers)
  - [Images and textures](#images-and-textures)
  - [Descriptors](#descriptors)
  - [Command pools and buffers](#command-pools-and-buffers)
  - [Synchronization](#synchronization)
  - [Math](#math)
  - [Presets](#presets)
- [Lifetime rules](#lifetime-rules)
- [CMake options](#cmake-options)
- [Troubleshooting](#troubleshooting)
- [Version history](#version-history)

---

## Requirements

- Windows 10 or newer
- CMake 3.20 or newer
- [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) (sets `VULKAN_SDK` and
  provides `glslc`)
- A C++20 compiler (MSVC 2019 or newer)
- GLFW 3.3+ (optional: if CMake can't find it, Squid downloads GLFW 3.4
  automatically)

---

## Setup

### Option 1: FetchContent (recommended)

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyVulkanApp)

include(FetchContent)
FetchContent_Declare(
    squid
    GIT_REPOSITORY https://github.com/BaptisteBoulan/squid.git
    GIT_TAG main
)
FetchContent_MakeAvailable(squid)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE squid::squid)
```

### Option 2: add_subdirectory

Clone Squid into your project (or add it as a submodule):

```cmd
git submodule add https://github.com/BaptisteBoulan/squid.git vendor/squid
```

```cmake
add_subdirectory(vendor/squid)
target_link_libraries(my_app PRIVATE squid::squid)
```

Linking `squid::squid` brings in Vulkan, GLFW and C++20 transitively.

### Build

```cmd
cmake -B build
cmake --build build --config Debug
build\Debug\my_app.exe
```

---

## Quick start

```cpp
#include <squid/squid.hpp>
#include <iostream>

int main() {
    try {
        sqd::Window window(1280, 720, "My Vulkan App");

        sqd::InstanceConfig instanceConfig;
        instanceConfig.extensions = sqd::Window::requiredInstanceExtensions();
        sqd::Instance instance(instanceConfig);

        sqd::Surface surface = window.createSurface(instance);
        sqd::Device device(instance, surface);

        sqd::SwapchainConfig swapchainConfig;
        swapchainConfig.extent = window.framebufferSize();
        sqd::Swapchain swapchain(device, surface, swapchainConfig);

        sqd::CommandPool commandPool(device);

        while (!window.shouldClose()) {
            window.pollEvents();
            // acquire, record, submit, present...
        }

        device.graphicsQueue().waitIdle();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
```

For a complete render loop (acquire, record, submit, present), see
[examples/triangle/main.cpp](./examples/triangle/main.cpp).

---

## Features

All classes follow the same pattern: construct with a parent and an optional
config, use `handle()` (or the implicit conversion) when calling Vulkan
directly. Errors are reported by throwing exceptions.

### Window, Instance and Surface

`Window` opens a GLFW window set up for Vulkan (no OpenGL context).

```cpp
sqd::Window window(1280, 720, "My App");   // all arguments optional
window.shouldClose();
window.pollEvents();
vk::Extent2D size = window.framebufferSize();
GLFWwindow* raw = window.nativeHandle();   // for input, callbacks, etc.
```

`Instance` creates the Vulkan instance. Pass the extensions GLFW needs:

```cpp
sqd::InstanceConfig config;
config.extensions = sqd::Window::requiredInstanceExtensions();
config.applicationName = "My App";
sqd::Instance instance(config);
```

| `InstanceConfig` field | Default |
|---|---|
| `applicationName` | `"Squid application"` |
| `applicationVersion` | `VK_MAKE_API_VERSION(0, 1, 0, 0)` |
| `apiVersion` | `VK_API_VERSION_1_0` |
| `enableValidation` | `true` in Debug, `false` when `NDEBUG` is defined |
| `extensions` | empty |
| `validationLayers` | `{"VK_LAYER_KHRONOS_validation"}` |

If validation is enabled and a requested layer is missing, construction fails
with an error.

`Surface` is created from the window:

```cpp
sqd::Surface surface = window.createSurface(instance);
```

### Device

`Device` picks a physical device and creates the logical device plus its
graphics and present queues. GPUs are filtered by required extensions and queue
support, and discrete GPUs are preferred.

```cpp
sqd::Device device(instance, surface);
vk::Queue gfx = device.graphicsQueue();
uint32_t family = device.graphicsQueueFamily();
```

Also available: `physicalDevice()`, `presentQueue()`, `presentQueueFamily()`,
`memoryProperties()`.

| `DeviceConfig` field | Default |
|---|---|
| `extensions` | `{VK_KHR_SWAPCHAIN_EXTENSION_NAME}` |
| `requiredExtensions` | empty |
| `requireGraphicsQueue` | `true` |
| `requirePresentQueue` | `true` |

### Swapchain

```cpp
sqd::SwapchainConfig config;
config.extent = window.framebufferSize();
sqd::Swapchain swapchain(device, surface, config);

swapchain.images();       // std::vector<vk::Image>
swapchain.imageViews();   // std::vector<vk::ImageView>
swapchain.format();
swapchain.extent();
```

| `SwapchainConfig` field | Default |
|---|---|
| `extent` | `{0, 0}` (set this) |
| `preferredFormat` | `eB8G8R8A8Srgb` |
| `preferredColorSpace` | `eSrgbNonlinear` |
| `preferredPresentMode` | `eFifo` (V-sync, always supported) |
| `imageUsage` | `eColorAttachment` |

"Preferred" values fall back to what the surface supports.

### Shaders

`Shader` loads a compiled SPIR-V file:

```cpp
sqd::Shader vert(device, "shaders/triangle.vert.spv");
```

Squid also provides a CMake helper that compiles GLSL to SPIR-V with `glslc`
at build time and passes the output path to your code as a macro:

```cmake
squid_compile_shader(my_app src/shaders/mesh.vert MESH_VERT_PATH)
squid_compile_shader(my_app src/shaders/mesh.frag MESH_FRAG_PATH)
squid_compile_shader(my_app src/shaders/sky.glsl  SKY_FRAG_PATH STAGE frag)
```

```cpp
sqd::Shader vert(device, MESH_VERT_PATH);
sqd::Shader frag(device, MESH_FRAG_PATH);
```

- The stage is inferred from `.vert` / `.frag` / etc. extensions. Use
  `STAGE <stage>` for other names.
- Call it in the same `CMakeLists.txt` that defines the target.
- Use distinct shader filenames within one target.

### Render pass and pipeline

`RenderPass` creates a single-subpass render pass with one color attachment and
an optional depth attachment:

```cpp
sqd::RenderPassConfig rpConfig;
rpConfig.colorFormat = swapchain.format();
rpConfig.enableDepth = true;          // optional
sqd::RenderPass renderPass(device, rpConfig);
```

| `RenderPassConfig` field | Default |
|---|---|
| `colorFormat` | `eB8G8R8A8Srgb` |
| `loadOp` / `storeOp` | `eClear` / `eStore` |
| `initialLayout` / `finalLayout` | `eUndefined` / `ePresentSrcKHR` |
| `enableDepth` | `false` |
| `depthFormat` | `eD32Sfloat` |

`Pipeline` builds either a graphics pipeline (vertex + fragment shader) or a
compute pipeline, and owns its pipeline layout:

```cpp
sqd::PipelineConfig config;
config.renderPass = renderPass;
config.extent = swapchain.extent();
config.vertexBindings = {/* vk::VertexInputBindingDescription */};
config.vertexAttributes = {/* vk::VertexInputAttributeDescription */};
config.descriptorSetLayouts = {materialLayout};
sqd::Pipeline pipeline(device, vert, frag, config);

vk::PipelineLayout layout = pipeline.layout();
```

| `PipelineConfig` field | Default |
|---|---|
| `renderPass`, `extent` | none (set these) |
| `topology` | `eTriangleList` |
| `polygonMode` | `eFill` |
| `cullMode` | `eBack` |
| `frontFace` | `eCounterClockwise` |
| `samples` | `e1` |
| `depthTest` / `depthWrite` | `false` / `false` |
| `vertexBindings`, `vertexAttributes` | empty |
| `descriptorSetLayouts` | empty |

The viewport and scissor are baked in from `extent`, and color blending is
off. For anything beyond that, build the pipeline with Vulkan-Hpp directly.

Compute pipeline:

```cpp
sqd::ComputePipelineConfig cConfig;
cConfig.descriptorSetLayouts = {layout};
cConfig.pushConstantRanges = {{vk::ShaderStageFlagBits::eCompute, 0, 16}};
sqd::Pipeline compute(device, computeShader, cConfig);
```

### Buffers

```cpp
sqd::BufferConfig config;
config.size = sizeof(vertices);
config.usage = vk::BufferUsageFlagBits::eVertexBuffer;
sqd::Buffer buffer(device, config);
buffer.upload(vertices.data(), sizeof(vertices));   // optional 3rd arg: offset
```

Buffers default to host-visible, coherent memory so `upload()` works directly.
`upload()` checks bounds and that the memory is host-visible. Memory is
allocated directly (no VMA).

### Images and textures

`Image` creates an image, its memory, an image view and (optionally) a sampler.

```cpp
sqd::ImageConfig config;
config.extent = {width, height, 1};
sqd::Image texture(device, config);

texture.uploadRgba8(device, commandPool, pixels.data(), pixels.size());

vk::ImageView view = texture.view();
vk::Sampler sampler = texture.sampler();
```

`uploadRgba8` copies tightly packed RGBA8 pixels through a staging buffer and
leaves the image in a shader-readable layout, ready to bind as a combined image
sampler. It requires a 2D `R8G8B8A8` image with sampled and transfer-dst usage
(the defaults). Squid does not decode image files: load PNG/JPG with a library
such as stb_image first.

| `ImageConfig` field | Default |
|---|---|
| `extent` | `{1, 1, 1}` |
| `format` | `eR8G8B8A8Srgb` |
| `type` / `tiling` | `e2D` / `eOptimal` |
| `usage` | `eSampled \| eTransferDst` |
| `memoryProperties` | `eDeviceLocal` |
| `aspect` | `eColor` |
| `createSampler` | `true` |
| `filter` / `addressMode` | `eLinear` / `eRepeat` |

For depth buffers and textures, see the [image helpers](#image-helpers).

### Descriptors

```cpp
sqd::DescriptorSetLayout layout(device, sqd::DescriptorPresets::textureLayout());

sqd::DescriptorPoolConfig poolConfig;
poolConfig.sizes = {{vk::DescriptorType::eCombinedImageSampler, 1}};
poolConfig.maxSets = 1;
sqd::DescriptorPool pool(device, poolConfig);

sqd::DescriptorSet set(pool, layout);
```

Descriptor sets belong to their pool and become invalid when it is destroyed.
Write to them with `vk::WriteDescriptorSet` as usual.

### Command pools and buffers

```cpp
sqd::CommandPool pool(device);      // graphics queue family, resettable buffers

sqd::CommandBuffer cmd = pool.allocateBuffer();
cmd.begin();
// record vkCmd* calls on cmd.handle()
cmd.end();

// One-off work: records, submits and waits for completion.
pool.immediateSubmit([&](vk::CommandBuffer c) {
    // c.copyBuffer(...);
});
```

`CommandPoolConfig` lets you change `flags` (default
`eResetCommandBuffer`) and `queueFamilyIndex` (default: the device's graphics
family).

### Synchronization

```cpp
sqd::Semaphore imageAvailable(device);   // GPU to GPU
sqd::Fence inFlight(device, true);       // GPU to CPU, created signaled

inFlight.wait();                         // optional timeout in ns
inFlight.reset();
```

### Math

`<squid/Math.hpp>` is header-only and has no Vulkan or GLM dependency, so it
can be included on its own.

```cpp
sqd::Vec3 a{1, 0, 0}, b{0, 1, 0};
sqd::Vec3 c = sqd::cross(a, b);
float d = sqd::dot(a, b);
sqd::Vec3 n = sqd::normalize(a + b);

const auto view = sqd::lookAt({0, 1.5f, -3}, {0, 1, 0}, {0, 1, 0});
const auto proj = sqd::perspectiveVulkan(0.9f, 16.0f / 9.0f, 0.1f, 100.0f);
const sqd::Mat4 viewProj = proj * view;
```

- `Vec3`: `+ - * /`, `dot`, `cross`, `length`, `normalize`.
  `normalize` throws `std::invalid_argument` if the length is not finite or is
  at or below the optional minimum (default `0`).
- `Mat4`: zero by default, `Mat4::identity()`, `m(row, column)` indexing,
  `*` multiplication. `data()` returns 16 column-major floats ready to upload
  to GLSL.
- `lookAt` and `perspectiveVulkan` use right-handed view space, Vulkan depth
  range `[0, 1]`, and flip Y for positive-height viewports.

### Presets

`<squid/Presets.hpp>` provides ready-made values for common configurations.
They are plain inline functions returning small structs.

#### Pipeline states

```cpp
auto raster = sqd::RasterState::wireframe();
auto depth  = sqd::DepthState::enabled();

sqd::PipelineConfig config;
config.polygonMode = raster.polygonMode;
config.cullMode    = raster.cullMode;
config.frontFace   = raster.frontFace;
config.depthTest   = depth.depthTest;
config.depthWrite  = depth.depthWrite;
```

| Preset | Values |
|---|---|
| `RasterState::solid()` | fill, back-face culling |
| `RasterState::wireframe()` | line, back-face culling |
| `RasterState::billboard()` / `doubleSided()` | fill, no culling |
| `RasterState::points()` | point, no culling |
| `DepthState::enabled()` | test + write, `eLess` |
| `DepthState::readOnly()` | test, no write |
| `DepthState::disabled()` / `billboard()` | no test, no write |
| `BlendState::opaque()` | no blending |
| `BlendState::transparent()` | `srcAlpha, 1 - srcAlpha` |
| `BlendState::additive()` | `one, one` |
| `BlendState::premultiplied()` | `one, 1 - srcAlpha` |

`BlendState` presets mirror `vk::PipelineColorBlendAttachmentState` fields.
`PipelineConfig` has no blend settings yet, so use them when building a
pipeline with Vulkan-Hpp directly.

#### Samplers and swapchain

| Preset | Values |
|---|---|
| `SamplerPresets::linear()` | linear, repeat |
| `SamplerPresets::nearest()` | nearest, repeat |
| `SamplerPresets::clampedLinear()` | linear, clamp to edge |
| `SamplerPresets::clampedNearest()` | nearest, clamp to edge (pixel art) |
| `SwapchainPresets::vSync()` | FIFO |
| `SwapchainPresets::noVSync()` | Mailbox |
| `SwapchainPresets::immediate()` | Immediate (may tear) |

```cpp
sqd::SwapchainConfig config;
config.extent = window.framebufferSize();
config.preferredPresentMode = sqd::SwapchainPresets::noVSync().presentMode;
```

#### Image helpers

```cpp
// Depth buffer: D32 float, depth attachment, no sampler
sqd::Image depth(device, sqd::createDepthImageConfig({w, h, 1}));

// Texture: RGBA8 sRGB, sampled + transfer-dst, sampler from a preset
sqd::Image tex(device, sqd::createTextureImageConfig({512, 512, 1},
                                                     sqd::SamplerPresets::clampedNearest()));
```

`createDepthImageConfig` takes an optional second argument for the depth
format.

#### Descriptor layouts

| Preset | Bindings |
|---|---|
| `DescriptorPresets::textureLayout()` | 0: combined image sampler (fragment) |
| `DescriptorPresets::bufferLayout(stages)` | 0: uniform buffer (vertex + fragment by default) |
| `DescriptorPresets::textureAndBufferLayout()` | 0: combined image sampler (fragment), 1: uniform buffer (vertex + fragment) |

```cpp
sqd::DescriptorSetLayout layout(device, sqd::DescriptorPresets::textureAndBufferLayout());
```

---

## Lifetime rules

Wrappers do not keep their parents alive. Keep parents around longer than
their children, and destroy in this order:

1. Command buffers and in-flight work
2. Swapchain-dependent resources (framebuffers, pipelines, render passes)
3. Swapchain
4. Device and Surface
5. Instance
6. Window

Declaring objects in creation order in one scope does this for you, since C++
destroys them in reverse.

Always wait for the GPU before destroying resources it may still use:

```cpp
device.graphicsQueue().waitIdle();   // or fence.wait()
```

---

## CMake options

| Option | Default | Effect |
|---|---|---|
| `SQUID_BUILD_EXAMPLES` | `OFF` | Builds `examples/triangle` |
| `SQUID_BUILD_TESTS` | `OFF` | Builds the math tests |

```cmd
cmake -B build -DSQUID_BUILD_EXAMPLES=ON -DSQUID_BUILD_TESTS=ON
cmake --build build --config Debug
build\examples\triangle\Debug\squid_triangle.exe
ctest --test-dir build -C Debug
```

---

## Troubleshooting

**`vulkan/vulkan.hpp` not found.** The Vulkan SDK isn't visible to CMake.
Check `echo %VULKAN_SDK%` in a new terminal. If it's empty, reinstall the SDK
or set it manually (`set VULKAN_SDK=C:\VulkanSDK\<version>`), then delete the
`build` folder and reconfigure.

**`glslc` not found.** It ships with the Vulkan SDK in `%VULKAN_SDK%\Bin`.
Make sure `VULKAN_SDK` is set or `glslc` is on your `PATH`.

**GLFW not found.** Squid downloads it automatically when CMake can't find it.
If the download fails, delete the `build` folder and reconfigure, or install
it with `vcpkg install glfw3:x64-windows`.

**Validation layer errors at startup.** The Khronos validation layer comes with
the Vulkan SDK. To run without it, set `InstanceConfig::enableValidation =
false`. It is already off in Release builds.

---

## Version history

| Version | Changes |
|---|---|
| **1.6** | Documentation cleanup: all docs merged into a single README. |
| **1.5** | Presets: raster, blend, depth, sampler, swapchain and descriptor layout presets, plus `createDepthImageConfig` / `createTextureImageConfig`. Default swapchain present mode changed from Mailbox to FIFO. |
| **1.4** | Header-only math: `Vec3`, `Mat4`, `lookAt`, `perspectiveVulkan`, with tests. |
| **1.3** | Texture upload: `Image::uploadRgba8` (staging buffer + layout transition). |
| **1.2** | Namespace shortened to `sqd`. Added the `squid_compile_shader` CMake helper. |
| **1.1** | Documentation cleanup. |
| **1.0** | First release: Window, Instance, Surface, Device, Swapchain, Shader, RenderPass, Pipeline, Buffer, Image, Descriptors, CommandPool, Semaphore, Fence. |
