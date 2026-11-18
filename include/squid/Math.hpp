#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <stdexcept>

namespace sqd {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3 &operator+=(Vec3 v) noexcept {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    constexpr Vec3 &operator-=(Vec3 v) noexcept {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    constexpr Vec3 &operator*=(float scale) noexcept {
        x *= scale;
        y *= scale;
        z *= scale;
        return *this;
    }

    constexpr Vec3 &operator/=(float scale) noexcept {
        x /= scale;
        y /= scale;
        z /= scale;
        return *this;
    }
};

static_assert(sizeof(Vec3) == 3 * sizeof(float));

constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept { return a += b; }
constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept { return a -= b; }
constexpr Vec3 operator-(Vec3 v) noexcept { return {-v.x, -v.y, -v.z}; }
constexpr Vec3 operator*(Vec3 v, float scale) noexcept { return v *= scale; }
constexpr Vec3 operator*(float scale, Vec3 v) noexcept { return v *= scale; }
constexpr Vec3 operator/(Vec3 v, float scale) noexcept { return v /= scale; }

constexpr float dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr Vec3 cross(Vec3 a, Vec3 b) noexcept {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float length(Vec3 v) noexcept {
    return std::hypot(v.x, v.y, v.z);
}

inline Vec3 normalize(Vec3 v, float minimumLength = 0.0f) {
    const float magnitude = length(v);
    if (!(minimumLength >= 0.0f && magnitude > minimumLength) || !std::isfinite(magnitude))
        throw std::invalid_argument("Vector length must be finite and exceed the nonnegative minimum");
    return v / magnitude;
}

// Column-major storage for GLSL column vectors; indexing is (row, column).
struct Mat4 {
    std::array<float, 16> elements{};

    constexpr float *data() noexcept { return elements.data(); }
    constexpr const float *data() const noexcept { return elements.data(); }
    constexpr float &operator()(std::size_t row, std::size_t column) noexcept {
        return elements[column * 4 + row];
    }
    constexpr float operator()(std::size_t row, std::size_t column) const noexcept {
        return elements[column * 4 + row];
    }

    static constexpr Mat4 identity() noexcept {
        return {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
    }
};

static_assert(sizeof(Mat4) == 16 * sizeof(float));

constexpr Mat4 operator*(const Mat4 &a, const Mat4 &b) noexcept {
    Mat4 result;
    for (std::size_t column = 0; column < 4; ++column)
        for (std::size_t row = 0; row < 4; ++row)
            for (std::size_t k = 0; k < 4; ++k)
                result(row, column) += a(row, k) * b(k, column);
    return result;
}

// Right-handed view, looking down -Z. Forward and right must be orthonormal.
constexpr Mat4 viewFromBasis(Vec3 eye, Vec3 forward, Vec3 right) noexcept {
    const Vec3 up = cross(right, forward);
    return {{
        right.x, up.x, -forward.x, 0.0f,
        right.y, up.y, -forward.y, 0.0f,
        right.z, up.z, -forward.z, 0.0f,
        -dot(right, eye), -dot(up, eye), dot(forward, eye), 1.0f,
    }};
}

inline Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    const Vec3 forward = normalize(target - eye);
    return viewFromBasis(eye, forward, normalize(cross(forward, up)));
}

// Right-handed Vulkan projection: depth [0, 1], Y flipped for a positive-height viewport.
inline Mat4 perspectiveVulkan(float verticalFovRadians, float aspect, float nearPlane, float farPlane) {
    if (!(verticalFovRadians > 0.0f && verticalFovRadians < std::numbers::pi_v<float>) ||
        !(aspect > 0.0f && std::isfinite(aspect)) ||
        !(nearPlane > 0.0f && nearPlane < farPlane && std::isfinite(farPlane)))
        throw std::invalid_argument("Invalid Vulkan perspective parameters");

    const float scale = 1.0f / std::tan(verticalFovRadians * 0.5f);
    const float depth = farPlane / (nearPlane - farPlane);
    Mat4 projection;
    projection(0, 0) = scale / aspect;
    projection(1, 1) = -scale;
    projection(2, 2) = depth;
    projection(2, 3) = depth * nearPlane;
    projection(3, 2) = -1.0f;
    return projection;
}

} // namespace sqd
