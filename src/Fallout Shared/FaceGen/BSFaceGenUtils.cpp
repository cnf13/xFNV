#include "Fallout Shared/FaceGen/BSFaceGenUtils.h"

#include "Fallout Shared/FaceGen/BSFaceGenBaseMorphExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"

// The extra data of apGeom named by T's tag, or null for a null geometry (the exe's BSFaceGenBaseMorphExtraData copy,
// 0x653430). Defined here, where NiTriBasedGeom is complete; BSFaceGenBaseMorphExtraData is the one instantiation.
template <class T> NiExtraData* BSFaceGenUtils::GetFaceGenExtraData(NiTriBasedGeom* apGeom) {
	if (!apGeom) {
		return nullptr;
	}
	return apGeom->GetExtraData(T::Tag());
}

template NiExtraData* BSFaceGenUtils::GetFaceGenExtraData<BSFaceGenBaseMorphExtraData>(NiTriBasedGeom* apGeom);
