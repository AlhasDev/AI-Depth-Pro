#pragma once

#include "Types.h"
#include <cmath>
#include <algorithm>

namespace AIDepthPro {
namespace Math {

template <typename T>
inline T clamp(T val, T minVal, T maxVal) {
    return (val < minVal) ? minVal : ((val > maxVal) ? maxVal : val);
}

template <typename T>
inline T lerp(T a, T b, float t) {
    return a + static_cast<T>((b - a) * t);
}

inline float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float rgbToLuminance(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vec3 normalized() const {
        float len = length();
        if (len > 1e-6f) {
            float inv = 1.0f / len;
            return Vec3(x * inv, y * inv, z * inv);
        }
        return Vec3(0.0f, 0.0f, 1.0f);
    }

    static float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
};

// Google Turbo Colormap official polynomial approximation
inline ColorRGBA turboColorMap(float x) {
    x = clamp(x, 0.0f, 1.0f);
    const float kRedVec4[4] = {0.13572138f, 4.61539260f, -42.66032258f, 132.13108234f};
    const float kGreenVec4[4] = {0.09140261f, 2.19418839f, 4.84296658f, -14.18503333f};
    const float kBlueVec4[4] = {0.52901961f, 12.64194608f, -60.58204836f, 110.36276771f};
    const float kRedVec2[2] = {-152.94239396f, 59.28637943f};
    const float kGreenVec2[2] = {4.27729857f, 2.82956604f};
    const float kBlueVec2[2] = {-89.90310912f, 27.34824973f};

    float x2 = x * x;
    float x3 = x2 * x;
    float x4 = x3 * x;
    float x5 = x4 * x;

    float r = kRedVec4[0] + kRedVec4[1] * x + kRedVec4[2] * x2 + kRedVec4[3] * x3 + kRedVec2[0] * x4 + kRedVec2[1] * x5;
    float g = kGreenVec4[0] + kGreenVec4[1] * x + kGreenVec4[2] * x2 + kGreenVec4[3] * x3 + kGreenVec2[0] * x4 + kGreenVec2[1] * x5;
    float b = kBlueVec4[0] + kBlueVec4[1] * x + kBlueVec4[2] * x2 + kBlueVec4[3] * x3 + kBlueVec2[0] * x4 + kBlueVec2[1] * x5;

    return { clamp(r, 0.0f, 1.0f), clamp(g, 0.0f, 1.0f), clamp(b, 0.0f, 1.0f), 1.0f };
}

// Magma Colormap approximation
inline ColorRGBA magmaColorMap(float x) {
    x = clamp(x, 0.0f, 1.0f);
    float r = clamp(-0.002f + 1.25f * x - 0.25f * x * x, 0.0f, 1.0f);
    float g = clamp(0.001f + 0.05f * x + 0.95f * std::pow(x, 2.5f), 0.0f, 1.0f);
    float b = clamp(0.01f + 1.9f * x - 2.8f * x * x + 1.8f * x * x * x, 0.0f, 1.0f);
    return { r, g, b, 1.0f };
}

// Cool-Warm (Blue to Cyan to Yellow to Red) Colormap
inline ColorRGBA coolWarmColorMap(float x) {
    x = clamp(x, 0.0f, 1.0f);
    float r = clamp(smoothstep(0.35f, 0.85f, x), 0.0f, 1.0f);
    float g = clamp(1.0f - 2.0f * std::abs(x - 0.5f), 0.0f, 1.0f);
    float b = clamp(smoothstep(0.65f, 0.15f, x), 0.0f, 1.0f);
    return { r, g, b, 1.0f };
}

inline ColorRGBA evaluateColorMap(float depthValue, ColorMapType type) {
    switch (type) {
        case ColorMapType::Turbo:
            return turboColorMap(depthValue);
        case ColorMapType::Magma:
        case ColorMapType::Inferno:
            return magmaColorMap(depthValue);
        case ColorMapType::CoolWarm:
        default:
            return coolWarmColorMap(depthValue);
    }
}

} // namespace Math
} // namespace AIDepthPro
