#pragma once

// The native hkRefPtr<hkaFootstepAnalysisInfo> destructor releases the info: the class is complete there.
#include "Havok/Source/Animation/Animation/MotionAnalysis/hkaFootstepAnalysisInfo.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

#include <stdint.h>

// Havok SDK signature (hkaAnimation.lib, tied by reloc,target).
const void* getVtablehkaFootstepAnalysisInfoContainer();

// Owns the footstep-analysis previews for an animation.
class hkaFootstepAnalysisInfoContainer : public hkReferencedObject {
public:
	hkaFootstepAnalysisInfoContainer();

	hkaFootstepAnalysisInfoContainer(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_previewInfo(aFlag) {}

	~hkaFootstepAnalysisInfoContainer() override;

	hkaFootstepAnalysisInfo* newPreviewInfo(int32_t numFrames, float duration, float posTol, float velTol,
		const char* name, const char* nameStrike, const char* nameLift, const char* nameLock, const char* nameUnlock);

	static const hkClass& staticClass();
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkArray<hkRefPtr<hkaFootstepAnalysisInfo>> m_previewInfo;
};

extern hkClass hkaFootstepAnalysisInfoContainerClass;
extern hkTypeInfo hkaFootstepAnalysisInfoContainerTypeInfo;
