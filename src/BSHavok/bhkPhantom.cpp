#include "BSHavok/bhkPhantom.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

NiRTTI bhkPhantom::ms_RTTI("bhkPhantom", &bhkWorldObject::ms_RTTI);

hkPhantomCinfo::hkPhantomCinfo() {
    m_broadPhaseType = hkpWorldObject::BROAD_PHASE_PHANTOM;
}

hkPhantomCinfo::~hkPhantomCinfo() {}

bhkPhantom::bhkPhantom() {
    ++iCount;
}

bhkPhantom::~bhkPhantom() {
    bhkSerializable::Kill();
    --iCount;
}

// The world object's lines, then the phantom's RTTI name; the whole body charges its allocations to Havok's heap
// context.
void bhkPhantom::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkWorldObject::GetViewerStrings(apStrings);

    char* pcString = NiGetViewerString(bhkPhantom::ms_RTTI.GetName());
    apStrings->Add(pcString);
    // SWEEP-INLINE 0x96AE90
}

bool bhkPhantom::Remove() {
    bhkWorld* v3;
    ahkpWorld* v2 = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    if (v2) {
        v3 = static_cast<bhkWorld*>(v2->m_userData);
    }
    else {
        v3 = nullptr;
    }
    return (bool)((uint8_t)(v3 && v3->Remove(static_cast<hkpPhantom*>(phkObject))));
}

void bhkPhantom::UpdateCFilter() {
    ahkpWorld* pkWorld = static_cast<ahkpWorld*>(GethkWorldRw());
    if (pkWorld) {
        pkWorld->updateCollisionFilterOnPhantom(
            static_cast<hkpPhantom*>(phkObject), HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
        );
    }
}

void bhkPhantom::ResetPhantomFlags() {
    ucFlags = 0;
}

// The wrapper apPhantom's user data points back to, null for a null apPhantom.
bhkPhantom* bhkPhantom::Getbhk(const hkpWorldObject* apPhantom) {
    return static_cast<bhkPhantom*>(bhkWorldObject::Getbhk(apPhantom));
}
