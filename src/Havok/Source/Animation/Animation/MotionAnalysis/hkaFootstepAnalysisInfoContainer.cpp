#include "Havok/Source/Animation/Animation/MotionAnalysis/hkaFootstepAnalysisInfoContainer.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/MotionAnalysis/hkaFootstepAnalysisInfo.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

static void finishLoadedObjecthkaFootstepAnalysisInfoContainer(void* p, int32_t finishing);
static void cleanupLoadedObjecthkaFootstepAnalysisInfoContainer(void* p);

// The class record's member table.
using hkaFootstepAnalysisInfoContainerClassMemberArray = const hkClassMember[1];
static hkaFootstepAnalysisInfoContainerClassMemberArray hkaFootstepAnalysisInfoContainerClass_Members{
    {"previewInfo",
     &hkaFootstepAnalysisInfoClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     0,
     0x8,
     nullptr}
};
hkClass hkaFootstepAnalysisInfoContainerClass(
    "hkaFootstepAnalysisInfoContainer",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkaFootstepAnalysisInfoContainerClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkaFootstepAnalysisInfoContainerTypeInfo = {
    "hkaFootstepAnalysisInfoContainer",
    finishLoadedObjecthkaFootstepAnalysisInfoContainer,
    cleanupLoadedObjecthkaFootstepAnalysisInfoContainer,
    getVtablehkaFootstepAnalysisInfoContainer(),
    &typeid(hkaFootstepAnalysisInfoContainer),
    sizeof(hkaFootstepAnalysisInfoContainer)
};

// The type info's finish-loaded-object callback for hkaFootstepAnalysisInfoContainer: the null check, then its finish
// constructor.
static void finishLoadedObjecthkaFootstepAnalysisInfoContainer(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkaFootstepAnalysisInfoContainer(kFlag);
    }
}

static void cleanupLoadedObjecthkaFootstepAnalysisInfoContainer(void* p) {
    static_cast<hkaFootstepAnalysisInfoContainer*>(p)->~hkaFootstepAnalysisInfoContainer();
}

const void* getVtablehkaFootstepAnalysisInfoContainer() {
    return hkGetVtableOf<hkaFootstepAnalysisInfoContainer>();
}

hkaFootstepAnalysisInfoContainer::hkaFootstepAnalysisInfoContainer() : m_previewInfo() {}

// The previews' references and the array go with the member.
hkaFootstepAnalysisInfoContainer::~hkaFootstepAnalysisInfoContainer() {}

const hkClass& hkaFootstepAnalysisInfoContainer::staticClass() {
    return hkaFootstepAnalysisInfoContainerClass;
}
