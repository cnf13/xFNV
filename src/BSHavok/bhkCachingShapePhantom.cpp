#include "BSHavok/bhkCachingShapePhantom.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/ModelLoader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantom.h"

NiRTTI bhkCachingShapePhantom::ms_RTTI("bhkCachingShapePhantom", &bhkShapePhantom::ms_RTTI);

bhkCachingShapePhantom::bhkCachingShapePhantom() {
    ++iCount;
    ucFlags = 0;
}

bhkCachingShapePhantom::bhkCachingShapePhantom(hkShapePhantomCinfo* arInfo) {
    bhkCachingShapePhantom::Init(*arInfo);
    ++iCount;
    ucFlags = 0;
}

// Builds the wrapped phantom from the info, hands it to SethkObject, drops the constructor's reference and then drops
// the creation info.
void bhkCachingShapePhantom::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkShapePhantomCinfo& rInfo = static_cast<const hkShapePhantomCinfo&>(arInfo);
    hkTransform kTransform;
    hkpCachingShapePhantom* pPhantom = new hkpCachingShapePhantom(
        rInfo.m_shape,
        rInfo.m_transform.ToHK(kTransform),
        rInfo.m_collisionFilterInfo
    );
    SethkObject(pPhantom);
    pPhantom->removeReference();
    PostInit(arInfo);
}

// The scope is charged to the Havok heap context; the info is deleted as the hkShapePhantomCinfo CreateCinfo made.
void bhkCachingShapePhantom::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkShapePhantomCinfo*>(pInfo);
        pInfo = nullptr;
    }
}

// Makes the phantom's creation info: a fresh hkShapePhantomCinfo, filled from the wrapped phantom when there is one.
hkSerializableCinfo& bhkCachingShapePhantom::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const bool bCreated = this->pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        hkShapePhantomCinfo* pInfo = new hkShapePhantomCinfo;
        const bool bHasObject = this->phkObject != nullptr;
        this->pInfo = pInfo;
        if (bHasObject) {
            CopyCinfo(*pInfo);
        }
    }
    return *this->pInfo;
}

const NiRTTI* bhkCachingShapePhantom::GetRTTI() const {
    return &bhkCachingShapePhantom::ms_RTTI;
}

bhkCachingShapePhantom::~bhkCachingShapePhantom() {
    bhkSerializable::Kill();
    --iCount;
}

// Clones the phantom into memory charged to the Havok context, under the engine's clone critical section.
NiObject* bhkCachingShapePhantom::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkCachingShapePhantom* pClone = nullptr;
    bhkSerializable::CloneCritLock();
    pClone = new bhkCachingShapePhantom;
    CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Adds the phantom's own RTTI title to the array, in the Havok context.
void bhkCachingShapePhantom::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkShapePhantom::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.m_pcName);
    uint32_t uiSize = apStrings->m_usSize;
    uint32_t uiMaxSize = apStrings->m_usMaxSize;
    if (uiSize >= uiMaxSize) {
        apStrings->SetSize(uiSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(uiSize, pcString);
}

// The class's factory: allocates the 0x18-byte phantom and constructs it in place.
NiObject* bhkCachingShapePhantom::CreateObject() {
    return new bhkCachingShapePhantom;
}

// Casts the ray into each cached collidable's space and against its shape, from the last detail to the first,
// skipping collidables the game's filter keeps apart from the ray; a hit nearer than the best so far gives arOutput
// its normal, hit fraction and root collidable (the rest of arOutput is left alone). The best normal is turned back
// into world space at the end.
void hkpCachingShapePhantomRay::castRay(const hkpWorldRayCastInput& arInput, hkpWorldRayCastOutput& arOutput) {
    HK_TIMER_BEGIN("rcCachingPhantom", nullptr);
    hkpShapeRayCastOutput sout;
    hkpShapeRayCastInput sinput;
    sinput.m_filterInfo = arInput.m_filterInfo;
    hkpCollisionFilter* pFilter;
    if (arInput.m_enableShapeCollectionFilter.m_bool && (pFilter = m_world->m_collisionFilter) != nullptr) {
        sinput.m_rayShapeCollectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(pFilter);
    }
    else {
        sinput.m_rayShapeCollectionFilter = nullptr;
    }
    bool bHit = false;
    for (int32_t i = m_collisionDetails.m_size - 1; i >= 0; --i) {
        hkpCollidable* pCollidable = m_collisionDetails.m_data[i].m_collidable;
        const hkpShape* pShape = pCollidable->m_shape;
        if (!pShape) {
            continue;
        }
        if (!bhkCollisionFilter::IsCollisionEnabled(
                CFilter{arInput.m_filterInfo},
                CFilter{pCollidable->m_broadPhaseHandle.m_collisionFilterInfo}
            )) {
            continue;
        }
        const hkTransform& kTransform = *pCollidable->m_motion.pMotion;
        sinput.m_from.setTransformedInversePos(kTransform, arInput.m_from);
        sinput.m_to.setTransformedInversePos(kTransform, arInput.m_to);
        sout.reset_0();
        if (pShape->castRay(sinput, sout).m_bool && (!bHit || arOutput.m_hitFraction > sout.m_hitFraction))
        // Only a strictly greater best fraction (ordered compare) is replaced.
        {
            arOutput.m_hitFraction = sout.m_hitFraction;
            bHit = true;
            arOutput.m_normal.m_quad = sout.m_normal.m_quad;
            arOutput.m_rootCollidable = pCollidable;
        }
    }
    if (arOutput.m_rootCollidable) {
        arOutput.m_normal.setRotatedDir(arOutput.m_rootCollidable->m_motion.pMotion->m_rotation, arOutput.m_normal);
    }
    HK_TIMER_END();
}
