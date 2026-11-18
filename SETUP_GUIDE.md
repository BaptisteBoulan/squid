# Squid - Setup Guide for Windows

This guide explains how to set up a new Squid project on Windows.

## Prerequisites

Before you begin, ensure you have:

1. **CMake 3.20 or newer**
   ```cmd
   cmake --version
   ```

2. **Vulkan SDK installed**
   - Download from https://www.lunarg.com/vulkan-sdk/
   - Install to default location: `C:\VulkanSDK\<version>`
   - The installer will automatically set the `VULKAN_SDK` environment variable
   - Verify installation by opening a new Command Prompt and checking:
     ```cmd
     echo %VULKAN_SDK%
     ```

3. **C++ Compiler (MSVC 2019 or newer)**
   - Install Visual Studio 2019 or later with C++ workload, or
   - Install Visual Studio Build Tools with MSVC compiler

4. **GLFW 3.3+ (optional)**
   - CMake will automatically fetch it, so no manual installation needed
   - If you prefer to install it via vcpkg:
     ```cmd
     vcpkg install glfw3:x64-windows
     ```

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
        sqd::Window window(1280, 720, "My Vulkan App");
        
        // Setup Vulkan instance
        sqd::InstanceConfig instanceConfig;
        instanceConfig.extensions = sqd::Window::requiredInstanceExtensions();
        instanceConfig.applicationName = "My Vulkan App";
        sqd::Instance instance(instanceConfig);
        
        // Create rendering surface
        sqd::Surface surface = window.createSurface(instance);
        
        // Select GPU and create logical device
        sqd::Device device(instance, surface.handle());
        
        // Create swapchain for presenting to screen
        sqd::SwapchainConfig swapchainConfig;
        swapchainConfig.extent = window.framebufferSize();
        sqd::Swapchain swapchain(device, surface.handle(), swapchainConfig);
        
        // Create command pool for recording GPU commands
        sqd::CommandPool commandPool(device);
        
        std::cout << "Vulkan application initialized successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

### Step 4: Build Your Project

Open Command Prompt in your project directory:

```cmd
# Create build directory
mkdir build

# Configure with CMake
cmake -B build

# Build the project
cmake --build build --config Debug

# Run your application
.\Debug\my_app.exe
```

## Method 2: Using add_subdirectory

This method gives you more control and is better for monorepo setups.

### Step 1: Clone Squid into Your Project

```cmd
git clone https://github.com/BaptisteBoulan/squid.git vendor/squid
```

Or add it as a git submodule:

```cmd
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
    sqd::Window window(1280, 720, "Render Loop Example");
    sqd::InstanceConfig config;
    config.extensions = sqd::Window::requiredInstanceExtensions();
    sqd::Instance instance(config);
    sqd::Surface surface = window.createSurface(instance);
    sqd::Device device(instance, surface.handle());
    
    sqd::SwapchainConfig swapchainConfig;
    swapchainConfig.extent = window.framebufferSize();
    sqd::Swapchain swapchain(device, surface.handle(), swapchainConfig);
    
    sqd::CommandPool commandPool(device);
    sqd::RenderPass renderPass(device);
    
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
sqd::BufferConfig bufferConfig;
bufferConfig.size = sizeof(vertices);
bufferConfig.usage = vk::BufferUsageFlagBits::eVertexBuffer;
sqd::Buffer vertexBuffer(device, bufferConfig);
vertexBuffer.upload(vertices.data(), sizeof(vertices));

// Create a texture
sqd::ImageConfig imageConfig;
imageConfig.extent = {textureWidth, textureHeight, 1};
imageConfig.format = vk::Format::eR8G8B8A8Srgb;
imageConfig.createSampler = true;
sqd::Image texture(device, imageConfig);
// rgbaPixels contains textureWidth * textureHeight tightly packed RGBA pixels.
texture.uploadRgba8(device, commandPool, rgbaPixels.data(), rgbaPixels.size());
```

`uploadRgba8` stages the pixels into the image and transitions it to a
shader-readable layout. Squid does not decode image files, so decode them to
RGBA8 in your application before uploading.

### Setting Up Pipelines

```cpp
// Load shaders
sqd::Shader vertexShader(device, "vertex.spv");
sqd::Shader fragmentShader(device, "fragment.spv");

// Configure pipeline
sqd::PipelineConfig pipelineConfig;
pipelineConfig.renderPass = renderPass.handle();
pipelineConfig.extent = swapchain.extent();
pipelineConfig.topology = vk::PrimitiveTopology::eTriangleList;
pipelineConfig.cullMode = vk::CullModeFlagBits::eBack;

// Create pipeline
sqd::Pipeline pipeline(device, vertexShader, fragmentShader, pipelineConfig);
```

## Troubleshooting

### "vulkan/vulkan.hpp: No such file or directory"

**Solution**: Ensure VULKAN_SDK is set correctly:
```cmd
# Check if VULKAN_SDK is set
echo %VULKAN_SDK%

# If not set, manually set it (replace with your SDK version)
set VULKAN_SDK=C:\VulkanSDK\1.3.250

# Then reconfigure CMake in a new Command Prompt window
cmake ..
```

If this doesn't work, reinstall the Vulkan SDK and make sure to close and reopen Command Prompt after installation.

### "GLFW not found"

**Solution**: Let CMake fetch it automatically (default behavior). If it still fails, clear the build directory and reconfigure:
```cmd
# Remove build directory to clear cache
rmdir /s /q build

# Create fresh build directory
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

CMake will automatically download and compile GLFW for you.

### Validation Layer Errors

**Solution**: Ensure validation layers are installed:
```bash
# From Vulkan SDK, run the layer installer
# Or disable validation in debug builds
```

To disable validation in code:
```cpp
sqd::InstanceConfig config;
config.enableValidation = false;  // Disable
```

### GLFW window doesn't appear

**Possible causes**:
1. GPU driver outdated - update to latest
2. Vulkan not properly installed - reinstall SDK
3. GLFW initialization failed - check console for errors

## Building the Examples

Squid includes a triangle rendering example:

```cmd
# In your build directory
cmake .. -DSQUID_BUILD_EXAMPLES=ON
cmake --build . --config Debug

# Run the example
.\Debug\examples\triangle\squid_triangle.exe
```

The example requires `glslc` from the Vulkan SDK (included in the standard installation).

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
   ```cmd
   cmake --build . --config Release
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
