cmake_minimum_required(VERSION 3.20)
project(squid VERSION 0.1.0 LANGUAGES CXX)

option(SQUID_BUILD_EXAMPLES "Build examples" OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 CONFIG QUIET)

if(TARGET glfw)
    set(SQUID_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
    set(SQUID_GLFW_TARGET glfw3::glfw)
elseif(TARGET glfw3)
    set(SQUID_GLFW_TARGET glfw3)
else()
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glfw)
    set(SQUID_GLFW_TARGET glfw)
endif()

add_library(squid
    src/Core/Window.cpp
    src/Core/Instance.cpp
    src/Core/Surface.cpp
    src/Core/Device.cpp
    src/Core/Swapchain.cpp
    src/Resources/Buffer.cpp
    src/Resources/Image.cpp
    src/Resources/Descriptor.cpp
    src/Pipeline/RenderPass.cpp
    src/Pipeline/Shader.cpp
    src/Pipeline/Pipeline.cpp
    src/Execution/CommandPool.cpp
    src/Execution/Sync.cpp
)
add_library(squid::squid ALIAS squid)

target_compile_features(squid PUBLIC cxx_std_20)
target_include_directories(squid PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(squid PUBLIC Vulkan::Vulkan ${SQUID_GLFW_TARGET})

include(cmake/SquidShaders.cmake)

if(SQUID_BUILD_EXAMPLES)
    add_subdirectory(examples/triangle)
endif()
