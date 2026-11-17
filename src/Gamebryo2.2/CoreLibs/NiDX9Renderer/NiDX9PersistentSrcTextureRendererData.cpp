#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PersistentSrcTextureRendererData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPersistentSrcTextureRendererData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <stdint.h>
#include <xmmintrin.h>

NiRTTI NiDX9PersistentSrcTextureRendererData::ms_RTTI(
    "NiDX9PersistentSrcTextureRendererData",
    &NiPersistentSrcTextureRendererData::ms_RTTI
);

NiObject* NiDX9PersistentSrcTextureRendererData_CreateObject() {
    void* pkMemory = NiMemObject::operator new(sizeof(NiDX9PersistentSrcTextureRendererData));
    if (!pkMemory) {
        return 0;
    }
    return ::new (pkMemory) NiDX9PersistentSrcTextureRendererData;
}

NiDX9PersistentSrcTextureRendererData::NiDX9PersistentSrcTextureRendererData() : NiPersistentSrcTextureRendererData() {}

// The base releases the pixel data.
NiDX9PersistentSrcTextureRendererData::~NiDX9PersistentSrcTextureRendererData() {}

const NiRTTI* NiDX9PersistentSrcTextureRendererData::GetRTTI() const {
    return &ms_RTTI;
}

void NiDX9PersistentSrcTextureRendererData::LinkObject(NiStream& arStream) {
    NiPersistentSrcTextureRendererData::LinkObject(arStream);
}

bool NiDX9PersistentSrcTextureRendererData::RegisterStreamables(NiStream& arStream) {
    return NiPersistentSrcTextureRendererData::RegisterStreamables(arStream);
}

void NiDX9PersistentSrcTextureRendererData::SaveBinary(NiStream& arStream) {
    NiPersistentSrcTextureRendererData::SaveBinary(arStream);
}

bool NiDX9PersistentSrcTextureRendererData::IsEqual(NiObject* apObject) {
    if (!NiObject::IsKindOf(&ms_RTTI, apObject)) {
        return false;
    }
    return NiPersistentSrcTextureRendererData::IsEqual(apObject);
}

void NiDX9PersistentSrcTextureRendererData::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiPersistentSrcTextureRendererData::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(NiDX9PersistentSrcTextureRendererData::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v6 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v6);
}

// The renderer data streams as the platform-independent class.
const NiRTTI* NiDX9PersistentSrcTextureRendererData::GetStreamableRTTI() const {
    return &NiPersistentSrcTextureRendererData::ms_RTTI;
}
