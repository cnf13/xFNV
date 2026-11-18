#include "BSHavok/bhkPoseArray.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "SDK/MSVC/ehvec.h"

NiRTTI bhkPoseArray::ms_RTTI("bhkPoseArray", &NiObject::ms_RTTI);

// One bone transform, one 4-byte write per component: the translation's x, y and z, the rotation's four, the scale's x,
// y and z.
void bhkPoseArray::bhkPose::SaveBoneTransf(NiStream* apStream, hkQsTransform* apBoneTransf) {
    uint32_t uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_translation(0), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_translation(1), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_translation(2), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_rotation.m_vec(0), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_rotation.m_vec(1), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_rotation.m_vec(2), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_rotation.m_vec(3), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_scale(0), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_scale(1), 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    apStream->Ostr()->BinaryWrite(&apBoneTransf->m_scale(2), 4, &uiComponentSize, 1);
}

bhkPoseArray::bhkPoseArray() : uiNumBones(0), BoneNameA(nullptr), uiNumPose(0), PoseA(nullptr) {}

const NiRTTI* bhkPoseArray::GetRTTI() const {
    return &bhkPoseArray::ms_RTTI;
}

// The base object, then the bone count and names, the pose count, and per pose its bone count and one read per bone
// transform into the pose's transform array as it stands. The name array is built as new[] builds it: the count in the
// word before the first name (the allocation's size saturated on overflow), then the NiFixedString constructor on each
// name. The scope is charged to the Havok context.
void bhkPoseArray::LoadBinary(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    NiObject::LoadBinary(arStream);
    uint32_t uiComponentSize = 4;
    arStream.Istr()->BinaryRead(&uiNumBones, 4, &uiComponentSize, 1);
    uint32_t uiNameCount = uiNumBones;
    if (uiNameCount != 0) {
        uint32_t uiNameBytes = static_cast<uint64_t>(uiNameCount) * sizeof(NiFixedString) > UINT32_MAX
            ? UINT32_MAX
            : static_cast<uint32_t>(uiNameCount * sizeof(NiFixedString));
        uint32_t uiBytes = uiNameBytes + static_cast<uint32_t>(sizeof(size_t));
        if (uiBytes < uiNameBytes) {
            uiBytes = UINT32_MAX;
        }
        size_t* puiHeader = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiBytes));
        NiFixedString* pBoneNames = nullptr;
        if (puiHeader != nullptr) {
            *puiHeader = uiNameCount;
            // The allocation is raw memory laid out as new[] lays it out: the count, then the names, which the iterator
            // constructs in place; Destruct reads the same layout back.
            pBoneNames = reinterpret_cast<NiFixedString*>(puiHeader + 1);
            rt::eh_vector_constructor_iterator(
                pBoneNames, sizeof(NiFixedString), static_cast<int32_t>(uiNameCount),
                &xfnv::constructElement<NiFixedString>, &xfnv::destructElement<NiFixedString>
            );
        }
        BoneNameA = pBoneNames;
    }
    for (uint32_t ui = 0; ui < uiNumBones; ++ui) {
        arStream.LoadFixedString(BoneNameA[ui]);
    }
    uiComponentSize = 4;
    arStream.Istr()->BinaryRead(&uiNumPose, 4, &uiComponentSize, 1);
    uint32_t uiPoseCount = uiNumPose;
    if (uiPoseCount != 0) {
        PoseA = static_cast<bhkPose**>(MemoryManager_Singleton.Allocate(
            static_cast<uint64_t>(uiPoseCount) * sizeof(bhkPose*) > UINT32_MAX
                ? UINT32_MAX
                : static_cast<uint32_t>(uiPoseCount * sizeof(bhkPose*))
        ));
    }
    for (uint32_t uiPose = 0; uiPose < uiNumPose; ++uiPose) {
        bhkPose* pPose = static_cast<bhkPose*>(MemoryManager_Singleton.Allocate(sizeof(bhkPose)));
        if (pPose != nullptr) {
            pPose->uiNumBones = 0;
            pPose->BoneTransfA = nullptr;
        }
        PoseA[uiPose] = pPose;
        pPose = PoseA[uiPose];
        uiComponentSize = 4;
        arStream.Istr()->BinaryRead(&pPose->uiNumBones, 4, &uiComponentSize, 1);
        for (uint32_t uiBone = 0; uiBone < pPose->uiNumBones; ++uiBone) {
            bhkStream::LoadBinary(arStream, pPose->BoneTransfA[uiBone]);
        }
    }
}

// The base object first, then every bone name in the stream's string table.
bool bhkPoseArray::RegisterStreamables(NiStream& arStream) {
    bool bRegistered = NiObject::RegisterStreamables(arStream);
    if (!bRegistered) {
        return false;
    }
    for (uint32_t i = 0; i < uiNumBones; ++i) {
        arStream.RegisterFixedString(BoneNameA[i]);
    }
    return bRegistered;
}

// Writes the base's empty save body, then the bone count and names, the pose count, and per pose its bone count and one
// write per bone transform.
void bhkPoseArray::SaveBinary(NiStream& arStream) {
    NiObject::SaveBinary(arStream);
    NiStreamSaveBinary(arStream, uiNumBones);
    for (uint32_t i = 0; i < uiNumBones; ++i) {
        arStream.SaveFixedString(BoneNameA[i]);
    }
    NiStreamSaveBinary(arStream, uiNumPose);
    for (uint32_t iPose = 0; iPose < uiNumPose; ++iPose) {
        bhkPose* pPose = PoseA[iPose];
        NiStreamSaveBinary(arStream, pPose->uiNumBones);
        for (uint32_t iBone = 0; iBone < pPose->uiNumBones; ++iBone) {
            bhkPose::SaveBoneTransf(&arStream, &pPose->BoneTransfA[iBone]);
        }
    }
}

// Frees the bone names (an array LoadBinary builds as new[] does) and the poses, charged to the Havok memory
// context.
bhkPoseArray::~bhkPoseArray() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    delete[] BoneNameA;
    if (PoseA != nullptr) {
        for (uint32_t iPose = 0; iPose < uiNumPose; ++iPose) {
            if (PoseA[iPose] != nullptr) {
                MemoryManager_Singleton.Deallocate(PoseA[iPose]);
            }
        }
        MemoryManager_Singleton.Deallocate(PoseA);
    }
}

// Every bone of apSkel must have its name in this array.
bool bhkPoseArray::BoneMatch(const hkaSkeleton* apSkel) {
    bool bResult = true;
    int32_t iBoneCount = apSkel->m_bones.getSize();
    for (int32_t iBone = 0; iBone < iBoneCount; ++iBone) {
        if (!bResult) {
            break;
        }
        const char* pcBoneName = apSkel->m_bones.m_data[iBone].m_name.cString();
        uint32_t uiNameCount = uiNumBones;
        bool bFound = false;
        for (uint32_t uiName = 0; uiName < uiNameCount; ++uiName) {
            if (bFound) {
                break;
            }
            const char* pcName = pcBoneName;
            const char* pcOther = BoneNameA[uiName].GetHandle();
            for (;;) {
                if (*pcName != *pcOther) {
                    break;
                }
                if (*pcName == '\0') {
                    bFound = true;
                    break;
                }
                ++pcName;
                ++pcOther;
            }
        }
        if (!bFound) {
            bResult = false;
        }
    }
    return bResult;
}

// Each bone of the skeleton takes the pose transform of the bone with the same name; a bone without a name in this
// array leaves its slot untouched.
void bhkPoseArray::CreatePoseForRagdoll(
    hkQsTransform* apPoseTransfA,
    const hkaSkeleton* apSkeleton,
    uint32_t uiPoseIdx
) {
    int32_t iBoneCount = apSkeleton->m_bones.getSize();
    for (int32_t iBone = 0; iBone < iBoneCount; ++iBone) {
        const char* pcBoneName = apSkeleton->m_bones.m_data[iBone].m_name.cString();
        uint32_t uiNameCount = uiNumBones;
        uint32_t uiName = 0;
        bool bFound = false;
        if (uiNameCount != 0) {
            while (!bFound) {
                const char* pcName = pcBoneName;
                const char* pcOther = BoneNameA[uiName].GetHandle();
                for (;;) {
                    if (*pcName != *pcOther) {
                        break;
                    }
                    if (*pcName == '\0') {
                        bFound = true;
                        break;
                    }
                    ++pcName;
                    ++pcOther;
                }
                if (++uiName >= uiNameCount) {
                    break;
                }
            }
            if (bFound) {
                // EXE-INLINED 0xC6CD30
                const hkQsTransform& rPose = PoseA[uiPoseIdx]->BoneTransfA[uiName - 1];
                hkQsTransform& rDest = apPoseTransfA[iBone];
                rDest.m_translation.m_quad = rPose.m_translation.m_quad;
                rDest.m_rotation.m_vec.m_quad = rPose.m_rotation.m_vec.m_quad;
                rDest.m_scale.m_quad = rPose.m_scale.m_quad;
            }
        }
    }
}

NiObject* bhkPoseArray::CreateObject() {
    bhkPoseArray* pObject = static_cast<bhkPoseArray*>(NiMemObject::operator new(sizeof(bhkPoseArray)));
    if (pObject == nullptr) {
        return nullptr;
    }
    return ::new (pObject) bhkPoseArray;
}
