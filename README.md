# Squid - Windows Vulkan Library

Squid is a lightweight C++20 RAII helper library for Vulkan-Hpp and GLFW. Its
wrappers own their Vulkan resources, are movable but not copyable, and expose
both `handle()` and implicit conversions to the corresponding `vk::` handle.
Use the underlying Vulkan-Hpp API whenever you need more control.

## Requirements (Windows)

- **Windows 10 or newer**
- **CMake 3.20 or newer**
- **Vulkan SDK** installed from https://www.lunarg.com/vulkan-sdk/
- **C++20 compiler** (MSVC 2019 or newer)
- **GLFW 3.3+** (CMake fetches automatically, or install via vcpkg)

## Installation & Setup

**New to Squid?** Start with [**SETUP_GUIDE.md**](./SETUP_GUIDE.md) for complete step-by-step instructions.

## Quick Start

### Basic Usage Example

```cpp
#include <squid/squid.hpp>

int main() {
    // Create a window
    sqd::Window window(1280, 720, "Squid Application");
    
    // Setup Vulkan instance
    sqd::InstanceConfig instanceConfig;
    instanceConfig.extensions = sqd::Window::requiredInstanceExtensions();
    sqd::Instance instance(instanceConfig);
    
    // Create surface from window
    sqd::Surface surface = window.createSurface(instance);
    
    // Select physical device and create logical device
    sqd::Device device(instance, surface.handle());

    // Create swapchain
    sqd::SwapchainConfig swapchainConfig;
    swapchainConfig.extent = window.framebufferSize();
    sqd::Swapchain swapchain(device, surface.handle(), swapchainConfig);
    
    // Create command pool for recording commands
    sqd::CommandPool commands(device);
    
    return 0;
}
```

## API Overview

### Math

Include `<squid/Math.hpp>` for header-only `sqd::Vec3` and `sqd::Mat4`
without Vulkan headers or GLM. `Vec3` supports arithmetic, `dot`, `cross`,
`length`, and `normalize`. Normalizing a vector with non-finite length or a
length at or below the optional minimum (zero by default) throws
`std::invalid_argument`.

```cpp
const auto view = sqd::lookAt({0, 1.5f, -3}, {0, 1, 0}, {0, 1, 0});
const auto projection = sqd::perspectiveVulkan(0.9f, 16.0f / 9.0f, 0.1f, 100.0f);
const sqd::Mat4 viewProjection = projection * view;
```

`Mat4` defaults to zero; use `Mat4::identity()` for the identity matrix.
Index elements as `(row, column)` or use `data()` to upload the 16 column-major
floats to GLSL. The camera helpers use right-handed view space, with Vulkan
depth in `[0, 1]` and a Y flip for positive-height viewports.
To run the math tests, configure with `-DSQUID_BUILD_TESTS=ON`, build, and run
`ctest --test-dir build -C Debug`.

### Core Classes

#### Window
Creates and manages a GLFW window for Vulkan rendering.

```cpp
sqd::Window window(1280, 720, "My App");
bool shouldClose = window.shouldClose();
window.pollEvents();
vk::Extent2D size = window.framebufferSize();
```

#### Instance
Manages the Vulkan instance with optional validation layers.

```cpp
sqd::InstanceConfig config;
config.extensions = sqd::Window::requiredInstanceExtensions();
config.enableValidation = true; // Auto-disabled in Release builds
sqd::Instance instance(config);
```

Validation layers are enabled by default in non-`NDEBUG` builds and disabled in
release builds. If validation is requested but a configured layer is missing,
instance creation reports an error.

#### Surface
Represents the rendering target (window surface).

```cpp
sqd::Surface surface = window.createSurface(instance);
```

#### Device
Selects a physical device and creates a logical device.

```cpp
sqd::DeviceConfig config;
config.extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
sqd::Device device(instance, surface.handle(), config);
```

Device selects a physical device that supports the requested extensions and
queue families, scoring discrete GPUs highest. Its default extension is
`VK_KHR_swapchain`.

#### Swapchain
Manages the swapchain for presenting rendered images.

```cpp
sqd::SwapchainConfig config;
config.extent = window.framebufferSize();
config.preferredFormat = vk::Format::eB8G8R8A8Srgb;
sqd::Swapchain swapchain(device, surface.handle(), config);
```

### Pipeline Classes

#### Shader
Loads and manages SPIR-V shader modules.

```cpp
sqd::Shader vertexShader(device, "shader.vert.spv");
sqd::Shader fragmentShader(device, "shader.frag.spv");
```

#### RenderPass
Defines the rendering operations and attachments.

```cpp
sqd::RenderPassConfig config;
config.colorFormat = swapchain.format();
sqd::RenderPass renderPass(device, config);
```

#### Pipeline
Manages graphics or compute pipelines.

```cpp
sqd::PipelineConfig config;
config.renderPass = renderPass.handle();
config.extent = swapchain.extent();
sqd::Pipeline pipeline(device, vertexShader, fragmentShader, config);
```

### Resource Classes

#### Buffer
Manages GPU buffer memory.

```cpp
sqd::BufferConfig config;
config.size = sizeof(vertices);
config.usage = vk::BufferUsageFlagBits::eVertexBuffer;
sqd::Buffer buffer(device, config);
buffer.upload(vertices.data(), sizeof(vertices));
```

BufferConfig defaults to host-visible, coherent vertex-buffer memory.
`Buffer::upload` checks bounds and memory visibility.

#### Image
Manages GPU image memory with views and samplers. RGBA8 pixel data can be
uploaded through a host-visible staging buffer and transitioned for shader
sampling.

```cpp
sqd::ImageConfig config;
config.extent = {1024, 1024, 1};
config.format = vk::Format::eR8G8B8A8Srgb;
sqd::Image image(device, config);
std::vector<uint8_t> rgbaPixels(1024 * 1024 * 4);
image.uploadRgba8(device, commandPool, rgbaPixels.data(), rgbaPixels.size());
```

`uploadRgba8` accepts tightly packed RGBA8 data for 2D `R8G8B8A8` images and
requires sampled and transfer-destination usage (included by default). Squid
does not decode image files; load or decode them in your application first.
The image view and sampler are available through `view()` and `sampler()`, and
can be bound with a combined-image-sampler descriptor.

Images default to device-local sampled images with a view and sampler.
Set `ImageConfig` fields to match your image use case.

### Synchronization Classes

#### Semaphore
GPU-to-GPU synchronization primitive.

```cpp
sqd::Semaphore semaphore(device);
```

#### Fence
GPU-to-CPU synchronization primitive.

```cpp
sqd::Fence fence(device, true); // Start signaled
fence.wait();
fence.reset();
```

### Execution Classes

#### CommandPool
Allocates command buffers for recording Vulkan commands.

```cpp
sqd::CommandPool pool(device);
sqd::CommandBuffer buffer = pool.allocateBuffer();
buffer.begin();
// Record commands
buffer.end();
```

#### CommandBuffer
Records Vulkan commands for execution.

```cpp
buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
// vkCmd* calls
buffer.end();
```

## Resource and Lifetime Notes

### Memory Management

Squid currently allocates buffer and image memory directly; it does not depend
on VMA (Vulkan Memory Allocator). `BufferConfig` defaults to host-visible, 
coherent vertex-buffer memory, and `Buffer::upload` checks bounds and memory 
visibility.

Images default to device-local sampled images with a view and sampler. Set 
`ImageConfig` fields to match the image use case.

### Ownership & Lifetime

All wrappers that refer to a `Device`, `Instance`, `Window`, or command pool are
**non-owning** with respect to that parent. Keep parents alive longer than their
children. Destroy resources in this order:

1. Command buffers and submission results
2. Swapchain-dependent resources (framebuffers, pipelines, render passes)
3. Swapchain
4. Device and Surface
5. Instance
6. Window

Descriptor sets are owned by their descriptor pool and are invalidated
when that pool is destroyed.

### GPU Synchronization

Synchronize GPU use before destroying resources. Use fences to wait for GPU work
to complete:

```cpp
device.graphicsQueue().waitIdle();
// or
fence.wait();
```

## Advanced Usage

### Using Raw Vulkan-Hpp API

Squid wrappers expose their underlying Vulkan-Hpp handles via `handle()` and
implicit conversions. Use raw Vulkan-Hpp calls when you need functionality not
exposed by Squid:

```cpp
vk::Device device_handle = device.handle();
// or implicitly:
vkDeviceWaitIdle(device);
```

### Customizing Pipelines and RenderPasses

`RenderPass` and `Pipeline` provide straightforward defaults, with constructors
for both graphics and compute pipelines. For advanced features, use their `vk::`
handles with Vulkan-Hpp directly or extend the configuration for your use case.

## Triangle Example

The optional `examples/triangle` target compiles its GLSL shaders with `glslc`
from the Vulkan SDK and renders a triangle using the wrapper objects and raw
Vulkan calls where needed. Enable it with `SQUID_BUILD_EXAMPLES=ON`.

## Troubleshooting

### Cannot find Vulkan SDK

Ensure your Vulkan SDK is installed and the environment variable `VULKAN_SDK` 
is set:

```bash
# On Windows
set VULKAN_SDK=C:\VulkanSDK\<version>

# On Linux/Mac
export VULKAN_SDK=/path/to/VulkanSDK
```

### GLFW not found

If CMake cannot find GLFW, Squid will automatically fetch and build it.
To use a system-installed GLFW, ensure it's installed via your package manager
and that CMake can find it (e.g., via `glfw3-config.cmake`).

### Validation Errors

Validation is enabled by default in Debug builds. To disable it:

```cpp
sqd::InstanceConfig config;
config.enableValidation = false;
```

For Release builds, validation is automatically disabled.

## Contributing

Contributions are welcome! Please ensure:
- Code follows C++20 standards
- Changes are well-tested
- Documentation is updated
- Commit messages are descriptive
