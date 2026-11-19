#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/MobileObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiRTTI NiTexturingProperty::ms_RTTI("NiTexturingProperty", &NiProperty::ms_RTTI);
NiPointer<NiTexturingProperty> NiTexturingProperty::ms_spDefault;
NiTexturingProperty::MapNameArray NiTexturingProperty::ms_akMapIDtoNames;

// No texture or transform; texture coordinate set 0, bilinear filter, wrap in both directions.
NiTexturingProperty::Map::Map()
  : m_uFlags(static_cast<uint16_t>((WRAP_S_WRAP_T << CLAMPMODE_POS) | (FILTER_BILERP << FILTERMODE_POS))),
    m_spTexture(nullptr), m_pkTextureTransform(nullptr) {}

// The source's flags and texture, and a new copy of its transform (raw MemoryManager storage, as ~Map frees it).
NiTexturingProperty::Map::Map(const Map& kObj)
  : m_uFlags(kObj.m_uFlags), m_spTexture(kObj.m_spTexture), m_pkTextureTransform(nullptr) {
    if (NiTextureTransform* pkSource = kObj.m_pkTextureTransform) {
        void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiTextureTransform));
        if (pvMemory) {
            m_pkTextureTransform = ::new (pvMemory) NiTextureTransform(
                pkSource->m_kTranslate, pkSource->m_fRotate, pkSource->m_kScale, pkSource->m_kCenter,
                pkSource->m_eMethod
            );
        }
    }
}

NiTexturingProperty::Map::Map(NiTexture* a1, int16_t a2, int16_t a3, uint16_t a4, NiTextureTransform* a5)
  : m_uFlags(static_cast<uint16_t>(a2 | (a4 << FILTERMODE_POS) | (a3 << CLAMPMODE_POS))), m_spTexture(a1),
    m_pkTextureTransform(a5) {}

// The transform is raw MemoryManager storage; the texture is released by its pointer's destructor.
NiTexturingProperty::Map::~Map() {
    MemoryManager_Singleton.Deallocate(m_pkTextureTransform);
}

// Reads the texture link and the flags (three separate words before 20.1.0.2), skips fields of old formats and
// loads the texture transform. The old transform is dropped, not deleted.
// KEEP: before 20.1.0.2 the first flags word is read into an uninitialised stack slot, which a short read leaves
// unwritten and the body stores into m_uFlags; the has-transform byte is read into the stream argument's slot.
void NiTexturingProperty::Map::LoadBinary(NiStream& kStream) {
    kStream.ReadLinkID();

    if (kStream.m_uiNifFileVersion >= 0x14010002) {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
    else {
        uint32_t uiValue;
        NiStreamLoadBinary(kStream, uiValue);
        m_uFlags = static_cast<uint16_t>((m_uFlags & 0xCFFF) | (uiValue << CLAMPMODE_POS));
        NiStreamLoadBinary(kStream, uiValue);
        m_uFlags = static_cast<uint16_t>((m_uFlags & 0xF0FF) | (uiValue << FILTERMODE_POS));
        NiStreamLoadBinary(kStream, uiValue);
        m_uFlags = static_cast<uint16_t>((m_uFlags & 0xFF00) | static_cast<uint16_t>(uiValue));
    }

    // The skipped fields of old formats are read as 2-byte values.
    if (kStream.m_uiNifFileVersion < 0x0A030004) {
        uint16_t usUnused;
        NiStreamLoadBinary(kStream, usUnused);
        NiStreamLoadBinary(kStream, usUnused);
    }

    if (kStream.m_uiNifFileVersion < 0x04010010) {
        uint16_t usUnused;
        NiStreamLoadBinary(kStream, usUnused);
    }

    if (kStream.m_uiNifFileVersion >= 0x0A00010A) {
        m_pkTextureTransform = nullptr;
        uint8_t ucHasTransform;
        NiStreamLoadBinary(kStream, ucHasTransform);
        if (ucHasTransform == 1) {
            void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiTextureTransform));
            NiTextureTransform* pkTransform = pvMemory ? ::new (pvMemory) NiTextureTransform : nullptr;
            m_pkTextureTransform = pkTransform;
            // Called even when the allocation failed, deliberately, as the game does.
            pkTransform->LoadBinary(kStream);
        }
    }
}

void NiTexturingProperty::Map::SaveBinary(NiStream& kStream) {
    kStream.SaveLinkID(m_spTexture.m_pObject);
    NiStreamSaveBinary(kStream, m_uFlags);
    bool bHasTransform = m_pkTextureTransform != nullptr;
    NiStreamSaveBinary(kStream, bHasTransform);
    if (m_pkTextureTransform) {
        m_pkTextureTransform->SaveBinary(kStream);
    }
}

bool NiTexturingProperty::Map::operator==(const Map& kObj) const {
    if (m_spTexture) {
        if (!m_spTexture->IsEqual(kObj.m_spTexture.m_pObject)) {
            return false;
        }
    }
    else if (kObj.m_spTexture) {
        return false;
    }

    if (m_uFlags != kObj.m_uFlags) {
        return false;
    }

    if (m_pkTextureTransform && kObj.m_pkTextureTransform) {
        return !(*m_pkTextureTransform != *kObj.m_pkTextureTransform);
    }
    return !m_pkTextureTransform && !kObj.m_pkTextureTransform;
}

void NiTexturingProperty::Map::SetClampMode(NiTexturingProperty::ClampMode eClampMode) {
    SetField(eClampMode, 0x3000u, 0xCu);
}

void NiTexturingProperty::Map::SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos) {
    m_uFlags = (uint16_t)((uVal << uPos) | ((~uMask) & m_uFlags));
}

void NiTexturingProperty::Map::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

// The base map's defaults (bilinear filter, wrap in both directions), the luma scale 1 and half the identity bump
// matrix.
NiTexturingProperty::BumpMap::BumpMap()
  : m_fLumaScale(1.0f), m_fLumaOffset(0.0f), m_fBumpMat00(0.5f), m_fBumpMat01(0.0f), m_fBumpMat10(0.0f),
    m_fBumpMat11(0.5f) {}

NiTexturingProperty::BumpMap::BumpMap(const BumpMap& kObj)
  : Map(kObj), m_fLumaScale(kObj.m_fLumaScale), m_fLumaOffset(kObj.m_fLumaOffset), m_fBumpMat00(kObj.m_fBumpMat00),
    m_fBumpMat01(kObj.m_fBumpMat01), m_fBumpMat10(kObj.m_fBumpMat10), m_fBumpMat11(kObj.m_fBumpMat11) {}

NiTexturingProperty::BumpMap::~BumpMap() {}

void NiTexturingProperty::BumpMap::LoadBinary(NiStream& kStream) {
    Map::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fLumaScale);
    NiStreamLoadBinary(kStream, m_fLumaOffset);
    NiStreamLoadBinary(kStream, m_fBumpMat00);
    NiStreamLoadBinary(kStream, m_fBumpMat01);
    NiStreamLoadBinary(kStream, m_fBumpMat10);
    NiStreamLoadBinary(kStream, m_fBumpMat11);
}

void NiTexturingProperty::BumpMap::SaveBinary(NiStream& kStream) {
    Map::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fLumaScale);
    NiStreamSaveBinary(kStream, m_fLumaOffset);
    NiStreamSaveBinary(kStream, m_fBumpMat00);
    NiStreamSaveBinary(kStream, m_fBumpMat01);
    NiStreamSaveBinary(kStream, m_fBumpMat10);
    NiStreamSaveBinary(kStream, m_fBumpMat11);
}

// The floats are compared with !=, so a NaN makes the maps unequal.
bool NiTexturingProperty::BumpMap::operator==(const BumpMap& kObj) const {
    if (!Map::operator==(kObj)) {
        return false;
    }
    if (m_fLumaScale != kObj.m_fLumaScale) {
        return false;
    }
    if (m_fLumaOffset != kObj.m_fLumaOffset) {
        return false;
    }
    if (m_fBumpMat00 != kObj.m_fBumpMat00) {
        return false;
    }
    if (m_fBumpMat01 != kObj.m_fBumpMat01) {
        return false;
    }
    if (m_fBumpMat10 != kObj.m_fBumpMat10) {
        return false;
    }
    if (m_fBumpMat11 != kObj.m_fBumpMat11) {
        return false;
    }
    return true;
}

NiTexturingProperty::ShaderMap::ShaderMap() : m_uiID(0) {}

NiTexturingProperty::ShaderMap::ShaderMap(const ShaderMap& kObj) : Map(kObj), m_uiID(kObj.m_uiID) {}

NiTexturingProperty::ShaderMap::~ShaderMap() {}

void NiTexturingProperty::ShaderMap::LoadBinary(NiStream& kStream) {
    Map::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_uiID);
}

void NiTexturingProperty::ShaderMap::SaveBinary(NiStream& kStream) {
    Map::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uiID);
}

bool NiTexturingProperty::ShaderMap::operator==(const ShaderMap& kObj) const {
    return Map::operator==(kObj) && m_uiID == kObj.m_uiID;
}

NiTexturingProperty::ParallaxMap::ParallaxMap() : m_fOffset(0.5f) {}

NiTexturingProperty::ParallaxMap::ParallaxMap(const ParallaxMap& kObj) : Map(kObj), m_fOffset(kObj.m_fOffset) {}

NiTexturingProperty::ParallaxMap::~ParallaxMap() {}

void NiTexturingProperty::ParallaxMap::LoadBinary(NiStream& kStream) {
    Map::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fOffset);
}

void NiTexturingProperty::ParallaxMap::SaveBinary(NiStream& kStream) {
    Map::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fOffset);
}

bool NiTexturingProperty::ParallaxMap::operator==(const ParallaxMap& kObj) const {
    return Map::operator==(kObj) && m_fOffset == kObj.m_fOffset;
}

// Nine empty maps: no decals and no multitexture, the modulate apply mode.
NiTexturingProperty::NiTexturingProperty()
  : m_uFlags(static_cast<uint16_t>(APPLY_MODULATE << APPLYMODE_POS)), m_kMaps(9, 1), m_pkShaderMaps(nullptr) {
    Map* pkNone = nullptr;
    for (uint32_t i = BASE_INDEX; i <= DECAL_BASE; ++i) {
        m_kMaps.SetAt(i, pkNone);
    }
}

// A base map holding a source texture made of pkPixelData, the other eight maps empty: no decals and no
// multitexture, the modulate apply mode.
NiTexturingProperty::NiTexturingProperty(
    NiPixelData* pkPixelData,
    const NiFixedString& kName,
    NiTexture::FormatPrefs& arPrefs
)
  : m_uFlags(static_cast<uint16_t>(APPLY_MODULATE << APPLYMODE_POS)), m_kMaps(9, 1), m_pkShaderMaps(nullptr) {
    NiTexture* pkTexture = NiSourceTexture::Create(pkPixelData, kName.GetHandle(), arPrefs);
    Map* pkMap = new Map(pkTexture, 0, WRAP_S_WRAP_T, FILTER_BILERP, nullptr);
    m_kMaps.SetAt(BASE_INDEX, pkMap);
    Map* pkNone = nullptr;
    for (uint32_t i = DARK_INDEX; i <= DECAL_BASE; ++i) {
        m_kMaps.SetAt(i, pkNone);
    }
}

// The maps, the shader maps and their array are owned; m_kMaps' buffer is released by its own destructor.
NiTexturingProperty::~NiTexturingProperty() {
    for (uint32_t i = 0; i < m_kMaps.m_usSize; ++i) {
        delete m_kMaps.m_pBase[i];
    }
    if (m_pkShaderMaps) {
        for (uint32_t i = 0; i < m_pkShaderMaps->m_usSize; ++i) {
            delete m_pkShaderMaps->m_pBase[i];
        }
        delete m_pkShaderMaps;
    }
}

NiObject* NiTexturingProperty::CreateClone(NiCloningProcess& kCloning) {
    if (m_spControllers) {
        NiTexturingProperty* v3 =
            static_cast<NiTexturingProperty*>(NiMemObject::operator new(sizeof(NiTexturingProperty)));
        if (v3) {
            NiTexturingProperty* v4 = ::new (v3) NiTexturingProperty;
            CopyMembers(v4, kCloning);
            return v4;
        }

        CopyMembers(nullptr, kCloning);
        return nullptr;
    }

    NiObject::CopyMembers(this, kCloning);
    return this;
}

// Reads the flags (before 20.1.0.2 only the apply mode), the map list (the bump and parallax maps as their
// subclasses; before 3.3.0.13 a link slot is skipped past the fifth map) and the shader maps, then sets the
// multitexture bit when any map past the base one is present. A missing map's slot is cleared.
// KEEP: the old-format apply mode and the map count are read into uninitialised stack slots, which a short read
// leaves unwritten; the body stores the first into m_uFlags and the second drives the map loop. Each has-map byte
// is read into the stream argument's slot.
void NiTexturingProperty::LoadBinary(NiStream& kStream) {
    NiProperty::LoadBinary(kStream);

    if (kStream.GetFileVersion() >= 0x14010002) {
        NiStreamLoadBinary(kStream, m_uFlags);
        m_uFlags &= static_cast<uint16_t>(~DECALCOUNT_MASK);
    }
    else {
        uint32_t uiApplyMode;
        NiStreamLoadBinary(kStream, uiApplyMode);
        m_uFlags = static_cast<uint16_t>((m_uFlags & ~APPLYMODE_MASK) | (uiApplyMode << APPLYMODE_POS));
    }

    uint32_t uiListSize;
    NiStreamLoadBinary(kStream, uiListSize);
    // Before 20.2.0.5 the list had no normal and parallax slots, so the decals start two slots further.
    uint32_t uiArraySize = uiListSize;
    if (kStream.GetFileVersion() < 0x14020005 && uiListSize > 6) {
        uiArraySize = uiListSize + 2;
    }
    m_kMaps.SetSize(uiArraySize);
    if (kStream.GetFileVersion() >= 0x0303000D) {
        kStream.SetNumberOfLinkIDs(uiListSize);
    }
    else {
        kStream.SetNumberOfLinkIDs(uiListSize + 1);
    }

    for (uint32_t i = 0; i < uiListSize; ++i) {
        uint32_t uiIndex = i;
        Map* pkMap = nullptr;
        uint8_t bHasMap;
        if (kStream.GetFileVersion() < 0x04010000) {
            int32_t iHasMap = 0;
            NiStreamLoadBinary(kStream, iHasMap);
            bHasMap = iHasMap != 0;
        }
        else {
            NiStreamLoadBinary(kStream, bHasMap);
        }

        uint32_t uiVersion = kStream.GetFileVersion();
        if (uiVersion >= 0x0303000D) {
            if (i == BUMP_INDEX) {
                kStream.SetNumberOfLinkIDs(bHasMap);
                if (bHasMap) {
                    void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(BumpMap));
                    pkMap = pvMemory ? ::new (pvMemory) BumpMap : nullptr;
                    pkMap->LoadBinary(kStream);
                }
                m_kMaps.SetAt(BUMP_INDEX, pkMap);
                continue;
            }
            if (i == PARALLAX_INDEX && uiVersion >= 0x14020005) {
                kStream.SetNumberOfLinkIDs(bHasMap);
                if (bHasMap) {
                    void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(ParallaxMap));
                    pkMap = pvMemory ? ::new (pvMemory) ParallaxMap : nullptr;
                    pkMap->LoadBinary(kStream);
                }
                m_kMaps.SetAt(PARALLAX_INDEX, pkMap);
                continue;
            }
        }
        else if (i >= BUMP_INDEX) {
            kStream.SetNumberOfLinkIDs(0);
            uiIndex = i + 1;
        }

        kStream.SetNumberOfLinkIDs(bHasMap);
        if (bHasMap) {
            Map* pkNew = static_cast<Map*>(MemoryManager_Singleton.Allocate(sizeof(Map)));
            if (pkNew) {
                ::new (pkNew) Map;
            }
            pkMap = pkNew;
            pkMap->LoadBinary(kStream);
        }
        if (uiIndex >= m_kMaps.m_usMaxSize) {
            m_kMaps.SetSize(uiIndex + m_kMaps.m_usGrowBy);
        }
        m_kMaps.SetAt(uiIndex, pkMap);
    }

    if (kStream.GetFileVersion() >= 0x05000011) {
        NiStreamLoadBinary(kStream, uiListSize);
        kStream.SetNumberOfLinkIDs(uiListSize);
        if (uiListSize) {
            void* pvArray = MemoryManager_Singleton.Allocate(sizeof(NiTPrimitiveArray<ShaderMap*>));
            m_pkShaderMaps = pvArray ? ::new (pvArray) NiTPrimitiveArray<ShaderMap*>(uiListSize, 1) : nullptr;
            for (uint32_t i = 0; i < uiListSize; ++i) {
                uint8_t bHasMap;
                NiStreamLoadBinary(kStream, bHasMap);
                kStream.SetNumberOfLinkIDs(bHasMap);
                ShaderMap* pkMap = nullptr;
                if (bHasMap) {
                    ShaderMap* pkNew = static_cast<ShaderMap*>(MemoryManager_Singleton.Allocate(sizeof(ShaderMap)));
                    if (pkNew) {
                        ::new (pkNew) ShaderMap;
                        pkMap = pkNew;
                    }
                    pkMap->LoadBinary(kStream);
                }
                m_pkShaderMaps->SetAt(i, pkMap);
            }
        }
    }

    uint32_t uiSize = m_kMaps.m_usSize;
    uint32_t i = 1;
    for (; i < uiSize; ++i) {
        if (m_kMaps.m_pBase[i]) {
            break;
        }
    }
    if (i < uiSize) {
        m_uFlags |= MULTITEXTURE_MASK;
    }
    else {
        m_uFlags &= static_cast<uint16_t>(~MULTITEXTURE_MASK);
    }
}

// Resolves the map textures. The decal count is rebuilt from the maps past DECAL_BASE. Files older than 20.2.0.5
// kept their normal and parallax maps in slots 6 and 7 as decals: they move to the first decal slots and the old
// decals shift up past them. There are no null, bounds or class checks on the maps.
void NiTexturingProperty::LinkObject(NiStream& kStream) {
    NiProperty::LinkObject(kStream);

    uint32_t uiCount = kStream.GetNumberOfLinkIDs();
    m_uFlags &= 0xF00F;
    for (uint32_t i = 0; i < uiCount; ++i) {
        if (kStream.GetNumberOfLinkIDs()) {
            m_kMaps.m_pBase[i]->m_spTexture = static_cast<NiTexture*>(kStream.GetObjectFromLinkID());
            if (i >= DECAL_BASE) {
                m_uFlags = static_cast<uint16_t>((m_uFlags & 0xF00F) | ((((m_uFlags >> 4) & 0xFF) + 1) << 4));
            }
        }
    }

    if (kStream.m_uiNifFileVersion < 0x14020005) {
        uint32_t uiSize = m_kMaps.GetSize();
        Map* pkNormal = uiSize > NORMAL_INDEX ? m_kMaps.m_pBase[NORMAL_INDEX] : nullptr;
        Map* pkParallax = uiSize > PARALLAX_INDEX ? m_kMaps.m_pBase[PARALLAX_INDEX] : nullptr;
        if (pkNormal || pkParallax) {
            uint32_t uiShift = 0;
            if (pkNormal) {
                uiShift = 1;
            }
            if (pkParallax) {
                ++uiShift;
            }

            if ((m_uFlags & DECALCOUNT_MASK) && uiShift) {
                // The moved element is passed by reference into the buffer, as the game does: when SetAtGrow grows the
                // array, it is read from the old buffer after SetSize has freed it.
                for (uint32_t s = uiSize - 1, d = s + uiShift; s >= DECAL_BASE; --s, --d) {
                    m_kMaps.SetAtGrow(d, m_kMaps.m_pBase[s]);
                }
            }

            Map* pkNull = nullptr;
            if (pkNormal) {
                m_kMaps.SetAtGrow(DECAL_BASE, pkNormal);
                m_kMaps.SetAt(NORMAL_INDEX, pkNull);
                m_uFlags = static_cast<uint16_t>((m_uFlags & 0xF00F) | ((((m_uFlags >> 4) & 0xFF) + 1) << 4));
            }
            else {
                m_kMaps.SetAt(DECAL_BASE, pkNull);
            }

            if (pkParallax) {
                m_kMaps.SetAtGrow(DECAL_BASE + 1, pkParallax);
                m_kMaps.SetAt(PARALLAX_INDEX, pkNull);
                m_uFlags = static_cast<uint16_t>((m_uFlags & 0xF00F) | ((((m_uFlags >> 4) & 0xFF) + 1) << 4));
            }
            else if (uiShift == 2) {
                m_kMaps.SetAt(DECAL_BASE + 1, pkNull);
            }
        }
    }

    if (kStream.m_uiNifFileVersion >= 0x5000011) {
        uint32_t uiShaderCount = kStream.GetNumberOfLinkIDs();
        for (uint32_t i = 0; i < uiShaderCount; ++i) {
            if (kStream.GetNumberOfLinkIDs()) {
                m_pkShaderMaps->m_pBase[i]->m_spTexture = static_cast<NiTexture*>(kStream.GetObjectFromLinkID());
            }
        }
    }
}

// The sizes are re-read on every iteration; the textures' own results are ignored.
bool NiTexturingProperty::RegisterStreamables(NiStream& kStream) {
    if (!NiProperty::RegisterStreamables(kStream)) {
        return false;
    }

    for (uint32_t i = 0; i < m_kMaps.GetSize(); ++i) {
        Map* pkMap = m_kMaps.m_pBase[i];
        if (pkMap && pkMap->m_spTexture) {
            pkMap->m_spTexture->RegisterStreamables(kStream);
        }
    }

    if (m_pkShaderMaps) {
        for (uint32_t i = 0; i < m_pkShaderMaps->GetSize(); ++i) {
            ShaderMap* pkMap = m_pkShaderMaps->m_pBase[i];
            if (pkMap && pkMap->m_spTexture) {
                pkMap->m_spTexture->RegisterStreamables(kStream);
            }
        }
    }
    return true;
}

// Each map slot is a presence byte followed by the map; a missing shader map array saves a count of 0.
void NiTexturingProperty::SaveBinary(NiStream& kStream) {
    NiProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);

    uint32_t uiCount = m_kMaps.GetSize();
    NiStreamSaveBinary(kStream, uiCount);
    for (uint32_t i = 0; i < uiCount; ++i) {
        Map* pkMap = m_kMaps.m_pBase[i];
        bool bHas = pkMap != nullptr;
        NiStreamSaveBinary(kStream, bHas);
        if (bHas) {
            pkMap->SaveBinary(kStream);
        }
    }

    if (m_pkShaderMaps) {
        uint32_t uiShaderCount = m_pkShaderMaps->GetSize();
        NiStreamSaveBinary(kStream, uiShaderCount);
        for (uint32_t i = 0; i < uiShaderCount; ++i) {
            ShaderMap* pkMap = m_pkShaderMaps->m_pBase[i];
            bool bHas = pkMap != nullptr;
            NiStreamSaveBinary(kStream, bHas);
            if (bHas) {
                pkMap->SaveBinary(kStream);
            }
        }
    }
    else {
        uint32_t uiZero = 0;
        NiStreamSaveBinary(kStream, uiZero);
    }
}

// The maps are compared slot by slot; the bump and parallax slots compare as their subclasses by index alone.
bool NiTexturingProperty::IsEqual(NiObject* pkObject) {
    if (!NiProperty::IsEqual(pkObject)) {
        return false;
    }

    NiTexturingProperty* pkOther = static_cast<NiTexturingProperty*>(pkObject);
    if (m_uFlags != pkOther->m_uFlags) {
        return false;
    }

    uint32_t uiCount = m_kMaps.GetSize();
    if (uiCount != pkOther->m_kMaps.GetSize()) {
        return false;
    }

    for (uint32_t i = 0; i < uiCount; ++i) {
        Map* pkMap = m_kMaps.m_pBase[i];
        Map* pkOtherMap = pkOther->m_kMaps.m_pBase[i];
        if (!pkMap) {
            if (pkOtherMap) {
                return false;
            }
            continue;
        }
        if (!pkOtherMap) {
            return false;
        }

        bool bEqual;
        if (i == BUMP_INDEX) {
            bEqual = *static_cast<BumpMap*>(pkMap) == *static_cast<BumpMap*>(pkOtherMap);
        }
        else if (i == PARALLAX_INDEX) {
            bEqual = *static_cast<ParallaxMap*>(pkMap) == *static_cast<ParallaxMap*>(pkOtherMap);
        }
        else {
            bEqual = *pkMap == *pkOtherMap;
        }
        if (!bEqual) {
            return false;
        }
    }

    if (!m_pkShaderMaps) {
        return pkOther->m_pkShaderMaps == nullptr;
    }
    if (!pkOther->m_pkShaderMaps) {
        return false;
    }

    uint32_t uiShaderCount = m_pkShaderMaps->GetSize();
    if (uiShaderCount != pkOther->m_pkShaderMaps->GetSize()) {
        return false;
    }

    for (uint32_t i = 0; i < uiShaderCount; ++i) {
        ShaderMap* pkMap = m_pkShaderMaps->m_pBase[i];
        ShaderMap* pkOtherMap = pkOther->m_pkShaderMaps->m_pBase[i];
        if (!pkMap) {
            if (pkOtherMap) {
                return false;
            }
            continue;
        }
        if (!pkOtherMap || !(*pkMap == *pkOtherMap)) {
            return false;
        }
    }
    return true;
}

void NiTexturingProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiProperty::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));

    uint32_t uiCount = m_kMaps.GetSize();
    char acName[64];
    NiSprintf(acName, sizeof(acName), "ApplyMode ");
    pkStrings->Add(NiGetViewerString(acName, static_cast<int32_t>((m_uFlags >> APPLYMODE_POS) & 7)));

    for (uint32_t i = 0; i < uiCount; ++i) {
        Map* pkMap = m_kMaps.m_pBase[i];
        if (!pkMap) {
            continue;
        }

        switch (i) {
            case BASE_INDEX: pkStrings->Add(NiGetViewerString("MultiTexture Base Map", 0u)); break;
            case DARK_INDEX: pkStrings->Add(NiGetViewerString("MultiTexture Dark Map", 1u)); break;
            case DETAIL_INDEX: pkStrings->Add(NiGetViewerString("MultiTexture Detail Map", 2u)); break;
            case GLOSS_INDEX: pkStrings->Add(NiGetViewerString("MultiTexture Gloss Map", 3u)); break;
            case GLOW_INDEX: pkStrings->Add(NiGetViewerString("MultiTexture Glow Map", 4u)); break;
            case BUMP_INDEX: {
                BumpMap* pkBump = static_cast<BumpMap*>(pkMap);
                pkStrings->Add(NiGetViewerString("MultiTexture Bump Map", 5u));
                pkStrings->Add(NiGetViewerString("m_fLumaScale", pkBump->m_fLumaScale));
                pkStrings->Add(NiGetViewerString("m_fLumaOffset", pkBump->m_fLumaOffset));
                pkStrings->Add(NiGetViewerString("m_fBumpMat00", pkBump->m_fBumpMat00));
                pkStrings->Add(NiGetViewerString("m_fBumpMat01", pkBump->m_fBumpMat01));
                pkStrings->Add(NiGetViewerString("m_fBumpMat10", pkBump->m_fBumpMat10));
                pkStrings->Add(NiGetViewerString("m_fBumpMat11", pkBump->m_fBumpMat11));
                break;
            }
            case NORMAL_INDEX: pkStrings->Add(NiGetViewerString("MultiTexture Normal Map", 6u)); break;
            case PARALLAX_INDEX:
                pkStrings->Add(NiGetViewerString("MultiTexture Parallax Map", 7u));
                pkStrings->Add(NiGetViewerString("m_fParallaxOffset", static_cast<ParallaxMap*>(pkMap)->m_fOffset));
                break;
            default: {
                char acDecal[64];
                NiSprintf(acDecal, sizeof(acDecal), "MultiTexture Decal Map %d", i - DECAL_BASE);
                pkStrings->Add(NiGetViewerString(acDecal, i));
                break;
            }
        }

        NiSprintf(acName, sizeof(acName), "m_spTexture ");
        pkStrings->Add(NiGetViewerString(acName, static_cast<const void*>(pkMap->m_spTexture.m_pObject)));
        NiSprintf(acName, sizeof(acName), "Clamp Mode ");
        pkStrings->Add(GetViewerString(acName, static_cast<ClampMode>((pkMap->m_uFlags >> Map::CLAMPMODE_POS) & 3)));
        NiSprintf(acName, sizeof(acName), "Filter Mode");
        pkStrings->Add(
            GetViewerString(acName, static_cast<FilterMode>((pkMap->m_uFlags >> Map::FILTERMODE_POS) & 0xF))
        );
        NiSprintf(acName, sizeof(acName), "Texture Coord Index ");
        pkStrings->Add(NiGetViewerString(acName, static_cast<uint32_t>(pkMap->m_uFlags & Map::TEXCOORD_MASK)));
    }

    if (m_pkShaderMaps) {
        uint32_t uiShaderCount = m_pkShaderMaps->GetSize();
        for (uint32_t j = 0; j < uiShaderCount; ++j) {
            ShaderMap* pkMap = m_pkShaderMaps->m_pBase[j];
            if (!pkMap) {
                continue;
            }

            pkStrings->Add(NiGetViewerString("Shader Map", j));
            pkStrings->Add(NiGetViewerString("m_uiID", pkMap->m_uiID));
            NiSprintf(acName, sizeof(acName), "m_spTexture ");
            pkStrings->Add(NiGetViewerString(acName, static_cast<const void*>(pkMap->m_spTexture.m_pObject)));
            NiSprintf(acName, sizeof(acName), "Clamp Mode ");
            pkStrings->Add(
                GetViewerString(acName, static_cast<ClampMode>((pkMap->m_uFlags >> Map::CLAMPMODE_POS) & 3))
            );
            NiSprintf(acName, sizeof(acName), "Filter Mode");
            pkStrings->Add(
                GetViewerString(acName, static_cast<FilterMode>((pkMap->m_uFlags >> Map::FILTERMODE_POS) & 0xF))
            );
            NiSprintf(acName, sizeof(acName), "Texture Coord Index ");
            pkStrings->Add(NiGetViewerString(acName, static_cast<uint32_t>(pkMap->m_uFlags & Map::TEXCOORD_MASK)));
        }
    }
}

NiTexturingProperty::ShaderMap* NiTexturingProperty::GetShaderMap(uint32_t uiIndex) {
    if (m_pkShaderMaps && uiIndex < m_pkShaderMaps->GetSize()) {
        return m_pkShaderMaps->GetAt(uiIndex);
    }
    return nullptr;
}

void NiTexturingProperty::SetApplyMode(ApplyMode eMode) {
    SetField(static_cast<uint16_t>(eMode), APPLYMODE_MASK, APPLYMODE_POS);
}

uint16_t NiTexturingProperty::GetField(uint16_t uMask, uint16_t uPos) const {
    return (m_uFlags & uMask) >> uPos;
}

void NiTexturingProperty::SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos) {
    m_uFlags = uint16_t((m_uFlags & ~uint32_t(uMask)) | (uint32_t(uVal) << uPos));
}

// Clones every map of the source into the destination: the bump, parallax and shader stages as their subclasses, the
// other stages as plain maps, and empty source slots stay empty. Both arrays are sized to the source's first, and the
// destination gets a shader map array when it has none.
void NiTexturingProperty::CopyMembers(NiTexturingProperty* pkDest, NiCloningProcess& kCloning) {
    NiProperty::CopyMembers(pkDest, kCloning);
    pkDest->m_uFlags = m_uFlags;

    uint32_t uiCount = m_kMaps.m_usSize;
    pkDest->m_kMaps.SetSize(uiCount);
    for (uint32_t i = 0; i < uiCount; ++i) {
        if (!m_kMaps.m_pBase[i]) {
            continue;
        }

        if (i == BUMP_INDEX) {
            BumpMap* pkBump = static_cast<BumpMap*>(MemoryManager_Singleton.Allocate(sizeof(BumpMap)));
            if (pkBump) {
                ::new (pkBump) BumpMap(*static_cast<BumpMap*>(m_kMaps.m_pBase[BUMP_INDEX]));
            }
            if (i >= pkDest->m_kMaps.m_usMaxSize) {
                pkDest->m_kMaps.SetSize(pkDest->m_kMaps.m_usGrowBy + i);
            }
            pkDest->m_kMaps.SetAt(i, static_cast<Map*>(pkBump));
        }
        else if (i == PARALLAX_INDEX) {
            ParallaxMap* pkParallax = static_cast<ParallaxMap*>(MemoryManager_Singleton.Allocate(sizeof(ParallaxMap)));
            if (pkParallax) {
                ::new (pkParallax) ParallaxMap(*static_cast<ParallaxMap*>(m_kMaps.m_pBase[PARALLAX_INDEX]));
            }
            if (i >= pkDest->m_kMaps.m_usMaxSize) {
                pkDest->m_kMaps.SetSize(pkDest->m_kMaps.m_usGrowBy + i);
            }
            pkDest->m_kMaps.SetAt(i, static_cast<Map*>(pkParallax));
        }
        else {
            Map* pkMap = nullptr;
            void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(Map));
            if (pvMemory) {
                pkMap = ::new (pvMemory) Map(*m_kMaps.m_pBase[i]);
            }
            if (i >= pkDest->m_kMaps.m_usMaxSize) {
                pkDest->m_kMaps.SetSize(i + pkDest->m_kMaps.m_usGrowBy);
            }
            pkDest->m_kMaps.SetAt(i, pkMap);
        }
    }

    if (!m_pkShaderMaps) {
        return;
    }

    if (!pkDest->m_pkShaderMaps) {
        NiTPrimitiveArray<ShaderMap*>* pkArray =
            static_cast<NiTPrimitiveArray<ShaderMap*>*>(MemoryManager_Singleton.Allocate(sizeof(*pkArray)));
        if (pkArray) {
            ::new (pkArray) NiTPrimitiveArray<ShaderMap*>(0, 1);
        }
        pkDest->m_pkShaderMaps = pkArray;
    }

    uint32_t uiShaderCount = m_pkShaderMaps->m_usSize;
    pkDest->m_pkShaderMaps->SetSize(uiShaderCount);
    for (uint32_t i = 0; i < uiShaderCount; ++i) {
        if (!m_pkShaderMaps->m_pBase[i]) {
            continue;
        }

        ShaderMap* pkMap = nullptr;
        ShaderMap* pkMemory = static_cast<ShaderMap*>(MemoryManager_Singleton.Allocate(sizeof(ShaderMap)));
        if (pkMemory) {
            ::new (pkMemory) ShaderMap(*static_cast<ShaderMap*>(m_pkShaderMaps->m_pBase[i]));
            pkMap = pkMemory;
        }

        NiTPrimitiveArray<ShaderMap*>* pkArray = pkDest->m_pkShaderMaps;
        if (i >= pkArray->m_usMaxSize) {
            pkArray->SetSize(i + pkArray->m_usGrowBy);
        }
        pkArray->SetAt(i, pkMap);
    }
}

void NiTexturingProperty::SetMultiTexture(NiTexturingProperty::Map* pkMap) {
    if (!pkMap) {
        uint32_t m_usSize = m_kMaps.m_usSize;
        uint32_t v3 = 1;
        if (m_usSize <= 1) {
        LABEL_6:;
            m_uFlags &= ~1u;
            return;
        }
        NiTexturingProperty::Map** v4 = m_kMaps.m_pBase + 1;
        while (!*v4) {
            ++v3;
            ++v4;
            if (v3 >= m_usSize) {
                goto LABEL_6;
            }
        }
    }
    m_uFlags |= 1u;
}

void NiTexturingProperty::SetDecalMap(uint32_t uiIndex, NiTexturingProperty::Map* pkMap) {
    NiTexturingProperty::Map* v5;
    uint32_t uiDecalIndex = uiIndex + 8;
    if (uiIndex + 8 >= m_kMaps.m_usSize) {
        NiTPrimitiveArray<NiTexturingProperty::Map*>* p_m_kMaps = &m_kMaps;
        if (uiDecalIndex >= m_kMaps.m_usMaxSize) {
            p_m_kMaps->SetSize(uiDecalIndex + m_kMaps.m_usGrowBy);
        }
        p_m_kMaps->SetAt(uiDecalIndex, pkMap);
        v5 = pkMap;
        if (pkMap) {
            m_uFlags = (m_uFlags & 0xF00F) | 16 * (static_cast<uint8_t>(m_uFlags >> 4) + 1);
        }
    }
    else {
        v5 = pkMap;
        NiTexturingProperty::Map* pkOld = m_kMaps.m_pBase[uiDecalIndex];
        if (pkMap) {
            if (!pkOld) {
                m_uFlags = (m_uFlags & 0xF00F) | 16 * (static_cast<uint8_t>(m_uFlags >> 4) + 1);
            }
        }
        else {
            if (pkOld) {
                m_uFlags = (m_uFlags & 0xF00F) | 16 * (static_cast<uint8_t>(m_uFlags >> 4) - 1);
            }
        }
        m_kMaps.SetAt(uiDecalIndex, pkMap);
        if (pkOld) {
            delete pkOld;
            SetMultiTexture(v5);
            return;
        }
    }
    SetMultiTexture(v5);
}

bool NiTexturingProperty::GetMapIDFromName(const NiFixedString& kName, uint32_t& uiID) {
    for (uint32_t i = 0; i < 10; i++) {
        const char* pcMap = ms_akMapIDtoNames[i].GetHandle();
        const char* pcName = kName.GetHandle();
        if (pcMap == pcName || (pcName && pcMap && NiStricmp(pcMap, pcName) == 0)) {
            uiID = i;
            return true;
        }
    }
    return false;
}

char* NiTexturingProperty::GetViewerString(const char* pcPrefix, FilterMode eMode) {
    size_t stLen = strlen(pcPrefix) + 30;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    switch (eMode) {
        case FILTER_NEAREST: NiSprintf(pcString, stLen, "%s = FILTER_NEAREST", pcPrefix); break;
        case FILTER_BILERP: NiSprintf(pcString, stLen, "%s = FILTER_BILERP", pcPrefix); break;
        case FILTER_TRILERP: NiSprintf(pcString, stLen, "%s = FILTER_TRILERP", pcPrefix); break;
        case FILTER_NEAREST_MIPNEAREST: NiSprintf(pcString, stLen, "%s = FILTER_NEAREST_MIPNEAREST", pcPrefix); break;
        case FILTER_NEAREST_MIPLERP: NiSprintf(pcString, stLen, "%s = FILTER_NEAREST_MIPLERP", pcPrefix); break;
        case FILTER_BILERP_MIPNEAREST: NiSprintf(pcString, stLen, "%s = FILTER_BILERP_MIPNEAREST", pcPrefix); break;
        case FILTER_ANISO: NiSprintf(pcString, stLen, "%s = FILTER_ANISO", pcPrefix); break;
        case FILTER_MAGLERP_MINANISO: NiSprintf(pcString, stLen, "%s = FILTER_MAGLERP_MINANISO", pcPrefix); break;
        default: NiSprintf(pcString, stLen, "%s = UNKNOWN!!!", pcPrefix); break;
    }
    return pcString;
}

char* NiTexturingProperty::GetViewerString(const char* pcPrefix, ClampMode eMode) {
    size_t stLen = strlen(pcPrefix) + 20;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    switch (eMode) {
        case CLAMP_S_CLAMP_T: NiSprintf(pcString, stLen, "%s = CLAMP_S_CLAMP_T", pcPrefix); break;
        case CLAMP_S_WRAP_T: NiSprintf(pcString, stLen, "%s = CLAMP_S_WRAP_T", pcPrefix); break;
        case WRAP_S_CLAMP_T: NiSprintf(pcString, stLen, "%s = WRAP_S_CLAMP_T", pcPrefix); break;
        case WRAP_S_WRAP_T: NiSprintf(pcString, stLen, "%s = WRAP_S_WRAP_T", pcPrefix); break;
        default: NiSprintf(pcString, stLen, "%s = UNKNOWN!!!", pcPrefix); break;
    }
    return pcString;
}

int32_t NiTexturingProperty::GetType() {
    return NiProperty::TEXTURING;
}

void NiTexturingProperty::_SDMShutdown() {
    NiTexturingProperty::ms_spDefault = nullptr;
    const char* m_kHandle = NiTexturingProperty::ms_akMapIDtoNames[0].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[0].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[0].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(m_kHandle);
    }
    const char* v2 = NiTexturingProperty::ms_akMapIDtoNames[1].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[1].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[1].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v2);
    }
    const char* v3 = NiTexturingProperty::ms_akMapIDtoNames[2].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[2].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[2].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v3);
    }
    const char* v4 = NiTexturingProperty::ms_akMapIDtoNames[3].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[3].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[3].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v4);
    }
    const char* v5 = NiTexturingProperty::ms_akMapIDtoNames[4].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[4].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[4].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v5);
    }
    const char* v6 = NiTexturingProperty::ms_akMapIDtoNames[5].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[5].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[5].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v6);
    }
    const char* v7 = NiTexturingProperty::ms_akMapIDtoNames[6].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[6].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[6].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v7);
    }
    const char* v8 = NiTexturingProperty::ms_akMapIDtoNames[7].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[7].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[7].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v8);
    }
    const char* v9 = NiTexturingProperty::ms_akMapIDtoNames[8].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[8].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[8].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v9);
    }
    const char* v10 = NiTexturingProperty::ms_akMapIDtoNames[9].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[9].GetHandle()) {
        NiTexturingProperty::ms_akMapIDtoNames[9].m_kHandle = NiGlobalStringTable::AddString(nullptr);
        NiGlobalStringTable::ReleaseHandle(v10);
    }
}

NiObject* NiTexturingProperty::CreateObject() {
    return new NiTexturingProperty;
}

void NiTexturingProperty::_SDMInit() {
    NiTexturingProperty* v1;
    v1 = new NiTexturingProperty;
    NiTexturingProperty::ms_spDefault = v1;
    const char* m_kHandle = NiTexturingProperty::ms_akMapIDtoNames[0].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[0].GetHandle() != aBase_0) {
        NiTexturingProperty::ms_akMapIDtoNames[0].m_kHandle = NiGlobalStringTable::AddString(aBase_0);
        NiGlobalStringTable::ReleaseHandle(m_kHandle);
    }
    const char* v4 = NiTexturingProperty::ms_akMapIDtoNames[1].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[1].GetHandle() != aDark) {
        NiTexturingProperty::ms_akMapIDtoNames[1].m_kHandle = NiGlobalStringTable::AddString(aDark);
        NiGlobalStringTable::ReleaseHandle(v4);
    }
    const char* v5 = NiTexturingProperty::ms_akMapIDtoNames[2].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[2].GetHandle() != aDetail) {
        NiTexturingProperty::ms_akMapIDtoNames[2].m_kHandle = NiGlobalStringTable::AddString(aDetail);
        NiGlobalStringTable::ReleaseHandle(v5);
    }
    const char* v6 = NiTexturingProperty::ms_akMapIDtoNames[3].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[3].GetHandle() != aGloss) {
        NiTexturingProperty::ms_akMapIDtoNames[3].m_kHandle = NiGlobalStringTable::AddString(aGloss);
        NiGlobalStringTable::ReleaseHandle(v6);
    }
    const char* v7 = NiTexturingProperty::ms_akMapIDtoNames[4].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[4].GetHandle() != aGlow) {
        NiTexturingProperty::ms_akMapIDtoNames[4].m_kHandle = NiGlobalStringTable::AddString(aGlow);
        NiGlobalStringTable::ReleaseHandle(v7);
    }
    const char* v8 = NiTexturingProperty::ms_akMapIDtoNames[5].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[5].GetHandle() != aBump) {
        NiTexturingProperty::ms_akMapIDtoNames[5].m_kHandle = NiGlobalStringTable::AddString(aBump);
        NiGlobalStringTable::ReleaseHandle(v8);
    }
    const char* v9 = NiTexturingProperty::ms_akMapIDtoNames[6].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[6].GetHandle() != aNormal_0) {
        NiTexturingProperty::ms_akMapIDtoNames[6].m_kHandle = NiGlobalStringTable::AddString(aNormal_0);
        NiGlobalStringTable::ReleaseHandle(v9);
    }
    const char* v10 = NiTexturingProperty::ms_akMapIDtoNames[7].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[7].GetHandle() != aParallax) {
        NiTexturingProperty::ms_akMapIDtoNames[7].m_kHandle = NiGlobalStringTable::AddString(aParallax);
        NiGlobalStringTable::ReleaseHandle(v10);
    }
    const char* v11 = NiTexturingProperty::ms_akMapIDtoNames[8].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[8].GetHandle() != pcString) {
        NiTexturingProperty::ms_akMapIDtoNames[8].m_kHandle = NiGlobalStringTable::AddString(pcString);
        NiGlobalStringTable::ReleaseHandle(v11);
    }
    const char* v12 = NiTexturingProperty::ms_akMapIDtoNames[9].GetHandle();
    if (NiTexturingProperty::ms_akMapIDtoNames[9].GetHandle() != aShader_0) {
        NiTexturingProperty::ms_akMapIDtoNames[9].m_kHandle = NiGlobalStringTable::AddString(aShader_0);
        NiGlobalStringTable::ReleaseHandle(v12);
    }
}
