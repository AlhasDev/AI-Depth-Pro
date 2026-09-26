#include "Relighting.h"
#include "../Core/ThreadPool.h"
#include <cmath>
#include <algorithm>

namespace AIDepthPro {

void Relighting::ComputeNormals(
    const DepthFrame& depth,
    std::vector<Math::Vec3>& outNormals,
    float normalStrength
) {
    int w = depth.width;
    int h = depth.height;
    if (w <= 0 || h <= 0) return;

    outNormals.resize(w * h);

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            float dL = depth.get(x - 1, y);
            float dR = depth.get(x + 1, y);
            float dT = depth.get(x, y - 1);
            float dB = depth.get(x, y + 1);

            // Depth gradient
            float dzdx = (dR - dL) * 0.5f * normalStrength * w;
            float dzdy = (dB - dT) * 0.5f * normalStrength * h;

            Math::Vec3 normal(-dzdx, -dzdy, 1.0f);
            outNormals[y * w + x] = normal.normalized();
        }
    });
}

void Relighting::ApplyRelighting(
    const ImageFrame& srcImage,
    const DepthFrame& depth,
    ImageFrame& dstImage,
    const EffectParams& params
) {
    int w = dstImage.width;
    int h = dstImage.height;
    if (w <= 0 || h <= 0 || !srcImage.isValid() || !dstImage.isValid()) return;

    std::vector<Math::Vec3> normals;
    ComputeNormals(depth, normals, 1.5f);

    Math::Vec3 lightDir(params.lightDirX, params.lightDirY, std::max(params.lightHeight, 0.05f));
    lightDir = lightDir.normalized();

    Math::Vec3 viewDir(0.0f, 0.0f, 1.0f);
    Math::Vec3 halfDir = (lightDir + viewDir).normalized();

    float ambient = Math::clamp(params.ambientLight, 0.0f, 1.0f);
    float strength = Math::clamp(params.lightStrength, 0.0f, 3.0f);
    float softness = Math::clamp(params.lightSoftness, 0.0f, 1.0f);
    float shininess = Math::lerp(32.0f, 4.0f, softness);

    float* dstFloat = static_cast<float*>(dstImage.data);
    uint8_t* dstByte = static_cast<uint8_t*>(dstImage.data);
    const float* srcFloat = static_cast<const float*>(srcImage.data);
    const uint8_t* srcByte = static_cast<const uint8_t*>(srcImage.data);
    int numChannels = (dstImage.components == PixelComponent::RGBA) ? 4 : 3;

    ThreadPool::getInstance().parallelFor(0, h, [&](int y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            const Math::Vec3& N = normals[idx];

            // Diffuse Lambertian with wrap-around softness
            float ndotl = Math::Vec3::dot(N, lightDir);
            float diffuse = Math::clamp((ndotl + softness) / (1.0f + softness), 0.0f, 1.0f);

            // Specular Blinn-Phong
            float ndoth = Math::clamp(Math::Vec3::dot(N, halfDir), 0.0f, 1.0f);
            float specular = std::pow(ndoth, shininess) * (1.0f - softness);

            float totalLight = ambient + strength * (diffuse + specular * 0.4f);

            float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

            if (srcImage.bitDepth == BitDepth::Float) {
                const float* s = (const float*)((const char*)srcFloat + y * srcImage.rowBytes) + x * numChannels;
                r = s[0] * totalLight;
                g = s[1] * totalLight;
                b = s[2] * totalLight;
                if (numChannels == 4) a = s[3];

                float* dPixel = (float*)((char*)dstFloat + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = Math::clamp(r, 0.0f, 1.0f);
                dPixel[1] = Math::clamp(g, 0.0f, 1.0f);
                dPixel[2] = Math::clamp(b, 0.0f, 1.0f);
                if (numChannels == 4) dPixel[3] = a;
            } else {
                const uint8_t* s = (const uint8_t*)((const char*)srcByte + y * srcImage.rowBytes) + x * numChannels;
                r = (s[0] / 255.0f) * totalLight;
                g = (s[1] / 255.0f) * totalLight;
                b = (s[2] / 255.0f) * totalLight;
                if (numChannels == 4) a = s[3] / 255.0f;

                uint8_t* dPixel = (uint8_t*)((char*)dstByte + y * dstImage.rowBytes) + x * numChannels;
                dPixel[0] = static_cast<uint8_t>(Math::clamp(r * 255.0f, 0.0f, 255.0f));
                dPixel[1] = static_cast<uint8_t>(Math::clamp(g * 255.0f, 0.0f, 255.0f));
                dPixel[2] = static_cast<uint8_t>(Math::clamp(b * 255.0f, 0.0f, 255.0f));
                if (numChannels == 4) dPixel[3] = static_cast<uint8_t>(Math::clamp(a * 255.0f, 0.0f, 255.0f));
            }
        }
    });
}

} // namespace AIDepthPro
