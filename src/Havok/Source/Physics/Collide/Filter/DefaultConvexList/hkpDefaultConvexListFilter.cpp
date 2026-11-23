#include "Havok/Source/Physics/Collide/Filter/DefaultConvexList/hkpDefaultConvexListFilter.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

static void finishLoadedObjecthkpDefaultConvexListFilter(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpDefaultConvexListFilter();
static void cleanupLoadedObjecthkpDefaultConvexListFilter(void* p);

hkClass hkpDefaultConvexListFilterClass(
    "hkpDefaultConvexListFilter",
    &hkpConvexListFilterClass,
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
hkTypeInfo hkpDefaultConvexListFilterTypeInfo = {
    "hkpDefaultConvexListFilter",
    finishLoadedObjecthkpDefaultConvexListFilter,
    cleanupLoadedObjecthkpDefaultConvexListFilter,
    getVtablehkpDefaultConvexListFilter(),
    &typeid(hkpDefaultConvexListFilter),
    sizeof(hkpDefaultConvexListFilter)
};

// The type info's finish-loaded-object callback for hkpDefaultConvexListFilter: its finish constructor.
static void finishLoadedObjecthkpDefaultConvexListFilter(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpDefaultConvexListFilter(kFlag);
    }
}

static const void* getVtablehkpDefaultConvexListFilter() {
    return hkGetVtableOf<hkpDefaultConvexListFilter>();
}

static void cleanupLoadedObjecthkpDefaultConvexListFilter(void* p) {
    static_cast<hkpDefaultConvexListFilter*>(p)->~hkpDefaultConvexListFilter();
}

hkpDefaultConvexListFilter::~hkpDefaultConvexListFilter() {}

hkpConvexListFilter::ConvexListCollisionType hkpDefaultConvexListFilter::getConvexListCollisionType(
    const hkpCdBody& convexListBody,
    const hkpCdBody& otherBody,
    const hkpCollisionInput& input
) const {
    return static_cast<hkpConvexListFilter::ConvexListCollisionType>(
        input.m_dispatcher->m_hasAlternateType[otherBody.m_shape->m_type] >> 8 & 1
    );
}

const hkClass& hkpDefaultConvexListFilter::staticClass() {
    return hkpDefaultConvexListFilterClass;
}
