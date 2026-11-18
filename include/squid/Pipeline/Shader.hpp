#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>

namespace sqd {

class Device;

class Shader {
public:
    Shader(const Device& device, const std::filesystem::path& spirvFile);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    vk::ShaderModule handle() const noexcept { return m_module; }
    operator vk::ShaderModule() const noexcept { return m_module; }

private:
    vk::Device m_device{};
    vk::ShaderModule m_module{};
};

} // namespace sqd
