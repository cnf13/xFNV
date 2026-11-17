#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

static void finishLoadedObjecthkLocalFrameGroup(void* p, int32_t finishing);
static void** getVtablehkLocalFrameGroup();
static void finishLoadedObjecthkSimpleLocalFrame(void* p, int32_t finishing);
// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
static const void* getVtablehkSimpleLocalFrame();

// hkLocalFrame's reflection class.
hkClass hkLocalFrameClass(
    "hkLocalFrame",
    &hkReferencedObjectClass,
    8,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);
hkLocalFrameGroup::MemberArray hkLocalFrameGroup::Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr}
};

// hkSimpleLocalFrameClass's member table.
using hkSimpleLocalFrameClass_MemberArray = const hkClassMember[5];
static hkSimpleLocalFrameClass_MemberArray hkSimpleLocalFrameClass_Members{
    {"transform", nullptr, nullptr, {hkClassMember::TYPE_TRANSFORM}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr},
    {"children",
     &hkLocalFrameClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     0,
     0x50,
     nullptr},
    {"parentFrame",
     &hkLocalFrameClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     hkClassMember::NOT_OWNED,
     0x5C,
     nullptr},
    {"group",
     &hkLocalFrameGroupClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x60,
     nullptr},
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, 0, 0x64, nullptr}
};
hkClass hkLocalFrameGroupClass(
    "hkLocalFrameGroup",
    &hkReferencedObjectClass,
    12,
    nullptr,
    0,
    nullptr,
    0,
    hkLocalFrameGroup::Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkLocalFrameGroupTypeInfo = {
    "hkLocalFrameGroup",          finishLoadedObjecthkLocalFrameGroup, cleanupLoadedObjecthkLocalFrameGroup,
    getVtablehkLocalFrameGroup(), &typeid(hkLocalFrameGroup),          sizeof(hkLocalFrameGroup)
};
hkClass hkSimpleLocalFrameClass(
    "hkSimpleLocalFrame",
    &hkLocalFrameClass,
    112,
    nullptr,
    0,
    nullptr,
    0,
    hkSimpleLocalFrameClass_Members,
    5,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkSimpleLocalFrameTypeInfo = {
    "hkSimpleLocalFrame",          finishLoadedObjecthkSimpleLocalFrame, cleanupLoadedObjecthkSimpleLocalFrame,
    getVtablehkSimpleLocalFrame(), &typeid(hkSimpleLocalFrame),          sizeof(hkSimpleLocalFrame)
};

static void finishLoadedObjecthkLocalFrameGroup(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkLocalFrameGroup(kFlag);
    }
}

static void** getVtablehkLocalFrameGroup() {
    return static_cast<void**>(const_cast<void*>(hkGetVtableOf<hkLocalFrameGroup>()));
}

static const void* getVtablehkSimpleLocalFrame() {
    return hkGetVtableOf<hkSimpleLocalFrame>();
}

// Runs the finish constructor hkSimpleLocalFrame(flag) in place.
static void finishLoadedObjecthkSimpleLocalFrame(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkSimpleLocalFrame(kFlag);
    }
}

void cleanupLoadedObjecthkpCollidable(void* p) {
    ;
}

void cleanupLoadedObjecthkpCollidableBoundingVolumeData(void* p) {
    ;
}

// The SDK's finish constructor of the bounding volume data over the object it is given, when there is one and it is
// being finished: no child AABBs, and invalidate()'s min above max.
void finishLoadedObjecthkpCollidableBoundingVolumeData(void* a1, int32_t a2) {
    hkpCollidable::BoundingVolumeData* pData = static_cast<hkpCollidable::BoundingVolumeData*>(a1);
    if (pData && a2) {
        pData->m_numChildShapeAabbs = 0;
        pData->m_capacityChildShapeAabbs = 0;
        pData->m_childShapeAabbs = nullptr;
        pData->m_childShapeKeys = nullptr;
        pData->invalidate();
    }
}

// The SDK's finish constructor of the collidable over the object it is given, when there is one and it is being
// finished: its bounding volume data's (as above), then the broad phase handle's owner set to the collidable.
void finishLoadedObjecthkpCollidable(void* a1, int32_t a2) {
    hkpCollidable* pCollidable = static_cast<hkpCollidable*>(a1);
    if (pCollidable && a2) {
        hkpCollidable::BoundingVolumeData& rData = pCollidable->m_boundingVolumeData;
        rData.m_childShapeAabbs = nullptr;
        rData.m_childShapeKeys = nullptr;
        rData.m_max[0] = 0;
        rData.m_numChildShapeAabbs = 0;
        rData.m_capacityChildShapeAabbs = 0;
        rData.m_min[0] = 1;
        // hkpTypedBroadPhaseHandle::setOwner(this): the owner's offset from the handle, which is minus the handle's
        // offset in the collidable.
        pCollidable->m_broadPhaseHandle.m_ownerOffset =
            static_cast<int8_t>(-static_cast<int32_t>(offsetof(hkpCollidable, m_broadPhaseHandle)));
    }
}

void cleanupLoadedObjecthkSimpleLocalFrame(void* p) {
    static_cast<hkSimpleLocalFrame*>(p)->~hkSimpleLocalFrame();
}

// Destroys the object in place through its virtual destructor, as the SDK's cleanupLoadedObject helpers do.
void cleanupLoadedObjecthkLocalFrameGroup(void* p) {
    static_cast<hkLocalFrameGroup*>(p)->~hkLocalFrameGroup();
}

// The finish constructor of the world it is given, when there is one.
void finishLoadedObjecthkpWorld(void* p, int32_t finishing) {
    hkpWorld* apWorld = static_cast<hkpWorld*>(p);
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (apWorld) {
        ::new (apWorld) hkpWorld(aFlag);
    }
}

// m_name frees itself.
hkLocalFrameGroup::~hkLocalFrameGroup() {}

const hkClass& hkLocalFrameGroup::staticClass() {
    return hkLocalFrameGroupClass;
}

void hkLocalFrame::getLocalPosition(hkVector4& position) const {
    hkTransform v2;
    getLocalTransform(v2);
    position = v2.m_translation;
}

// getParentFrame is called twice, once for the test and once for the recursion, and the local transforms are aligned to
// 16.
void hkLocalFrame::getTransformToRoot(hkTransform& transform) const {
    if (getParentFrame()) {
        alignas(16) hkTransform parentTransform;
        getParentFrame()->getTransformToRoot(parentTransform);
        alignas(16) hkTransform localTransform;
        getLocalTransform(localTransform);
        transform.setMul(parentTransform, localTransform);
    }
    else {
        getLocalTransform(transform);
    }
}

void hkLocalFrame::getPositionInRoot(hkVector4& position) const {
    getLocalPosition(position);
    for (const hkLocalFrame* pParent = getParentFrame(); pParent; pParent = pParent->getParentFrame()) {
        alignas(16) hkTransform t;
        pParent->getLocalTransform(t);
        position.setTransformedPos(t, position);
    }
}

// Drops the references on the children and the group; the name and the child array free themselves.
hkSimpleLocalFrame::~hkSimpleLocalFrame() {
    for (int32_t i = 0; i < m_children.m_size; ++i) {
        m_children.m_data[i]->removeReference();
    }
    if (m_group) {
        m_group->removeReference();
    }
}

void hkSimpleLocalFrame::getLocalTransform(hkTransform& transform) const {
    transform = m_transform;
}

void hkSimpleLocalFrame::setLocalTransform(const hkTransform& transform) {
    m_transform = transform;
}

void hkSimpleLocalFrame::getLocalPosition(hkVector4& position) const {
    position = m_transform.m_translation;
}

void hkSimpleLocalFrame::getNearbyFrames(
    const hkVector4& target,
    float maxDistance,
    hkLocalFrameCollector& collector
) const {
    hkVector4 diff;
    diff.setSub4(target, m_transform.m_translation);
    const float distance = diff.length3();
    if (maxDistance >= distance) {
        collector.addFrame(this, distance);
    }
    if (m_children.m_size > 0) {
        alignas(16) hkVector4 targetLocal;
        targetLocal.setTransformedInversePos(m_transform, target);
        for (int32_t i = 0; i < m_children.m_size; ++i) {
            m_children.m_data[i]->getNearbyFrames(targetLocal, maxDistance, collector);
        }
    }
}

int32_t hkSimpleLocalFrame::getNumChildFrames() const {
    return (int32_t)m_children.m_size;
}

hkLocalFrame* hkSimpleLocalFrame::getChildFrame(int32_t i) const {
    return m_children.m_data[i];
}

void hkSimpleLocalFrame::setGroup(const hkLocalFrameGroup* group) {
    if (group) {
        group->addReference();
    }
    const hkLocalFrameGroup* m_group = this->m_group;
    if (m_group) {
        m_group->removeReference();
    }
    this->m_group = group;
}

const hkClass& hkSimpleLocalFrame::staticClass() {
    return hkSimpleLocalFrameClass;
}
