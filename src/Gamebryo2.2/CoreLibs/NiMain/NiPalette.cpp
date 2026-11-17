#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

NiRTTI NiPalette::ms_RTTI("NiPalette", &NiObject::ms_RTTI);
NiCriticalSection NiPalette::ms_kPaletteListCriticalSection;

// An empty palette, which joins the global list.
NiPalette::NiPalette()
  : m_bAlpha(false), m_uiEntries(0), m_uiRevID(1), m_pkPalette(nullptr), m_pkRendererData(nullptr) {
    AddPaletteToList();
}

// A copy of kPal's entries; the renderer builds its data, then the palette joins the global list.
NiPalette::NiPalette(const NiPalette& kPal)
  : m_bAlpha(kPal.m_bAlpha), m_uiEntries(kPal.m_uiEntries), m_uiRevID(1),
    m_pkPalette(
        static_cast<PaletteEntry*>(MemoryManager_Singleton.Allocate(
            kPal.m_uiEntries <= SIZE_MAX / sizeof(PaletteEntry) ? sizeof(PaletteEntry) * kPal.m_uiEntries : SIZE_MAX
        ))
    ),
    m_pkRendererData(nullptr) {
    rt::memcpy(m_pkPalette, kPal.m_pkPalette, sizeof(NiPalette::PaletteEntry) * kPal.m_uiEntries);
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->CreatePaletteRendererData(this);
    }
    AddPaletteToList();
}

// A copy of the uiEntries entries at pkEntries; the renderer builds its data, then the palette joins the global list.
NiPalette::NiPalette(bool bHasAlpha, uint32_t uiEntries, const PaletteEntry* pkEntries)
  : m_bAlpha(bHasAlpha), m_uiEntries(uiEntries), m_uiRevID(1),
    m_pkPalette(
        static_cast<PaletteEntry*>(MemoryManager_Singleton.Allocate(
            uiEntries <= SIZE_MAX / sizeof(PaletteEntry) ? sizeof(PaletteEntry) * uiEntries : SIZE_MAX
        ))
    ),
    m_pkRendererData(nullptr) {
    rt::memcpy(m_pkPalette, pkEntries, sizeof(NiPalette::PaletteEntry) * m_uiEntries);
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->CreatePaletteRendererData(this);
    }
    AddPaletteToList();
}

// The entries are freed, the renderer data deleted, then the palette leaves the global list.
NiPalette::~NiPalette() {
    MemoryManager_Singleton.Deallocate(m_pkPalette);
    if (m_pkRendererData) {
        delete m_pkRendererData;
    }
    RemovePaletteFromList();
}

// KEEP (niread): the exe reads the entry count into a local nothing initialised, which a short read leaves
// unwritten and the body then compares and stores; it also reads the alpha byte into the low byte of the stream
// argument's own stack slot, which src cannot express.
void NiPalette::LoadBinary(NiStream& kStream) {
    uint8_t ucAlpha;
    uint32_t uiEntries;
    uint32_t uiCount;
    NiObject::LoadBinary(kStream);
    NiBinaryStream* pkIstr = kStream.Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
    uiCount = 1;
    pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &ucAlpha, 1, &uiCount, 1);
    m_bAlpha = ucAlpha != 0;
    pkIstr = kStream.Istr();
    pfnRead = pkIstr->m_pfnRead;
    uiCount = 4;
    pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &uiEntries, 4, &uiCount, 1);
    if (uiEntries != m_uiEntries) {
        MemoryManager_Singleton.Deallocate(m_pkPalette);
        m_uiEntries = uiEntries;
        m_pkPalette = static_cast<NiPalette::PaletteEntry*>(MemoryManager_Singleton.Allocate(
            uiEntries <= SIZE_MAX / sizeof(NiPalette::PaletteEntry) ? sizeof(NiPalette::PaletteEntry) * uiEntries
                                                                    : SIZE_MAX
        ));
    }
    pkIstr = kStream.Istr();
    uint32_t uiSize = 4 * m_uiEntries;
    pfnRead = pkIstr->m_pfnRead;
    NiPalette::PaletteEntry* pkPalette = m_pkPalette;
    uiCount = 1;
    pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, pkPalette, uiSize, &uiCount, 1);
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->CreatePaletteRendererData(this);
    }
}

void NiPalette::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiPalette::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiPalette::SaveBinary(NiStream& kStream) {
    uint32_t uiCount;
    NiObject::SaveBinary(kStream);
    NiBinaryStream* pkOstr = kStream.Ostr();
    uint8_t ucAlpha = m_bAlpha;
    NiBinaryStream::NIBINARYSTREAMWRITEFN pfnWrite = pkOstr->m_pfnWrite;
    uiCount = 1;
    pkOstr->m_uiAbsoluteCurrentPos += pfnWrite(pkOstr, &ucAlpha, 1, &uiCount, 1);
    pkOstr = kStream.Ostr();
    pfnWrite = pkOstr->m_pfnWrite;
    uiCount = 4;
    pkOstr->m_uiAbsoluteCurrentPos += pfnWrite(pkOstr, &m_uiEntries, 4, &uiCount, 1);
    pkOstr = kStream.Ostr();
    pfnWrite = pkOstr->m_pfnWrite;
    uint32_t uiSize = 4 * m_uiEntries;
    NiPalette::PaletteEntry* pkPalette = m_pkPalette;
    uiCount = 1;
    pkOstr->m_uiAbsoluteCurrentPos += pfnWrite(pkOstr, pkPalette, uiSize, &uiCount, 1);
}

// The memcmp gets the byte count `m_uiEntries * 4 != 0` (0 or 1), so at most the first byte of the two palettes is
// compared, as the game does.
bool NiPalette::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }
    NiPalette* pkOther = static_cast<NiPalette*>(pkObject);
    if (m_bAlpha != pkOther->m_bAlpha || m_uiEntries != pkOther->m_uiEntries) {
        return false;
    }
    const uint8_t* pucThis = reinterpret_cast<const uint8_t*>(m_pkPalette);
    const uint8_t* pucOther = reinterpret_cast<const uint8_t*>(pkOther->m_pkPalette);
    if (m_uiEntries * 4 != 0 && pucOther[0] != pucThis[0]) {
        return false;
    }
    return true;
}

void NiPalette::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
}

void NiPalette::AddPaletteToList() {
    ms_kPaletteListCriticalSection.Lock();
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
    ms_kPaletteListCriticalSection.Unlock();
}

void NiPalette::RemovePaletteFromList() {
    ms_kPaletteListCriticalSection.Lock();
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
    ms_kPaletteListCriticalSection.Unlock();
}

NiPalette& NiPalette::operator=(const NiPalette& kPal) {
    if (this != &kPal) {
        NiPalette::RendererData* m_pkRendererData = this->m_pkRendererData;
        if (m_pkRendererData) {
            delete m_pkRendererData;
            this->m_pkRendererData = 0;
        }
        uint32_t m_uiEntries = this->m_uiEntries;
        m_bAlpha = kPal.m_bAlpha;
        m_uiRevID = 1;
        if (m_uiEntries != kPal.m_uiEntries) {
            MemoryManager_Singleton.Deallocate(m_pkPalette);
            uint32_t v5 = kPal.m_uiEntries;
            this->m_uiEntries = v5;
            m_pkPalette = static_cast<NiPalette::PaletteEntry*>(MemoryManager_Singleton.Allocate(
                v5 <= SIZE_MAX / sizeof(NiPalette::PaletteEntry) ? sizeof(NiPalette::PaletteEntry) * v5 : SIZE_MAX
            ));
        }
        rt::memcpy(m_pkPalette, kPal.m_pkPalette, sizeof(NiPalette::PaletteEntry) * kPal.m_uiEntries);
    }
    return *this;
}

NiObject* NiPalette::CreateObject() {
    NiPalette* pkObject = static_cast<NiPalette*>(NiMemObject::operator new(sizeof(NiPalette)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiPalette;
}
