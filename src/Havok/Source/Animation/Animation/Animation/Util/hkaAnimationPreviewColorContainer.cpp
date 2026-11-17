#include "Havok/Source/Animation/Animation/Animation/Util/hkaAnimationPreviewColorContainer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

static void finishLoadedObjecthkaAnimationPreviewColorContainer(void* p, int32_t finishing);
static void cleanupLoadedObjecthkaAnimationPreviewColorContainer(void* p);

// The reflected members of hkaAnimationPreviewColorContainer.
using hkaAnimationPreviewColorContainerClass_MembersArray = const hkClassMember[1];
static hkaAnimationPreviewColorContainerClass_MembersArray hkaAnimationPreviewColorContainerClass_Members{
    {"previewColor", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x8, nullptr}
};
hkClass hkaAnimationPreviewColorContainerClass(
    "hkaAnimationPreviewColorContainer",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkaAnimationPreviewColorContainerClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkaAnimationPreviewColorContainerTypeInfo = {
    "hkaAnimationPreviewColorContainer",
    finishLoadedObjecthkaAnimationPreviewColorContainer,
    cleanupLoadedObjecthkaAnimationPreviewColorContainer,
    getVtablehkaAnimationPreviewColorContainer(),
    &typeid(hkaAnimationPreviewColorContainer),
    sizeof(hkaAnimationPreviewColorContainer)
};

// The type info's finish-loaded-object callback for hkaAnimationPreviewColorContainer: the null check, then its finish
// constructor.
static void finishLoadedObjecthkaAnimationPreviewColorContainer(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkaAnimationPreviewColorContainer(kFlag);
    }
}

static void cleanupLoadedObjecthkaAnimationPreviewColorContainer(void* p) {
    static_cast<hkaAnimationPreviewColorContainer*>(p)->~hkaAnimationPreviewColorContainer();
}

const void* getVtablehkaAnimationPreviewColorContainer() {
    return hkGetVtableOf<hkaAnimationPreviewColorContainer>();
}

hkaAnimationPreviewColorContainer::hkaAnimationPreviewColorContainer() : m_previewColor() {}

// The color array goes with its member.
hkaAnimationPreviewColorContainer::~hkaAnimationPreviewColorContainer() {}

void hkaAnimationPreviewColorContainer::newPreviewColor(uint32_t color) {
    hkArray<uint32_t, hkContainerHeapAllocator>* p_m_previewColor = &m_previewColor;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_previewColor->m_size == (p_m_previewColor->m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_previewColor, sizeof(uint32_t));
    }
    p_m_previewColor->m_data[p_m_previewColor->m_size++] = color;
}

const hkClass& hkaAnimationPreviewColorContainer::staticClass() {
    return hkaAnimationPreviewColorContainerClass;
}
