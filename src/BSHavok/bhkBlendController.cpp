#include "BSHavok/bhkBlendController.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkForceController.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpScalar.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cstring>

template void NiTLargeArray<BLENDKEY, NiTMallocInterface<BLENDKEY>>::SetSize(uint32_t uiMaxSize);

NiRTTI bhkBlendController::ms_RTTI("bhkBlendController", &NiTimeController::ms_RTTI);

// Knocks the ragdolls below apObject down: a blend collision object gets a two-key blend from its gains to the
// part's knock-down time and, without a force controller, the pass-out force along arDirection; a plain collision
// object's body turns dynamic and gets the force at once. Then the children.
void bhkBlendController::DoKnockDown(
    NiAVObject* apObject,
    const NiPoint3& arDirection,
    int32_t abKill,
    float afTime,
    int32_t abNoForce
) {
    if (!apObject) {
        return;
    }
    bhkBlendCollisionObject* pc = nullptr;
    if (apObject->m_spCollisionObject.m_pObject) {
        pc = static_cast<bhkBlendCollisionObject*>(
            apObject->m_spCollisionObject.m_pObject->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI)
        );
    }
    if (pc) {
        pc->SynchronizeMotionType();
        bhkRigidBody* prb = pc->spWorldObject;
        uint32_t uiFilter = 0;
        if (prb) {
            hkpWorldObject* pkHkObject = static_cast<hkpWorldObject*>(prb->phkObject);
            const hkpCollidable* pkCollidable = pkHkObject ? pkHkObject->getCollidable() : nullptr;
            if (pkCollidable) {
                uiFilter = pkCollidable->getCollisionFilterInfo();
            }
        }
        float fBlend = pKnockDownBlendA[(uiFilter >> 8) & 0x1F];
        bhkBlendController* pcon =
            static_cast<bhkBlendController*>(apObject->GetController(&bhkBlendController::ms_RTTI));
        if (pcon && fBlend >= 0.0f) {
            if (static_cast<uint8_t>(abNoForce)) {
                pc->SetMotionType(hkpMotion::MOTION_DYNAMIC, nullptr, false);
            }
            else {
                float fHeirGain = pc->fHeirGain;
                float fVelGain = pc->fVelGain;
                pcon->ClearKeys();
                pcon->iPriority = 2;
                if (pcon->pBlendKeyA.m_uiESize == 0) {
                    // The two keys are built with BLENDKEY::BLENDKEY through the eh vector constructor iterator, in a
                    // 16-aligned slot: typed storage for which C++ runs no constructor.
                    using BLENDKEYArray2 = BLENDKEY[2];
                    alignas(16) FNVD_INLINED_LOCAL(BLENDKEYArray2, akKeys, std::nullopt, std::nullopt);
                    rt::eh_vector_constructor_iterator(akKeys, sizeof(BLENDKEY), 2, &fnvd::constructElement<BLENDKEY>);
                    akKeys[0].fGain = fHeirGain;
                    akKeys[0].fVelGain = fVelGain;
                    akKeys[0].fTime = 0.0f;
                    akKeys[1].fGain = 0.0f;
                    akKeys[1].fVelGain = 0.0f;
                    akKeys[1].fTime = fBlend;
                    pcon->pBlendKeyA.SetSize(2);
                    for (int32_t i = 0; i < 2; ++i) {
                        pcon->AddKey(akKeys[i]);
                    }
                    uint16_t usFlags = pcon->m_usFlags;
                    pcon->m_fLoKeyTime = afTime;
                    pcon->m_fHiKeyTime = fBlend;
                    pcon->m_fPhase = 0.0f;
                    pcon->m_usFlags = static_cast<uint16_t>((usFlags & 0xFEF5) | 0xC5);
                    pcon->m_fFrequency = 1.0f;
                    NiTimeController* pkTimeController = pcon;
                    pkTimeController->Start(-3.4028235e+38f);
                }
                if (!apObject->GetController(&bhkForceController::ms_RTTI)) {
                    NiPoint3 kForce = arDirection * fPassOutForce;
                    float fEndTime = fPassOutTime;
                    alignas(16) hkVector4 kHkForce;
                    bhkForceController::TestForce2(apObject, NI2HK(kHkForce, kForce), fEndTime);
                }
            }
        }
    }
    else if (apObject->m_spCollisionObject.m_pObject) {
        bhkCollisionObject* pkCollision = static_cast<bhkCollisionObject*>(
            apObject->m_spCollisionObject.m_pObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI)
        );
        if (pkCollision) {
            bhkRigidBody* prb = pkCollision->spWorldObject;
            if (prb) {
                CFilter kResult;
                uint32_t uiFilter = prb->GetCollisionFilter(&kResult)->iFilter;
                // EXE-INLINED 0x43B4D0
                uint32_t uiLayer = uiFilter & 0x7F;
                // EXE-INLINED 0x4D9DA0
                if ((uiLayer != L_BIPED && uiLayer != L_DEADBIP) || (uiFilter & 0x1F00) == (P_WEAPON << 8)) {
                    prb->SetMotionType(hkpMotion::MOTION_DYNAMIC);
                    if (!static_cast<uint8_t>(abNoForce)) {
                        NiPoint3 kForce = arDirection * fPassOutForce;
                        alignas(16) hkVector4 kHkForce;
                        static_cast<hkpRigidBody*>(prb->phkObject)
                            ->applyForce(bhkWorld::fDeltaTime, NI2HK(kHkForce, kForce));
                    }
                }
            }
        }
    }
    NiNode* pnode = apObject->IsNode();
    if (pnode) {
        for (uint32_t i = 0; i < pnode->m_kChildren.m_usSize; ++i) {
            DoKnockDown(pnode->m_kChildren.m_pBase[i].m_pObject, arDirection, abKill, afTime, abNoForce);
        }
    }
}

void bhkBlendController::ClearKeys() {
    pBlendKeyA.RemoveAll();
    fOriginalVelGain = -1.0f;
    iPriority = 0;
    fOriginalGain = -1.0f;
}

bool bhkBlendController::CalculateGain(float afTime, float& arGain, float& arVelGain) {
    uint32_t m_uiESize = pBlendKeyA.m_uiESize;
    if (!m_uiESize) {
        return 0;
    }
    if (m_uiESize == 1) {
        arGain = pBlendKeyA.m_pBase->fGain;
        arVelGain = pBlendKeyA.m_pBase->fVelGain;
        return 1;
    }
    else {
        double v12;
        BLENDKEY* m_pBase = pBlendKeyA.m_pBase;
        uint32_t v8 = m_uiESize - 1;
        float fTime = m_pBase[iLastIndex].fTime;
        double v9 = afTime;
        if (fTime > static_cast<double>(afTime)) {
            iLastIndex = 0;
            fTime = m_pBase->fTime;
        }
        uint32_t v10 = iLastIndex + 1;
        if (static_cast<int32_t>(v8 - iLastIndex) < 4) {
            v12 = fTime;
        LABEL_15:;
            if (v10 <= v8) {
                BLENDKEY* v15 = &pBlendKeyA.m_pBase[v10];
                do {
                    v12 = v15->fTime;
                    if (v12 >= v9) {
                        break;
                    }
                    ++iLastIndex;
                    fTime = v12;
                    ++v10;
                    ++v15;
                }
                while (v10 <= v8);
            }
        }
        else {
            float* p_fTime = &m_pBase[v10 + 2].fTime;
            while (1) {
                v12 = *(p_fTime - 6);
                if (v12 >= v9) {
                    break;
                }
                ++iLastIndex;
                fTime = v12;
                float afTimea = *(p_fTime - 3);
                v12 = afTimea;
                if (afTimea >= v9) {
                    ++v10;
                    break;
                }
                int32_t v13 = iLastIndex + 1;
                fTime = afTimea;
                iLastIndex = v13;
                float afTimeb = *p_fTime;
                v12 = afTimeb;
                if (afTimeb >= v9) {
                    v10 += 2;
                    break;
                }
                int32_t v14 = v13 + 1;
                fTime = afTimeb;
                iLastIndex = v14;
                float afTimec = p_fTime[3];
                v12 = afTimec;
                if (afTimec >= v9) {
                    v10 += 3;
                    break;
                }
                fTime = afTimec;
                v10 += 4;
                p_fTime += 12;
                iLastIndex = v14 + 1;
                if (v10 > m_uiESize - 4) {
                    goto LABEL_15;
                }
            }
        }
        BLENDKEY* v16 = pBlendKeyA.m_pBase;
        double v17 = v12 - fTime;
        double v18 = fTime;
        BLENDKEY* v19 = &v16[iLastIndex];
        float afTimed = v17;
        float fP = v19->fGain;
        float fVelGain = v19->fVelGain;
        if (afTimed == 0.0) {
            arGain = fP;
            arVelGain = fVelGain;
        }
        else {
            float afTimee = (v9 - v18) / afTimed;
            arGain = NiInterpScalar::Linear(afTimee, fP, v16[v10].fGain);
            arVelGain = NiInterpScalar::Linear(afTimee, fVelGain, pBlendKeyA.m_pBase[v10].fVelGain);
        }
        return 1;
    }
}

void bhkBlendController::RestoreOriginalGains(bhkBlendCollisionObject* apCol) {
    if (((m_usFlags & 256) != 0) && (fOriginalGain >= 0.0)) {
        apCol->fHeirGain = fOriginalGain;
        apCol->fVelGain = fOriginalVelGain;
        fOriginalVelGain = (-0x1p+0f);
        fOriginalGain = (-0x1p+0f);
    }
}

void bhkBlendController::AddKey(const BLENDKEY& arKey) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    uint32_t m_uiSize = pBlendKeyA.m_uiSize;
    if (m_uiSize >= pBlendKeyA.m_uiMaxSize) {
        pBlendKeyA.SetSize(m_uiSize + pBlendKeyA.m_uiGrowBy);
    }
    pBlendKeyA.SetAt(m_uiSize, arKey);
    iLastIndex = 0;
    FixStartAndEndTimes();
    v3->eMemContext = eMemContext;
}

// The key array frees its own buffer.
bhkBlendController::~bhkBlendController() {}

const NiRTTI* bhkBlendController::GetRTTI() const {
    return &bhkBlendController::ms_RTTI;
}

// Gives each blend collision object below apObject the get-up filter and a two-key blend from its default gains, and
// makes the bodies of plain collision objects keyframed, then walks the children.
void bhkBlendController::DoGetUpRecursive(NiAVObject* apObject, float afTime) {
    if (!apObject) {
        return;
    }
    bhkBlendCollisionObject* pkBlendObject = nullptr;
    if (apObject->m_spCollisionObject.m_pObject) {
        pkBlendObject = static_cast<bhkBlendCollisionObject*>(
            apObject->m_spCollisionObject.m_pObject->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI)
        );
    }
    if (pkBlendObject) {
        bhkRigidBody* pkBody = pkBlendObject->spWorldObject;
        uint32_t uiFilter = 0;
        if (pkBody) {
            hkpWorldObject* pkHkObject = static_cast<hkpWorldObject*>(pkBody->phkObject);
            uint32_t uiOldFilter = 0;
            if (pkHkObject) {
                uiOldFilter = pkHkObject->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo;
            }
            // The mask keeps bit 3 as well as the high bits.
            uiFilter = (uiOldFilter & 0xFFFFFF88) | 8;
            pkBody->SetCollisionFilter(CFilter{uiFilter});
        }
        uint32_t uiPart = (uiFilter >> 8) & 0x1F;
        float fGetUpTime = pGetUpBlendA[uiPart];
        bhkBlendController* pkController =
            static_cast<bhkBlendController*>(apObject->GetController(&bhkBlendController::ms_RTTI));
        if (pkController && fGetUpTime >= 0.0f) {
            float fGain = bhkBlendCollisionObject::pDefaultBlendA[uiPart].fGain;
            float fVelGain = bhkBlendCollisionObject::pDefaultBlendA[uiPart].fVelGain;
            pkController->ClearKeys();
            pkController->iPriority = 2;
            if (pkController->pBlendKeyA.m_uiESize == 0) {
                alignas(BLENDKEY) uint8_t aKeyStorage[2 * sizeof(BLENDKEY)];
                rt::eh_vector_constructor_iterator(aKeyStorage, sizeof(BLENDKEY), 2, &fnvd::constructElement<BLENDKEY>);
                // Raw memory the constructor has just built into two keys.
                BLENDKEY* akKeys = reinterpret_cast<BLENDKEY*>(aKeyStorage);
                akKeys[0].fGain = 0.0f;
                akKeys[0].fVelGain = 0.0f;
                akKeys[0].fTime = 0.0f;
                akKeys[1].fGain = fGain;
                akKeys[1].fVelGain = fVelGain;
                akKeys[1].fTime = fGetUpTime;
                pkController->pBlendKeyA.SetSize(2);
                for (int32_t i = 0; i < 2; ++i) {
                    pkController->AddKey(akKeys[i]);
                }
                uint16_t usFlags = pkController->m_usFlags;
                pkController->m_fLoKeyTime = afTime;
                pkController->m_fHiKeyTime = fGetUpTime;
                pkController->m_fPhase = 0.0f;
                pkController->m_fFrequency = 1.0f;
                pkController->m_usFlags = static_cast<uint16_t>((usFlags & 0xFEF5) | 0xC5);
                NiTimeController* pkTimeController = pkController;
                pkTimeController->Start(-3.4028235e+38f);
            }
        }
    }
    else if (apObject->m_spCollisionObject.m_pObject) {
        bhkCollisionObject* pkCollision = static_cast<bhkCollisionObject*>(
            apObject->m_spCollisionObject.m_pObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI)
        );
        if (pkCollision) {
            bhkRigidBody* pkBody = pkCollision->spWorldObject;
            if (pkBody) {
                CFilter kResult;
                uint32_t uiFilter = pkBody->GetCollisionFilter(&kResult)->iFilter;
                // EXE-INLINED 0x43B4D0
                uint32_t uiLayer = uiFilter & 0x7F;
                // EXE-INLINED 0x4D9DA0
                if ((uiLayer != L_BIPED && uiLayer != L_DEADBIP) || (uiFilter & 0x1F00) == (P_WEAPON << 8)) {
                    pkBody->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
                }
            }
        }
    }
    NiNode* pkNode = apObject->IsNode();
    if (pkNode) {
        for (uint32_t i = 0; i < pkNode->m_kChildren.m_usSize; ++i) {
            DoGetUpRecursive(pkNode->m_kChildren.m_pBase[i].m_pObject, afTime);
        }
    }
}

void bhkBlendController::CopyMembers(bhkBlendController* pDest, NiCloningProcess& arCloneData) {
    TLSData* v4 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v4->eMemContext;
    v4->eMemContext = MEM_CONTEXT_HAVOK;
    uint32_t v5 = 0;
    NiTimeController::CopyMembers(pDest, arCloneData);
    pDest->iLastIndex = iLastIndex;
    pDest->pBlendKeyA.SetSize(pBlendKeyA.m_uiESize);
    if (pBlendKeyA.m_uiESize) {
        int32_t v6 = 0;
        do {
            pDest->AddKey(pBlendKeyA.m_pBase[v6]);
            ++v5;
            ++v6;
        }
        while (v5 < pBlendKeyA.m_uiESize);
        v4->eMemContext = eMemContext;
    }
    else {
        v4->eMemContext = eMemContext;
    }
}

int32_t BLENDKEY::IsEqual(const BLENDKEY* a2) const {
    return (int32_t)((uint8_t)((a2->fTime == fTime) && ((a2->fGain == fGain) && (a2->fVelGain == fVelGain))));
}

int32_t BLENDKEY::IsDifferent(const BLENDKEY* a2) const {
    return (int32_t)((uint8_t)((a2->fTime != fTime) || ((a2->fGain != fGain) || (a2->fVelGain != fVelGain))));
}

void BLENDKEY::DefaultConstructorClosure(BLENDKEY* apThis) {
    apThis->fTime = -1.0f;
    float v1 = 0.0 / 100.0;
    apThis->fGain = v1;
    apThis->fVelGain = v1;
}

// Heap-context guard: the work below is charged to the Havok context and the thread's previous context goes back at the
// end.
bool bhkBlendController::ClearForceController() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    if (((m_usFlags >> CLEARFORCE_POS) & 1) != 0) {
        NiObjectNET* pkTarget = m_pkTarget;
        NiTimeController* pkController = pkTarget->GetController(&bhkForceController::ms_RTTI);
        if (pkController) {
            pkTarget->RemoveController(pkController);
            return true;
        }
    }
    return false;
}

// No keys yet (the array grows by one); the original gains are unset (-1).
bhkBlendController::bhkBlendController()
  : iLastIndex(0), pBlendKeyA(0, 1), fOriginalGain(-1.0f), fOriginalVelGain(-1.0f), iPriority(0) {}

// A 4-byte key count, then twelve bytes per key, swapped here when the file is big-endian and written as one
// component so the stream leaves them alone.
void bhkBlendController::SaveBinary(NiStream& arStream) {
    NiTimeController::SaveBinary(arStream);

    uint32_t uiCount = pBlendKeyA.m_uiESize;
    NiBinaryStream* pkOstr = arStream.m_pkOstr;
    uint32_t uiFour = 4;
    pkOstr->BinaryWrite(&uiCount, 4, &uiFour, 1);

    for (uint32_t i = 0; i < uiCount; ++i) {
        uint32_t auiKey[3];
        memcpy(auiKey, &pBlendKeyA.m_pBase[i], sizeof(auiKey));
        if (!arStream.m_bSaveLittleEndian) {
            auiKey[0] = __builtin_bswap32(auiKey[0]);
            auiKey[1] = __builtin_bswap32(auiKey[1]);
            auiKey[2] = __builtin_bswap32(auiKey[2]);
        }
        uint32_t uiTwelve = 12;
        arStream.m_pkOstr->BinaryWrite(auiKey, 12, &uiTwelve, 1);
    }
}

// The stream's factory: allocates the object and runs the constructor on it.
NiObject* bhkBlendController::CreateObject() {
    bhkBlendController* pkObject =
        static_cast<bhkBlendController*>(NiMemObject::operator new(sizeof(bhkBlendController)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) bhkBlendController;
}

// Drives the target's blend collision object from the key list, and at the list's last key drops the keys and the
// force controller, hands the object its gains back and stops. The restore-gains bit gates the whole body.
void bhkBlendController::Update(NiUpdateData& UpdateData) {
    NiAVObject* pkTarget = static_cast<NiAVObject*>(m_pkTarget);
    if (!pkTarget) {
        return;
    }
    if (((m_usFlags >> RESTOREGAINS_POS) & 1) == 0) {
        return;
    }
    if (pBlendKeyA.m_uiESize == 0) {
        return;
    }

    // Loads the collision object once, tests it, and only then casts it.
    bhkNiCollisionObject* pkCollisionObject = pkTarget->m_spCollisionObject.m_pObject;
    bhkBlendCollisionObject* pkBlendObject = nullptr;
    if (pkCollisionObject) {
        pkBlendObject = static_cast<bhkBlendCollisionObject*>(
            pkCollisionObject->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI)
        );
    }
    if (!pkBlendObject) {
        m_fLastTime = UpdateData.fTime;
        return;
    }

    float fScaledTime = ComputeScaledTime(UpdateData.fTime);
    float fHeirGain;
    float fVelGain;
    if (CalculateGain(fScaledTime, fHeirGain, fVelGain)) {
        if (fOriginalGain < 0.0f) {
            fOriginalGain = pkBlendObject->fHeirGain;
            fOriginalVelGain = pkBlendObject->fVelGain;
        }
        pkBlendObject->fHeirGain = fHeirGain;
        pkBlendObject->fVelGain = fVelGain;
    }

    if (m_fHiKeyTime != fScaledTime) {
        return;
    }
    uint16_t usFlags = m_usFlags;
    // Compares the masked cycle-type bits with 4: CLAMP's bit pair in the word.
    if ((usFlags & CYCLETYPE_MASK) != (CLAMP << CYCLETYPE_POS)) {
        return;
    }
    if (((usFlags >> CLEARKEYS_POS) & 1) != 0) {
        ClearKeys();
    }
    ClearForceController();
    RestoreOriginalGains(pkBlendObject);
    Stop();
}

// A 4-byte key count, then that many twelve-byte keys, each read as one component and swapped here when the file's
// byte order differs.
void bhkBlendController::LoadBinary(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    uint32_t uiCount = 0;
    NiTimeController::LoadBinary(arStream);

    uint32_t uiFour = 4;
    arStream.m_pkIstr->BinaryRead(&uiCount, 4, &uiFour, 1);

    pBlendKeyA.SetSize(uiCount);

    for (uint32_t i = 0; i < uiCount; ++i) {
        BLENDKEY kKey;
        arStream.m_pkIstr->BinaryRead(&kKey, 12, nullptr, 0);
        if (!arStream.m_bSourceIsLittleEndian) {
            uint32_t auiKey[3];
            memcpy(auiKey, &kKey, sizeof(auiKey));
            auiKey[0] = __builtin_bswap32(auiKey[0]);
            auiKey[1] = __builtin_bswap32(auiKey[1]);
            auiKey[2] = __builtin_bswap32(auiKey[2]);
            memcpy(&kKey, auiKey, sizeof(kKey));
        }
        AddKey(kKey);
    }
}

// Raises the ragdoll below apObject: only the node's first child's first child matters, and only when it carries a
// bhkCollisionObject, whose body becomes keyframed and still. The recursive walk follows on every path.
void bhkBlendController::DoGetUp(NiAVObject* apObject, float afTime) {
    if (apObject && apObject->IsKindOf(&NiNode::ms_RTTI) && static_cast<NiNode*>(apObject)->m_kChildren.m_usSize != 0) {
        NiAVObject* pkChild = static_cast<NiNode*>(apObject)->m_kChildren.m_pBase[0].m_pObject;
        if (pkChild && pkChild->IsKindOf(&NiNode::ms_RTTI)) {
            NiNode* pkChildNode = static_cast<NiNode*>(pkChild);
            if (pkChildNode->m_kChildren.m_usSize != 0) {
                NiAVObject* pkGrandChild = pkChildNode->m_kChildren.m_pBase[0].m_pObject;
                if (pkGrandChild) {
                    bhkNiCollisionObject* pkCollisionObject = pkGrandChild->m_spCollisionObject.m_pObject;
                    if (pkCollisionObject) {
                        bhkCollisionObject* pkCollision = static_cast<bhkCollisionObject*>(
                            pkCollisionObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI)
                        );
                        if (pkCollision) {
                            bhkRigidBody* pkBody = pkCollision->spWorldObject;
                            if (pkBody) {
                                // hkpMotion::MOTION_KEYFRAMED: the scene graph drives the body again.
                                pkBody->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
                                pkBody->SetLinearVelocity(hkVector4Zero);
                                pkBody->SetAngularVelocity(hkVector4Zero);
                            }
                        }
                    }
                }
            }
        }
    }
    DoGetUpRecursive(apObject, afTime);
}

// Allocates and constructs the clone (null when the allocation failed) and copies the members into it.
NiObject* bhkBlendController::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bhkBlendController* pkClone = nullptr;
    bhkBlendController* pkNew = static_cast<bhkBlendController*>(NiMemObject::operator new(sizeof(bhkBlendController)));
    if (pkNew) {
        pkClone = ::new (pkNew) bhkBlendController;
    }
    CopyMembers(pkClone, arCloneData);
    return pkClone;
}

// Equal when the controller parts are and the keys match one by one; the key count is reloaded on every pass.
bool bhkBlendController::IsEqual(NiObject* apObject) {
    if (!NiTimeController::IsEqual(apObject)) {
        return false;
    }
    bhkBlendController* pkOther = static_cast<bhkBlendController*>(apObject);
    bool bResult = pBlendKeyA.m_uiESize == pkOther->pBlendKeyA.m_uiESize;
    for (uint32_t i = 0; bResult && i < pBlendKeyA.m_uiESize; ++i) {
        const BLENDKEY& rKey = pBlendKeyA.m_pBase[i];
        const BLENDKEY& rOther = pkOther->pBlendKeyA.m_pBase[i];
        bResult = rOther.fTime == rKey.fTime && rOther.fGain == rKey.fGain && rOther.fVelGain == rKey.fVelGain;
    }
    return bResult;
}

void bhkBlendController::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    NiTimeController::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    // EXE-INLINED none: NiTArray<char*>::Add
    uint32_t uiIndex = apStrings->m_usSize;
    if (uiIndex >= apStrings->m_usMaxSize) {
        apStrings->SetSize(uiIndex + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(uiIndex, pcString);
    pcString = NiGetViewerString("iKeys", pBlendKeyA.m_uiESize);
    apStrings->Add(pcString);
    for (uint32_t i = 0; i < pBlendKeyA.m_uiESize; ++i) {
        char acBuffer[64];
        const BLENDKEY& rKey = pBlendKeyA.m_pBase[i];
        BSsprintf(acBuffer, 64, "Key%d: %.4f, %.4f, %.4f", i, rKey.fTime, rKey.fGain, rKey.fVelGain);
        // EXE-INLINED 0xEC6130
        size_t stLength = 0;
        while (acBuffer[stLength]) {
            ++stLength;
        }
        char* pcKey = static_cast<char*>(MemoryManager_Singleton.Allocate(stLength + 1));
        strcpy_s(pcKey, stLength + 1, acBuffer);
        // EXE-INLINED none: NiTArray<char*>::Add
        uiIndex = apStrings->m_usSize;
        if (uiIndex >= apStrings->m_usMaxSize) {
            apStrings->SetSize(uiIndex + apStrings->m_usGrowBy);
        }
        // EXE-INLINED 0x96AE90
        if (uiIndex >= apStrings->m_usSize) {
            apStrings->m_usSize = static_cast<uint16_t>(uiIndex + 1);
            if (pcKey) {
                ++apStrings->m_usESize;
            }
        }
        else if (pcKey) {
            if (!apStrings->m_pBase[uiIndex]) {
                ++apStrings->m_usESize;
            }
        }
        else if (apStrings->m_pBase[uiIndex]) {
            --apStrings->m_usESize;
        }
        apStrings->m_pBase[uiIndex] = pcKey;
    }
    pTLSData->eMemContext = eMemContext;
}

void bhkBlendController::FixStartAndEndTimes() {
    BLENDKEY* m_pBase;
    double v3;
    double v4;
    float fTime;
    float v9;
    uint32_t m_uiESize = pBlendKeyA.m_uiESize;
    float m_fLoKeyTime = 0.0;
    float m_fHiKeyTime = 0.0;
    if (!m_uiESize) {
        goto LABEL_6;
    }
    m_pBase = pBlendKeyA.m_pBase;
    fTime = m_pBase->fTime;
    m_fLoKeyTime = this->m_fLoKeyTime;
    m_fHiKeyTime = this->m_fHiKeyTime;
    if (fTime < static_cast<double>(m_fLoKeyTime)) {
        v3 = fTime;
    }
    else {
        v3 = fTime;
        if (m_fLoKeyTime != 3.4028234663852886e+38) {
            goto LABEL_4;
        }
    }
    m_fLoKeyTime = v3;
LABEL_4:;
    v9 = m_pBase[m_uiESize - 1].fTime;
    if (v9 < static_cast<double>(m_fHiKeyTime)) {
        v4 = v9;
    }
    else {
        v4 = v9;
        if (m_fHiKeyTime != -3.4028234663852886e+38) {
        LABEL_6:;
            this->m_fLoKeyTime = m_fLoKeyTime;
            this->m_fHiKeyTime = m_fHiKeyTime;
            return;
        }
    }
    float v7 = v4;
    this->m_fLoKeyTime = m_fLoKeyTime;
    this->m_fHiKeyTime = v7;
}

// Starts the hit blend of each blend collision object's ragdoll below apObject: a three-key blend from the hit gains of
// the body's biped part (the quadruped table when abQuad), capped by the object's own gains, then walks the children.
void bhkBlendController::DoHit(NiAVObject* apObject, float afTime, int32_t abQuad) {
    if (apObject) {
        bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
        if (m_pObject) {
            bhkBlendCollisionObject* v5 =
                static_cast<bhkBlendCollisionObject*>(m_pObject->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI));
            if (v5) {
                double fVelGain;
                float fminGain;
                bhkRigidBody* spWorldObject = v5->spWorldObject;
                uint32_t v8 = 0;
                if (spWorldObject) {
                    hkpWorldObject* pkHkObject = static_cast<hkpWorldObject*>(spWorldObject->phkObject);
                    const hkpCollidable* pkCollidable = pkHkObject ? pkHkObject->getCollidable() : nullptr;
                    if (!pkCollidable) {
                        v8 = 0;
                    }
                    else {
                        v8 = pkCollidable->getCollisionFilterInfo();
                    }
                }
                BIPED_PART epart = static_cast<BIPED_PART>(v8 >> 8 & 31);
                if (static_cast<uint8_t>(abQuad)) {
                    fminGain = bhkBlendController::pHitBlendQuadA[epart].fGain;
                    fVelGain = bhkBlendController::pHitBlendQuadA[epart].fVelGain;
                }
                else {
                    fminGain = bhkBlendController::pHitBlendA[epart].fGain;
                    fVelGain = bhkBlendController::pHitBlendA[epart].fVelGain;
                }
                float v25 = fVelGain;
                double fHeirGain = v5->fHeirGain;
                if (!(fminGain > fHeirGain)) {
                    fHeirGain = fminGain;
                }
                float v26 = fHeirGain;
                double v13 = v5->fVelGain;
                if (!(v25 > v13)) {
                    v13 = v25;
                }
                bhkBlendController* pcon =
                    static_cast<bhkBlendController*>(apObject->GetController(&bhkBlendController::ms_RTTI));
                if (pcon) {
                    if (pcon->iPriority <= 1u) {
                        pcon->ClearKeys();
                        bool v16 = pcon->pBlendKeyA.m_uiESize == 0;
                        pcon->iPriority = 1;
                        if (v16) {
                            // The three keys are built with BLENDKEY::BLENDKEY through the eh vector constructor
                            // iterator, because keys is typed storage for which C++ runs no constructor.
                            using BLENDKEYArray3 = BLENDKEY[3];
                            FNVD_INLINED_LOCAL(BLENDKEYArray3, keys, std::nullopt, std::nullopt);
                            rt::eh_vector_constructor_iterator(
                                keys, sizeof(BLENDKEY), 3, &fnvd::constructElement<BLENDKEY>
                            );
                            keys[0].fGain = v26;
                            float v27 = v13;
                            keys[0].fVelGain = v27;
                            keys[0].fTime = 0.0f;
                            keys[1].fGain = fminGain;
                            keys[1].fVelGain = v25;
                            keys[1].fTime = 0.15f;
                            keys[2].fGain = 1.0f;
                            keys[2].fVelGain = 1.0f;
                            keys[2].fTime = 0.5f;
                            pcon->pBlendKeyA.SetSize(3u);
                            BLENDKEY* v17 = keys;
                            int32_t v18 = 3;
                            do {
                                pcon->AddKey(*v17++);
                                --v18;
                            }
                            while (v18);
                            uint16_t m_usFlags = pcon->m_usFlags;
                            pcon->m_fLoKeyTime = 0.0f;
                            pcon->m_fHiKeyTime = 1.0f;
                            pcon->m_fFrequency = 1.0f;
                            pcon->m_usFlags = static_cast<uint16_t>((m_usFlags & 0xFFF5) | 0x1C5);
                            pcon->m_fPhase = 0.0f;
                            NiTimeController* pkTimeController = pcon;
                            pkTimeController->Start(-3.4028235e+38f);
                        }
                    }
                }
            }
        }
        NiNode* pnode = apObject->IsNode();
        if (pnode) {
            uint32_t v24 = 0;
            if (pnode->m_kChildren.m_usSize) {
                do {
                    bhkBlendController::DoHit(pnode->m_kChildren.m_pBase[v24++].m_pObject, afTime, abQuad);
                }
                while (v24 < pnode->m_kChildren.m_usSize);
            }
        }
    }
}

// The target must be a scene graph object.
bool bhkBlendController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}
