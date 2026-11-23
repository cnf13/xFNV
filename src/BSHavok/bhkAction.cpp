#include "BSHavok/bhkAction.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorld.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"

NiRTTI bhkAction::ms_RTTI("bhkAction", &bhkSerializable::ms_RTTI);

bhkAction::bhkAction() {
    ++iCount;
}

bhkAction::~bhkAction() {
    bhkSerializable::Kill();
    --iCount;
}

// The base's lines, then the class's RTTI name.
void bhkAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    bhkSerializable::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkAction::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v6 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v6);
}

// The Havok action's user data at +0x10 holds this while it is attached.
void bhkAction::Link(bool abAttach) {
    hkpAction* pAction = this ? static_cast<hkpAction*>(phkObject) : nullptr;
    if (pAction) {
        pAction->m_userData = abAttach ? reinterpret_cast<hkUlong>(this) : 0;
    }
    bhkRefObject::Link(abAttach);
}

// WeaponObject::Constrain calls it on an
// action: the action's world (hkpAction::m_world, at the hkpWorldObject's m_world offset).
const hkpWorld* bhkAction::GethkWorld() const {
    hkpAction* pAction = static_cast<hkpAction*>(phkObject);
    if (pAction) {
        return pAction->m_world;
    }
    return nullptr;
}

// The action's world, or null for a null wrapper or one without an action.
hkpWorld* bhkAction::GethkWorldRw() {
    if (this) {
        hkpAction* pAction = static_cast<hkpAction*>(phkObject);
        if (pAction) {
            return pAction->m_world;
        }
    }
    return nullptr;
}

// Adds the wrapped action to apWorld, first taking it out of the world it is in when that is another one (or when there
// is no world); the whole body runs under the Havok heap context.
bool bhkAction::Add(bhkWorld* apWorld) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bAdded = false;
    hkpAction* pAction = this ? static_cast<hkpAction*>(phkObject) : nullptr;
    if (pAction) {
        if (apWorld) {
            if (GethkWorld() != static_cast<ahkpWorld*>(apWorld->phkObject)) {
                Remove();
                apWorld->Add(pAction);
                bAdded = true;
            }
        }
        else {
            Remove();
        }
    }
    return bAdded;
}

// Takes the wrapped action out of the world its wrapper points at, under the Havok heap context; the result is the
// world's own.
bool bhkAction::Remove() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    bhkWorld* pWorldM = pWorld ? pWorld->m_userData : nullptr;
    bool bRemoved = false;
    if (pWorldM) {
        bRemoved = pWorldM->RemoveAction(static_cast<hkpAction*>(phkObject));
    }
    return bRemoved;
}

void bhkAction::CopyCinfo(hkActionCinfo& arInfo) {
    hkpAction* pAction = static_cast<hkpAction*>(phkObject);
    arInfo.m_userData = pAction ? pAction->m_userData : 0;
}
