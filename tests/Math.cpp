#include <squid/Math.hpp>

#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {

bool close(float a, float b) {
    return std::abs(a - b) < 0.00001f;
}

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

template <typename Function>
void requireInvalidArgument(Function function, const char *message) {
    try {
        function();
    } catch (const std::invalid_argument &) {
        return;
    }
    throw std::runtime_error(message);
}

std::array<float, 4> clipPosition(const sqd::Mat4 &matrix, sqd::Vec3 point) {
    const auto *m = matrix.data();
    std::array<float, 4> result{};
    for (std::size_t row = 0; row < result.size(); ++row)
        result[row] = m[row] * point.x + m[4 + row] * point.y + m[8 + row] * point.z + m[12 + row];
    return result;
}

} // namespace

int main() {
    try {
        constexpr sqd::Vec3 eye{2, 3, 4};
        constexpr sqd::Vec3 right{0, 0, 1};
        constexpr sqd::Vec3 up{0, 1, 0};
        const auto forward = sqd::normalize(sqd::Vec3{5, 3, 4} - eye);
        const auto cameraRight = sqd::cross(forward, up);
        require(close(sqd::length(forward), 1) && close(sqd::dot(cameraRight, right), 1) &&
                    close(sqd::dot(cameraRight, forward), 0),
                "Camera basis is not orthonormal or has the wrong handedness");
        requireInvalidArgument([] { sqd::normalize({}); }, "Zero vector was normalized");
        requireInvalidArgument([] { sqd::normalize({0, 0, 0.001f}, 0.01f); },
                               "Normalization ignored the minimum length");
        requireInvalidArgument([&] { sqd::lookAt(eye, eye + up, up); }, "Parallel view direction and up were accepted");
        requireInvalidArgument([] { sqd::perspectiveVulkan(1, 0, 1, 10); }, "Zero aspect ratio was accepted");

        const auto view = sqd::lookAt(eye, eye + forward, up);
        const auto projection = sqd::perspectiveVulkan(std::numbers::pi_v<float> / 2, 2, 1, 10);
        const auto viewProjection = projection * view;
        const auto nearClip = clipPosition(viewProjection, eye + forward);
        const auto farClip = clipPosition(viewProjection, eye + 10 * forward);
        require(close(nearClip[2] / nearClip[3], 0) && close(farClip[2] / farClip[3], 1),
                "Near and far planes do not map to Vulkan's depth range");
        const auto corner = clipPosition(viewProjection, eye + forward + right * 2 + up);
        require(close(corner[0] / corner[3], 1) && close(corner[1] / corner[3], -1),
                "Projection has incorrect aspect, handedness, or Vulkan Y flip");
        require(clipPosition(viewProjection, eye - forward)[3] < 0, "Point behind the camera has positive clip W");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
