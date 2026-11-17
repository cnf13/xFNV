#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBox.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionConvert.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphere.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/stdlib.h"

NiSphereBV NiCollisionConvert::ms_kSphereABV(1.0f, NiPoint3_ZERO);

// The radius is stored first, then the center.
NiBoundingVolume* NiCollisionConvert::ConvertToSphereBV_Fast(float fRadius, const NiPoint3& kCenter) {
    ms_kSphereABV.m_kSphere.m_fRadius = fRadius;
    ms_kSphereABV.m_kSphere.m_kCenter = kCenter;
    return &ms_kSphereABV;
}

NiBoundingVolume* NiCollisionConvert::ConvertToSphereBV_Safe(float fRadius, const NiPoint3& kCenter) {
    NiSphereBV* pkSphereBV;
    NiSphereBV* v2 = static_cast<NiSphereBV*>(MemoryManager_Singleton.Allocate(sizeof(NiSphereBV)));
    if (v2) {
        pkSphereBV = ::new (v2) NiSphereBV(1.0f, NiPoint3_ZERO);
    }
    else {
        pkSphereBV = nullptr;
    }
    pkSphereBV->m_kSphere.m_fRadius = fRadius;
    pkSphereBV->m_kSphere.m_kCenter = kCenter;
    return pkSphereBV;
}

NiBoundingVolume* NiCollisionConvert::ConvertToBoxBV(int32_t usQuantity, const NiPoint3* pkVertex) {
    NiBoxBV* pkBoxBV;
    NiBox v4;
    if (!static_cast<uint16_t>(usQuantity) || !pkVertex) {
        return nullptr;
    }
    v4.CreateFromData(usQuantity, pkVertex);
    NiBoxBV* v2 = static_cast<NiBoxBV*>(MemoryManager_Singleton.Allocate(sizeof(NiBoxBV)));
    if (v2) {
        pkBoxBV =
            ::new (v2) NiBoxBV(NiPoint3_UNIT_ALL, NiPoint3_ZERO, NiPoint3_UNIT_X, NiPoint3_UNIT_Y, NiPoint3_UNIT_Z);
    }
    else {
        pkBoxBV = nullptr;
    }
    pkBoxBV->m_kBox = v4;
    return pkBoxBV;
}
