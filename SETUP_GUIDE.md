# Squid - Complete Setup Guide for Users

This guide explains how to set up a new project using the Squid library.

## Prerequisites

Before you begin, ensure you have:

1. **CMake 3.20 or newer**
   ```bash
   cmake --version
   ```

2. **Vulkan SDK installed**
   - Download from https://www.lunarg.com/vulkan-sdk/
   - Set environment variable: `VULKAN_SDK`
     - **Windows**: `C:\VulkanSDK\<version>`
     - **Linux**: `/opt/vulkan/<version>`
     - **macOS**: `$HOME/VulkanSDK/<version>`

3. **C++20 compatible compiler**
   - **Windows**: MSVC 2019+, Clang 12+, MinGW G++ 10+
   - **Linux**: GCC 10+, Clang 12+
   - **macOS**: Clang 12+ (Xcode 12+)

4. **GLFW 3.3+ (optional)**
   - If not installed, CMake will automatically fetch it
   - To install system-wide:
     - **Windows (vcpkg)**: `vcpkg install glfw3:x64-windows`
     - **Linux (Ubuntu)**: `sudo apt-get install libglfw3-dev`
     - **Linux (Fedora)**: `sudo dnf install glfw-devel`
     - **macOS (Homebrew)**: `brew install glfw`

## Method 1: Using FetchContent (Recommended)

This is the simplest method for most projects.

### Step 1: Create Project Structure

```
my-vulkan-app/
├── CMakeLists.txt
└── src/
    └── main.cpp
```

### Step 2: Configure CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyVulkanApp)

# Fetch Squid library
include(FetchContent)
FetchContent_Declare(
    squid
    GIT_REPOSITORY https://github.com/BaptisteBoulan/squid.git
    GIT_TAG main
)
FetchContent_MakeAvailable(squid)

# Create your executable
add_executable(my_app src/main.cpp)

# Link against Squid
target_link_libraries(my_app PRIVATE squid::squid)

# Ensure C++20
target_compile_features(my_app PRIVATE cxx_std_20)
```

### Step 3: Write Your Application

```cpp
#include <squid/squid.hpp>
#include <iostream>

int main() {
    try {
        // Create window
        squid::Window window(1280, 720, "My Vulkan App");
        
        // Setup Vulkan instance
        squid::InstanceConfig instanceConfig;
        instanceConfig.extensions = squid::Window::requiredInstanceExtensions();
        instanceConfig.applicationName = "My Vulkan App";
        squid::Instance instance(instanceConfig);
        
        // Create rendering surface
        squid::Surface surface = window.createSurface(instance);
        
        // Select GPU and create logical device
        squid::Device device(instance, surface.handle());
        
        // Create swapchain for presenting to screen
        squid::SwapchainConfig swapchainConfig;
        swapchainConfig.extent = window.framebufferSize();
        squid::Swapchain swapchain(device, surface.handle(), swapchainConfig);
        
        // Create command pool for recording GPU commands
        squid::CommandPool commandPool(device);
        
        std::cout << "Vulkan application initialized successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

### Step 4: Build Your Project

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build .

# Run your application
./Debug/my_app  # or ./Debug/my_app.exe on Windows
```

## Method 2: Using add_subdirectory

This method gives you more control and is better for monorepo setups.

### Step 1: Clone Squid into Your Project

```bash
git clone https://github.com/BaptisteBoulan/squid.git vendor/squid
```

Or add it as a git submodule:

```bash
git submodule add https://github.com/BaptisteBoulan/squid.git vendor/squid
```

### Step 2: Configure CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyVulkanApp)

# Add Squid subdirectory
add_subdirectory(vendor/squid)

# Your application
add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE squid::squid)
target_compile_features(my_app PRIVATE cxx_std_20)
```

### Step 3-4: Same as Method 1

Write your code and build as described above.

## Common Patterns

### Creating a Basic Rendering Loop

```cpp
#include <squid/squid.hpp>

int main() {
    squid::Window window(1280, 720, "Render Loop Example");
    squid::InstanceConfig config;
    config.extensions = squid::Window::requiredInstanceExtensions();
    squid::Instance instance(config);
    squid::Surface surface = window.createSurface(instance);
    squid::Device device(instance, surface.handle());
    
    squid::SwapchainConfig swapchainConfig;
    swapchainConfig.extent = window.framebufferSize();
    squid::Swapchain swapchain(device, surface.handle(), swapchainConfig);
    
    squid::CommandPool commandPool(device);
    squid::RenderPass renderPass(device);
    
    // Main loop
    while (!window.shouldClose()) {
        window.pollEvents();
        
        // Record and submit commands here
        // Present to screen
    }
    
    return 0;
}
```

### Working with Buffers and Images

```cpp
// Create a vertex buffer
squid::BufferConfig bufferConfig;
bufferConfig.size = sizeof(vertices);
bufferConfig.usage = vk::BufferUsageFlagBits::eVertexBuffer;
squid::Buffer vertexBuffer(device, bufferConfig);
vertexBuffer.upload(vertices.data(), sizeof(vertices));

// Create a texture
squid::ImageConfig imageConfig;
imageConfig.extent = {textureWidth, textureHeight, 1};
imageConfig.format = vk::Format::eR8G8B8A8Srgb;
imageConfig.createSampler = true;
squid::Image texture(device, imageConfig);
```

### Setting Up Pipelines

```cpp
// Load shaders
squid::Shader vertexShader(device, "vertex.spv");
squid::Shader fragmentShader(device, "fragment.spv");

// Configure pipeline
squid::PipelineConfig pipelineConfig;
pipelineConfig.renderPass = renderPass.handle();
pipelineConfig.extent = swapchain.extent();
pipelineConfig.topology = vk::PrimitiveTopology::eTriangleList;
pipelineConfig.cullMode = vk::CullModeFlagBits::eBack;

// Create pipeline
squid::Pipeline pipeline(device, vertexShader, fragmentShader, pipelineConfig);
```

## Troubleshooting

### "vulkan/vulkan.hpp: No such file or directory"

**Solution**: Ensure VULKAN_SDK is set:
```bash
# Windows
set VULKAN_SDK=C:\VulkanSDK\1.3.250

# Linux
export VULKAN_SDK=/opt/vulkan/1.3.250

# Then reconfigure CMake
cmake ..
```

### "GLFW not found"

**Solution 1**: Let CMake fetch it (happens automatically)

**Solution 2**: Install system GLFW and clear CMake cache:
```bash
rm -rf build
mkdir build
cd build
cmake ..
```

### Validation Layer Errors

**Solution**: Ensure validation layers are installed:
```bash
# From Vulkan SDK, run the layer installer
# Or disable validation in debug builds
```

To disable validation in code:
```cpp
squid::InstanceConfig config;
config.enableValidation = false;  // Disable
```

### GLFW window doesn't appear

**Possible causes**:
1. GPU driver outdated - update to latest
2. Vulkan not properly installed - reinstall SDK
3. GLFW initialization failed - check console for errors

## Building the Examples

Squid includes a triangle rendering example:

```bash
# In your build directory
cmake .. -DSQUID_BUILD_EXAMPLES=ON
cmake --build .

# Run the example
./examples/triangle/squid_triangle  # or squid_triangle.exe
```

The example requires `glslc` from the Vulkan SDK.

## Project Organization Best Practices

```
my-vulkan-app/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── renderer.hpp
│   ├── renderer.cpp
│   ├── scene.hpp
│   └── scene.cpp
├── include/
│   └── my_app/
│       └── config.hpp
├── assets/
│   ├── shaders/
│   │   ├── vertex.vert
│   │   └── fragment.frag
│   └── textures/
└── vendor/
    └── squid/  # (if using add_subdirectory)
```

## Performance Tips

1. **Use Release builds for performance testing**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

2. **Disable validation in Release builds**
   - Squid automatically disables validation in Release builds
   - Define `NDEBUG` to control this

3. **Use device-local memory for static resources**
   - Images use device-local memory by default (optimal)
   - Buffers use host-visible memory for convenience

4. **Synchronize appropriately**
   ```cpp
   device.graphicsQueue().waitIdle();  // CPU waits for GPU
   // or use fences for more precise control
   ```

## Next Steps

1. **Review the [API documentation](./README.md#api-overview)** for complete class reference
2. **Study the [triangle example](./examples/triangle/main.cpp)** for practical usage
3. **Build your first project** using Method 1 or 2
4. **Join the community** and contribute improvements

## Support

- **Issues**: Report on GitHub
- **Questions**: Check existing discussions or create a new one
- **Contributions**: Pull requests welcome!

---

**Happy Vulkan programming with Squid!** 🦑
