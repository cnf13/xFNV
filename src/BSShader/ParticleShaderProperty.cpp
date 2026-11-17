#include "BSShader/ParticleShaderProperty.h"

#include "BSAudio/BSAudioManager.h"
#include "BSCore/BSRandom.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

NiRTTI ParticleShaderProperty::ms_RTTI("ParticleShaderProperty", &BSShaderProperty::ms_RTTI);

ParticleShaderProperty::ParticleShaderProperty()
  : eEmitterType(EMIT_GEOMETRY), bWorldspace(true), iParticleCount(0), fGenerateRemainder(0.0f),
    fMaxParticleRatio(1.0f), fLifetime(0.0f), fLifeVar(0.0f), fNormalSpeed(0.0f), fNormalSpeedVar(0.0f),
    fNormalAcceleration(0.0f), Velocity(0.0f, 0.0f, 0.0f), Acceleration(0.0f, 0.0f, 0.0f), fRotate(0.0f),
    fRotateVar(0.0f), fRotateSpeed(0.0f), fRotateSpeedVar(0.0f), Color1(0.0f, 0.0f, 0.0f, 0.0f),
    Color2(0.0f, 0.0f, 0.0f, 0.0f), Color3(0.0f, 0.0f, 0.0f, 0.0f), fTimer(0.0f), spBaseTexture(nullptr),
    TargetArray(10, 10), pMyGeometry(nullptr), fFade(1.0f), fTextureCountX(1.0f), fTextureCountY(1.0f),
    WindPoint(0.0f, 0.0f, 0.0f), fWindSpeed(0.0f) {
    TargetArray.RemoveAll();
    ParticleShaderProperty::GetInstanceIndex();
    uint32_t uiInstances = ParticleShader::GetMaxInstanceSize();
    pInstanceData = static_cast<ParticleShader::ParticleData*>(
        MemoryManager_Singleton.Allocate(sizeof(ParticleShader::ParticleData) * uiInstances)
    );
    std::memset(pInstanceData, 0, sizeof(ParticleShader::ParticleData) * uiInstances);
    ++ParticleShaderProperty::iRefCount;
    iShader = 24;
    iShaderPropertyType = 17;
}

ParticleShaderProperty::~ParticleShaderProperty() {
    spBaseTexture = nullptr;
    TargetArray.RemoveAll();
    MemoryManager_Singleton.Deallocate(pInstanceData);
    if (!--ParticleShaderProperty::iRefCount) {
        MemoryManager_Singleton.Deallocate(pfWindData);
        pfWindData = nullptr;
    }
}

// The clone is built in an engine block through the constructor, then the copy body fills it in.
NiObject* ParticleShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    ParticleShaderProperty* pClone =
        static_cast<ParticleShaderProperty*>(NiMemObject::operator new(sizeof(ParticleShaderProperty)));
    if (pClone) {
        pClone = ::new (pClone) ParticleShaderProperty;
    }
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void ParticleShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderProperty::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(ParticleShaderProperty::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v6 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v6);
}

bool ParticleShaderProperty::SetupGeometry(NiGeometry* apGeometry) {
    pMyGeometry = apGeometry;
    return 1;
}

BSShaderProperty::RenderPassArray* ParticleShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    BSShaderProperty::RenderPassArray* pRenderPassArray;
    BSShaderProperty::CheckCreateRenderPassArray(0xFFFFFFFFu);
    pRenderPassArray = this->pRenderPassList;
    if (!pRenderPassArray->iSize) {
        pRenderPassArray->Add(apGeom, 0x41u, 1, 0, nullptr, nullptr, nullptr, nullptr);
        this->pRenderPassList->pPassA.m_pBase[this->pRenderPassList->iSize - 1]->eAccumulationHint =
            BSShaderProperty::BSSP_HINT_GROUP_FADING;
        this->pRenderPassList->Add(apGeom, 0x241u, 1, 0, nullptr, nullptr, nullptr, nullptr);
        this->pRenderPassList->pPassA.m_pBase[this->pRenderPassList->iSize - 1]->eAccumulationHint =
            BSShaderProperty::BSSP_HINT_SORT;
    }
    return this->pRenderPassList;
}

void ParticleShaderProperty::InitParticleData(float afTime) {
    uint32_t MaxInstanceSize = ParticleShader::GetMaxInstanceSize();
    int32_t v4 = 0;
    float v10 = fLifeVar / fLifetime;
    float v12 = fNormalSpeedVar / fNormalSpeed;
    fTimer = afTime;
    if (MaxInstanceSize) {
        bool v8;
        uint32_t a2 = MaxInstanceSize;
        float v17 = 1.0 - v10;
        float v11 = v10 + 1.0;
        float v30 = 1.0 - v12;
        float v29 = v12 + 1.0;
        do {
            pInstanceData[v4].fXPos = 3.4028235e+38f;
            pInstanceData[v4].fYPos = 3.4028235e+38f;
            pInstanceData[v4].fZPos = 3.4028235e+38f;
            pInstanceData[v4].fBirthTime = -10000.0f;
            pInstanceData[v4].fXVel = 0.0f;
            pInstanceData[v4].fYVel = 0.0f;
            pInstanceData[v4].fZVel = 0.0f;
            if ((Singleton_guard & 1) == 0) {
                Singleton_guard |= 1u;
                BSRandom::Singleton.~BSRandom();
                ::new (&BSRandom::Singleton) BSRandom;
            }
            float v13 = v17 +
                (v11 - v17) * static_cast<double>(BSRandom::Singleton.UnsignedInt(0xFFFFFFFFu)) *
                    2.3283064365386963e-10;
            pInstanceData[v4].fLifeAdjust = v13;
            float v18 = fRotate - fRotateVar;
            float v14 = fRotateVar + fRotate;
            if ((Singleton_guard & 1) == 0) {
                Singleton_guard |= 1u;
                BSRandom::Singleton.~BSRandom();
                ::new (&BSRandom::Singleton) BSRandom;
            }
            float v19 = v18 +
                (v14 - v18) * static_cast<double>(BSRandom::Singleton.UnsignedInt(0xFFFFFFFFu)) *
                    2.3283064365386963e-10;
            float v20 = v19 * 0.01745329238474369;
            pInstanceData[v4].fRotationStart = v20;
            float v15 = fRotateSpeed - fRotateSpeedVar;
            float v21 = fRotateSpeedVar + fRotateSpeed;
            if ((Singleton_guard & 1) == 0) {
                Singleton_guard |= 1u;
                BSRandom::Singleton.~BSRandom();
                ::new (&BSRandom::Singleton) BSRandom;
            }
            float v22 = v15 +
                (v21 - v15) * static_cast<double>(BSRandom::Singleton.UnsignedInt(0xFFFFFFFFu)) *
                    2.3283064365386963e-10;
            float v23 = v22 * 0.01745329238474369;
            pInstanceData[v4].fRotationSpeed = v23;
            if ((Singleton_guard & 1) == 0) {
                Singleton_guard |= 1u;
                BSRandom::Singleton.~BSRandom();
                ::new (&BSRandom::Singleton) BSRandom;
            }
            float v24 = v30 +
                (v29 - v30) * static_cast<double>(BSRandom::Singleton.UnsignedInt(0xFFFFFFFFu)) *
                    2.3283064365386963e-10;
            pInstanceData[v4].fSpeedMult = v24;
            double fTextureCountX = this->fTextureCountX;
            if ((Singleton_guard & 1) == 0) {
                Singleton_guard |= 1u;
                BSRandom::Singleton.~BSRandom();
                ::new (&BSRandom::Singleton) BSRandom;
            }
            // The X frame index is converted to float as a signed integer, and the Y one likewise below.
            int32_t iTexX = BSRandom::Singleton.UnsignedInt(static_cast<int32_t>(fTextureCountX));
            float v16 = static_cast<float>(iTexX);
            double fTextureCountY = this->fTextureCountY;
            if ((Singleton_guard & 1) == 0) {
                Singleton_guard |= 1u;
                BSRandom::Singleton.~BSRandom();
                ::new (&BSRandom::Singleton) BSRandom;
            }
            int32_t v7 = BSRandom::Singleton.UnsignedInt(static_cast<int32_t>(fTextureCountY));
            ++v4;
            v8 = a2-- == 1;
            float v25 = static_cast<float>(v7);
            float v26 = v25 / this->fTextureCountY;
            double v9 = v26 * 100.0;
            float v27 = v16 / this->fTextureCountX;
            float v28 = v9 + v27;
            pInstanceData[v4 - 1].fTexCoords = v28;
        }
        while (!v8);
        iParticleCount = 0;
    }
    else {
        iParticleCount = 0;
    }
}

// Retires the expired particles and emits new ones in their slots, at the rate the elapsed time and the lifetime give;
// with abNeedsOffset new particles are placed relative to the Bip01 node's offset from its parent. The double casts
// keep the game's unrounded intermediates. The emitter switch uses the game's values (0 collision bones, 1 geometry, 2
// ray), not the EMITTER_TYPE names.
void ParticleShaderProperty::UpdateParticles(float afTime, bool abNeedsOffset, bool abGenerateNew) {
    float fDelta = afTime - fTimer;
    float fElapsed = __builtin_fabsf(fDelta);
    int32_t iMaxSize = static_cast<int32_t>(ParticleShader::GetMaxInstanceSize());
    int32_t iCap = static_cast<int32_t>(static_cast<double>(fMaxParticleRatio) * iMaxSize);
    int32_t iLimit = iMaxSize;
    if (iMaxSize >= iCap) {
        iLimit = iCap;
    }
    float fGenerate = static_cast<double>(fElapsed) / fLifetime * iLimit + fGenerateRemainder;
    int32_t iGenerate = static_cast<int32_t>(fGenerate);
    fGenerateRemainder = fGenerate - static_cast<double>(iGenerate);
    NiPoint3 kOffset = NiPoint3_ZERO;
    if (abNeedsOffset) {
        for (NiNode* pNode = pMyGeometry->m_pkParent; pNode; pNode = pNode->m_pkParent) {
            // SWEEP-INLINE 0x43B1B0
            const char* pcName = pNode->m_kName;
            // EXE-INLINED none: strcmp
            const char* pcBip = "Bip01";
            while (*pcName == *pcBip && *pcName) {
                ++pcName;
                ++pcBip;
            }
            if (*pcName == *pcBip) {
                NiNode* pParent = pNode->m_pkParent;
                kOffset.x = pNode->m_kWorld.m_Translate.x - pParent->m_kWorld.m_Translate.x;
                kOffset.y = pNode->m_kWorld.m_Translate.y - pParent->m_kWorld.m_Translate.y;
                kOffset.z = pNode->m_kWorld.m_Translate.z - pParent->m_kWorld.m_Translate.z;
                break;
            }
        }
    }
    for (int32_t i = 0; i < iMaxSize; ++i) {
        ParticleShader::ParticleData* pData = pInstanceData + i;
        float fAge = afTime - pData->fBirthTime;
        if (!(fAge < 0.0f) && !(fLifetime < static_cast<double>(fAge) * pData->fLifeAdjust)) {
            continue;
        }
        if (pData->fXPos != 3.4028234663852886e+38) {
            pData->fXPos = 3.4028235e+38f;
            pData->fYPos = 3.4028235e+38f;
            pData->fZPos = 3.4028235e+38f;
            pData->fBirthTime = -10000.0f;
            pData->fXVel = 0.0f;
            pData->fYVel = 0.0f;
            pData->fZVel = 0.0f;
            --iParticleCount;
        }
        if (!abGenerateNew || iGenerate <= 0 || iParticleCount >= iLimit) {
            continue;
        }
        switch (static_cast<int32_t>(eEmitterType)) {
            case 0: GenerateFromCollisionBones(i); break;
            case 1: GenerateFromGeometry(i); break;
            case 2: GenerateFromRay(i); break;
        }
        --iGenerate;
        ++iParticleCount;
        if (abNeedsOffset) {
            pData->fXPos -= kOffset.x;
            pData->fYPos -= kOffset.y;
            pData->fZPos -= kOffset.z;
        }
    }
    fTimer = afTime;
}

// With targets but a null first target object, every store to the position and velocity locals is skipped and they are
// copied, uninitialised, into the instance data, as the game does.
void ParticleShaderProperty::GenerateFromRay(uint32_t aiIndex) {
    NiPoint3 vertexNormal;
    NiPoint3 a3;
    NiPoint3 vertexPosition__vertexNormal;
    if (TargetArray.m_usSize) {
        NiAVObject* m_pObject = TargetArray.m_pBase->m_pObject;
        if (m_pObject) {
            float v17 = static_cast<double>(rt::rand()) / 32767.0;
            vertexNormal.x = 0.0f;
            double v4 = fRayLength * v17;
            a3.x = 0.0f;
            vertexNormal.y = v4;
            a3.y = vertexNormal.y;
            a3.z = 0.0f;
            int32_t v5 = rt::rand();
            float v18 = (static_cast<double>(v5) + static_cast<double>(v5)) / 32767.0 - 1.0;
            int32_t v6 = rt::rand();
            float v19 = (static_cast<double>(v6) + static_cast<double>(v6)) / 32767.0 - 1.0;
            vertexPosition__vertexNormal.x = v19;
            vertexNormal.x = v19;
            vertexPosition__vertexNormal.y = 0.0f;
            vertexPosition__vertexNormal.z = v18;
            vertexNormal.y = 0.0f;
            vertexNormal.z = v18;
            vertexNormal.Unitize();
            if (bWorldspace) {
                NiTransform* p_m_kWorld = &m_pObject->m_kWorld;
                NiPoint3* v8 = ::new (&vertexPosition__vertexNormal) NiPoint3(*p_m_kWorld * a3);
                float x = v8->x;
                float y = v8->y;
                float z = v8->z;
                a3.x = x;
                a3.y = y;
                a3.z = z;
                NiPoint3* v12 = ::new (&vertexPosition__vertexNormal) NiPoint3(p_m_kWorld->m_Rotate * vertexNormal);
                float v13 = v12->x;
                float v14 = v12->y;
                float v15 = v12->z;
                vertexNormal.x = v13;
                vertexNormal.y = v14;
                vertexNormal.z = v15;
            }
        }
    }
    else {
        a3 = NiPoint3_ZERO;
        vertexNormal = NiPoint3_ZERO;
    }
    pInstanceData[aiIndex].fXPos = a3.x;
    pInstanceData[aiIndex].fYPos = a3.y;
    pInstanceData[aiIndex].fZPos = a3.z;
    pInstanceData[aiIndex].fBirthTime = fTimer;
    pInstanceData[aiIndex].fXVel = vertexNormal.x;
    pInstanceData[aiIndex].fYVel = vertexNormal.y;
    pInstanceData[aiIndex].fZVel = vertexNormal.z;
}

// Divisions of rand() are unsigned: the 16-bit counts are taken as unsigned, as NiTArray::GetSize returns them.
void ParticleShaderProperty::GenerateFromCollisionBones(uint32_t aiIndex) {
    NiPoint3 result;
    NiPoint3 a2;
    bool v3 = TargetArray.m_usSize == 0;
    NiPoint3 a3 = NiPoint3_ZERO;
    NiPoint3 pt = NiPoint3_ZERO;
    if (v3) {
        a3 = NiPoint3_ZERO;
        pt = NiPoint3_ZERO;
    }
    else {
        NiGeometry* m_pObject;
        do {
            do {
                m_pObject = static_cast<NiGeometry*>(
                    TargetArray.m_pBase[static_cast<uint32_t>(rt::rand()) % TargetArray.m_usSize].m_pObject
                );
            }
            while (!m_pObject);
        }
        while (!m_pObject->m_spModelData.m_pObject);
        NiSkinInstance* v5 = m_pObject->m_spSkinInstance.m_pObject;
        if (v5) {
            uint32_t m_uiBones = v5->m_spSkinData.m_pObject->m_uiBones;
            NiAVObject* v7 = v5->m_ppkBones[rt::rand() % m_uiBones];
            NiNode* m_pkParent = v7->m_pkParent;
            float v46 = static_cast<double>(rt::rand()) / 32767.0;
            double v9 = v46;
            float v47 = 1.0 - v46;
            result.x = m_pkParent->m_kWorld.m_Translate.x * v47;
            result.y = m_pkParent->m_kWorld.m_Translate.y * v47;
            result.z = v47 * m_pkParent->m_kWorld.m_Translate.z;
            float v53 = v7->m_kWorld.m_Translate.x * v9;
            float v54 = v7->m_kWorld.m_Translate.y * v9;
            float v55 = v9 * v7->m_kWorld.m_Translate.z;
            a2.x = v53 + result.x;
            a2.y = v54 + result.y;
            a2.z = v55 + result.z;
            a3 = a2;
            int32_t v10 = rt::rand();
            float v48 = (static_cast<double>(v10) + static_cast<double>(v10)) / 32767.0 - 1.0;
            int32_t v11 = rt::rand();
            float v49 = (static_cast<double>(v11) + static_cast<double>(v11)) / 32767.0 - 1.0;
            a2.x = v49;
            a2.y = 0.0f;
            a2.z = v48;
            a2.Unitize();
            NiPoint3* v12 = ::new (&result) NiPoint3(m_pObject->m_kWorld.m_Rotate * a2);
            float x = v12->x;
            float y = v12->y;
            float z = v12->z;
            pt.x = x;
            pt.y = y;
            pt.z = z;
        }
        else {
            uint16_t m_usVertices = m_pObject->m_spModelData.m_pObject->m_usVertices;
            uint32_t v17 = static_cast<uint32_t>(rt::rand()) % m_usVertices;
            NiGeometryData* v18 = m_pObject->m_spModelData.m_pObject;
            if (v18) {
                NiAdditionalGeometryData* v20 = v18->m_spAdditionalGeomData.m_pObject;
                if (v20 && v20->GetVBReady()) {
                    NiGeometryDataIteratorNiPoint3 kVertices;
                    // EXE-INLINED 0x534550
                    kVertices.m_pvCurrentPosition = nullptr;
                    kVertices.m_iStride = 0;
                    kVertices.m_bCompressed = false;
                    NiGeometryDataIteratorNiPoint3 kNormals;
                    kNormals.m_pvCurrentPosition = nullptr;
                    kNormals.m_iStride = 0;
                    kNormals.m_bCompressed = false;
                    if (v18->LockPackedVertexData(true)) {
                        v18->GetVerticesIterator(kVertices);
                        v18->GetNormalsIterator(kNormals);
                        // Stride iterator indexing: element v17 lies v17 strides past the current position. The stream
                        // is raw vertex memory (void*), read untyped.
                        const NiPoint3* pVertex = reinterpret_cast<const NiPoint3*>(
                            static_cast<const char*>(kVertices.m_pvCurrentPosition) + v17 * kVertices.m_iStride
                        );
                        a3 = *pVertex;
                        const NiPoint3* pNormal = reinterpret_cast<const NiPoint3*>(
                            static_cast<const char*>(kNormals.m_pvCurrentPosition) + v17 * kNormals.m_iStride
                        );
                        pt = *pNormal;
                        v18->UnlockPackedVertexData();
                    }
                }
                else {
                    NiPoint3* v29;
                    NiPoint3* v33;
                    NiPoint3* m_pkVertex = v18->m_pkVertex;
                    if (m_pkVertex) {
                        v29 = &m_pkVertex[v17];
                    }
                    else {
                        v29 = &NiPoint3_ZERO;
                    }
                    NiPoint3* m_pkNormal = v18->m_pkNormal;
                    float v31 = v29->y;
                    a3.x = v29->x;
                    float v32 = v29->z;
                    a3.y = v31;
                    a3.z = v32;
                    if (m_pkNormal) {
                        v33 = &m_pkNormal[v17];
                    }
                    else {
                        v33 = &NiPoint3_ZERO;
                    }
                    float v34 = v33->x;
                    float v35 = v33->y;
                    float v36 = v33->z;
                    pt.x = v34;
                    pt.y = v35;
                    pt.z = v36;
                }
                if (bWorldspace) {
                    NiPoint3* v37 = ::new (&a2) NiPoint3(m_pObject->m_kWorld * a3);
                    float v38 = v37->x;
                    float v39 = v37->y;
                    float v40 = v37->z;
                    a3.x = v38;
                    a3.y = v39;
                    a3.z = v40;
                    NiPoint3* v41 = ::new (&a2) NiPoint3(m_pObject->m_kWorld.m_Rotate * pt);
                    float v42 = v41->x;
                    float v43 = v41->y;
                    float v44 = v41->z;
                    pt.x = v42;
                    pt.y = v43;
                    pt.z = v44;
                }
            }
        }
    }
    pInstanceData[aiIndex].fXPos = a3.x;
    pInstanceData[aiIndex].fYPos = a3.y;
    pInstanceData[aiIndex].fZPos = a3.z;
    pInstanceData[aiIndex].fBirthTime = fTimer;
    pInstanceData[aiIndex].fXVel = pt.x;
    pInstanceData[aiIndex].fYVel = pt.y;
    pInstanceData[aiIndex].fZVel = pt.z;
}

// The targets here are the nodes CollectCollisionBonesRecursive gathers: a node with children and a parent emits
// between its parent and itself. The divisions of rand() are unsigned.
void ParticleShaderProperty::GenerateFromGeometry(uint32_t aiIndex) {
    NiPoint3 randNormal;
    NiPoint3 v29;
    NiPoint3 result;
    if (TargetArray.m_usSize) {
        NiAVObject* m_pObject;
        NiNode* m_pkParent;
        do {
            m_pObject = TargetArray.m_pBase[static_cast<uint32_t>(rt::rand()) % TargetArray.m_usSize].m_pObject;
        }
        while (!m_pObject);
        // m_kChildren.m_usESize is the children array's effective size, i.e. the child count.
        if (static_cast<NiNode*>(m_pObject)->m_kChildren.m_usESize && (m_pkParent = m_pObject->m_pkParent) != 0) {
            float v18 = static_cast<double>(rt::rand()) / 32767.0;
            double v5 = v18;
            float v19 = 1.0 - v18;
            v29.x = m_pkParent->m_kWorld.m_Translate.x * v19;
            v29.y = m_pkParent->m_kWorld.m_Translate.y * v19;
            v29.z = v19 * m_pkParent->m_kWorld.m_Translate.z;
            randNormal.x = m_pObject->m_kWorld.m_Translate.x * v5;
            randNormal.y = m_pObject->m_kWorld.m_Translate.y * v5;
            randNormal.z = v5 * m_pObject->m_kWorld.m_Translate.z;
            result.x = randNormal.x + v29.x;
            result.y = randNormal.y + v29.y;
            result.z = v29.z + randNormal.z;
            v29 = result;
            int32_t v6 = rt::rand();
            float v20 = (static_cast<double>(v6) + static_cast<double>(v6)) / 32767.0 - 1.0;
            int32_t v7 = rt::rand();
            float v21 = (static_cast<double>(v7) + static_cast<double>(v7)) / 32767.0 - 1.0;
            result.x = v21;
            randNormal.x = v21;
            result.y = 0.0f;
            result.z = v20;
            randNormal.y = 0.0f;
            randNormal.z = v20;
            randNormal.Unitize();
            NiPoint3* v8 = ::new (&result) NiPoint3(m_pObject->m_kWorld.m_Rotate * randNormal);
            float x = v8->x;
            float y = v8->y;
            float z = v8->z;
            randNormal.x = x;
            randNormal.y = y;
            randNormal.z = z;
        }
        else {
            float v12 = m_pObject->m_kWorld.m_Translate.y;
            float v13 = m_pObject->m_kWorld.m_Translate.z;
            v29.x = m_pObject->m_kWorld.m_Translate.x;
            v29.y = v12;
            v29.z = v13;
            int32_t v14 = rt::rand();
            float v24 = (static_cast<double>(v14) + static_cast<double>(v14)) / 32767.0 - 1.0;
            int32_t v15 = rt::rand();
            float v25 = (static_cast<double>(v15) + static_cast<double>(v15)) / 32767.0 - 1.0;
            int32_t v16 = rt::rand();
            float v26 = (static_cast<double>(v16) + static_cast<double>(v16)) / 32767.0 - 1.0;
            result.x = v26;
            randNormal.x = v26;
            result.y = v25;
            result.z = v24;
            randNormal.y = v25;
            randNormal.z = v24;
            randNormal.Unitize();
        }
    }
    else {
        v29 = NiPoint3_ZERO;
        randNormal = NiPoint3_ZERO;
    }
    pInstanceData[aiIndex].fXPos = v29.x;
    pInstanceData[aiIndex].fYPos = v29.y;
    pInstanceData[aiIndex].fZPos = v29.z;
    pInstanceData[aiIndex].fBirthTime = fTimer;
    pInstanceData[aiIndex].fXVel = randNormal.x;
    pInstanceData[aiIndex].fYVel = randNormal.y;
    pInstanceData[aiIndex].fZVel = randNormal.z;
}

void ParticleShaderProperty::CopyMembers(ParticleShaderProperty* pDest, NiCloningProcess& arCloneData) {
    NiShadeProperty::CopyMembers(pDest, arCloneData);
    pDest->ulFlags[0] = ulFlags[0];
    pDest->ulFlags[1] = ulFlags[1];
    pDest->fAlpha = fAlpha;
    double fFadeAlpha = this->fFadeAlpha;
    pDest->iLastRenderPassState = 0;
    pDest->fFadeAlpha = fFadeAlpha;
    pDest->iShader = iShader;
    pDest->fDepthBias = fDepthBias;
}

void ParticleShaderProperty::SetColor2(NiColorA aColor) {
    Color2 = aColor;
}

void ParticleShaderProperty::SetLifetimeVar(float a2) {
    fLifeVar = a2;
}

void ParticleShaderProperty::SetBlendOp(D3DBLENDOP a2) {
    eBlendOperation = a2;
}

void ParticleShaderProperty::SetZTestFunc(D3DCMPFUNC a2) {
    eZTestFunction = a2;
}

void ParticleShaderProperty::SetExplosionWindSpeed(float a2) {
    fWindSpeed = a2;
}

void ParticleShaderProperty::SetFade(float a2) {
    fFade = a2;
}

void ParticleShaderProperty::SetLifetime(float afLifetime) {
    fLifetime = afLifetime;
}

void ParticleShaderProperty::SetNormalSpeed(float afNormalSpeed) {
    fNormalSpeed = afNormalSpeed;
}

void ParticleShaderProperty::SetNormalSpeedVar(float afValue) {
    fNormalSpeedVar = afValue;
}

void ParticleShaderProperty::SetNormalAcceleration(float afNormalAcc) {
    fNormalAcceleration = afNormalAcc;
}

void ParticleShaderProperty::SetRotate(float afValue) {
    fRotate = afValue;
}

void ParticleShaderProperty::SetRotateVar(float afValue) {
    fRotateVar = afValue;
}

void ParticleShaderProperty::SetRotateSpeed(float afValue) {
    fRotateSpeed = afValue;
}

void ParticleShaderProperty::SetRotateSpeedVar(float afValue) {
    fRotateSpeedVar = afValue;
}

void ParticleShaderProperty::SetColorTime1(float aColorTime) {
    ColorTime1 = aColorTime;
}

void ParticleShaderProperty::SetColorTime2(float aColorTime) {
    ColorTime2 = aColorTime;
}

void ParticleShaderProperty::SetColorTime3(float aColorTime) {
    ColorTime3 = aColorTime;
}

void ParticleShaderProperty::SetScale2(float aScale) {
    Scale2 = aScale;
}

void ParticleShaderProperty::SetScaleTime1(float aScaleTime) {
    ScaleTime1 = aScaleTime;
}

void ParticleShaderProperty::SetTextureCountX(float afValue) {
    fTextureCountX = afValue;
}

void ParticleShaderProperty::SetTextureCountY(float afValue) {
    fTextureCountY = afValue;
}

void ParticleShaderProperty::SetMaxParticleRatio(float afMaxParticle) {
    fMaxParticleRatio = afMaxParticle;
}

void ParticleShaderProperty::SetScale1(float a2) {
    Scale1 = a2;
}

void ParticleShaderProperty::SetByte6C(bool abWorldSpace) {
    bWorldspace = abWorldSpace;
}

// Shifts the birth time of every live particle (fXPos not FLT_MAX) by the change of the timer. The loop is signed.
void ParticleShaderProperty::RewindTimer(float afNewTime) {
    float fDelta = afNewTime - fTimer;
    int32_t iMaxSize = static_cast<int32_t>(ParticleShader::GetMaxInstanceSize());
    for (int32_t i = 0; i < iMaxSize; ++i) {
        if (pInstanceData[i].fXPos != 3.4028234663852886e+38) {
            pInstanceData[i].fBirthTime = fDelta + pInstanceData[i].fBirthTime;
        }
    }
    fTimer = afNewTime;
}

void ParticleShaderProperty::CollectGeometryRecursive(NiAVObject* apObject) {
    NiTriBasedGeom* pgeometry = static_cast<NiTriBasedGeom*>(apObject);
    if (apObject) {
        const NiRTTI* v4 = apObject->GetRTTI();
        if (v4) {
            while (v4 != &NiTriBasedGeom::ms_RTTI) {
                v4 = v4->GetBaseRTTI();
                if (!v4) {
                    goto LABEL_5;
                }
            }
            NiProperty* Property = pgeometry->GetProperty(3);
            if (Property) {
                const NiRTTI* v10 = Property->GetRTTI();
                if (v10) {
                    while (v10 != &BSShaderLightingProperty::ms_RTTI) {
                        v10 = v10->GetBaseRTTI();
                        if (!v10) {
                            return;
                        }
                    }
                    NiPointer<NiAVObject> spGeometry = pgeometry;
                    NiAVObjectArray_AddFirstEmpty(&TargetArray, &spGeometry);
                }
            }
        }
        else {
        LABEL_5:;
            NiNode* pnode = pgeometry->IsNode();
            if (pnode) {
                const char* m_kHandle = pnode->m_kName.GetHandle();
                if (m_kHandle == NiGlobalStringTable::NULL_STRING || std::strncmp(m_kHandle, "Bip", 3u)) {
                    for (uint32_t i = 0; i < pnode->m_kChildren.m_usSize;
                         CollectGeometryRecursive(pnode->m_kChildren.m_pBase[i++].m_pObject)) {
                        ;
                    }
                }
            }
        }
    }
}

void ParticleShaderProperty::CollectCollisionBonesRecursive(NiAVObject* apObject) {
    NiAVObject* v3 = apObject;
    if (apObject) {
        bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
        NiNode* v5 = apObject->IsNode();
        if (m_pObject) {
            if (!v5) {
                return;
            }
            NiPointer<NiAVObject> spObject(v3);
            NiAVObjectArray_AddFirstEmpty(&TargetArray, &spObject);
        }
        if (v5) {
            for (uint32_t i = 0; i < v5->m_kChildren.m_usSize;
                 CollectCollisionBonesRecursive(v5->m_kChildren.m_pBase[i++].m_pObject)) {
                ;
            }
        }
    }
}

void ParticleShaderProperty::CollectTargets(NiAVObject* apObject) {
    NiTObjectArray<NiPointer<NiAVObject>>* p_TargetArray = &TargetArray;
    TargetArray.RemoveAll();
    ParticleShaderProperty::EMITTER_TYPE eEmitterType = this->eEmitterType;
    if (eEmitterType) {
        int32_t v4 = eEmitterType - 1;
        if (v4) {
            if (v4 == 1) {
                NiPointer<NiAVObject> spObject(apObject);
                NiAVObjectArray_AddFirstEmpty(p_TargetArray, &spObject);
            }
        }
        else {
            CollectCollisionBonesRecursive(apObject);
        }
    }
    else {
        CollectGeometryRecursive(apObject);
    }
    NiAVObjectArray_Compact(p_TargetArray);
}

// Allocates four floats per particle instance and stores each instance's index in all four.
uint8_t* ParticleShaderProperty::GetInstanceIndex() {
    void* pData = ParticleShaderProperty::pfWindData;
    if (!ParticleShaderProperty::pfWindData) {
        uint32_t uiMaxInstances = ParticleShader::GetMaxInstanceSize();
        pData = MemoryManager_Singleton.Allocate(
            4 * uiMaxInstances <= SIZE_MAX / sizeof(float) ? sizeof(float) * (4 * uiMaxInstances) : SIZE_MAX
        );
        uint32_t i = 0;
        ParticleShaderProperty::pfWindData = pData;
        if (uiMaxInstances) {
            float* pfEntry = static_cast<float*>(pData);
            do {
                float fIndex = static_cast<float>(i++);
                pfEntry[0] = fIndex;
                pfEntry[1] = fIndex;
                pfEntry[2] = fIndex;
                pfEntry[3] = fIndex;
                pfEntry += 4;
            }
            while (i < uiMaxInstances);
            return static_cast<uint8_t*>(ParticleShaderProperty::pfWindData);
        }
    }
    return static_cast<uint8_t*>(pData);
}
