#include "Fallout Shared/Region System/TESRegionObject.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"

bool TESRegionObject::IsFormLandTexture() {
	return pForm && FNVD_DYNAMIC_CAST_VOID(TESLandTexture)(pForm, RTTI_TESForm, RTTI_TESLandTexture) != 0;
}

float TESRegionObject::ConvertNormalToAngle(NiPoint3 aNormal) {
	NiPoint3 v4 = aNormal;
	float v3 = (&v4)->UnitizeGetLength();
	float x_4 = v4.z / v3;
	return std::acosf(x_4);
}
