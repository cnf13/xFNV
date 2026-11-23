#include "Fallout Shared/Region System/TESRegionObject.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"

bool TESRegionObject::IsFormLandTexture() {
    return pForm && dynamic_cast<TESLandTexture*>(pForm) != nullptr;
}

float TESRegionObject::ConvertNormalToAngle(NiPoint3 aNormal) {
    NiPoint3 kNormal = aNormal;
    float fLength = kNormal.UnitizeGetLength();
    return std::acosf(kNormal.z / fLength);
}
