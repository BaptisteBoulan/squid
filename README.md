# Squid

Squid is a lightweight C++20 RAII helper library for Vulkan-Hpp and GLFW. Its
wrappers own their Vulkan resources, are movable but not copyable, and expose
both `handle()` and implicit conversions to the corresponding `vk::` handle.
Use the underlying Vulkan-Hpp API whenever you need more control.

## Requirements

- CMake 3.20 or newer
- A Vulkan SDK and a Vulkan-capable system
- C++20 compiler
- GLFW 3.3 or newer (CMake fetches GLFW when the parent project has not already
  provided a `glfw` or `glfw3::glfw` target)

## Installation & Setup

### Option 1: Using FetchContent (Recommended)

Add Squid to your CMakeLists.txt:

```cmake
include(FetchContent)
FetchContent_Declare(
    squid
    GIT_REPOSITORY https://github.com/BaptisteBoulan/squid.git
    GIT_TAG main
)
FetchContent_MakeAvailable(squid)

target_link_libraries(YourApp PRIVATE squid::squid)
```

### Option 2: Using add_subdirectory

Clone the repository into your project and add it as a subdirectory:

```cmake
add_subdirectory(squid)
target_link_libraries(YourApp PRIVATE squid::squid)
```

### Building Examples

The library includes an optional triangle example that demonstrates basic usage. 
Enable it with:

```cmake
set(SQUID_BUILD_EXAMPLES ON)
```

The example requires `glslc` from the Vulkan SDK to compile shaders.

## Quick Start

### Basic Usage Example

```cpp
#include <squid/squid.hpp>

int main() {
    // Create a window
    squid::Window window(1280, 720, "Squid Application");
    
    // Setup Vulkan instance
    squid::InstanceConfig instanceConfig;
    instanceConfig.extensions = squid::Window::requiredInstanceExtensions();
    squid::Instance instance(instanceConfig);
    
    // Create surface from window
    squid::Surface surface = window.createSurface(instance);
    
    // Select physical device and create logical device
    squid::Device device(instance, surface.handle());

    // Create swapchain
    squid::SwapchainConfig swapchainConfig;
    swapchainConfig.extent = window.framebufferSize();
    squid::Swapchain swapchain(device, surface.handle(), swapchainConfig);
    
    // Create command pool for recording commands
    squid::CommandPool commands(device);
    
    return 0;
}
```

## API Overview

### Core Classes

#### Window
Creates and manages a GLFW window for Vulkan rendering.

```cpp
squid::Window window(1280, 720, "My App");
bool shouldClose = window.shouldClose();
window.pollEvents();
vk::Extent2D size = window.framebufferSize();
```

#### Instance
Manages the Vulkan instance with optional validation layers.

```cpp
squid::InstanceConfig config;
config.extensions = squid::Window::requiredInstanceExtensions();
config.enableValidation = true; // Auto-disabled in Release builds
squid::Instance instance(config);
```

Validation layers are enabled by default in non-`NDEBUG` builds and disabled in
release builds. If validation is requested but a configured layer is missing,
instance creation reports an error.

#### Surface
Represents the rendering target (window surface).

```cpp
squid::Surface surface = window.createSurface(instance);
```

#### Device
Selects a physical device and creates a logical device.

```cpp
squid::DeviceConfig config;
config.extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
squid::Device device(instance, surface.handle(), config);
```

Device selects a physical device that supports the requested extensions and
queue families, scoring discrete GPUs highest. Its default extension is
`VK_KHR_swapchain`.

#### Swapchain
Manages the swapchain for presenting rendered images.

```cpp
squid::SwapchainConfig config;
config.extent = window.framebufferSize();
config.preferredFormat = vk::Format::eB8G8R8A8Srgb;
squid::Swapchain swapchain(device, surface.handle(), config);
```

### Pipeline Classes

#### Shader
Loads and manages SPIR-V shader modules.

```cpp
squid::Shader vertexShader(device, "shader.vert.spv");
squid::Shader fragmentShader(device, "shader.frag.spv");
```

#### RenderPass
Defines the rendering operations and attachments.

```cpp
squid::RenderPassConfig config;
config.colorFormat = swapchain.format();
squid::RenderPass renderPass(device, config);
```

#### Pipeline
Manages graphics or compute pipelines.

```cpp
squid::PipelineConfig config;
config.renderPass = renderPass.handle();
config.extent = swapchain.extent();
squid::Pipeline pipeline(device, vertexShader, fragmentShader, config);
```

### Resource Classes

#### Buffer
Manages GPU buffer memory.

```cpp
squid::BufferConfig config;
config.size = sizeof(vertices);
config.usage = vk::BufferUsageFlagBits::eVertexBuffer;
squid::Buffer buffer(device, config);
buffer.upload(vertices.data(), sizeof(vertices));
```

BufferConfig defaults to host-visible, coherent vertex-buffer memory.
`Buffer::upload` checks bounds and memory visibility.

#### Image
Manages GPU image memory with views and samplers.

```cpp
squid::ImageConfig config;
config.extent = {1024, 1024, 1};
config.format = vk::Format::eR8G8B8A8Srgb;
squid::Image image(device, config);
```

Images default to device-local sampled images with a view and sampler.
Set `ImageConfig` fields to match your image use case.

### Synchronization Classes

#### Semaphore
GPU-to-GPU synchronization primitive.

```cpp
squid::Semaphore semaphore(device);
```

#### Fence
GPU-to-CPU synchronization primitive.

```cpp
squid::Fence fence(device, true); // Start signaled
fence.wait();
fence.reset();
```

### Execution Classes

#### CommandPool
Allocates command buffers for recording Vulkan commands.

```cpp
squid::CommandPool pool(device);
squid::CommandBuffer buffer = pool.allocateBuffer();
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
squid::InstanceConfig config;
config.enableValidation = false;
```

For Release builds, validation is automatically disabled.

## Contributing

Contributions are welcome! Please ensure:
- Code follows C++20 standards
- Changes are well-tested
- Documentation is updated
- Commit messages are descriptive
