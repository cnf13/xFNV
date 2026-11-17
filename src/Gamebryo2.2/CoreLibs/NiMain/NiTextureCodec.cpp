#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureCodec.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"

// The colour decoder slot holds DecodeColorBlock, called with the codec as its first argument. The lookup table
// expands each byte's two 4-bit explicit alpha values to 0..255.
NiTextureCodec::NiTextureCodec()
  : m_pfnAlphaBlockDecoder(nullptr),
    m_pfnColorBlockDecoder([](NiTextureCodec* apThis, uint8_t* apDest) { apThis->DecodeColorBlock(apDest); }),
    m_pucSrcPtr(nullptr), m_uiDestRowStride(0) {
    for (uint32_t i = 0; i < 256; ++i) {
        m_aaucExplicitAlphaLookup[i][0] = static_cast<uint8_t>(static_cast<int32_t>((i & 15) / 15.0 * 255.0));
        m_aaucExplicitAlphaLookup[i][1] = static_cast<uint8_t>(static_cast<int32_t>((i >> 4) / 15.0 * 255.0));
    }
}

void NiTextureCodec::DecodeColorBlock(uint8_t* pucDest) {
    uint16_t usColor0 = static_cast<uint16_t>((static_cast<uint16_t>(m_pucSrcPtr[1]) << 8) + m_pucSrcPtr[0]);
    m_pucSrcPtr += 2;
    uint16_t usColor1 = static_cast<uint16_t>((static_cast<uint16_t>(m_pucSrcPtr[1]) << 8) + m_pucSrcPtr[0]);
    m_pucSrcPtr += 2;

    // The green mask is deliberately 0x07F0 (equivalent to `>> 3 & 0xFE`), as the game does.
    uint8_t aaucColors[4][3];
    aaucColors[0][0] = static_cast<uint8_t>((usColor0 & 0xF800) >> 8);
    aaucColors[0][1] = static_cast<uint8_t>((usColor0 & 0x07F0) >> 3);
    aaucColors[0][2] = static_cast<uint8_t>((usColor0 & 0x001F) << 3);

    aaucColors[1][0] = static_cast<uint8_t>((usColor1 & 0xF800) >> 8);
    aaucColors[1][1] = static_cast<uint8_t>((usColor1 & 0x07F0) >> 3);
    aaucColors[1][2] = static_cast<uint8_t>((usColor1 & 0x001F) << 3);

    aaucColors[2][0] = static_cast<uint8_t>(
        ((static_cast<uint32_t>(aaucColors[0][0]) << 1) + static_cast<uint32_t>(aaucColors[1][0])) / 3
    );
    aaucColors[2][1] = static_cast<uint8_t>(
        ((static_cast<uint32_t>(aaucColors[0][1]) << 1) + static_cast<uint32_t>(aaucColors[1][1])) / 3
    );
    aaucColors[2][2] = static_cast<uint8_t>(
        ((static_cast<uint32_t>(aaucColors[0][2]) << 1) + static_cast<uint32_t>(aaucColors[1][2])) / 3
    );

    aaucColors[3][0] = static_cast<uint8_t>(
        ((static_cast<uint32_t>(aaucColors[1][0]) << 1) + static_cast<uint32_t>(aaucColors[0][0])) / 3
    );
    aaucColors[3][1] = static_cast<uint8_t>(
        ((static_cast<uint32_t>(aaucColors[1][1]) << 1) + static_cast<uint32_t>(aaucColors[0][1])) / 3
    );
    aaucColors[3][2] = static_cast<uint8_t>(
        ((static_cast<uint32_t>(aaucColors[1][2]) << 1) + static_cast<uint32_t>(aaucColors[0][2])) / 3
    );

    for (uint32_t uiBlockRow = 0; uiBlockRow < 4; uiBlockRow++) {
        uint8_t ucPixel = *m_pucSrcPtr;
        for (uint32_t uiBlockCol = 0; uiBlockCol < 4; uiBlockCol++) {
            const uint8_t* pucFinalPixel = aaucColors[ucPixel & 0x03];
            pucDest[0] = pucFinalPixel[0];
            pucDest[1] = pucFinalPixel[1];
            pucDest[2] = pucFinalPixel[2];
            pucDest += DEST_PIXEL_STRIDE;
            ucPixel >>= 2;
        }
        m_pucSrcPtr++;
    }
}

uint8_t* NiTextureCodec::DecodeT3AlphaBlock(uint8_t* pucAlphas) {
    *pucAlphas = m_aaucExplicitAlphaLookup[*m_pucSrcPtr][0u];
    pucAlphas[4] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr++][1];
    pucAlphas[8] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr][0u];
    pucAlphas[12] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr++][1];
    pucAlphas[16] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr][0u];
    pucAlphas[20] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr++][1];
    pucAlphas[24] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr][0u];
    pucAlphas[28] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr++][1];
    pucAlphas[32] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr][0u];
    pucAlphas[36] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr++][1];
    pucAlphas[40] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr][0u];
    pucAlphas[44] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr++][1];
    pucAlphas[48] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr][0u];
    pucAlphas[52] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr++][1];
    pucAlphas[56] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr][0u];
    pucAlphas[60] = m_aaucExplicitAlphaLookup[*m_pucSrcPtr++][1];
    return pucAlphas + 56;
}

// Callers go through m_pfnAlphaBlockDecoder and ignore the return value, which holds the last alpha written,
// zero-extended.
int32_t NiTextureCodec::DecodeT5AlphaBlock(uint8_t* pucAlphas) {
    // The eight interpolated alphas, one stack array the 3-bit indices select from.
    uint8_t aucAlpha[8];
    const uint8_t* m_pucSrcPtr = this->m_pucSrcPtr;
    uint8_t v3 = *m_pucSrcPtr++;
    this->m_pucSrcPtr = m_pucSrcPtr;
    aucAlpha[0] = v3;
    uint8_t v7 = *m_pucSrcPtr;
    aucAlpha[1] = v7;
    uint32_t v4 = v7;
    this->m_pucSrcPtr = m_pucSrcPtr + 1;
    if (v3 <= v4) {
        aucAlpha[2] = (v4 + 4 * v3) / 5;
        aucAlpha[3] = (3 * v3 + 2 * v4) / 5;
        aucAlpha[4] = (3 * v4 + 2 * v3) / 5;
        aucAlpha[5] = (v3 + 4 * v4) / 5;
        aucAlpha[6] = 0;
        aucAlpha[7] = 0xFF;
    }
    else {
        aucAlpha[2] = (v4 + 6 * v3) / 7;
        aucAlpha[3] = (5 * v3 + 2 * v4) / 7;
        aucAlpha[4] = (3 * v4 + 4 * v3) / 7;
        aucAlpha[5] = (3 * v3 + 4 * v4) / 7;
        aucAlpha[6] = (5 * v4 + 2 * v3) / 7;
        aucAlpha[7] = (v3 + 6 * v4) / 7;
    }
    pucAlphas[0] = aucAlpha[*this->m_pucSrcPtr & 7];
    pucAlphas[4] = aucAlpha[*this->m_pucSrcPtr >> 3 & 7];
    pucAlphas[8] = aucAlpha[4 * (this->m_pucSrcPtr[1] & 1) + (*this->m_pucSrcPtr >> 6)];
    pucAlphas[12] = aucAlpha[this->m_pucSrcPtr[1] >> 1 & 7];
    pucAlphas[16] = aucAlpha[this->m_pucSrcPtr[1] >> 4 & 7];
    pucAlphas[20] = aucAlpha[2 * (this->m_pucSrcPtr[2] & 3) + (this->m_pucSrcPtr[1] >> 7)];
    pucAlphas[24] = aucAlpha[this->m_pucSrcPtr[2] >> 2 & 7];
    pucAlphas[28] = aucAlpha[this->m_pucSrcPtr[2] >> 5];
    this->m_pucSrcPtr += 3;
    pucAlphas[32] = aucAlpha[*this->m_pucSrcPtr & 7];
    pucAlphas[36] = aucAlpha[*this->m_pucSrcPtr >> 3 & 7];
    pucAlphas[40] = aucAlpha[4 * (this->m_pucSrcPtr[1] & 1) + (*this->m_pucSrcPtr >> 6)];
    pucAlphas[44] = aucAlpha[this->m_pucSrcPtr[1] >> 1 & 7];
    pucAlphas[48] = aucAlpha[this->m_pucSrcPtr[1] >> 4 & 7];
    pucAlphas[52] = aucAlpha[2 * (this->m_pucSrcPtr[2] & 3) + (this->m_pucSrcPtr[1] >> 7)];
    pucAlphas[56] = aucAlpha[this->m_pucSrcPtr[2] >> 2 & 7];
    int32_t result = aucAlpha[this->m_pucSrcPtr[2] >> 5];
    pucAlphas[60] = result;
    this->m_pucSrcPtr += 3;
    return result;
}

// The four RGBA colours the 2-bit indices select from.
int32_t NiTextureCodec::DecodeColorAlphaBlock(uint8_t* a1) {
    uint8_t aaucColor[4][4];
    const uint8_t* m_pucSrcPtr = this->m_pucSrcPtr;
    uint16_t v4 = *m_pucSrcPtr + (m_pucSrcPtr[1] << 8);
    m_pucSrcPtr += 2;
    this->m_pucSrcPtr = m_pucSrcPtr;
    uint16_t v8 = *m_pucSrcPtr + (m_pucSrcPtr[1] << 8);
    this->m_pucSrcPtr = m_pucSrcPtr + 2;
    aaucColor[0][0] = static_cast<uint8_t>(v4 >> 8) & 0xF8;
    aaucColor[0][1] = v4 >> 3 & 0xFE;
    aaucColor[0][2] = 8 * v4;
    aaucColor[0][3] = 0xFF;
    aaucColor[1][0] = static_cast<uint8_t>(v8 >> 8) & 0xF8;
    aaucColor[1][1] = v8 >> 3 & 0xFE;
    aaucColor[1][2] = 8 * v8;
    aaucColor[1][3] = 0xFF;
    aaucColor[2][3] = 0xFF;
    if (v4 <= v8) {
        for (int32_t i = 0; i < 3; ++i) {
            aaucColor[2][i] = (aaucColor[0][i] + static_cast<uint32_t>(aaucColor[1][i])) >> 1;
            aaucColor[3][i] = aaucColor[2][i];
        }
        aaucColor[3][3] = 0;
    }
    else {
        for (int32_t i = 0; i < 3; ++i) {
            aaucColor[2][i] = (aaucColor[1][i] + 2 * static_cast<uint32_t>(aaucColor[0][i])) / 3;
            aaucColor[3][i] = (aaucColor[0][i] + 2 * static_cast<uint32_t>(aaucColor[1][i])) / 3;
        }
        aaucColor[3][3] = 0xFF;
    }
    int32_t result = 0;
    for (int32_t iRow = 4; iRow; --iRow) {
        uint8_t ucIndices = *this->m_pucSrcPtr;
        for (int32_t iPixel = 0; iPixel < 4; ++iPixel) {
            const uint8_t* pucColor = aaucColor[ucIndices >> (2 * iPixel) & 3];
            a1[4 * iPixel] = pucColor[0];
            a1[4 * iPixel + 1] = pucColor[1];
            a1[4 * iPixel + 2] = pucColor[2];
            a1[4 * iPixel + 3] = pucColor[3];
            result = pucColor[3];
        }
        ++this->m_pucSrcPtr;
        a1 += 16;
    }
    return result;
}

NiPixelData* NiTextureCodec::ExpandCompressedData(const NiPixelData& kSrc) {
    switch (kSrc.m_kPixelFormat.m_eFormat) {
        case NiPixelFormat::FORMAT_DXT1:
            m_pfnAlphaBlockDecoder = nullptr;
            m_pfnColorBlockDecoder = &fnvd::callMember<&NiTextureCodec::DecodeColorAlphaBlock>;
            break;
        case NiPixelFormat::FORMAT_DXT3:
            m_pfnAlphaBlockDecoder = &fnvd::callMember<&NiTextureCodec::DecodeT3AlphaBlock>;
            m_pfnColorBlockDecoder = &fnvd::callMember<&NiTextureCodec::DecodeColorBlock>;
            break;
        case NiPixelFormat::FORMAT_DXT5:
            m_pfnAlphaBlockDecoder = &fnvd::callMember<&NiTextureCodec::DecodeT5AlphaBlock>;
            m_pfnColorBlockDecoder = &fnvd::callMember<&NiTextureCodec::DecodeColorBlock>;
            break;
        default: return nullptr;
    }

    NiPixelData* pkDest = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
    if (pkDest) {
        pkDest = ::new (pkDest) NiPixelData(
            *kSrc.m_puiWidth, *kSrc.m_puiHeight, NiPixelFormat_RGBA32, kSrc.m_uiMipmapLevels, kSrc.m_uiFaces
        );
    }

    uint32_t uiMipmapLevels = kSrc.m_uiMipmapLevels;
    uint32_t uiNumFaces = kSrc.m_uiFaces;
    for (uint32_t uiFace = 0; uiFace < uiNumFaces; uiFace++) {
        for (uint32_t i = 0; i < uiMipmapLevels; i++) {
            m_pucSrcPtr =
                &kSrc.m_pucPixels[kSrc.m_puiOffsetInBytes[i] + uiFace * kSrc.m_puiOffsetInBytes[kSrc.m_uiMipmapLevels]];
            uint8_t* pucDestPixels =
                &pkDest->m_pucPixels
                     [pkDest->m_puiOffsetInBytes[i] + uiFace * pkDest->m_puiOffsetInBytes[pkDest->m_uiMipmapLevels]];

            uint32_t uiWidth = kSrc.m_puiWidth[i];
            uint32_t uiHeight = kSrc.m_puiHeight[i];

            uint32_t uiBlockCols = uiWidth >> 2;
            uint32_t uiBlockRows = uiHeight >> 2;
            uint32_t uiExtraCols = uiWidth & 0x3;
            uint32_t uiExtraRows = uiHeight & 0x3;

            m_uiDestRowStride = uiWidth * DEST_PIXEL_STRIDE;

            uint32_t uiDestCol;
            for (uint32_t uiDestRow = 0; uiDestRow < uiBlockRows; uiDestRow++) {
                for (uiDestCol = 0; uiDestCol < uiBlockCols; uiDestCol++) {
                    // EXE-INLINED 0xA8BDF0
                    // DecodeBlock(pucDestPixels, 4, 4), its 16-byte row copies inline (four dword moves).
                    uint8_t aucColorBlock[DEST_PIXEL_STRIDE * 16];
                    uint8_t* pucColor = aucColorBlock;
                    if (m_pfnAlphaBlockDecoder) {
                        m_pfnAlphaBlockDecoder(this, pucColor + 3);
                    }
                    m_pfnColorBlockDecoder(this, pucColor);
                    uint8_t* pucRow = pucDestPixels;
                    for (uint32_t uiBlockRow = 0; uiBlockRow < 4; uiBlockRow++) {
                        for (uint32_t uiByte = 0; uiByte < DEST_PIXEL_STRIDE * 4; uiByte++) {
                            pucRow[uiByte] = pucColor[uiByte];
                        }
                        pucRow += m_uiDestRowStride;
                        pucColor += DEST_PIXEL_STRIDE << 2;
                    }

                    pucDestPixels += DEST_PIXEL_STRIDE << 2;
                }

                if (uiExtraCols) {
                    DecodeBlock(pucDestPixels, uiExtraCols, 4);
                    pucDestPixels += uiExtraCols * DEST_PIXEL_STRIDE;
                }

                pucDestPixels += 3 * uiWidth * DEST_PIXEL_STRIDE;
            }

            if (uiExtraRows) {
                for (uiDestCol = 0; uiDestCol < uiBlockCols; uiDestCol++) {
                    DecodeBlock(pucDestPixels, 4, uiExtraRows);
                    pucDestPixels += DEST_PIXEL_STRIDE << 2;
                }

                if (uiExtraCols) {
                    DecodeBlock(pucDestPixels, uiExtraCols, uiExtraRows);
                }
            }
        }
    }

    return pkDest;
}

void NiTextureCodec::DecodeBlock(uint8_t* pucDest, uint32_t uiCols, uint32_t uiRows) {
    uint8_t aucColorBlock[DEST_PIXEL_STRIDE * 16];
    uint8_t* pucColor = aucColorBlock;

    if (m_pfnAlphaBlockDecoder) {
        m_pfnAlphaBlockDecoder(this, pucColor + 3);
    }
    m_pfnColorBlockDecoder(this, pucColor);

    uint32_t uiDestRowLength = uiCols * DEST_PIXEL_STRIDE;
    for (uint32_t uiBlockRow = 0; uiBlockRow < uiRows; uiBlockRow++) {
        rt::memcpy(pucDest, pucColor, uiDestRowLength);
        pucDest += m_uiDestRowStride;
        pucColor += DEST_PIXEL_STRIDE << 2;
    }
}
