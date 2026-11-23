#include "BSHavok/bhkSphereRepShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI bhkSphereRepShape::ms_RTTI("bhkSphereRepShape", &bhkShape::ms_RTTI);

const NiRTTI* bhkSphereRepShape::GetRTTI() const {
    return &bhkSphereRepShape::ms_RTTI;
}

void bhkSphereRepShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShape::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkSphereRepShape::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v8 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v8);
}

bhkSphereRepShape::~bhkSphereRepShape() {
    bhkSerializable::Kill();
    --iCount;
}

hkSphereRepShapeCinfo::hkSphereRepShapeCinfo() {}
