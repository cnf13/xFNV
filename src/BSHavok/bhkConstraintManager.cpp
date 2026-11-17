#include "BSHavok/bhkConstraintManager.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBallAndSocketConstraint.h"
#include "BSHavok/bhkBreakableConstraint.h"
#include "BSHavok/bhkGenericConstraint.h"
#include "BSHavok/bhkHingeConstraint.h"
#include "BSHavok/bhkLimitedHingeConstraint.h"
#include "BSHavok/bhkMalleableConstraint.h"
#include "BSHavok/bhkPrismaticConstraint.h"
#include "BSHavok/bhkRagdollConstraint.h"
#include "BSHavok/bhkStiffSpringConstraint.h"
#include "BSHavok/bhkWheelConstraint.h"
#include "BSHavok/hkPointToPathConstraintCinfo.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

const char* bhkConstraintManager::GetTypeName(uint32_t aiType) {
    const char* result;
    switch (aiType) {
        case 0u: {
            result = "hkBallAndSocketConstraint";
            break;
        }
        case 1u: {
            result = "hkHingeConstraint";
            break;
        }
        case 2u: {
            result = "hkLimitedHingeConstraint";
            break;
        }
        case 3u: {
            result = "hkPointToPathConstraint";
            break;
        }
        case 6u: {
            result = "hkPrismaticConstraint";
            break;
        }
        case 7u: {
            result = "hkRagdollConstraint";
            break;
        }
        case 8u: {
            result = "hkStiffSpringConstraintC";
            break;
        }
        case 9u: {
            result = "hkWheelConstraint";
            break;
        }
        case 10u: {
            result = "hkGenericConstraint";
            break;
        }
        case 12u: {
            result = "hkBreakableConstraint";
            break;
        }
        case 13u: {
            result = "hkMalleableConstraint";
            break;
        }
        default: {
            result = const_cast<char*>("Unknown");
            break;
        }
    }
    return result;
}

hkConstraintCinfo* bhkConstraintManager::GetInfo(uint32_t aiType) {
    hkConstraintCinfo* result;
    TLSData* v1 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v1->eMemContext;
    v1->eMemContext = MEM_CONTEXT_HAVOK;
    switch (aiType) {
        case 0u: {
            hkBallAndSocketConstraintCinfo* v3 = static_cast<hkBallAndSocketConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkBallAndSocketConstraintCinfo))
            );
            if (!v3) {
                goto LABEL_24;
            }
            result = ::new (v3) hkBallAndSocketConstraintCinfo;
            break;
        }
        case 1u: {
            hkHingeConstraintCinfo* v5 =
                static_cast<hkHingeConstraintCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkHingeConstraintCinfo)));
            if (!v5) {
                goto LABEL_24;
            }
            result = ::new (v5) hkHingeConstraintCinfo;
            break;
        }
        case 2u: {
            hkLimitedHingeConstraintCinfo* v6 = static_cast<hkLimitedHingeConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkLimitedHingeConstraintCinfo))
            );
            if (!v6) {
                goto LABEL_24;
            }
            result = ::new (v6) hkLimitedHingeConstraintCinfo;
            break;
        }
        case 3u: {
            hkPointToPathConstraintCinfo* v7 = static_cast<hkPointToPathConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkPointToPathConstraintCinfo))
            );
            if (!v7) {
                goto LABEL_24;
            }
            result = ::new (v7) hkPointToPathConstraintCinfo;
            break;
        }
        case 6u: {
            hkPrismaticConstraintCinfo* v8 = static_cast<hkPrismaticConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkPrismaticConstraintCinfo))
            );
            if (!v8) {
                goto LABEL_24;
            }
            result = ::new (v8) hkPrismaticConstraintCinfo;
            break;
        }
        case 7u: {
            hkRagdollConstraintCinfo* v9 = static_cast<hkRagdollConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkRagdollConstraintCinfo))
            );
            if (!v9) {
                goto LABEL_24;
            }
            result = ::new (v9) hkRagdollConstraintCinfo;
            break;
        }
        case 8u: {
            hkStiffSpringConstraintCinfo* v10 = static_cast<hkStiffSpringConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkStiffSpringConstraintCinfo))
            );
            if (!v10) {
                goto LABEL_24;
            }
            result = ::new (v10) hkStiffSpringConstraintCinfo;
            break;
        }
        case 9u: {
            hkWheelConstraintCinfo* v11 =
                static_cast<hkWheelConstraintCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkWheelConstraintCinfo)));
            if (!v11) {
                goto LABEL_24;
            }
            result = ::new (v11) hkWheelConstraintCinfo;
            break;
        }
        case 10u: {
            hkGenericConstraintCinfo* v12 = static_cast<hkGenericConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkGenericConstraintCinfo))
            );
            if (!v12) {
                goto LABEL_24;
            }
            result = ::new (v12) hkGenericConstraintCinfo;
            break;
        }
        case 12u: {
            hkBreakableConstraintCinfo* v13 = static_cast<hkBreakableConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkBreakableConstraintCinfo))
            );
            if (!v13) {
                goto LABEL_24;
            }
            result = ::new (v13) hkBreakableConstraintCinfo;
            break;
        }
        case 13u: {
            hkMalleableConstraintCinfo* v14 = static_cast<hkMalleableConstraintCinfo*>(
                MemoryManager_Singleton.Allocate(sizeof(hkMalleableConstraintCinfo))
            );
            if (!v14) {
                goto LABEL_24;
            }
            result = ::new (v14) hkMalleableConstraintCinfo;
            break;
        }
        default: {
        LABEL_24:;
            result = nullptr;
            break;
        }
    }
    v1->eMemContext = eMemContext;
    return result;
}

hkpConstraintData* bhkConstraintManager::Clone(hkpConstraintData* apData, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    hkConstraintCinfo* pkInfo = GetInfo(apData->getType());
    if (!pkInfo) {
        return nullptr;
    }
    pkInfo->SetData(apData);
    // Passes a null constraint data as CloneData's first argument, as the game does.
    pkInfo->CloneData(nullptr, arCloneData);
    hkReferencedObject* pkData = pkInfo->pData;
    pkData->addReference();
    delete pkInfo;
    return static_cast<hkpConstraintData*>(pkData);
}

hkpConstraintData* bhkConstraintManager::LoadBinary(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    int32_t iType;
    NiStreamLoadBinary(arStream, iType);
    hkConstraintCinfo* pkInfo = GetInfo(iType);
    if (!pkInfo) {
        return nullptr;
    }
    pkInfo->LoadBinary(arStream);
    hkReferencedObject* pkData = pkInfo->pData;
    pkData->addReference();
    delete pkInfo;
    return static_cast<hkpConstraintData*>(pkData);
}

void bhkConstraintManager::SaveBinary(NiStream& arStream, hkpConstraintData* apData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    int32_t iType = apData->getType();
    hkConstraintCinfo* pkInfo = GetInfo(iType);
    pkInfo->SetData(apData);
    NiStreamSaveBinary(arStream, iType);
    pkInfo->SaveBinary(arStream);
    delete pkInfo;
}

// The constructor stores only hkConstraintCinfo's fields, as the base's constructor does.
hkPointToPathConstraintCinfo::hkPointToPathConstraintCinfo() {}

// The class adds no fields.
hkPointToPathConstraintCinfo::~hkPointToPathConstraintCinfo() {}

void hkPointToPathConstraintCinfo::CloneData(hkpConstraintData* a1, NiCloningProcess& a2) {
    hkVector4 v11;
    TLSData* v3 = GetThreadTLSData();
    NiCloningProcess* v4 = &a2;
    hkpPointToPathConstraintData* pnd = static_cast<hkpPointToPathConstraintData*>(a1);
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    hkpPointToPathConstraintData* pd = static_cast<hkpPointToPathConstraintData*>(pData);
    if (!a1) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpPointToPathConstraintData* v9 = static_cast<hkpPointToPathConstraintData*>(
            Instance->m_heap->blockAlloc(sizeof(hkpPointToPathConstraintData))
        );
        v9->m_memSizeAndFlags = sizeof(hkpPointToPathConstraintData);
        hkpPointToPathConstraintData* pnd_2 = ::new (v9) hkpPointToPathConstraintData;
        v4 = &a2;
        pnd = pnd_2;
    }
    pnd->m_maxFrictionForce = pd->m_maxFrictionForce;
    pnd->m_angularConstrainedDOF.m_storage = pd->m_angularConstrainedDOF.m_storage;
    pnd->m_transform_OS_KS[0].m_rotation.m_col0 = pd->m_transform_OS_KS[0].m_rotation.m_col0;
    pnd->m_transform_OS_KS[0].m_rotation.m_col1 = pd->m_transform_OS_KS[0].m_rotation.m_col1;
    pnd->m_transform_OS_KS[0].m_rotation.m_col2 = pd->m_transform_OS_KS[0].m_rotation.m_col2;
    pnd->m_transform_OS_KS[0].m_translation = pd->m_transform_OS_KS[0].m_translation;
    pnd->m_transform_OS_KS[1].m_rotation = pd->m_transform_OS_KS[1].m_rotation;
    v11.m_quad = pd->m_transform_OS_KS[1].m_translation.m_quad;
    pnd->m_transform_OS_KS[1].m_translation.m_quad = v11.m_quad;
    // The cloning process is a BSCloningProcess: its scale's x (+0x10; NiCloningProcess is 0x10 bytes).
    float v12 = static_cast<BSCloningProcess*>(v4)->Scale.x;
    if (1.0f != v12) {
        v11.m_quad[0] = v12;
        pnd->m_transform_OS_KS[0].m_translation.m_quad =
            _mm_mul_ps(_mm_shuffle_ps(v11.m_quad, v11.m_quad, 0), pnd->m_transform_OS_KS[0].m_translation.m_quad);
        pnd->m_transform_OS_KS[1].m_translation.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(v12), _mm_set_ss(v12), 0), pnd->m_transform_OS_KS[1].m_translation.m_quad
        );
    }
    hkConstraintCinfo::CloneData(pnd, *v4);
    v3->eMemContext = eMemContext;
}

void hkPointToPathConstraintCinfo::LoadBinary(NiStream& a1) {
    uint32_t v13;
    NiStream* v2 = &a1;
    hkConstraintCinfo::LoadBinary(a1);
    hkpPointToPathConstraintData* pd = static_cast<hkpPointToPathConstraintData*>(pData);
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v13 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &pd->m_maxFrictionForce, 4, &v13, 1);
    NiBinaryStream* v7 = v2->Istr();
    uint32_t (*v8)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v7->m_pfnRead;
    v13 = 1;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pd->m_angularConstrainedDOF, 1, &v13, 1);
    NiBinaryStream* v9 = v2->Istr();
    uint32_t (*v10)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v9->m_pfnRead;
    v13 = 64;
    v9->m_uiAbsoluteCurrentPos += v10(v9, pd->m_transform_OS_KS, 64, &v13, 1);
    NiBinaryStream* v11 = v2->Istr();
    uint32_t (*v12)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v11->m_pfnRead;
    v13 = 64;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &pd->m_transform_OS_KS[1], 64, &v13, 1);
}

void hkPointToPathConstraintCinfo::SaveBinary(NiStream& a1) {
    uint32_t v12;
    hkpPointToPathConstraintData* pd = static_cast<hkpPointToPathConstraintData*>(pData);
    NiStream* v3 = &a1;
    hkConstraintCinfo::SaveBinary(a1);
    NiBinaryStream* m_pkOstr = v3->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v12 = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pd->m_maxFrictionForce, 4, &v12, 1);
    NiBinaryStream* v6 = v3->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v7 = v6->m_pfnWrite;
    v12 = 1;
    v6->m_uiAbsoluteCurrentPos += v7(v6, &pd->m_angularConstrainedDOF, 1, &v12, 1);
    NiBinaryStream* v8 = v3->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v9 = v8->m_pfnWrite;
    v12 = 64;
    v8->m_uiAbsoluteCurrentPos += v9(v8, pd->m_transform_OS_KS, 64, &v12, 1);
    NiBinaryStream* v10 = v3->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v11 = v10->m_pfnWrite;
    v12 = 64;
    v10->m_uiAbsoluteCurrentPos += v11(v10, &pd->m_transform_OS_KS[1], 64, &v12, 1);
}

uint32_t hkPointToPathConstraintCinfo::CreateData() {
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    if (!pData) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpPointToPathConstraintData* v5 = static_cast<hkpPointToPathConstraintData*>(
            Instance->m_heap->blockAlloc(sizeof(hkpPointToPathConstraintData))
        );
        v5->m_memSizeAndFlags = sizeof(hkpPointToPathConstraintData);
        pData = ::new (v5) hkpPointToPathConstraintData;
    }
    v2->eMemContext = eMemContext;
    return sizeof(hkpPointToPathConstraintData);
}

void hkPointToPathConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkConstraintCinfo::GetViewerStrings(apStrings);
}
