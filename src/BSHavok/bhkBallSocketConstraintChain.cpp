#include "BSHavok/bhkBallSocketConstraintChain.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConstraintChain.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkStream.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/BallSocket/hkpBallSocketChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

#include <xmmintrin.h>

NiRTTI bhkBallSocketConstraintChain::ms_RTTI("bhkBallSocketConstraintChain", &bhkConstraint::ms_RTTI);

hkConstraintChainCinfo::hkConstraintChainCinfo() {}

// The arrays free their own buffers (EntitiesA, PivotsA); hkConstraintCinfo's destructor clears the constraint data.
hkConstraintChainCinfo::~hkConstraintChainCinfo() {}

void hkConstraintChainCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintCinfo::GetViewerStrings(apStrings);
}

hkBallSocketChainCinfo::hkBallSocketChainCinfo() {}

hkBallSocketChainCinfo::~hkBallSocketChainCinfo() {}

// Copies this cinfo's chain data into apData (made from the Havok heap when null): the per-link pivots, then scaled by
// the cloning process's scale on both vectors of every link, then the base's entities and priority.
void hkBallSocketChainCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpBallSocketChainData* pSource = static_cast<hkpBallSocketChainData*>(pData);
    hkpBallSocketChainData* pDest = static_cast<hkpBallSocketChainData*>(apData);
    if (!pDest) {
        pDest = new hkpBallSocketChainData;
    }
    const int32_t iNumInfos = pSource->m_infos.m_size;
    pDest->m_infos.reserve(iNumInfos);
    pDest->m_infos.m_size = static_cast<uint32_t>(iNumInfos);
    for (int32_t i = 0; i < pSource->m_infos.m_size; ++i) {
        pDest->m_infos.m_data[i] = pSource->m_infos.m_data[i];
    }
    // The clone process is always a BSCloningProcess; Scale.x is read from it.
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        for (hkpBallSocketChainData::ConstraintInfo& rInfo : pDest->m_infos) {
            const hkSimdReal kScale(fScale);
            rInfo.m_pivotInA.mul4(kScale);
            rInfo.m_pivotInB.mul4(kScale);
        }
    }
    hkConstraintCinfo::CloneData(pDest, arCloneData);
}

// Creates the chain data, reads the pivots and adds them to the data in pairs (each link's pivot in A and in B),
// reads the four solver floats, then the base part (the entities' link IDs and hkConstraintCinfo's).
void hkBallSocketChainCinfo::LoadBinary(NiStream& arStream) {
    CreateData();
    bhkStream::LoadBinary(arStream, PivotsA);
    hkpBallSocketChainData* pd = static_cast<hkpBallSocketChainData*>(pData);
    for (int32_t i = 0; i < PivotsA.m_size; i += 2) {
        pd->addConstraintInfoInBodySpace(PivotsA.m_data[i], PivotsA.m_data[i + 1]);
    }
    uint32_t uiSize = 4;
    arStream.Istr()->BinaryRead(&pd->m_damping, 4, &uiSize, 1);
    uiSize = 4;
    arStream.Istr()->BinaryRead(&pd->m_tau, 4, &uiSize, 1);
    uiSize = 4;
    arStream.Istr()->BinaryRead(&pd->m_cfm, 4, &uiSize, 1);
    uiSize = 4;
    arStream.Istr()->BinaryRead(&pd->m_maxErrorDistance, 4, &uiSize, 1);
    hkConstraintChainCinfo::LoadBinary(arStream);
}

void hkBallSocketChainCinfo::SaveBinary(NiStream& arStream) {
    uint32_t v13;
    NiStream* v2 = &arStream;
    hkpBallSocketChainData* pd = static_cast<hkpBallSocketChainData*>(pData);
    bhkStream::SaveBinary(arStream, PivotsA);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v13 = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_damping, 4, &v13, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v13 = 4;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_tau, 4, &v13, 1);
    NiBinaryStream* v9 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v10 = v9->m_pfnWrite;
    v13 = 4;
    v9->m_uiAbsoluteCurrentPos += v10(v9, &pd->m_cfm, 4, &v13, 1);
    NiBinaryStream* v11 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v12 = v11->m_pfnWrite;
    v13 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_maxErrorDistance, 4, &v13, 1);
    hkConstraintChainCinfo::SaveBinary(*v2);
}

uint32_t hkBallSocketChainCinfo::CreateData() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (!pData) {
        pData = new hkpBallSocketChainData;
    }
    return sizeof(hkpBallSocketChainData);
}

void hkBallSocketChainCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintChainCinfo::GetViewerStrings(apStrings);
}

bhkBallSocketConstraintChain::bhkBallSocketConstraintChain() {
    ++iCount;
}

bhkBallSocketConstraintChain::~bhkBallSocketConstraintChain() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkBallSocketConstraintChain::GetRTTI() const {
    return &bhkBallSocketConstraintChain::ms_RTTI;
}

// A new chain built under the engine's clone critical section and filled from this one; the whole body charges its
// allocations to Havok's heap context. The clone is built even when the allocation fails (the copy then gets null).
NiObject* bhkBallSocketConstraintChain::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bhkSerializable::CloneCritLock();
    bhkBallSocketConstraintChain* pClone = new bhkBallSocketConstraintChain;
    bhkConstraintChain::CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

uint32_t bhkBallSocketConstraintChain::QCinfoSize() const {
    return 44;
}

// Keeps the cinfo's hkSerializableCinfo base and passes the object itself to CopyCinfo.
hkSerializableCinfo& bhkBallSocketConstraintChain::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        hkBallSocketChainCinfo* pCinfo = new hkBallSocketChainCinfo;
        pInfo = pCinfo;
        if (phkObject) {
            CopyCinfo(pCinfo);
        }
    }
    return *pInfo;
}

// Copies the chain's members, then the two pivots of each link of the wrapped instance's data. The link count is read
// on every pass, as is the source array.
void bhkBallSocketConstraintChain::CopyCinfo(hkBallSocketChainCinfo* arInfo) {
    bhkConstraintChain::CopyCinfo(arInfo);
    hkpConstraintInstance* pInstance = static_cast<hkpConstraintInstance*>(phkObject);
    hkArray<hkVector4>& rPivots = arInfo->PivotsA;
    rPivots.m_size = 0;
    hkpBallSocketChainData* pData = static_cast<hkpBallSocketChainData*>(pInstance->m_data);
    rPivots.setSize(2 * pData->m_infos.m_size);
    for (int32_t i = 0; i < pData->m_infos.m_size; ++i) {
        rPivots.m_data[2 * i] = pData->m_infos.m_data[i].m_pivotInA;
        rPivots.m_data[2 * i + 1] = pData->m_infos.m_data[i].m_pivotInB;
    }
}

// A new chain with no creation info yet.
NiObject* bhkBallSocketConstraintChain::CreateObject() {
    return new bhkBallSocketConstraintChain;
}

bhkConstraintChain::bhkConstraintChain() {
    ++iCount;
}

bhkConstraintChain::~bhkConstraintChain() {
    bhkSerializable::Kill();
    --bhkConstraintChain::iCount;
}

const NiRTTI* bhkConstraintChain::GetRTTI() const {
    return &bhkConstraintChain::ms_RTTI;
}
