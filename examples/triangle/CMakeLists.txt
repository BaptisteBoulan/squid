find_program(SQUID_GLSLC glslc
    HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
)
if(NOT SQUID_GLSLC)
    message(FATAL_ERROR "SQUID_BUILD_EXAMPLES requires glslc from the Vulkan SDK")
endif()

set(SQUID_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${SQUID_SHADER_OUTPUT_DIR}")

foreach(shader triangle.vert triangle.frag)
    add_custom_command(
        OUTPUT "${SQUID_SHADER_OUTPUT_DIR}/${shader}.spv"
        COMMAND "${SQUID_GLSLC}" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}"
                -o "${SQUID_SHADER_OUTPUT_DIR}/${shader}.spv"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}"
        VERBATIM
    )
    list(APPEND SQUID_SHADER_BINARIES "${SQUID_SHADER_OUTPUT_DIR}/${shader}.spv")
endforeach()
add_custom_target(squid_triangle_shaders DEPENDS ${SQUID_SHADER_BINARIES})

add_executable(squid_triangle main.cpp)
add_dependencies(squid_triangle squid_triangle_shaders)
target_link_libraries(squid_triangle PRIVATE squid::squid)
target_compile_definitions(squid_triangle PRIVATE
    SQUID_TRIANGLE_VERTEX_SHADER="${SQUID_SHADER_OUTPUT_DIR}/triangle.vert.spv"
    SQUID_TRIANGLE_FRAGMENT_SHADER="${SQUID_SHADER_OUTPUT_DIR}/triangle.frag.spv"
)
