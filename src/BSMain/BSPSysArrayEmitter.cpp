#include "BSMain/BSPSysArrayEmitter.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstring>

NiRTTI BSPSysArrayEmitter::ms_RTTI("BSPSysArrayEmitter", &NiPSysVolumeEmitter::ms_RTTI);
NiRTTI PArrayPoint::ms_RTTI("PArrayPoint", &NiExtraData::ms_RTTI);

void ObjectArray_SetAt(
    NiTPrimitiveArray<NiPointer<NiAVObject>>* apArray,
    uint32_t auiIndex,
    const NiPointer<NiAVObject>& arElement
) {
    // tcmp is the zero element.
    static NiPointer<NiAVObject> tcmp;
    if (auiIndex < apArray->m_usSize) {
        if (arElement.m_pObject == tcmp.m_pObject) {
            if (apArray->m_pBase[auiIndex].m_pObject != tcmp.m_pObject) {
                --apArray->m_usESize;
            }
        }
        else {
            if (apArray->m_pBase[auiIndex].m_pObject == tcmp.m_pObject) {
                ++apArray->m_usESize;
            }
        }
    }
    else {
        apArray->m_usSize = auiIndex + 1;
        if (arElement.m_pObject != tcmp.m_pObject) {
            ++apArray->m_usESize;
        }
    }
    apArray->m_pBase[auiIndex].operator=(arElement);
}

uint32_t ObjectArray_Add(NiTPrimitiveArray<NiPointer<NiAVObject>>* apArray, const NiPointer<NiAVObject>& arElement) {
    uint32_t auiIndex = apArray->m_usSize;
    if (auiIndex >= apArray->m_usMaxSize) {
        apArray->SetSize(auiIndex + apArray->m_usGrowBy);
    }
    ObjectArray_SetAt(apArray, auiIndex, arElement);
    return auiIndex;
}

void ObjectArray_Compact(NiTPrimitiveArray<NiPointer<NiAVObject>>* apArray) {
    if (apArray->m_usESize != apArray->m_usSize) {
        if (apArray->m_usESize) {
            uint16_t v4 = 0;
            uint16_t v3 = 0;
            while (v4 < static_cast<int32_t>(apArray->m_usSize)) {
                if (apArray->m_pBase[v4].m_pObject) {
                    if (apArray->m_pBase[v3].m_pObject != apArray->m_pBase[v4].m_pObject) {
                        apArray->m_pBase[v3].operator=(apArray->m_pBase[v4]);
                    }
                    ++v3;
                }
                ++v4;
            }
        }
        NiPointer<NiAVObject>* pvMem = apArray->m_pBase;
        apArray->m_usSize = apArray->m_usESize;
        apArray->m_usMaxSize = apArray->m_usSize;
        if (apArray->m_usMaxSize) {
            apArray->m_pBase =
                static_cast<NiPointer<NiAVObject>*>(_NiMalloc(sizeof(NiPointer<NiAVObject>) * apArray->m_usMaxSize));
            for (uint16_t i = 0; i < static_cast<int32_t>(apArray->m_usSize); ++i) {
                apArray->m_pBase[i].operator=(pvMem[i]);
            }
        }
        else {
            apArray->m_pBase = 0;
        }
        _NiFree(pvMem);
    }
}

PArrayPoint::PArrayPoint() : NiExtraData(*pTag) {}

PArrayPoint::~PArrayPoint() {}

NiObject* PArrayPoint::CreateClone(NiCloningProcess& arCloneData) {
    return this;
}

// The tag is one interned NiFixedString in a MemoryManager block, taking AddString's reference.
void PArrayPoint::InitSDM() {
    NiFixedString* pkTag = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pkTag) {
        pkTag->SetHandle(NiGlobalStringTable::AddString("PAP"));
    }
    PArrayPoint::pTag = pkTag;
}

// This is the whole `NiNew PArrayPoint` expansion: it allocates the object, constructs it, then clears the name.
PArrayPoint* PArrayPoint::CreateInstance() {
    void* pvMemory = NiMemObject::operator new(sizeof(PArrayPoint));
    PArrayPoint* pkPoint;
    if (pvMemory) {
        pkPoint = ::new (pvMemory) PArrayPoint;
    }
    else {
        pkPoint = nullptr;
    }
    // The name is cleared on this path too, so a failed allocation dereferences null here as well, as the game does.
    pkPoint->SetName(EmptyFS);
    return pkPoint;
}

void PArrayPoint::KillSDM() {
    NiFixedString* Block = PArrayPoint::pTag;
    if (PArrayPoint::pTag) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(PArrayPoint::pTag->GetHandle());
        MemoryManager_Singleton.Deallocate(Block);
    }
    PArrayPoint::pTag = 0;
}

BSPSysArrayEmitter::BSPSysArrayEmitter() : eType(PAT_NODES), aObjects(0, 1) {}

BSPSysArrayEmitter::BSPSysArrayEmitter(
    char* pcName,
    NiAVObject* uiOrder,
    int32_t aeType,
    float fSpeed,
    float fSpeedVar,
    float fDeclination,
    float fDeclinationVar,
    float fPlanarAngle,
    float fPlanarAngleVar,
    NiColorA aInitialColor,
    float fInitialRadius,
    float fLifeSpan,
    float fLifeSpanVar,
    float fRadiusVar
)
  : NiPSysVolumeEmitter(
        pcName,
        uiOrder,
        fSpeed,
        fSpeedVar,
        fDeclination,
        fDeclinationVar,
        fPlanarAngle,
        fPlanarAngleVar,
        aInitialColor,
        fInitialRadius,
        fLifeSpan,
        fLifeSpanVar,
        fRadiusVar
    ),
    eType(static_cast<PARTICLEARRAYTYPE>(aeType)), aObjects(0, 1) {
    SetupArray();
}

// aObjects frees its block itself; the array's entries are deliberately not released, as the game does.
BSPSysArrayEmitter::~BSPSysArrayEmitter() {}

NiObject* BSPSysArrayEmitter::CreateClone(NiCloningProcess& arCloneData) {
    BSPSysArrayEmitter* v3;
    BSPSysArrayEmitter* apMemory =
        static_cast<BSPSysArrayEmitter*>(NiMemObject::operator new(sizeof(BSPSysArrayEmitter)));
    if (apMemory) {
        v3 = ::new (apMemory) BSPSysArrayEmitter;
    }
    else {
        v3 = 0;
    }
    CopyMembers(v3, arCloneData);
    return v3;
}

// The array type is not streamed.
void BSPSysArrayEmitter::LoadBinary(NiStream& arStream) {
    NiPSysVolumeEmitter::LoadBinary(arStream);
}

void BSPSysArrayEmitter::LinkObject(NiStream& arStream) {
    NiPSysVolumeEmitter::LinkObject(arStream);
}

bool BSPSysArrayEmitter::RegisterStreamables(NiStream& arStream) {
    return NiPSysVolumeEmitter::RegisterStreamables(arStream);
}

void BSPSysArrayEmitter::SaveBinary(NiStream& arStream) {
    NiPSysVolumeEmitter::SaveBinary(arStream);
}

// Neither the array type nor the array is compared.
bool BSPSysArrayEmitter::IsEqual(NiObject* apObject) {
    return NiPSysVolumeEmitter::IsEqual(apObject);
}

// The objects are listed from the last to the first, each labelled "E".
void BSPSysArrayEmitter::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiPSysVolumeEmitter::GetViewerStrings(apStrings);

    apStrings->Add(NiGetViewerString(BSPSysArrayEmitter::ms_RTTI.GetName()));
    apStrings->Add(GetViewerString("ArrayType", eType));
    for (uint32_t i = aObjects.GetSize(); i--;) {
        NiAVObject* pkObject = aObjects.m_pBase[i];
        if (pkObject) {
            apStrings->Add(NiGetViewerString("E", (const char*)pkObject->m_kName));
        }
    }
}

// Entries without a clone are removed, and the array is compacted when any was.
void BSPSysArrayEmitter::ProcessClone(NiCloningProcess& arCloneData) {
    NiPSysVolumeEmitter::ProcessClone(arCloneData);

    NiObject* pkObject;
    if (!arCloneData.m_pkCloneMap->GetAt(this, pkObject)) {
        return;
    }
    BSPSysArrayEmitter* pkClone = static_cast<BSPSysArrayEmitter*>(pkObject);
    uint32_t uiCloned = 0;
    for (uint32_t i = pkClone->aObjects.GetSize(); i--;) {
        NiObject* pkClonedObject;
        if (arCloneData.m_pkCloneMap->GetAt(pkClone->aObjects.m_pBase[i], pkClonedObject)) {
            uiCloned++;
            ObjectArray_SetAt(&pkClone->aObjects, i, NiPointer<NiAVObject>(static_cast<NiAVObject*>(pkClonedObject)));
        }
        else {
            pkClone->aObjects.RemoveAt(i);
        }
    }
    if (uiCloned != pkClone->aObjects.GetSize()) {
        ObjectArray_Compact(&pkClone->aObjects);
    }
}

// The array is set up before the base call, which does nothing.
void BSPSysArrayEmitter::PostLinkObject(NiStream& arStream) {
    SetupArray();
    NiPSysVolumeEmitter::PostLinkObject(arStream);
}

void BSPSysArrayEmitter::SetEmitterObj(NiAVObject* apEmitter) {
    m_pkEmitterObj = apEmitter;
    SetupArray();
}

// The emitter object's transform is not used: the points come from the array's objects.
void BSPSysArrayEmitter::ComputeVolumeInitialPositionAndVelocity(
    NiTransform& arEmitterToPSys,
    NiPoint3& arPosition,
    NiPoint3& arVelocity
) {
    GetAPoint(arPosition, arVelocity);
}

// Only an emitter object that is a node is walked, and only below it: the node itself is never added.
void BSPSysArrayEmitter::SetupArray() {
    aObjects.RemoveAll();

    NiNode* pkNode = m_pkEmitterObj ? m_pkEmitterObj->IsNode() : nullptr;
    if (pkNode) {
        NiUpdateData kUpdateData;
        kUpdateData.fTime = 0.0f;
        kUpdateData.bUpdateControllers = false;
        kUpdateData.bParallelUpdate = false;
        kUpdateData.bFoundParticles = false;
        kUpdateData.bFoundMorphController = false;
        kUpdateData.bSceneGraphChange = false;
        pkNode->Update(kUpdateData);
        for (uint32_t i = pkNode->m_kChildren.GetSize(); i--;) {
            SetObjectPointers(pkNode->m_kChildren.m_pBase[i]);
        }
    }
}

void BSPSysArrayEmitter::CopyMembers(BSPSysArrayEmitter* apDest, NiCloningProcess& arCloneData) {
    NiPSysVolumeEmitter::CopyMembers(apDest, arCloneData);
    uint32_t auiIndex = (uint32_t)aObjects.m_usSize;
    if (aObjects.m_usSize) {
        apDest->aObjects.SetSize(auiIndex);
        while (auiIndex--) {
            ObjectArray_SetAt(&apDest->aObjects, auiIndex, aObjects.m_pBase[auiIndex]);
        }
    }
}

// The emitter object itself is never added, nor walked below.
void BSPSysArrayEmitter::SetObjectPointers(NiAVObject* pObject) {
    if (!pObject) {
        return;
    }
    NiNode* pkNode = pObject->IsNode();
    if (pObject == m_pkEmitterObj) {
        return;
    }

    bool bAdd;
    switch (eType) {
        case PAT_VERTICES: bAdd = pObject->IsGeometry() != nullptr; break;
        case PAT_SPECIFICOBJS: bAdd = pObject->GetExtraData(*PArrayPoint::pTag) != nullptr; break;
        default: bAdd = pkNode != nullptr; break;
    }
    if (bAdd) {
        ObjectArray_Add(&aObjects, NiPointer<NiAVObject>(pObject));
    }

    if (pkNode) {
        for (uint32_t i = pkNode->m_kChildren.GetSize(); i--;) {
            SetObjectPointers(pkNode->m_kChildren.m_pBase[i]);
        }
    }
}

// The index is drawn in double precision and clamped below the size (the random can be exactly 1).
void BSPSysArrayEmitter::GetAPoint(NiPoint3& arPos, NiPoint3& arVel) {
    NiAVObject* pkObject;
    uint32_t uiSize = aObjects.GetSize();
    if (uiSize) {
        double dSize = uiSize;
        uint32_t uiIndex = static_cast<uint32_t>(NiUnitRandom() * dSize);
        if (uiIndex == uiSize) {
            uiIndex--;
        }
        pkObject = aObjects.m_pBase[uiIndex];
    }
    else {
        pkObject = nullptr;
    }

    if (eType == PAT_VERTICES) {
        GetPointFromGeometry(pkObject, arPos, arVel);
    }
    else {
        GetPointFromObject(pkObject, arPos, arVel);
    }
}

// Nodes give a random point on the segment from their parent to them, brought out of the particle system's world
// scale; specific objects give their own position and turn the velocity by their world rotation.
void BSPSysArrayEmitter::GetPointFromObject(NiAVObject* pObject, NiPoint3& arPos, NiPoint3& arVel) {
    if (pObject) {
        NiNode* pkParent = pObject->m_pkParent;
        if (eType != PAT_SPECIFICOBJS && pkParent) {
            arPos = pObject->m_kWorld.m_Translate - pkParent->m_kWorld.m_Translate;
            arPos = pkParent->m_kWorld.m_Translate + arPos * NiUnitRandom();
            if (m_pkTarget) {
                float fScale = m_pkTarget->m_kWorld.m_fScale;
                if (fScale != 1.0 && fScale != 0.0) {
                    arPos *= 1.0f / fScale;
                }
            }
        }
        else {
            arPos = pObject->m_kWorld.m_Translate;
            if (eType == PAT_SPECIFICOBJS) {
                arVel = pObject->m_kWorld.m_Rotate * arVel;
            }
        }
    }
    else {
        arPos = NiPoint3_ZERO;
    }
}

// The vertex is scaled but neither rotated nor translated; the velocity keeps its speed.
void BSPSysArrayEmitter::GetPointFromGeometry(NiAVObject* pObject, NiPoint3& arPos, NiPoint3& arVel) {
    NiGeometry* pkGeometry = pObject ? pObject->IsGeometry() : nullptr;
    if (pkGeometry) {
        uint16_t usVertices = pkGeometry->m_spModelData->GetActiveVertexCount();
        double dVertices = usVertices;
        uint32_t uiIndex = static_cast<uint32_t>(NiUnitRandom() * dVertices);
        if (uiIndex == usVertices) {
            uiIndex--;
        }
        arPos = pkGeometry->m_spModelData->m_pkVertex[uiIndex];
        arPos *= pObject->m_kWorld.m_fScale;
        if (pkGeometry->m_spModelData->m_pkNormal) {
            NiPoint3 kNormal = pkGeometry->m_spModelData->m_pkNormal[uiIndex];
            kNormal = pObject->m_kWorld.m_Rotate * kNormal;
            arVel = kNormal * arVel.Length();
        }
    }
    else {
        arPos = NiPoint3_ZERO;
    }
}

// Allocated with room for the prefix and the longest suffix; an unknown type leaves the block unwritten.
char* BSPSysArrayEmitter::GetViewerString(const char* apPrefix, PARTICLEARRAYTYPE aeType) {
    size_t stLength = strlen(apPrefix) + 16;
    char* pcString = static_cast<char*>(MemoryManager_Singleton.Allocate(stLength));
    switch (aeType) {
        case PAT_VERTICES: BSsprintf(pcString, stLength, "%s = AT_VERTICES", apPrefix); break;
        case PAT_SPECIFICOBJS: BSsprintf(pcString, stLength, "%s = AT_SPECIFICOBJS", apPrefix); break;
        case PAT_NODES: BSsprintf(pcString, stLength, "%s = AT_NODES", apPrefix); break;
    }
    return pcString;
}

NiObject* BSPSysArrayEmitter::CreateObject() {
    void* v2 = NiMemObject::operator new(sizeof(BSPSysArrayEmitter));
    if (v2) {
        return ::new (v2) BSPSysArrayEmitter;
    }
    else {
        return nullptr;
    }
}

NiPSysEmitter::~NiPSysEmitter() {}

NiPSysVolumeEmitter::~NiPSysVolumeEmitter() {}
