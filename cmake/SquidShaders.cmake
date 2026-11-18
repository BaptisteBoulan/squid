include_guard(GLOBAL)

function(squid_compile_shader target source path_definition)
    cmake_parse_arguments(PARSE_ARGV 3 shader "" "STAGE" "")
    if(shader_UNPARSED_ARGUMENTS OR shader_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "Usage: squid_compile_shader(target source path_definition [STAGE stage])")
    endif()

    find_program(SQUID_GLSLC glslc
        HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
        REQUIRED
    )

    get_filename_component(source "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(filename "${source}" NAME)
    set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/shaders/${target}")
    set(output "${output_dir}/${filename}.spv")
    set(stage_arg)
    if(DEFINED shader_STAGE)
        set(stage_arg "-fshader-stage=${shader_STAGE}")
    endif()

    add_custom_command(
        OUTPUT "${output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_dir}"
        COMMAND "${SQUID_GLSLC}" ${stage_arg} "${source}" -o "${output}"
        DEPENDS "${source}"
        VERBATIM
    )
    target_sources(${target} PRIVATE "${output}")
    target_compile_definitions(${target} PRIVATE "${path_definition}=\"${output}\"")
endfunction()
