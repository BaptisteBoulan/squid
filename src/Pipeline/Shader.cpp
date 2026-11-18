#include <squid/Core/Device.hpp>
#include <squid/Pipeline/Shader.hpp>

#include "../VulkanUtils.hpp"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sqd {

Shader::Shader(const Device& device, const std::filesystem::path& spirvFile) : m_device(device.handle()) {
    std::ifstream file(spirvFile, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Unable to open SPIR-V shader: " + spirvFile.string());
    const std::streamsize byteCount = file.tellg();
    if (byteCount < static_cast<std::streamsize>(sizeof(uint32_t)) ||
        byteCount % static_cast<std::streamsize>(sizeof(uint32_t)) != 0) {
        throw std::runtime_error("Invalid SPIR-V file size: " + spirvFile.string());
    }
    std::vector<uint32_t> code(static_cast<size_t>(byteCount) / sizeof(uint32_t));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(code.data()), byteCount)) {
        throw std::runtime_error("Unable to read SPIR-V shader: " + spirvFile.string());
    }
    if (code.front() != 0x07230203) throw std::runtime_error("Invalid SPIR-V magic number: " + spirvFile.string());

    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = static_cast<size_t>(byteCount);
    createInfo.pCode = code.data();
    VkShaderModule module = VK_NULL_HANDLE;
    detail::check(vkCreateShaderModule(static_cast<VkDevice>(m_device), &createInfo, nullptr, &module),
                  "Create shader module");
    m_module = vk::ShaderModule(module);
}

Shader::~Shader() {
    if (m_module) vkDestroyShaderModule(static_cast<VkDevice>(m_device), static_cast<VkShaderModule>(m_module), nullptr);
}

Shader::Shader(Shader&& other) noexcept
    : m_device(std::exchange(other.m_device, vk::Device{})),
      m_module(std::exchange(other.m_module, vk::ShaderModule{})) {}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_module) vkDestroyShaderModule(static_cast<VkDevice>(m_device), static_cast<VkShaderModule>(m_module), nullptr);
        m_device = std::exchange(other.m_device, vk::Device{});
        m_module = std::exchange(other.m_module, vk::ShaderModule{});
    }
    return *this;
}

} // namespace sqd
