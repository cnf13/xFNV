#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

#include "Gamebryo2.2/CoreLibs/NiMain/BSDegradePriority.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "SDK/MSVC/stdlib.h"

NiTexture::RendererData::TEXTUREFAILUREFUNC NiTexture::RendererData::ms_pfnTextureFailureCallback{
    &NiTexture::RendererData::DefaultTextureFailureCallback
};
NiRTTI NiTexture::ms_RTTI("NiTexture", &NiObjectNET::ms_RTTI);
NiCriticalSection NiTexture::ms_kTextureListCriticalSection;
NiTexture::RendererData::ReplacementDataArray NiTexture::RendererData::ms_aspReplacementData;
NiPixelFormat NiTexture::RendererData::ms_kReplacementFormat;

NiTexture::RendererData::~RendererData() {}

// The renderer data adds no alias memory of its own.
uint32_t NiTexture::RendererData::ComputeAliasSize() {
    return 0;
}

void NiTexture::RendererData::_SDMInit() {
    ms_kReplacementFormat = NiPixelFormat_RGBA32;
}

// Each dropped texture goes through DeleteThis.
void NiTexture::RendererData::_SDMShutdown() {
    for (uint32_t ui = 0; ui < 10; ++ui) {
        ms_aspReplacementData[ui] = nullptr;
    }
}

void NiTexture::RendererData::SetTextureReplacementFormat(const NiPixelFormat& kFormat) {
    ms_kReplacementFormat = kFormat;
    for (uint32_t i = 0; i < 10; ++i) {
        ms_aspReplacementData[i] = nullptr;
    }
}

NiPixelData* NiTexture::RendererData::GetReplacementData(NiTexture* pkTexture, CreationFailure eFailure) {
    // Tests the sign: every failure value is negative.
    if (eFailure >= NONE) {
        return nullptr;
    }
    TEXTUREFAILUREFUNC pfnCallback = ms_pfnTextureFailureCallback;
    if (!pfnCallback) {
        pfnCallback = &NiTexture::RendererData::DefaultTextureFailureCallback;
        ms_pfnTextureFailureCallback = pfnCallback;
    }
    return pfnCallback(pkTexture, eFailure);
}

// KEEP: each memcpy copies ucBitsPerPixel / 8 bytes (up to 31) from the 8-byte colour pair in the frame (0xA62611
// `lea eax, [esp+edx*4+0x20]`), so a format wider than 32 bits for the second colour, or 64 for the first, reads past
// it into the exe's frame (return address, argument slots reused as loop counters): an uninitialised read.
NiPixelData* NiTexture::RendererData::MakeDataFromColors(
    const NiColorA& kTopLeftColor,
    const NiColorA& kBottomLeftColor,
    const NiPixelFormat& kDesc,
    uint32_t uiNumFaces
) {
    NiPixelData* pkData;
    uint32_t auiColors[2];
    NiPixelData* v4 = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
    if (v4) {
        pkData = ::new (v4) NiPixelData(32u, 32u, kDesc, 1u, uiNumFaces);
    }
    else {
        pkData = nullptr;
    }
    uint8_t Shift = kDesc.GetShift(NiPixelFormat::COMP_RED);
    uint32_t uiRed = static_cast<uint32_t>(static_cast<int64_t>(kTopLeftColor.r * 255.0));
    uint32_t v6 = uiRed << Shift & kDesc.GetMask(NiPixelFormat::COMP_RED);
    uint8_t v7 = kDesc.GetShift(NiPixelFormat::COMP_GREEN);
    uint32_t uiGreen = static_cast<uint32_t>(static_cast<int64_t>(kTopLeftColor.g * 255.0));
    uint32_t v8 = (uiGreen << v7 & kDesc.GetMask(NiPixelFormat::COMP_GREEN)) | v6;
    uint8_t v9 = kDesc.GetShift(NiPixelFormat::COMP_BLUE);
    uint32_t uiBlue = static_cast<uint32_t>(static_cast<int64_t>(kTopLeftColor.b * 255.0));
    uint32_t v10 = (uiBlue << v9 & kDesc.GetMask(NiPixelFormat::COMP_BLUE)) | v8;
    uint8_t v11 = kDesc.GetShift(NiPixelFormat::COMP_ALPHA);
    uint32_t uiAlpha = static_cast<uint32_t>(static_cast<int64_t>(kTopLeftColor.a * 255.0));
    auiColors[0] = (uiAlpha << v11 & kDesc.GetMask(NiPixelFormat::COMP_ALPHA)) | v10;
    uint32_t v12 = static_cast<uint32_t>(static_cast<int64_t>(kBottomLeftColor.r * 255.0))
        << kDesc.GetShift(NiPixelFormat::COMP_RED);
    uint32_t v13 = v12 & kDesc.GetMask(NiPixelFormat::COMP_RED);
    uint32_t v14 = static_cast<uint32_t>(static_cast<int64_t>(kBottomLeftColor.g * 255.0))
        << kDesc.GetShift(NiPixelFormat::COMP_GREEN);
    uint32_t v15 = (v14 & kDesc.GetMask(NiPixelFormat::COMP_GREEN)) | v13;
    uint32_t v16 = static_cast<uint32_t>(static_cast<int64_t>(kBottomLeftColor.b * 255.0))
        << kDesc.GetShift(NiPixelFormat::COMP_BLUE);
    uint32_t v17 = (v16 & kDesc.GetMask(NiPixelFormat::COMP_BLUE)) | v15;
    uint32_t v18 = static_cast<uint32_t>(static_cast<int64_t>(kBottomLeftColor.a * 255.0))
        << kDesc.GetShift(NiPixelFormat::COMP_ALPHA);
    auiColors[1] = (v18 & kDesc.GetMask(NiPixelFormat::COMP_ALPHA)) | v17;
    for (uint32_t kBottomLeftColora = 0; kBottomLeftColora < uiNumFaces; ++kBottomLeftColora) {
        uint8_t* v19 = &pkData->m_pucPixels
                            [*pkData->m_puiOffsetInBytes +
                             kBottomLeftColora * pkData->m_puiOffsetInBytes[pkData->m_uiMipmapLevels]];
        uint32_t kTopLeftColora = 0;
        size_t v20 = kDesc.m_ucBitsPerPixel >> 3;
        do {
            uint32_t v21 = kTopLeftColora >> 4;
            uint32_t v22 = 0;
            while (true) {
                rt::memcpy(v19, &auiColors[v21 != v22++ >> 4], v20);
                v19 += v20;
                if (v22 >= 32) {
                    break;
                }
                v21 = kTopLeftColora >> 4;
            }
            ++kTopLeftColora;
        }
        while (kTopLeftColora < 32);
    }
    return pkData;
}

NiPixelData* NiTexture::RendererData::DefaultTextureFailureCallback(
    NiTexture* pkTexture,
    NiTexture::RendererData::CreationFailure eFailure
) {
    FNVD_INLINED_LOCAL(NiColorA, kBottomLeft_2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiColorA, kTopLeft, std::nullopt, std::nullopt);
    // A failure value is its index with the top bit set; the top bit is taken off before indexing the table.
    const uint32_t uiIndex = eFailure & 0x7FFFFFFFu;
    if (uiIndex >= 10) {
        return nullptr;
    }
    NiPointer<NiPixelData>* v3 = &NiTexture::RendererData::ms_aspReplacementData[uiIndex];
    uint32_t uiNumFaces = 1;
    if (!v3->m_pObject) {
        float v5;
        float v6;
        float v7;
        double v8;
        double v9;
        float a;
        float b;
        FNVD_INLINED_LOCAL(NiColorA, kBottomLeft, std::nullopt, std::nullopt);
        switch (eFailure) {
            case NiTexture::RendererData::NO2DMATCH: {
                kTopLeft.r = 0.0f;
                kTopLeft.g = 0.0f;
                kBottomLeft.r = 1.0f;
                kBottomLeft.g = 1.0f;
                kBottomLeft.a = 1.0f;
                kTopLeft.b = 1.0f;
                kTopLeft.a = 1.0f;
                kBottomLeft.b = 0.0f;
                kBottomLeft_2.r = 1.0f;
                kBottomLeft_2.g = 1.0f;
                kBottomLeft_2.b = 0.0f;
                kBottomLeft_2.a = 1.0f;
                goto LABEL_18;
            }
            case NiTexture::RendererData::NOCUBEMAPMATCH: {
                kTopLeft.r = 0.0f;
                v5 = 1.0f;
                v6 = 1.0f;
                v7 = 0.0f;
                kBottomLeft.r = 1.0f;
                kBottomLeft.a = 1.0f;
                kBottomLeft.g = 0.0f;
                kBottomLeft.b = 0.0f;
                goto LABEL_7;
            }
            case NiTexture::RendererData::NOCONVERTER: {
                v8 = 0.0;
                kBottomLeft.r = 0.0f;
                v9 = 1.0;
                kBottomLeft.g = 1.0f;
                kBottomLeft.b = 1.0f;
                goto LABEL_9;
            }
            case NiTexture::RendererData::FAILEDCONVERT: {
                kBottomLeft.r = 1.0f;
                kBottomLeft.g = 0.0f;
                kBottomLeft.b = 0.0f;
                v9 = 1.0;
                v8 = 0.0;
            LABEL_9:;
                float g = kBottomLeft.g;
                kBottomLeft.a = v9;
                float r = kBottomLeft.r;
                kBottomLeft.r = v9;
                kBottomLeft.g = v9;
                kTopLeft.g = g;
                a = kBottomLeft.a;
                kBottomLeft.a = v9;
                kTopLeft.r = r;
                b = kBottomLeft.b;
                kBottomLeft.b = v8;
                goto LABEL_16;
            }
            case NiTexture::RendererData::BADDIMENSIONS: {
                kTopLeft.r = 0.0f;
                b = 1.0f;
                kBottomLeft.r = 0.0f;
                kTopLeft.g = 1.0f;
                a = 1.0f;
                kBottomLeft.g = 1.0f;
                kBottomLeft.b = 1.0f;
                kBottomLeft.a = 1.0f;
                goto LABEL_16;
            }
            case NiTexture::RendererData::CUBEMAPFAILEDCONVERT: {
                kTopLeft.r = 0.0f;
                kTopLeft.g = 1.0f;
                kBottomLeft.r = 1.0f;
                kBottomLeft.g = 1.0f;
                kTopLeft.a = 1.0f;
                kBottomLeft.a = 1.0f;
                kTopLeft.b = 0.0f;
                kBottomLeft_2.r = 1.0f;
                kBottomLeft.b = 0.0f;
                kBottomLeft_2.b = 0.0f;
                kBottomLeft_2.g = 1.0f;
                kBottomLeft_2.a = 1.0f;
                uiNumFaces = 6;
                goto LABEL_18;
            }
            case NiTexture::RendererData::CUBEMAPFILENOTFOUND: {
                kTopLeft.r = 0.0f;
                v5 = 1.0f;
                v6 = 1.0f;
                v7 = 0.0f;
                kBottomLeft.r = 1.0f;
                kBottomLeft.b = 1.0f;
                kBottomLeft.a = 1.0f;
                kBottomLeft.g = 0.0f;
            LABEL_7:;
                kTopLeft.g = v5;
                kBottomLeft_2 = kBottomLeft;
                kTopLeft.a = v6;
                kTopLeft.b = v7;
                uiNumFaces = 6;
                goto LABEL_18;
            }
            case NiTexture::RendererData::CUBEMAPBADDIMENSIONS: {
                kTopLeft.r = 0.0f;
                kBottomLeft.r = 0.0f;
                kTopLeft.g = 1.0f;
                kTopLeft.b = 0.0f;
                kBottomLeft.g = 1.0f;
                kBottomLeft.b = 1.0f;
                kBottomLeft_2.g = 1.0f;
                kBottomLeft.a = 1.0f;
                kTopLeft.a = 1.0f;
                kBottomLeft_2.r = 0.0f;
                uiNumFaces = 6;
                goto LABEL_17;
            }
            default: {
                kTopLeft.r = 1.0f;
                kBottomLeft.b = 1.0f;
                kTopLeft.g = 0.0f;
                b = 1.0f;
                kBottomLeft.r = 1.0f;
                a = 1.0f;
                kBottomLeft.a = 1.0f;
                kBottomLeft.g = 0.0f;
            LABEL_16:;
                kTopLeft.a = a;
                kTopLeft.b = b;
                kBottomLeft_2.g = kBottomLeft.g;
                kBottomLeft_2.r = kBottomLeft.r;
            LABEL_17:;
                kBottomLeft_2.b = kBottomLeft.b;
                kBottomLeft_2.a = kBottomLeft.a;
            LABEL_18:;
                v3->SetNew(
                    NiTexture::RendererData::MakeDataFromColors(
                        kTopLeft, kBottomLeft_2, NiTexture::RendererData::ms_kReplacementFormat, uiNumFaces
                    )
                );
                break;
            }
        }
    }
    return v3->m_pObject;
}

// Every texture is on the global list, so that NiDX9TextureData::ClearTextureData releases its renderer data before
// the renderer goes away.
NiTexture::NiTexture() : m_pkRendererData(nullptr) {
    AddTextureToList();
}

NiTexture::~NiTexture() {
    if (m_pkRendererData) {
        delete m_pkRendererData;
    }
    RemoveTextureFromList();
}

void NiTexture::LoadBinary(NiStream& kStream) {
    NiObjectNET::LoadBinary(kStream);
}

// NiSourceTexture::LinkObject calls this slot body directly, so the qualified base call goes to that address.
void NiTexture::LinkObject(NiStream& kStream) {
    NiObjectNET::LinkObject(kStream);
}

bool NiTexture::RegisterStreamables(NiStream& kStream) {
    return NiObjectNET::RegisterStreamables(kStream);
}

void NiTexture::SaveBinary(NiStream& kStream) {
    NiObjectNET::SaveBinary(kStream);
}

bool NiTexture::IsEqual(NiObject* pkObject) {
    return NiObjectNET::IsEqual(pkObject);
}

void NiTexture::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    int32_t cState;
    NiObjectNET::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiTexture::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* v12 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, v12);
    NiTexture::RendererData* m_pkRendererData = this->m_pkRendererData;
    if (m_pkRendererData) {
        cState = m_pkRendererData->DegradePriorityRange.cState;
    }
    else {
        cState = 0;
    }
    char* ViewerString_5 = NiGetViewerString("State", cState);
    uint32_t v10 = pkStrings->m_usSize;
    uint32_t v11 = pkStrings->m_usMaxSize;
    v12 = ViewerString_5;
    if (v10 >= v11) {
        pkStrings->SetSize(v10 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v10, v12);
}

uint32_t NiTexture::GetMemorySize() {
    uint32_t result = 0;
    if (m_pkRendererData) {
        return m_pkRendererData->m_uiWidth *
            (m_pkRendererData->m_uiHeight *
             ((int32_t)(((uint32_t)((int32_t)m_pkRendererData->m_kPixelFormat.m_ucBitsPerPixel)) >> 3)));
    }
    return result;
}

void NiTexture::AddTextureToList() {
    ms_kTextureListCriticalSection.Lock();
    if (!ms_pkHead) {
        ms_pkHead = this;
    }
    if (ms_pkTail) {
        ms_pkTail->m_pkNext = this;
        m_pkPrev = ms_pkTail;
    }
    else {
        m_pkPrev = nullptr;
    }
    ms_pkTail = this;
    m_pkNext = nullptr;
    ms_kTextureListCriticalSection.Unlock();
}

void NiTexture::RemoveTextureFromList() {
    ms_kTextureListCriticalSection.Lock();
    if (ms_pkHead == this) {
        ms_pkHead = m_pkNext;
    }
    if (ms_pkTail == this) {
        ms_pkTail = m_pkPrev;
    }
    if (m_pkPrev) {
        m_pkPrev->m_pkNext = m_pkNext;
    }
    if (m_pkNext) {
        m_pkNext->m_pkPrev = m_pkPrev;
    }
    ms_kTextureListCriticalSection.Unlock();
}

NiTexture* NiTexture::GetListHead() {
    return NiTexture::ms_pkHead;
}

// The executable accumulates the shift and the component-width sum in byte registers, not bools.
uint8_t NiPixelFormat::GetShift(Component eComp) const {
    uint8_t ucShift = 0;
    for (uint32_t uiComponent = 0; uiComponent < NUM_COMPS; ++uiComponent) {
        if (m_akComponents[uiComponent].m_eComponent == eComp) {
            if ((m_uFlags & 1) == 0) {
                const uint8_t ucEnd = ucShift + m_akComponents[uiComponent].m_ucBitsPerComponent;
                return m_ucBitsPerPixel - ucEnd;
            }
            return ucShift;
        }
        ucShift += m_akComponents[uiComponent].m_ucBitsPerComponent;
    }
    return 0;
}
