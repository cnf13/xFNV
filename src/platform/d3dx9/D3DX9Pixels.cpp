// The pixel side of the texture functions: the formats the game's textures and render targets use, read into and
// written from float RGBA (the way d3dx9 converts: a channel is its value over its maximum, written back as
// value * maximum + 0.5 truncated), the DXT1/3/5 codec and d3dx9's box filter for generated mip levels.

#include "D3DX9Internal.h"

#include <cmath>
#include <cstdarg>
#include <cstring>

namespace d3dx {

    void Log(const char* apFormat, ...) {
        va_list kArgs;
        va_start(kArgs, apFormat);
        std::fputs("d3dx9: ", stderr);
        std::vfprintf(stderr, apFormat, kArgs);
        std::fputc('\n', stderr);
        va_end(kArgs);
    }

    namespace {

        constexpr FormatInfo kFormats[] = {
            {D3DFMT_A8R8G8B8, 32, false, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, false},
            {D3DFMT_X8R8G8B8, 32, false, 0x00FF0000, 0x0000FF00, 0x000000FF, 0, false},
            {D3DFMT_A8B8G8R8, 32, false, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, false},
            {D3DFMT_X8B8G8R8, 32, false, 0x000000FF, 0x0000FF00, 0x00FF0000, 0, false},
            {D3DFMT_R8G8B8, 24, false, 0xFF0000, 0x00FF00, 0x0000FF, 0, false},
            {D3DFMT_R5G6B5, 16, false, 0xF800, 0x07E0, 0x001F, 0, false},
            {D3DFMT_X1R5G5B5, 16, false, 0x7C00, 0x03E0, 0x001F, 0, false},
            {D3DFMT_A1R5G5B5, 16, false, 0x7C00, 0x03E0, 0x001F, 0x8000, false},
            {D3DFMT_A4R4G4B4, 16, false, 0x0F00, 0x00F0, 0x000F, 0xF000, false},
            {D3DFMT_X4R4G4B4, 16, false, 0x0F00, 0x00F0, 0x000F, 0, false},
            {D3DFMT_A8, 8, false, 0, 0, 0, 0xFF, false},
            {D3DFMT_L8, 8, false, 0xFF, 0, 0, 0, true},
            {D3DFMT_A8L8, 16, false, 0x00FF, 0, 0, 0xFF00, true},
            {D3DFMT_DXT1, 8, true, 0, 0, 0, 0, false},
            {D3DFMT_DXT2, 16, true, 0, 0, 0, 0, false},
            {D3DFMT_DXT3, 16, true, 0, 0, 0, 0, false},
            {D3DFMT_DXT4, 16, true, 0, 0, 0, 0, false},
            {D3DFMT_DXT5, 16, true, 0, 0, 0, 0, false},
        };

        uint32_t Shift(uint32_t auiMask) {
            uint32_t uiShift = 0;
            while (auiMask && !(auiMask & 1)) {
                auiMask >>= 1;
                ++uiShift;
            }
            return uiShift;
        }

        // A channel as float: its value times the reciprocal of its maximum, as d3dx9 computes it (a division would
        // round the DXT1 three-color mean of 0.5 differently).
        float Unpack(uint32_t auiPixel, uint32_t auiMask) {
            const uint32_t uiShift = Shift(auiMask);
            const uint32_t uiMax = auiMask >> uiShift;
            return static_cast<float>((auiPixel & auiMask) >> uiShift) * (1.0f / static_cast<float>(uiMax));
        }

        float Clamp01(float afValue) {
            return afValue < 0.0f ? 0.0f : (afValue > 1.0f ? 1.0f : afValue);
        }

        // A float channel written at auiMax steps: rounded (afOffset 0.5), or dithered by d3dx9's ordered offsets.
        uint32_t Quantize(float afValue, uint32_t auiMax, float afOffset = 0.5f) {
            return static_cast<uint32_t>(Clamp01(afValue) * static_cast<float>(auiMax) + afOffset);
        }

        uint32_t Pack(float afValue, uint32_t auiMask, float afOffset) {
            const uint32_t uiShift = Shift(auiMask);
            return Quantize(afValue, auiMask >> uiShift, afOffset) << uiShift;
        }

        // d3dx9's dither (D3DX_FILTER_DITHER, part of D3DX_DEFAULT): a 4x4 ordered pattern by the pixel's position, the
        // offset added before truncation being (31 - 2 * B) / 32 for the Bayer index B (measured against the DLL).
        constexpr uint8_t kBayer[4][4] = {{0, 8, 2, 10}, {6, 14, 4, 12}, {3, 11, 1, 9}, {5, 13, 7, 15}};

        float DitherOffset(uint32_t x, uint32_t y) {
            return static_cast<float>(31 - 2 * kBayer[y & 3][x & 3]) / 32.0f;
        }

        // --- DXT
        // ------------------------------------------------------------------------------------------------------------

        Color Expand565(uint16_t ausColor) {
            return {
                static_cast<float>(ausColor >> 11) * (1.0f / 31.0f),
                static_cast<float>((ausColor >> 5) & 0x3F) * (1.0f / 63.0f),
                static_cast<float>(ausColor & 0x1F) * (1.0f / 31.0f), 1.0f
            };
        }

        uint16_t Pack565(const Color& arColor) {
            return static_cast<uint16_t>(
                Quantize(arColor.r, 31) << 11 | Quantize(arColor.g, 63) << 5 | Quantize(arColor.b, 31)
            );
        }

        // The four colors of a color block. abFourColor: DXT3/5 blocks always, DXT1 when c0 > c1; otherwise the third
        // color is the mean and the fourth transparent black.
        void ColorPalette(uint16_t aus0, uint16_t aus1, bool abFourColor, Color* apPalette) {
            const Color k0 = Expand565(aus0);
            const Color k1 = Expand565(aus1);
            apPalette[0] = k0;
            apPalette[1] = k1;
            if (abFourColor) {
                apPalette[2] = {
                    (2.0f * k0.r + k1.r) / 3.0f, (2.0f * k0.g + k1.g) / 3.0f, (2.0f * k0.b + k1.b) / 3.0f, 1.0f
                };
                apPalette[3] = {
                    (k0.r + 2.0f * k1.r) / 3.0f, (k0.g + 2.0f * k1.g) / 3.0f, (k0.b + 2.0f * k1.b) / 3.0f, 1.0f
                };
            }
            else {
                apPalette[2] = {(k0.r + k1.r) / 2.0f, (k0.g + k1.g) / 2.0f, (k0.b + k1.b) / 2.0f, 1.0f};
                apPalette[3] = {0.0f, 0.0f, 0.0f, 0.0f};
            }
        }

        void AlphaPalette(uint8_t auc0, uint8_t auc1, float* apPalette) {
            const float f0 = static_cast<float>(auc0) * (1.0f / 255.0f);
            const float f1 = static_cast<float>(auc1) * (1.0f / 255.0f);
            apPalette[0] = f0;
            apPalette[1] = f1;
            if (auc0 > auc1) {
                for (int32_t i = 1; i < 7; ++i) {
                    apPalette[i + 1] = (static_cast<float>(7 - i) * f0 + static_cast<float>(i) * f1) / 7.0f;
                }
            }
            else {
                for (int32_t i = 1; i < 5; ++i) {
                    apPalette[i + 1] = (static_cast<float>(5 - i) * f0 + static_cast<float>(i) * f1) / 5.0f;
                }
                apPalette[6] = 0.0f;
                apPalette[7] = 1.0f;
            }
        }

        uint16_t Read16(const uint8_t* apSrc) {
            return static_cast<uint16_t>(apSrc[0] | apSrc[1] << 8);
        }

        uint32_t Read32(const uint8_t* apSrc) {
            return static_cast<uint32_t>(apSrc[0]) | static_cast<uint32_t>(apSrc[1]) << 8 |
                static_cast<uint32_t>(apSrc[2]) << 16 | static_cast<uint32_t>(apSrc[3]) << 24;
        }

        void Write16(uint8_t* apDst, uint16_t ausValue) {
            apDst[0] = static_cast<uint8_t>(ausValue);
            apDst[1] = static_cast<uint8_t>(ausValue >> 8);
        }

        void Write32(uint8_t* apDst, uint32_t auiValue) {
            for (int32_t i = 0; i < 4; ++i) {
                apDst[i] = static_cast<uint8_t>(auiValue >> (8 * i));
            }
        }

        // One 4x4 block into apBlock (row-major, 16 colors).
        void DecodeBlock(uint32_t auiFormat, const uint8_t* apSrc, Color* apBlock) {
            float afAlpha[16];
            bool bAlpha = false;
            if (auiFormat == D3DFMT_DXT2 || auiFormat == D3DFMT_DXT3) {
                for (int32_t i = 0; i < 16; ++i) {
                    afAlpha[i] = static_cast<float>((apSrc[i / 2] >> (4 * (i & 1))) & 0xF) * (1.0f / 15.0f);
                }
                bAlpha = true;
                apSrc += 8;
            }
            else if (auiFormat == D3DFMT_DXT4 || auiFormat == D3DFMT_DXT5) {
                float afPalette[8];
                AlphaPalette(apSrc[0], apSrc[1], afPalette);
                uint64_t ulBits = 0;
                for (int32_t i = 0; i < 6; ++i) {
                    ulBits |= static_cast<uint64_t>(apSrc[2 + i]) << (8 * i);
                }
                for (int32_t i = 0; i < 16; ++i) {
                    afAlpha[i] = afPalette[(ulBits >> (3 * i)) & 7];
                }
                bAlpha = true;
                apSrc += 8;
            }
            const uint16_t us0 = Read16(apSrc);
            const uint16_t us1 = Read16(apSrc + 2);
            Color akPalette[4];
            // d3dx9 reads every format's color block as DXT1 does: three colors when c0 <= c1 (the fourth black).
            ColorPalette(us0, us1, us0 > us1, akPalette);
            const uint32_t uiIndices = Read32(apSrc + 4);
            for (int32_t i = 0; i < 16; ++i) {
                apBlock[i] = akPalette[(uiIndices >> (2 * i)) & 3];
                if (bAlpha) {
                    apBlock[i].a = afAlpha[i];
                }
            }
        }

        float Distance(const Color& arA, const Color& arB) {
            const float fR = arA.r - arB.r;
            const float fG = arA.g - arB.g;
            const float fB = arA.b - arB.b;
            return fR * fR + fG * fG + fB * fB;
        }

        // The nearest of the first auiCount palette colors.
        uint32_t Nearest(const Color& arColor, const Color* apPalette, uint32_t auiCount) {
            uint32_t uiBest = 0;
            float fBest = Distance(arColor, apPalette[0]);
            for (uint32_t i = 1; i < auiCount; ++i) {
                const float fDistance = Distance(arColor, apPalette[i]);
                if (fDistance < fBest) {
                    fBest = fDistance;
                    uiBest = i;
                }
            }
            return uiBest;
        }

        // The squared error of the used texels against the nearest palette entry of the endpoints us0, us1.
        float FitError(const Color* apBlock, const bool* apUse, uint16_t aus0, uint16_t aus1, bool abFourColor) {
            Color akPalette[4];
            ColorPalette(aus0, aus1, abFourColor, akPalette);
            float fError = 0.0f;
            for (int32_t i = 0; i < 16; ++i) {
                if (apUse[i]) {
                    fError += Distance(apBlock[i], akPalette[Nearest(apBlock[i], akPalette, abFourColor ? 4 : 3)]);
                }
            }
            return fError;
        }

        // A color block for the valid texels of apBlock: endpoints along the colors' principal axis, refined by least
        // squares on their index assignment. abDXT1 with a texel of alpha below one half: DXT1's three-color mode,
        // those texels transparent.
        void EncodeColorBlock(const Color* apBlock, const bool* apValid, bool abDXT1, uint8_t* apDst) {
            bool bTransparent = false;
            bool abUse[16];
            uint32_t uiCount = 0;
            for (int32_t i = 0; i < 16; ++i) {
                abUse[i] = apValid[i];
                if (apValid[i] && abDXT1 && apBlock[i].a < 0.5f) {
                    bTransparent = true;
                    abUse[i] = false;
                }
                uiCount += abUse[i];
            }
            Color kLow{0, 0, 0, 1};
            Color kHigh{0, 0, 0, 1};
            if (uiCount) {
                float fMean[3] = {0, 0, 0};
                for (int32_t i = 0; i < 16; ++i) {
                    if (abUse[i]) {
                        fMean[0] += apBlock[i].r;
                        fMean[1] += apBlock[i].g;
                        fMean[2] += apBlock[i].b;
                    }
                }
                for (float& f : fMean) {
                    f /= static_cast<float>(uiCount);
                }
                float fCov[6] = {0, 0, 0, 0, 0, 0};
                for (int32_t i = 0; i < 16; ++i) {
                    if (!abUse[i]) {
                        continue;
                    }
                    const float r = apBlock[i].r - fMean[0];
                    const float g = apBlock[i].g - fMean[1];
                    const float b = apBlock[i].b - fMean[2];
                    fCov[0] += r * r;
                    fCov[1] += r * g;
                    fCov[2] += r * b;
                    fCov[3] += g * g;
                    fCov[4] += g * b;
                    fCov[5] += b * b;
                }
                float fAxis[3] = {1.0f, 1.0f, 1.0f};
                for (int32_t iIteration = 0; iIteration < 8; ++iIteration) {
                    const float x = fCov[0] * fAxis[0] + fCov[1] * fAxis[1] + fCov[2] * fAxis[2];
                    const float y = fCov[1] * fAxis[0] + fCov[3] * fAxis[1] + fCov[4] * fAxis[2];
                    const float z = fCov[2] * fAxis[0] + fCov[4] * fAxis[1] + fCov[5] * fAxis[2];
                    const float fLength = std::sqrt(x * x + y * y + z * z);
                    if (fLength < 1e-12f) {
                        break;
                    }
                    fAxis[0] = x / fLength;
                    fAxis[1] = y / fLength;
                    fAxis[2] = z / fLength;
                }
                float fMin = 1e30f;
                float fMax = -1e30f;
                for (int32_t i = 0; i < 16; ++i) {
                    if (abUse[i]) {
                        const float t = (apBlock[i].r - fMean[0]) * fAxis[0] + (apBlock[i].g - fMean[1]) * fAxis[1] +
                            (apBlock[i].b - fMean[2]) * fAxis[2];
                        fMin = t < fMin ? t : fMin;
                        fMax = t > fMax ? t : fMax;
                    }
                }
                kLow = {
                    Clamp01(fMean[0] + fAxis[0] * fMin), Clamp01(fMean[1] + fAxis[1] * fMin),
                    Clamp01(fMean[2] + fAxis[2] * fMin), 1.0f
                };
                kHigh = {
                    Clamp01(fMean[0] + fAxis[0] * fMax), Clamp01(fMean[1] + fAxis[1] * fMax),
                    Clamp01(fMean[2] + fAxis[2] * fMax), 1.0f
                };
                // Least squares on the index weights of the current endpoints, twice, each kept only when it fits
                // better (a block whose texels all take one index makes the system near singular, and its solution
                // wild).
                const uint32_t uiColors = bTransparent ? 3 : 4;
                static constexpr float kThreeColor[] = {0.0f, 1.0f, 0.5f};
                static constexpr float kFourColor[] = {0.0f, 1.0f, 1.0f / 3.0f, 2.0f / 3.0f};
                const float* pWeights = bTransparent ? kThreeColor : kFourColor;
                for (int32_t iIteration = 0; iIteration < 2; ++iIteration) {
                    Color akPalette[4];
                    const uint16_t us0 = Pack565(kLow);
                    const uint16_t us1 = Pack565(kHigh);
                    ColorPalette(us0, us1, !bTransparent, akPalette);
                    float fAA = 0, fAB = 0, fBB = 0;
                    float fAX[3] = {0, 0, 0};
                    float fBX[3] = {0, 0, 0};
                    for (int32_t i = 0; i < 16; ++i) {
                        if (!abUse[i]) {
                            continue;
                        }
                        const float t = pWeights[Nearest(apBlock[i], akPalette, uiColors)];
                        const float s = 1.0f - t;
                        fAA += s * s;
                        fAB += s * t;
                        fBB += t * t;
                        const float afX[3] = {apBlock[i].r, apBlock[i].g, apBlock[i].b};
                        for (int32_t c = 0; c < 3; ++c) {
                            fAX[c] += s * afX[c];
                            fBX[c] += t * afX[c];
                        }
                    }
                    const float fDet = fAA * fBB - fAB * fAB;
                    if (std::fabs(fDet) < 1e-8f) {
                        break;
                    }
                    float afLow[3], afHigh[3];
                    for (int32_t c = 0; c < 3; ++c) {
                        afLow[c] = Clamp01((fAX[c] * fBB - fBX[c] * fAB) / fDet);
                        afHigh[c] = Clamp01((fBX[c] * fAA - fAX[c] * fAB) / fDet);
                    }
                    const Color kNewLow{afLow[0], afLow[1], afLow[2], 1.0f};
                    const Color kNewHigh{afHigh[0], afHigh[1], afHigh[2], 1.0f};
                    if (FitError(apBlock, abUse, Pack565(kNewLow), Pack565(kNewHigh), !bTransparent) >=
                        FitError(apBlock, abUse, us0, us1, !bTransparent)) {
                        break;
                    }
                    kLow = kNewLow;
                    kHigh = kNewHigh;
                }
            }
            uint16_t us0 = Pack565(kLow);
            uint16_t us1 = Pack565(kHigh);
            // The mode is in the endpoint order: c0 > c1 four colors, otherwise three and transparent.
            if (bTransparent ? us0 > us1 : us0 < us1) {
                const uint16_t usSwap = us0;
                us0 = us1;
                us1 = usSwap;
            }
            Color akPalette[4];
            const bool bFourColor = !bTransparent && us0 > us1;
            ColorPalette(us0, us1, bFourColor || !abDXT1, akPalette);
            uint32_t uiIndices = 0;
            for (int32_t i = 0; i < 16; ++i) {
                uint32_t uiIndex = 0;
                if (apValid[i]) {
                    if (bTransparent && apBlock[i].a < 0.5f) {
                        uiIndex = 3;
                    }
                    else if (bFourColor || bTransparent) {
                        uiIndex = Nearest(apBlock[i], akPalette, bTransparent ? 3 : 4);
                    }
                }
                uiIndices |= uiIndex << (2 * i);
            }
            Write16(apDst, us0);
            Write16(apDst + 2, us1);
            Write32(apDst + 4, uiIndices);
        }

        void EncodeBlock(uint32_t auiFormat, const Color* apBlock, const bool* apValid, uint8_t* apDst) {
            if (auiFormat == D3DFMT_DXT2 || auiFormat == D3DFMT_DXT3) {
                std::memset(apDst, 0, 8);
                for (int32_t i = 0; i < 16; ++i) {
                    apDst[i / 2] |= static_cast<uint8_t>(Quantize(apBlock[i].a, 15) << (4 * (i & 1)));
                }
                apDst += 8;
            }
            else if (auiFormat == D3DFMT_DXT4 || auiFormat == D3DFMT_DXT5) {
                // Both alpha modes, the one with the smaller squared error: eight steps between the extremes, or six
                // between the extremes other than 0 and 255, which the mode adds.
                uint32_t uiMin = 255, uiMax = 0, uiMin6 = 255, uiMax6 = 0;
                for (int32_t i = 0; i < 16; ++i) {
                    if (apValid[i]) {
                        const uint32_t uiAlpha = Quantize(apBlock[i].a, 255);
                        uiMin = uiAlpha < uiMin ? uiAlpha : uiMin;
                        uiMax = uiAlpha > uiMax ? uiAlpha : uiMax;
                        if (uiAlpha && uiAlpha != 255) {
                            uiMin6 = uiAlpha < uiMin6 ? uiAlpha : uiMin6;
                            uiMax6 = uiAlpha > uiMax6 ? uiAlpha : uiMax6;
                        }
                    }
                }
                if (uiMin > uiMax) {
                    uiMin = uiMax = 255;
                }
                if (uiMin6 > uiMax6) {
                    uiMin6 = uiMax6 = uiMin;
                }
                // Eight-step mode needs a0 > a1; equal extremes are a constant block, index 0.
                const uint8_t aucEnds[2][2] = {
                    {static_cast<uint8_t>(uiMax), static_cast<uint8_t>(uiMin)},
                    {static_cast<uint8_t>(uiMin6), static_cast<uint8_t>(uiMax6)}
                };
                float fBestError = 1e30f;
                for (int32_t iMode = 0; iMode < 2; ++iMode) {
                    float afPalette[8];
                    AlphaPalette(aucEnds[iMode][0], aucEnds[iMode][1], afPalette);
                    const uint32_t uiEntries = aucEnds[iMode][0] == aucEnds[iMode][1] && iMode == 0 ? 1 : 8;
                    uint64_t ulBits = 0;
                    float fError = 0.0f;
                    for (int32_t i = 0; i < 16; ++i) {
                        uint32_t uiBest = 0;
                        float fBest = 2.0f;
                        for (uint32_t j = 0; j < uiEntries; ++j) {
                            const float fDistance = std::fabs(afPalette[j] - apBlock[i].a);
                            if (fDistance < fBest) {
                                fBest = fDistance;
                                uiBest = j;
                            }
                        }
                        fError += apValid[i] ? fBest * fBest : 0.0f;
                        ulBits |= static_cast<uint64_t>(uiBest) << (3 * i);
                    }
                    if (fError < fBestError) {
                        fBestError = fError;
                        apDst[0] = aucEnds[iMode][0];
                        apDst[1] = aucEnds[iMode][1];
                        for (int32_t i = 0; i < 6; ++i) {
                            apDst[2 + i] = static_cast<uint8_t>(ulBits >> (8 * i));
                        }
                    }
                }
                apDst += 8;
            }
            EncodeColorBlock(apBlock, apValid, auiFormat == D3DFMT_DXT1, apDst);
        }

    } // namespace

    const FormatInfo* FindFormat(uint32_t aFormat) {
        for (const FormatInfo& rInfo : kFormats) {
            if (rInfo.format == aFormat) {
                return &rInfo;
            }
        }
        return nullptr;
    }

    uint32_t RowBytes(const FormatInfo& arInfo, uint32_t auiWidth) {
        if (arInfo.block) {
            return (auiWidth + 3) / 4 * arInfo.bits;
        }
        return auiWidth * arInfo.bits / 8;
    }

    uint32_t RowCount(const FormatInfo& arInfo, uint32_t auiHeight) {
        return arInfo.block ? (auiHeight + 3) / 4 : auiHeight;
    }

    void DecodeSlice(const FormatInfo& arInfo, const uint8_t* apSrc, uint32_t auiPitch, Image& arImage, uint32_t auiZ) {
        const uint32_t uiWidth = arImage.width;
        const uint32_t uiHeight = arImage.height;
        if (arInfo.block) {
            Color akBlock[16];
            for (uint32_t by = 0; by < (uiHeight + 3) / 4; ++by) {
                const uint8_t* pRow = apSrc + by * auiPitch;
                for (uint32_t bx = 0; bx < (uiWidth + 3) / 4; ++bx) {
                    DecodeBlock(arInfo.format, pRow + bx * arInfo.bits, akBlock);
                    for (uint32_t y = 0; y < 4 && by * 4 + y < uiHeight; ++y) {
                        for (uint32_t x = 0; x < 4 && bx * 4 + x < uiWidth; ++x) {
                            arImage.at(bx * 4 + x, by * 4 + y, auiZ) = akBlock[y * 4 + x];
                        }
                    }
                }
            }
            return;
        }
        const uint32_t uiBytes = arInfo.bits / 8;
        for (uint32_t y = 0; y < uiHeight; ++y) {
            const uint8_t* pPixel = apSrc + y * auiPitch;
            for (uint32_t x = 0; x < uiWidth; ++x, pPixel += uiBytes) {
                uint32_t uiPixel = 0;
                for (uint32_t i = 0; i < uiBytes; ++i) {
                    uiPixel |= static_cast<uint32_t>(pPixel[i]) << (8 * i);
                }
                Color& rColor = arImage.at(x, y, auiZ);
                rColor.r = arInfo.rMask ? Unpack(uiPixel, arInfo.rMask) : 0.0f;
                rColor.g = arInfo.luminance ? rColor.r : (arInfo.gMask ? Unpack(uiPixel, arInfo.gMask) : 0.0f);
                rColor.b = arInfo.luminance ? rColor.r : (arInfo.bMask ? Unpack(uiPixel, arInfo.bMask) : 0.0f);
                rColor.a = arInfo.aMask ? Unpack(uiPixel, arInfo.aMask) : 1.0f;
            }
        }
    }

    void EncodeSlice(
        const FormatInfo& arInfo,
        const Image& arImage,
        uint32_t auiZ,
        uint8_t* apDst,
        uint32_t auiPitch,
        bool abDither
    ) {
        const uint32_t uiWidth = arImage.width;
        const uint32_t uiHeight = arImage.height;
        if (arInfo.block) {
            Color akBlock[16];
            bool abValid[16];
            for (uint32_t by = 0; by < (uiHeight + 3) / 4; ++by) {
                uint8_t* pRow = apDst + by * auiPitch;
                for (uint32_t bx = 0; bx < (uiWidth + 3) / 4; ++bx) {
                    for (uint32_t y = 0; y < 4; ++y) {
                        for (uint32_t x = 0; x < 4; ++x) {
                            const bool bValid = bx * 4 + x < uiWidth && by * 4 + y < uiHeight;
                            abValid[y * 4 + x] = bValid;
                            akBlock[y * 4 + x] = bValid ? arImage.at(bx * 4 + x, by * 4 + y, auiZ) : Color{0, 0, 0, 1};
                        }
                    }
                    EncodeBlock(arInfo.format, akBlock, abValid, pRow + bx * arInfo.bits);
                }
            }
            return;
        }
        const uint32_t uiBytes = arInfo.bits / 8;
        // An absent alpha channel's bits (X8R8G8B8's top byte) are written as ones.
        uint32_t uiUnused = (uiBytes == 4 ? 0xFFFFFFFFu : (1u << (8 * uiBytes)) - 1) &
            ~(arInfo.rMask | arInfo.gMask | arInfo.bMask | arInfo.aMask);
        for (uint32_t y = 0; y < uiHeight; ++y) {
            uint8_t* pPixel = apDst + y * auiPitch;
            for (uint32_t x = 0; x < uiWidth; ++x, pPixel += uiBytes) {
                const Color& rColor = arImage.at(x, y, auiZ);
                const float fOffset = abDither ? DitherOffset(x, y) : 0.5f;
                uint32_t uiPixel = uiUnused;
                if (arInfo.luminance) {
                    // d3dx9's luminance weights (Rec. 709).
                    uiPixel |=
                        Pack(rColor.r * 0.2125f + rColor.g * 0.7154f + rColor.b * 0.0721f, arInfo.rMask, fOffset);
                }
                else {
                    uiPixel |= arInfo.rMask ? Pack(rColor.r, arInfo.rMask, fOffset) : 0;
                    uiPixel |= arInfo.gMask ? Pack(rColor.g, arInfo.gMask, fOffset) : 0;
                    uiPixel |= arInfo.bMask ? Pack(rColor.b, arInfo.bMask, fOffset) : 0;
                }
                uiPixel |= arInfo.aMask ? Pack(rColor.a, arInfo.aMask, fOffset) : 0;
                for (uint32_t i = 0; i < uiBytes; ++i) {
                    pPixel[i] = static_cast<uint8_t>(uiPixel >> (8 * i));
                }
            }
        }
    }

    Image BoxDown(const Image& arSrc) {
        Image kDst;
        kDst.width = arSrc.width > 1 ? arSrc.width / 2 : 1;
        kDst.height = arSrc.height > 1 ? arSrc.height / 2 : 1;
        kDst.depth = arSrc.depth > 1 ? arSrc.depth / 2 : 1;
        kDst.pixels.resize(static_cast<size_t>(kDst.width) * kDst.height * kDst.depth);
        const uint32_t uiSpanX = arSrc.width > 1 ? 2 : 1;
        const uint32_t uiSpanY = arSrc.height > 1 ? 2 : 1;
        const uint32_t uiSpanZ = arSrc.depth > 1 ? 2 : 1;
        const float fScale = 1.0f / static_cast<float>(uiSpanX * uiSpanY * uiSpanZ);
        for (uint32_t z = 0; z < kDst.depth; ++z) {
            for (uint32_t y = 0; y < kDst.height; ++y) {
                for (uint32_t x = 0; x < kDst.width; ++x) {
                    Color kSum{0, 0, 0, 0};
                    for (uint32_t dz = 0; dz < uiSpanZ; ++dz) {
                        for (uint32_t dy = 0; dy < uiSpanY; ++dy) {
                            for (uint32_t dx = 0; dx < uiSpanX; ++dx) {
                                const Color& rColor = arSrc.at(x * uiSpanX + dx, y * uiSpanY + dy, z * uiSpanZ + dz);
                                kSum.r += rColor.r;
                                kSum.g += rColor.g;
                                kSum.b += rColor.b;
                                kSum.a += rColor.a;
                            }
                        }
                    }
                    kDst.at(x, y, z) = {kSum.r * fScale, kSum.g * fScale, kSum.b * fScale, kSum.a * fScale};
                }
            }
        }
        return kDst;
    }

} // namespace d3dx
