# Squid - Quick Reference

## Installation (Choose One)

### FetchContent
```cmake
include(FetchContent)
FetchContent_Declare(squid GIT_REPOSITORY https://github.com/BaptisteBoulan/squid.git GIT_TAG main)
FetchContent_MakeAvailable(squid)
target_link_libraries(YourApp PRIVATE squid::squid)
```

### add_subdirectory
```cmake
add_subdirectory(squid)
target_link_libraries(YourApp PRIVATE squid::squid)
```

## Core Classes

| Class | Purpose | Key Methods |
|-------|---------|------------|
| `Window` | GLFW window | `shouldClose()`, `pollEvents()`, `framebufferSize()` |
| `Instance` | Vulkan instance | `handle()` |
| `Surface` | Render target | `handle()` |
| `Device` | GPU logical device | `handle()`, `physicalDevice()`, `graphicsQueue()` |
| `Swapchain` | Present images | `handle()`, `images()`, `format()`, `extent()` |
| `RenderPass` | Rendering operations | `handle()` |
| `Pipeline` | Graphics/compute pipeline | `handle()`, `layout()` |
| `Shader` | GPU shader modules | `handle()` |
| `Buffer` | GPU memory buffer | `handle()`, `size()`, `upload()` |
| `Image` | GPU texture/image | `handle()`, `view()`, `sampler()` |
| `CommandPool` | Allocate command buffers | `allocateBuffer()`, `immediateSubmit()` |
| `CommandBuffer` | Record GPU commands | `begin()`, `end()` |
| `Semaphore` | GPU-to-GPU sync | `handle()` |
| `Fence` | GPU-to-CPU sync | `handle()`, `wait()`, `reset()` |

## Basic Setup

```cpp
#include <squid/squid.hpp>

sqd::Window window(1280, 720, "My App");
sqd::InstanceConfig instanceConfig;
instanceConfig.extensions = sqd::Window::requiredInstanceExtensions();
sqd::Instance instance(instanceConfig);
sqd::Surface surface = window.createSurface(instance);
sqd::Device device(instance, surface.handle());

sqd::SwapchainConfig swapchainConfig;
swapchainConfig.extent = window.framebufferSize();
sqd::Swapchain swapchain(device, surface.handle(), swapchainConfig);
```

## Resource Creation

### Buffer
```cpp
sqd::BufferConfig config;
config.size = sizeof(data);
config.usage = vk::BufferUsageFlagBits::eVertexBuffer;
sqd::Buffer buffer(device, config);
buffer.upload(data, sizeof(data));
```

### Image
```cpp
sqd::ImageConfig config;
config.extent = {width, height, 1};
config.format = vk::Format::eR8G8B8A8Srgb;
sqd::Image image(device, config);
image.uploadRgba8(device, commandPool, rgbaPixels.data(), rgbaPixels.size());
```

### Shader

After adding Squid with `FetchContent` or `add_subdirectory`, call
`squid_compile_shader` in the same `CMakeLists.txt` that creates your app target:

```cmake
squid_compile_shader(YourApp src/shaders/vertex.glsl VERTEX_SHADER_PATH STAGE vert)
squid_compile_shader(YourApp src/shaders/fragment.glsl FRAGMENT_SHADER_PATH STAGE frag)
```

The helper uses `glslc` from the Vulkan SDK or `PATH` to compile GLSL to SPIR-V
when the app builds. It passes the generated file paths to C++ as string
macros. For filenames ending in `.vert` or `.frag`, omit `STAGE` and `glslc`
will infer it. Use distinct shader filenames within each target.

```cpp
sqd::Shader vertexShader(device, VERTEX_SHADER_PATH);
sqd::Shader fragmentShader(device, FRAGMENT_SHADER_PATH);
```

### Pipeline
```cpp
sqd::PipelineConfig config;
config.renderPass = renderPass.handle();
config.extent = swapchain.extent();
sqd::Pipeline pipeline(device, vertexShader, fragmentShader, config);
```

## Rendering Loop

```cpp
while (!window.shouldClose()) {
    window.pollEvents();
    
    // Acquire swapchain image
    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(...);
    
    // Record commands
    sqd::CommandBuffer cmd = commandPool.allocateBuffer();
    cmd.begin();
    // ... record vkCmd* calls ...
    cmd.end();
    
    // Submit and present
    vkQueueSubmit(...);
    vkQueuePresentKHR(...);
}
```

## Synchronization

```cpp
// Wait for GPU to finish
device.graphicsQueue().waitIdle();

// Or use fence
sqd::Fence fence(device);
fence.wait();
fence.reset();

// Or use semaphore for GPU-to-GPU sync
sqd::Semaphore semaphore(device);
```

## Error Handling

```cpp
try {
    sqd::Instance instance(config);
    // ... rest of your code ...
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
```

## Configuration Objects

### InstanceConfig
```cpp
sqd::InstanceConfig config;
config.applicationName = "My App";
config.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
config.apiVersion = VK_API_VERSION_1_0;
config.enableValidation = true;  // Auto-disabled in Release
config.extensions = {...};
config.validationLayers = {"VK_LAYER_KHRONOS_validation"};
```

### DeviceConfig
```cpp
sqd::DeviceConfig config;
config.extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
config.requiredExtensions = {...};
config.requireGraphicsQueue = true;
config.requirePresentQueue = true;
```

### SwapchainConfig
```cpp
sqd::SwapchainConfig config;
config.extent = {width, height};
config.preferredFormat = vk::Format::eB8G8R8A8Srgb;
config.preferredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
config.preferredPresentMode = vk::PresentModeKHR::eMailbox;
config.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
```

### PipelineConfig
```cpp
sqd::PipelineConfig config;
config.renderPass = renderPass.handle();
config.extent = {width, height};
config.topology = vk::PrimitiveTopology::eTriangleList;
config.polygonMode = vk::PolygonMode::eFill;
config.cullMode = vk::CullModeFlagBits::eBack;
config.frontFace = vk::FrontFace::eCounterClockwise;
config.depthTest = false;
config.depthWrite = false;
```

### BufferConfig
```cpp
sqd::BufferConfig config;
config.size = bytes;
config.usage = vk::BufferUsageFlagBits::eVertexBuffer;
config.memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible 
                        | vk::MemoryPropertyFlagBits::eHostCoherent;
```

### ImageConfig
```cpp
sqd::ImageConfig config;
config.extent = {width, height, 1};
config.format = vk::Format::eR8G8B8A8Srgb;
config.type = vk::ImageType::e2D;
config.tiling = vk::ImageTiling::eOptimal;
config.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
config.memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
config.aspect = vk::ImageAspectFlagBits::eColor;
config.createSampler = true;
config.filter = vk::Filter::eLinear;
config.addressMode = vk::SamplerAddressMode::eRepeat;
```

## Lifetime Rules

1. ✅ Keep `Window` alive longer than other resources
2. ✅ Keep `Instance` alive longer than `Device`, `Surface`
3. ✅ Keep `Device` alive longer than resources
4. ✅ Keep `CommandPool` alive longer than `CommandBuffer`s
5. ✅ Destroy swapchain-dependent resources before swapchain
6. ✅ Synchronize GPU before destroying resources

## Common CMake Options

```cmake
# Build examples
-DSQUID_BUILD_EXAMPLES=ON

# Release build
-DCMAKE_BUILD_TYPE=Release

# Specific compiler
-DCMAKE_CXX_COMPILER=clang++
```

## Implicit Conversions

All Squid wrappers support implicit conversion to their Vulkan-Hpp handles:

```cpp
sqd::Device device(instance, surface.handle());
vk::Device handle = device;  // Implicit conversion

// Or explicit:
vk::Device explicit_handle = device.handle();
```

## Building Examples

```bash
cmake .. -DSQUID_BUILD_EXAMPLES=ON
cmake --build .
./examples/triangle/squid_triangle
```

---

**See [README.md](./README.md) for complete documentation**
**See [SETUP_GUIDE.md](./SETUP_GUIDE.md) for detailed setup instructions**
