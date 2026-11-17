#include "Havok/Source/Animation/Animation/Compression/Common/hkaProcessFlyingColors.h"
#include "Havok/Source/Animation/Animation/Compression/Quantization/hkaQuantizeReal.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"

// The SDK's file-local table (release hkaInternal.lib, hkaExpandReal.obj: `_oneOverTwoToBitWidthLookupTable`, 68 bytes
// of .rdata): 1 / 2^bitWidth for bit widths 0 to 16.
using oneOverTwoToBitWidthLookupTable_t = const float[17];
static oneOverTwoToBitWidthLookupTable_t oneOverTwoToBitWidthLookupTable{
    1.0f,
    0.5f,
    0.25f,
    0.125f,
    0.0625f,
    0.03125f,
    0.015625f,
    0.0078125f,
    0.00390625f,
    0.001953125f,
    0.0009765625f,
    0.00048828125f,
    0.000244140625f,
    0.0001220703125f,
    0.00006103515625f,
    0.000030517578125f,
    0.0000152587890625f
};

// The first m_preserved values are stored raw (four unaligned bytes each); the rest are m_bitWidth-bit codes, each
// mapped back to (code + 0.5) * scale + m_offset.
void hkExpandReal(const uint8_t* quantized, const hkaQuantizeDesc& desc, float* data, uint32_t nValues) {
    int32_t iPreserved = desc.m_preserved;
    for (int32_t i = 0; i < iPreserved; i++) {
        union {
            float r;
            uint8_t c[4];
        } raw;
        raw.c[0] = quantized[0];
        raw.c[1] = quantized[1];
        raw.c[2] = quantized[2];
        raw.c[3] = quantized[3];
        quantized += 4;
        data[i] = raw.r;
    }
    if (!HK_flyingcolors_animation) {
        hkaProcessFlyingColors(&HK_flyingcolors_animation);
        if (!HK_flyingcolors_animation) {
            return;
        }
    }
    uint8_t ucBitWidth = desc.m_bitWidth;
    float* pOut = &data[desc.m_preserved];
    uint32_t uiCount = nValues - desc.m_preserved;
    float fScale = oneOverTwoToBitWidthLookupTable[ucBitWidth] * desc.m_scale;
    if (ucBitWidth == 8) {
        for (uint32_t i = 0; i < uiCount; i++) {
            pOut[i] = (static_cast<double>(quantized[i]) + 0.5) * fScale + desc.m_offset;
        }
    }
    else if (ucBitWidth == 16) {
        for (uint32_t i = 0; i < uiCount; i++) {
            uint16_t usCode = static_cast<uint16_t>(quantized[1] << 8 | quantized[0]);
            quantized += 2;
            pOut[i] = (static_cast<double>(usCode) + 0.5) * fScale + desc.m_offset;
        }
    }
    else {
        // Codes are read from a bit buffer refilled sixteen bits (two bytes, little-endian) at a time.
        uint32_t uiMask = static_cast<uint16_t>((1 << ucBitWidth) - 1);
        uint32_t uiBuffer = 0;
        int32_t iBits = 0;
        uint32_t i = 0;
        while (i < uiCount) {
            while (iBits >= ucBitWidth) {
                if (i < uiCount) {
                    pOut[i] =
                        (static_cast<double>(static_cast<uint16_t>(uiBuffer & uiMask)) + 0.5) * fScale + desc.m_offset;
                }
                uiBuffer >>= ucBitWidth;
                iBits -= ucBitWidth;
                ++i;
            }
            uiBuffer |= static_cast<uint32_t>(static_cast<uint16_t>(quantized[1] << 8 | quantized[0])) << iBits;
            quantized += 2;
            iBits += 16;
        }
    }
}
