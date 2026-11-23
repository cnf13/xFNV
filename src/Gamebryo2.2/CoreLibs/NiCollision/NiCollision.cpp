#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBox.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionConvert.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphere.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/stdlib.h"

NiSphereBV NiCollisionConvert::ms_kSphereABV(1.0f, NiPoint3::ZERO);

// The radius is stored first, then the center.
NiBoundingVolume* NiCollisionConvert::ConvertToSphereBV_Fast(float fRadius, const NiPoint3& kCenter) {
    ms_kSphereABV.m_kSphere.m_fRadius = fRadius;
    ms_kSphereABV.m_kSphere.m_kCenter = kCenter;
    return &ms_kSphereABV;
}

NiBoundingVolume* NiCollisionConvert::ConvertToSphereBV_Safe(float fRadius, const NiPoint3& kCenter) {
    NiSphereBV* pkSphereBV;
    pkSphereBV = ::new NiSphereBV(1.0f, NiPoint3::ZERO);
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
    pkBoxBV = ::new NiBoxBV(NiPoint3::UNIT_ALL, NiPoint3::ZERO, NiPoint3::UNIT_X, NiPoint3::UNIT_Y, NiPoint3::UNIT_Z);
    pkBoxBV->m_kBox = v4;
    return pkBoxBV;
}
