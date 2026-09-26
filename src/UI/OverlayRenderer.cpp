#include "OverlayRenderer.h"
#include "../Core/MathUtils.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace AIDepthPro {

namespace {

void blendPixel(ImageFrame& img, int x, int y, const ColorRGBA& col) {
    if (x < 0 || x >= img.width || y < 0 || y >= img.height) return;
    int numChannels = (img.components == PixelComponent::RGBA) ? 4 : 3;

    if (img.bitDepth == BitDepth::Float) {
        float* p = (float*)((char*)img.data + y * img.rowBytes) + x * numChannels;
        p[0] = Math::lerp(p[0], col.r, col.a);
        p[1] = Math::lerp(p[1], col.g, col.a);
        p[2] = Math::lerp(p[2], col.b, col.a);
    } else {
        uint8_t* p = (uint8_t*)((char*)img.data + y * img.rowBytes) + x * numChannels;
        p[0] = static_cast<uint8_t>(Math::lerp(static_cast<float>(p[0]), col.r * 255.0f, col.a));
        p[1] = static_cast<uint8_t>(Math::lerp(static_cast<float>(p[1]), col.g * 255.0f, col.a));
        p[2] = static_cast<uint8_t>(Math::lerp(static_cast<float>(p[2]), col.b * 255.0f, col.a));
    }
}

// 5x7 bitmap font getter for HUD statistics
const uint8_t* getGlyph5x7(char c) {
    static const uint8_t kBlank[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const uint8_t k0[7] = {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E};
    static const uint8_t k1[7] = {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E};
    static const uint8_t k2[7] = {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F};
    static const uint8_t k3[7] = {0x1F,0x02,0x04,0x06,0x01,0x11,0x0E};
    static const uint8_t k4[7] = {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02};
    static const uint8_t k5[7] = {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E};
    static const uint8_t k6[7] = {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E};
    static const uint8_t k7[7] = {0x1F,0x01,0x02,0x04,0x08,0x08,0x08};
    static const uint8_t k8[7] = {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E};
    static const uint8_t k9[7] = {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C};
    static const uint8_t kColon[7] = {0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00};
    static const uint8_t kDot[7] = {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C};
    static const uint8_t kPercent[7] = {0x19,0x19,0x02,0x04,0x08,0x13,0x13};
    static const uint8_t kMinus[7] = {0x00,0x00,0x00,0x1F,0x00,0x00,0x00};
    static const uint8_t kSlash[7] = {0x01,0x02,0x04,0x08,0x10,0x00,0x00};
    static const uint8_t kPipe[7] = {0x04,0x04,0x04,0x04,0x04,0x04,0x04};
    static const uint8_t kParenL[7] = {0x02,0x04,0x08,0x08,0x08,0x04,0x02};
    static const uint8_t kParenR[7] = {0x08,0x04,0x02,0x02,0x02,0x04,0x08};
    static const uint8_t kA[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t kB[7] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    static const uint8_t kC[7] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E};
    static const uint8_t kD[7] = {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C};
    static const uint8_t kE[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    static const uint8_t kF[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
    static const uint8_t kG[7] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F};
    static const uint8_t kH[7] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t kI[7] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E};
    static const uint8_t kL[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
    static const uint8_t kM[7] = {0x11,0x1B,0x15,0x11,0x11,0x11,0x11};
    static const uint8_t kN[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    static const uint8_t kO[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t kP[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    static const uint8_t kR[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
    static const uint8_t kS[7] = {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E};
    static const uint8_t kT[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
    static const uint8_t kU[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t kV[7] = {0x11,0x11,0x11,0x11,0x11,0x0A,0x04};
    static const uint8_t kX[7] = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11};
    static const uint8_t ka[7] = {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F};
    static const uint8_t kc[7] = {0x00,0x00,0x0E,0x10,0x10,0x11,0x0E};
    static const uint8_t kd[7] = {0x01,0x01,0x0D,0x13,0x11,0x11,0x0F};
    static const uint8_t ke[7] = {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E};
    static const uint8_t kf[7] = {0x06,0x09,0x08,0x1C,0x08,0x08,0x08};
    static const uint8_t kg[7] = {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E};
    static const uint8_t kh[7] = {0x10,0x10,0x16,0x19,0x11,0x11,0x11};
    static const uint8_t ki[7] = {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E};
    static const uint8_t kl[7] = {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E};
    static const uint8_t km[7] = {0x00,0x00,0x1A,0x15,0x15,0x11,0x11};
    static const uint8_t kn[7] = {0x00,0x00,0x16,0x19,0x11,0x11,0x11};
    static const uint8_t ko[7] = {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E};
    static const uint8_t kp[7] = {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10};
    static const uint8_t kr[7] = {0x00,0x00,0x16,0x19,0x10,0x10,0x10};
    static const uint8_t ks[7] = {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E};
    static const uint8_t kt[7] = {0x08,0x08,0x1C,0x08,0x08,0x09,0x06};
    static const uint8_t ku[7] = {0x00,0x00,0x11,0x11,0x11,0x13,0x0D};
    static const uint8_t kx[7] = {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11};
    static const uint8_t ky[7] = {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E};

    switch (c) {
        case '0': return k0;
        case '1': return k1;
        case '2': return k2;
        case '3': return k3;
        case '4': return k4;
        case '5': return k5;
        case '6': return k6;
        case '7': return k7;
        case '8': return k8;
        case '9': return k9;
        case ':': return kColon;
        case '.': return kDot;
        case '%': return kPercent;
        case '-': return kMinus;
        case '/': return kSlash;
        case '|': return kPipe;
        case '(': return kParenL;
        case ')': return kParenR;
        case 'A': return kA;
        case 'B': return kB;
        case 'C': return kC;
        case 'D': return kD;
        case 'E': return kE;
        case 'F': return kF;
        case 'G': return kG;
        case 'H': return kH;
        case 'I': return kI;
        case 'L': return kL;
        case 'M': return kM;
        case 'N': return kN;
        case 'O': return kO;
        case 'P': return kP;
        case 'R': return kR;
        case 'S': return kS;
        case 'T': return kT;
        case 'U': return kU;
        case 'V': return kV;
        case 'X': return kX;
        case 'a': return ka;
        case 'c': return kc;
        case 'd': return kd;
        case 'e': return ke;
        case 'f': return kf;
        case 'g': return kg;
        case 'h': return kh;
        case 'i': return ki;
        case 'l': return kl;
        case 'm': return km;
        case 'n': return kn;
        case 'o': return ko;
        case 'p': return kp;
        case 'r': return kr;
        case 's': return ks;
        case 't': return kt;
        case 'u': return ku;
        case 'x': return kx;
        case 'y': return ky;
        default: return kBlank;
    }
}

void drawText(ImageFrame& img, int startX, int startY, const std::string& str, const ColorRGBA& col) {
    int curX = startX;
    for (char c : str) {
        const uint8_t* glyph = getGlyph5x7(c);
        for (int r = 0; r < 7; ++r) {
            uint8_t row = glyph[r];
            for (int colBit = 0; colBit < 5; ++colBit) {
                if (row & (0x10 >> colBit)) {
                    blendPixel(img, curX + colBit, startY + r, col);
                }
            }
        }
        curX += 7;
    }
}

} // anonymous namespace

void OverlayRenderer::DrawFocusTarget(
    ImageFrame& image,
    const Point2D& focusPoint
) {
    if (!image.isValid()) return;

    int cx = static_cast<int>(focusPoint.x * (image.width - 1));
    int cy = static_cast<int>(focusPoint.y * (image.height - 1));

    ColorRGBA cyanCol = { 0.0f, 1.0f, 1.0f, 0.9f };
    ColorRGBA shadowCol = { 0.0f, 0.0f, 0.0f, 0.6f };

    int r = 16;
    for (int a = 0; a < 360; a += 10) {
        float rad = a * 3.14159265f / 180.0f;
        int px = static_cast<int>(cx + r * std::cos(rad));
        int py = static_cast<int>(cy + r * std::sin(rad));
        blendPixel(image, px + 1, py + 1, shadowCol);
        blendPixel(image, px, py, cyanCol);
    }

    // Crosshairs
    for (int d = -r - 6; d <= r + 6; ++d) {
        if (std::abs(d) < 4) continue;
        blendPixel(image, cx + d, cy, cyanCol);
        blendPixel(image, cx, cy + d, cyanCol);
    }
}

void OverlayRenderer::DrawPerformanceOverlay(
    ImageFrame& image,
    const EngineStats& stats,
    float renderTimeMs,
    size_t cacheFrames,
    float cacheHitRate
) {
    if (!image.isValid() || image.width < 400 || image.height < 250) return;

    int boxW = 320;
    int boxH = 95;
    int boxX = 20;
    int boxY = 20;

    // Dark translucent background box
    ColorRGBA bgCol = { 0.05f, 0.07f, 0.1f, 0.85f };
    for (int y = boxY; y < boxY + boxH; ++y) {
        for (int x = boxX; x < boxX + boxW; ++x) {
            blendPixel(image, x, y, bgCol);
        }
    }

    // Header border
    ColorRGBA borderCol = { 0.2f, 0.6f, 1.0f, 0.9f };
    for (int x = boxX; x < boxX + boxW; ++x) {
        blendPixel(image, x, boxY, borderCol);
        blendPixel(image, x, boxY + boxH - 1, borderCol);
    }
    for (int y = boxY; y < boxY + boxH; ++y) {
        blendPixel(image, boxX, y, borderCol);
        blendPixel(image, boxX + boxW - 1, y, borderCol);
    }

    ColorRGBA textCol = { 0.95f, 0.95f, 0.95f, 1.0f };
    ColorRGBA highlightCol = { 0.2f, 0.9f, 0.4f, 1.0f };

    drawText(image, boxX + 10, boxY + 8, "AI DEPTH PRO - RESOLVE 21", borderCol);

    std::string backendLine = "Engine: " + stats.backendName;
    drawText(image, boxX + 10, boxY + 24, backendLine, textCol);

    char perfBuf[128];
    snprintf(perfBuf, sizeof(perfBuf), "Infer: %.1f ms | Render: %.1f ms | FPS: %.1f",
             stats.inferenceTimeMs, renderTimeMs, stats.currentFps);
    drawText(image, boxX + 10, boxY + 40, perfBuf, highlightCol);

    char resBuf[128];
    snprintf(resBuf, sizeof(perfBuf), "Res: %dx%d | VRAM: %.1f MB",
             image.width, image.height, static_cast<float>(stats.memoryUsedBytes) / (1024.0f * 1024.0f));
    drawText(image, boxX + 10, boxY + 56, resBuf, textCol);

    char cacheBuf[128];
    snprintf(cacheBuf, sizeof(cacheBuf), "Cache: %d frames (Hit Rate: %.0f%%)",
             static_cast<int>(cacheFrames), cacheHitRate * 100.0f);
    drawText(image, boxX + 10, boxY + 72, cacheBuf, textCol);
}

} // namespace AIDepthPro
