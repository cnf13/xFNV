#include "Havok/Source/Physics/Utilities/Deprecated/H1Group/hkpGroupCollisionFilter.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

static void cleanupLoadedObjecthkpGroupCollisionFilter(void* p);

// The member table of hkpGroupCollisionFilter's class record, as the Havok SDK's class file has it.
using hkpGroupCollisionFilterClass_MembersArray = const hkClassMember[2];
static hkpGroupCollisionFilterClass_MembersArray hkpGroupCollisionFilterClass_Members{
    {"noGroupCollisionEnabled",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr},
    {"collisionGroups",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     32,
     {0},
     0x34,
     nullptr}
};
hkClass hkpGroupCollisionFilterClass(
    "hkpGroupCollisionFilter",
    &hkpCollisionFilterClass,
    180,
    nullptr,
    0,
    nullptr,
    0,
    hkpGroupCollisionFilterClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpGroupCollisionFilterTypeInfo = {
    "hkpGroupCollisionFilter",
    finishLoadedObjecthkpGroupCollisionFilter,
    cleanupLoadedObjecthkpGroupCollisionFilter,
    getVtablehkpGroupCollisionFilter(),
    &typeid(hkpGroupCollisionFilter),
    sizeof(hkpGroupCollisionFilter)
};

static void cleanupLoadedObjecthkpGroupCollisionFilter(void* p) {
    static_cast<hkpGroupCollisionFilter*>(p)->~hkpGroupCollisionFilter();
}

// Objects without a group collide; no group collides with another yet.
hkpGroupCollisionFilter::hkpGroupCollisionFilter() : m_collisionGroups{} {
    m_noGroupCollisionEnabled.m_bool = 1;
}

// The filter owns nothing.
hkpGroupCollisionFilter::~hkpGroupCollisionFilter() {}

hkBool hkpGroupCollisionFilter::isCollisionEnabled(const hkpCollidable& a, const hkpCollidable& b) const {
    hkBool result;
    isCollisionEnabled(&result, a.m_broadPhaseHandle.m_collisionFilterInfo, b.m_broadPhaseHandle.m_collisionFilterInfo);
    return result;
}

hkBool hkpGroupCollisionFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& a,
    const hkpCdBody& b,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool apResult;
    uint32_t CollisionFilterInfo = bContainer.getCollisionFilterInfo(bKey);
    // EXE-INLINED 0x624020
    const hkpCdBody* root = &a;
    while (root->m_parent) {
        root = root->m_parent;
    }
    const hkpCollidable* rootCollidable = static_cast<const hkpCollidable*>(root);
    isCollisionEnabled(&apResult, rootCollidable->m_broadPhaseHandle.m_collisionFilterInfo, CollisionFilterInfo);
    return apResult;
}

hkBool hkpGroupCollisionFilter::isCollisionEnabled(
    const hkpCollisionInput& input,
    const hkpCdBody& collectionBodyA,
    const hkpCdBody& collectionBodyB,
    const hkpShapeContainer& containerShapeA,
    const hkpShapeContainer& containerShapeB,
    uint32_t keyA,
    uint32_t keyB
) const {
    hkBool result;
    uint32_t v10 = containerShapeA.getCollisionFilterInfo(keyA);
    uint32_t v11 = containerShapeB.getCollisionFilterInfo(keyB);
    isCollisionEnabled(&result, v10, v11);
    return result;
}

hkBool hkpGroupCollisionFilter::isCollisionEnabled(
    const hkpShapeRayCastInput& aInput,
    const hkpShape& bShape,
    const hkpShapeContainer& bContainer,
    uint32_t bKey
) const {
    hkBool result;
    uint32_t v7 = bContainer.getCollisionFilterInfo(bKey);
    isCollisionEnabled(&result, aInput.m_filterInfo, v7);
    return result;
}

hkBool
hkpGroupCollisionFilter::isCollisionEnabled(const hkpWorldRayCastInput& a, const hkpCollidable& collidableB) const {
    hkBool result;
    isCollisionEnabled(&result, a.m_filterInfo, collidableB.m_broadPhaseHandle.m_collisionFilterInfo);
    return result;
}

// Walks the 32 group bits with a left shift; the bit never wraps within the loop.
void hkpGroupCollisionFilter::enableCollisionGroups(uint32_t groupBitsA, uint32_t groupBitsB) {
    if (groupBitsA || groupBitsB) {
        uint32_t uiBit = 1;
        uint32_t* pGroups = m_collisionGroups;
        int32_t iCount = 32;
        do {
            if ((uiBit & groupBitsA) != 0) {
                *pGroups |= groupBitsB;
            }
            if ((uiBit & groupBitsB) != 0) {
                *pGroups |= groupBitsA;
            }
            ++pGroups;
            uiBit <<= 1;
            --iCount;
        }
        while (iCount);
    }
    else {
        m_noGroupCollisionEnabled.m_bool = 1;
    }
}

// Walks the 32 group bits with a left shift; the bit never wraps within the loop.
void hkpGroupCollisionFilter::disableCollisionGroups(uint32_t groupBitsA, uint32_t groupBitsB) {
    if (groupBitsA || groupBitsB) {
        uint32_t v3 = 1;
        uint32_t* m_collisionGroups = this->m_collisionGroups;
        int32_t v5 = 32;
        do {
            if ((v3 & groupBitsA) != 0) {
                *m_collisionGroups &= ~groupBitsB;
            }
            if ((v3 & groupBitsB) != 0) {
                *m_collisionGroups &= ~groupBitsA;
            }
            ++m_collisionGroups;
            v3 <<= 1;
            --v5;
        }
        while (v5);
    }
    else {
        m_noGroupCollisionEnabled.m_bool = 0;
    }
}

hkBool* hkpGroupCollisionFilter::isCollisionEnabled(hkBool* apResult, uint32_t infoA, uint32_t infoB) const {
    if (infoA && infoB) {
        uint32_t v49;
        int32_t v50;
        uint32_t v51;
        int32_t v52;
        uint32_t v53;
        int32_t v54;
        uint32_t v55;
        int32_t v56;
        uint32_t v57;
        int32_t v58;
        uint32_t v59;
        int32_t v60;
        uint32_t v61;
        int32_t v4 = 0;
        if (static_cast<uint8_t>(infoA)) {
            uint32_t v5;
            uint32_t v6;
            uint32_t v8;
            uint32_t v10;
            uint32_t v12;
            uint32_t v14;
            uint32_t v16;
            uint32_t v18;
            if ((infoA & 1) != 0) {
                v5 = m_collisionGroups[0];
            }
            else {
                v5 = 0;
            }
            if ((infoA & 2) != 0) {
                v6 = m_collisionGroups[1];
            }
            else {
                v6 = 0;
            }
            int32_t v7 = v6 | v5;
            if ((infoA & 4) != 0) {
                v8 = m_collisionGroups[2];
            }
            else {
                v8 = 0;
            }
            int32_t v9 = v8 | v7;
            if ((infoA & 8) != 0) {
                v10 = m_collisionGroups[3];
            }
            else {
                v10 = 0;
            }
            int32_t v11 = v10 | v9;
            if ((infoA & 16) != 0) {
                v12 = m_collisionGroups[4];
            }
            else {
                v12 = 0;
            }
            int32_t v13 = v12 | v11;
            if ((infoA & 32) != 0) {
                v14 = m_collisionGroups[5];
            }
            else {
                v14 = 0;
            }
            int32_t v15 = v14 | v13;
            if ((infoA & 64) != 0) {
                v16 = m_collisionGroups[6];
            }
            else {
                v16 = 0;
            }
            int32_t v17 = v16 | v15;
            if ((infoA & 128) == 0) {
                v18 = 0;
            }
            else {
                v18 = m_collisionGroups[7];
            }
            v4 = v18 | v17;
        }
        if ((infoA & 0xFF00) != 0) {
            uint32_t v19;
            uint32_t v21;
            uint32_t v23;
            uint32_t v25;
            uint32_t v27;
            uint32_t v29;
            uint32_t v31;
            uint32_t v33;
            if ((infoA & 256) != 0) {
                v19 = m_collisionGroups[8];
            }
            else {
                v19 = 0;
            }
            int32_t v20 = v19 | v4;
            if ((infoA & 512) != 0) {
                v21 = m_collisionGroups[9];
            }
            else {
                v21 = 0;
            }
            int32_t v22 = v21 | v20;
            if ((infoA & 1024) != 0) {
                v23 = m_collisionGroups[10];
            }
            else {
                v23 = 0;
            }
            int32_t v24 = v23 | v22;
            if ((infoA & 2048) != 0) {
                v25 = m_collisionGroups[11];
            }
            else {
                v25 = 0;
            }
            int32_t v26 = v25 | v24;
            if ((infoA & 0x1000) != 0) {
                v27 = m_collisionGroups[12];
            }
            else {
                v27 = 0;
            }
            int32_t v28 = v27 | v26;
            if ((infoA & 0x2000) != 0) {
                v29 = m_collisionGroups[13];
            }
            else {
                v29 = 0;
            }
            int32_t v30 = v29 | v28;
            if ((infoA & 0x4000) != 0) {
                v31 = m_collisionGroups[14];
            }
            else {
                v31 = 0;
            }
            int32_t v32 = v31 | v30;
            if ((infoA & 0x8000) != 0) {
                v33 = m_collisionGroups[15];
            }
            else {
                v33 = 0;
            }
            v4 = v33 | v32;
        }
        if ((infoA & 0xFF0000) != 0) {
            uint32_t v34;
            uint32_t v36;
            uint32_t v38;
            uint32_t v40;
            uint32_t v42;
            uint32_t v44;
            uint32_t v46;
            uint32_t v48;
            if ((infoA & 0x10000) != 0) {
                v34 = m_collisionGroups[16];
            }
            else {
                v34 = 0;
            }
            int32_t v35 = v34 | v4;
            if ((infoA & 0x20000) != 0) {
                v36 = m_collisionGroups[17];
            }
            else {
                v36 = 0;
            }
            int32_t v37 = v36 | v35;
            if ((infoA & 0x40000) != 0) {
                v38 = m_collisionGroups[18];
            }
            else {
                v38 = 0;
            }
            int32_t v39 = v38 | v37;
            if ((infoA & 0x80000) != 0) {
                v40 = m_collisionGroups[19];
            }
            else {
                v40 = 0;
            }
            int32_t v41 = v40 | v39;
            if ((infoA & 0x100000) != 0) {
                v42 = m_collisionGroups[20];
            }
            else {
                v42 = 0;
            }
            int32_t v43 = v42 | v41;
            if ((infoA & 0x200000) != 0) {
                v44 = m_collisionGroups[21];
            }
            else {
                v44 = 0;
            }
            int32_t v45 = v44 | v43;
            if ((infoA & 0x400000) != 0) {
                v46 = m_collisionGroups[22];
            }
            else {
                v46 = 0;
            }
            int32_t v47 = v46 | v45;
            if ((infoA & 0x800000) != 0) {
                v48 = m_collisionGroups[23];
            }
            else {
                v48 = 0;
            }
            v4 = v48 | v47;
        }
        if ((infoA & 0xFF000000u) != 0 &&
            ((infoA & 0x1000000) == 0 ? v49 = 0 : v49 = m_collisionGroups[24],
             ((v50 = v49 | v4, (infoA & 0x2000000) == 0) ? v51 = 0 : v51 = m_collisionGroups[25],
              ((v52 = v51 | v50, (infoA & 0x4000000) == 0) ? v53 = 0 : v53 = m_collisionGroups[26],
               ((v54 = v53 | v52, (infoA & 0x8000000) == 0) ? v55 = 0 : v55 = m_collisionGroups[27],
                ((v56 = v55 | v54, (infoA & 0x10000000) == 0) ? v57 = 0 : v57 = m_collisionGroups[28],
                 ((v58 = v57 | v56, (infoA & 0x20000000) == 0) ? v59 = 0 : v59 = m_collisionGroups[29],
                  ((v60 = v59 | v58, (infoA & 0x40000000) == 0) ? v61 = 0 : v61 = m_collisionGroups[30],
                   (v4 = v61 | v60, (infoA & 0x80000000u) != 0))))))))) {
            apResult->m_bool = ((m_collisionGroups[31] | v4) & infoB) != 0;
            return apResult;
        }
        else {
            apResult->m_bool = (v4 & infoB) != 0;
            return apResult;
        }
    }
    else {
        apResult->m_bool = m_noGroupCollisionEnabled.m_bool;
        return apResult;
    }
}

const hkClass& hkpGroupCollisionFilter::staticClass() {
    return hkpGroupCollisionFilterClass;
}
