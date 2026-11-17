#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysModifier::ms_RTTI("NiPSysModifier", &NiObject::ms_RTTI);

NiPSysModifier::NiPSysModifier() : m_uiOrder(0), m_pkTarget(nullptr), m_bActive(true) {}

NiPSysModifier::NiPSysModifier(const NiFixedString& kName, uint32_t uiOrder)
  : m_kName(kName), m_uiOrder(uiOrder), m_pkTarget(nullptr), m_bActive(true) {}

// The name's reference is dropped by NiFixedString's destructor.
NiPSysModifier::~NiPSysModifier() {}

// Older files store the name as a plain string. The target link is read and dropped: the particle system sets
// the pointer when it takes the modifier.
void NiPSysModifier::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1)) {
        kStream.LoadCStringAsFixedString(m_kName);
    }
    else {
        kStream.LoadFixedString(m_kName);
    }
    NiStreamLoadBinary(kStream, m_uiOrder);
    kStream.ReadLinkID();

    uint8_t ucActive;
    NiStreamLoadBinary(kStream, ucActive);
    m_bActive = ucActive != 0;
}

// KEEP: ucActive is read into arStream's argument slot (lea eax,[esp+0x20] at 0xC31924); a short read leaves it
// holding the argument's low byte, which the exe then tests and stores in m_bActive (0xC3193A).
// The target link is consumed without being resolved (see LoadBinary).
void NiPSysModifier::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
    kStream.GetObjectFromLinkID();
}

bool NiPSysModifier::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }

    kStream.RegisterFixedString(m_kName);
    return true;
}

void NiPSysModifier::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    kStream.SaveFixedString(m_kName);
    NiStreamSaveBinary(kStream, m_uiOrder);
    kStream.SaveLinkID(m_pkTarget);
    uint8_t ucActive = m_bActive;
    NiStreamSaveBinary(kStream, ucActive);
}

// The targets are not compared.
bool NiPSysModifier::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiPSysModifier* pkOther = static_cast<NiPSysModifier*>(pkObject);
    return !(!(m_kName == pkOther->m_kName) || m_uiOrder != pkOther->m_uiOrder || m_bActive != pkOther->m_bActive);
}

void NiPSysModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Name", static_cast<const char*>(m_kName)));
    pkStrings->Add(NiGetViewerString("Order", m_uiOrder));
    pkStrings->Add(NiGetViewerString("Active", m_bActive));
}

// The clone points at the clone of the target. A target that was not cloned leaves the lookup's value unchanged,
// so the clone then points at itself.
void NiPSysModifier::ProcessClone(NiCloningProcess& kCloning) {
    NiObject::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysModifier* pkClone = static_cast<NiPSysModifier*>(pkObject);
    kCloning.m_pkCloneMap->GetAt(m_pkTarget, pkObject);
    pkClone->m_pkTarget = static_cast<NiParticleSystem*>(pkObject);
}

void NiPSysModifier::CopyMembers(NiPSysModifier* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);
    const char* m_kHandle = m_kName.GetHandle();
    if (pkDest->m_kName.GetHandle() != m_kHandle) {
        NiGlobalStringTable::AcquireHandle(m_kHandle);
        const char* v5 = pkDest->m_kName.GetHandle();
        NiGlobalStringTable::ReleaseHandle(v5);
        pkDest->m_kName.m_kHandle = m_kHandle;
    }
    pkDest->m_uiOrder = m_uiOrder;
    pkDest->m_bActive = m_bActive;
}
