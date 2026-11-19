#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"

NiRTTI NiGeometry::ms_RTTI("NiGeometry", &NiAVObject::ms_RTTI);

NiGeometry::NiGeometry() : m_spShader(nullptr) {}

NiGeometry::NiGeometry(NiGeometryData* a1) : m_spModelData(a1), m_spShader(nullptr) {}

// The model data, skin instance and property state are released by their own destructors; m_spShader is not owned.
NiGeometry::~NiGeometry() {}

// Reads the base object and the two link IDs, then this version's shader block: from 20.2.0.5 a count of (name,
// extra-data) pairs and an active-shader value, from 5.0.0.21 to it a flag byte and one name and extra-data value; from
// 20.2.0.7 a dirty flag byte. Every value read is dropped.
void NiGeometry::LoadBinary(NiStream& kStream) {
    uint32_t uiSize;
    NiAVObject::LoadBinary(kStream);
    kStream.ReadLinkID();
    kStream.ReadLinkID();
    uint32_t uiVersion = kStream.GetFileVersion();
    if (uiVersion >= NiStream::GetVersion(20, 2, 0, 5)) {
        uint32_t uiNumShaders = 0;
        uiSize = 4;
        kStream.Istr()->BinaryRead(&uiNumShaders, 4, &uiSize, 1);
        for (uint32_t i = 0; i < uiNumShaders; ++i) {
            uint32_t uiExtraData;
            NiFixedString kName;
            kStream.LoadFixedString(kName);
            uiSize = 4;
            kStream.Istr()->BinaryRead(&uiExtraData, 4, &uiSize, 1);
        }
        uint32_t uiActiveShader;
        uiSize = 4;
        kStream.Istr()->BinaryRead(&uiActiveShader, 4, &uiSize, 1);
    }
    else if (uiVersion >= NiStream::GetVersion(5, 0, 0, 21)) {
        uint8_t ucHasShader = 0;
        uiSize = 1;
        kStream.Istr()->BinaryRead(&ucHasShader, 1, &uiSize, 1);
        if (ucHasShader) {
            bool bCString = kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1);
            NiFixedString kName;
            if (bCString) {
                kStream.LoadCStringAsFixedString(kName);
            }
            else {
                kStream.LoadFixedString(kName);
            }
            uint32_t uiExtraData;
            NiStreamLoadBinary<uint32_t>(kStream, uiExtraData);
        }
    }
    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 2, 0, 7)) {
        uint8_t ucDirty = 0;
        uiSize = 1;
        kStream.Istr()->BinaryRead(&ucDirty, 1, &uiSize, 1);
    }
}

void NiGeometry::LinkObject(NiStream& kStream) {
    NiAVObject::LinkObject(kStream);
    NiObject* ObjectFromLinkID = kStream.GetObjectFromLinkID();
    m_spModelData = static_cast<NiGeometryData*>(ObjectFromLinkID);
    NiObject* v6 = kStream.GetObjectFromLinkID();
    m_spSkinInstance = static_cast<NiSkinInstance*>(v6);
}

// Registers the object, its model data (never null-checked) and its skin instance.
bool NiGeometry::RegisterStreamables(NiStream& kStream) {
    if (!NiAVObject::RegisterStreamables(kStream)) {
        return false;
    }
    m_spModelData.m_pObject->RegisterStreamables(kStream);
    if (m_spSkinInstance.m_pObject) {
        m_spSkinInstance.m_pObject->RegisterStreamables(kStream);
    }
    return true;
}

// Writes the base object, the model-data and skin-instance link IDs, then this version's material block: a 4-byte
// count, a 4-byte extra-data value and a 1-byte dirty flag, all literal zeros (there is no material list here, unlike
// the reference source).
void NiGeometry::SaveBinary(NiStream& kStream) {
    NiAVObject::SaveBinary(kStream);
    kStream.SaveLinkID(m_spModelData.m_pObject);
    kStream.SaveLinkID(m_spSkinInstance.m_pObject);
    uint32_t auiFrame[4];
    auiFrame[1] = 0;
    auiFrame[0] = 4;
    kStream.m_pkOstr->BinaryWrite(&auiFrame[1], 4, &auiFrame[0], 1);
    auiFrame[2] = 0;
    auiFrame[0] = 4;
    kStream.m_pkOstr->BinaryWrite(&auiFrame[2], 4, &auiFrame[0], 1);
    auiFrame[3] = 0;
    auiFrame[2] = 1;
    kStream.m_pkOstr->BinaryWrite(&auiFrame[3], 1, &auiFrame[2], 1);
}

// Equal when the base parts match, both or neither have skin instances that compare equal, and the model data match.
bool NiGeometry::IsEqual(NiObject* pkObject) {
    if (!NiAVObject::IsEqual(pkObject)) {
        return false;
    }
    NiGeometry* pkGeometry = static_cast<NiGeometry*>(pkObject);
    NiObject* pkSkin = m_spSkinInstance.m_pObject;
    NiObject* pkOtherSkin = pkGeometry->m_spSkinInstance.m_pObject;
    if ((pkOtherSkin == nullptr) != (pkSkin == nullptr)) {
        return false;
    }
    if (pkSkin && !pkSkin->IsEqual(pkOtherSkin)) {
        return false;
    }
    NiObject* pkModelData = m_spModelData.m_pObject;
    return pkModelData->IsEqual(pkGeometry->m_spModelData.m_pObject);
}

void NiGeometry::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiAVObject::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiGeometry::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* v20 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, v20);
    char* ViewerString_8 = NiGetViewerString("GeomData", static_cast<const void*>(m_spModelData.m_pObject));
    uint32_t v8 = pkStrings->m_usSize;
    uint32_t v9 = pkStrings->m_usMaxSize;
    v20 = ViewerString_8;
    if (v8 >= v9) {
        pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v8, v20);
    BSShader* m_spShader = this->GetShader();
    if (m_spShader) {
        const NiRTTI* v11 = m_spShader->GetRTTI();
        char* ViewerString_9 = NiGetViewerString("shader", v11->GetName());
        uint32_t v13 = pkStrings->m_usSize;
        uint32_t v14 = pkStrings->m_usMaxSize;
        v20 = ViewerString_9;
        if (v13 >= v14) {
            pkStrings->SetSize(v13 + pkStrings->m_usGrowBy);
        }
        pkStrings->SetAt(v13, v20);
    }
    NiSkinInstance* m_pObject = m_spSkinInstance.m_pObject;
    if (m_pObject) {
        NiSkinPartition* v16 = m_pObject->m_spSkinPartition.m_pObject;
        if (v16) {
            char* ViewerString_6 = NiGetViewerString("skin partitions", v16->m_uiPartitions);
            uint32_t v18 = pkStrings->m_usSize;
            uint32_t v19 = pkStrings->m_usMaxSize;
            v20 = ViewerString_6;
            if (v18 >= v19) {
                pkStrings->SetSize(v18 + pkStrings->m_usGrowBy);
            }
            pkStrings->SetAt(v18, v20);
        }
    }
}

void NiGeometry::ProcessClone(NiCloningProcess& kCloning) {
    NiAVObject::ProcessClone(kCloning);
    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiSkinInstance* pkSkin = m_spSkinInstance.m_pObject;
    NiGeometry* pkGeom = static_cast<NiGeometry*>(pkObject);
    if (pkSkin) {
        if (kCloning.m_pkCloneMap->GetAt(pkSkin, pkObject)) {
            pkGeom->m_spSkinInstance = static_cast<NiSkinInstance*>(pkObject);
        }
        else {
            pkGeom->m_spSkinInstance = static_cast<NiSkinInstance*>(m_spSkinInstance.m_pObject->CreateClone(kCloning));
            m_spSkinInstance.m_pObject->ProcessClone(kCloning);
        }
    }
    if (GetShader()) {
        if (GetShader()->GetRTTI() == &NiShader::ms_RTTI) {
            // A plain NiShader is copied: a new one with its name and implementation id. It is stored in the clone's
            // m_spShader as it is.
            NiShader* pkShader = new NiShader;
            pkShader->SetName(GetShader()->m_kName);
            pkShader->m_uiImplementation = GetShader()->m_uiImplementation;
            pkGeom->m_spShader = static_cast<BSShader*>(pkShader);
        }
        else {
            pkGeom->m_spShader = GetShader();
            GetShader()->SetupGeometry(pkGeom);
            NiAVObject::kPropertyStateLock.Lock(nullptr);
            pkGeom->UpdatePropertiesDownward(&m_PropertyState);
            if (!--NiAVObject::kPropertyStateLock.uiLockCount) {
                NiAVObject::kPropertyStateLock.uiOwningThread = 0;
            }
        }
    }
}

void NiGeometry::PostLinkObject(NiStream& kStream) {
    NiAVObject::PostLinkObject(kStream);
}

void NiGeometry::SetBound(BSNiBound* apBound) {
    NiAVObject::SetBound(apBound);
}

void NiGeometry::ApplyTransform(const NiMatrix3& kMat, const NiPoint3& kTrn, bool bOnLeft) {
    NiMatrix3 inv;
    uint16_t v5 = m_spModelData.m_pObject->GetActiveVertexCount();
    NiTriShapeData* m_pObject = static_cast<NiTriShapeData*>(m_spModelData.m_pObject);
    NiPoint3* m_pkVertex = m_pObject->m_pkVertex;
    NiPoint3* m_pkNormal = m_pObject->m_pkNormal;
    NiPoint3* pkData = m_pkNormal;
    if (bOnLeft) {
        NiMatrix3 kInvMat;
        NiMatrix3* v10 = &(inv = kMat * m_kLocal.m_Rotate);
        kInvMat = m_kLocal.m_Rotate.TransposeTimes(*v10);
        float v44 = kMat.m_pEntry[0][1] * m_kLocal.m_Translate.y + kMat.m_pEntry[0][0] * m_kLocal.m_Translate.x +
            kMat.m_pEntry[0][2] * m_kLocal.m_Translate.z;
        float v47 = kMat.m_pEntry[1][1] * m_kLocal.m_Translate.y + kMat.m_pEntry[1][0] * m_kLocal.m_Translate.x +
            kMat.m_pEntry[1][2] * m_kLocal.m_Translate.z;
        float v50 = kMat.m_pEntry[2][1] * m_kLocal.m_Translate.y + kMat.m_pEntry[2][0] * m_kLocal.m_Translate.x +
            kMat.m_pEntry[2][2] * m_kLocal.m_Translate.z;
        float v53 = kTrn.x + v44;
        float v57 = kTrn.y + v47;
        float v61 = kTrn.z + v50;
        float v45 = v53 - m_kLocal.m_Translate.x;
        float v48 = v57 - m_kLocal.m_Translate.y;
        float v51 = v61 - m_kLocal.m_Translate.z;
        float v54 = m_kLocal.m_Rotate.m_pEntry[2][0] * v51 +
            (m_kLocal.m_Rotate.m_pEntry[0][0] * v45 + m_kLocal.m_Rotate.m_pEntry[1][0] * v48);
        float v58 = m_kLocal.m_Rotate.m_pEntry[1][1] * v48 + m_kLocal.m_Rotate.m_pEntry[0][1] * v45 +
            m_kLocal.m_Rotate.m_pEntry[2][1] * v51;
        float v62 = v45 * m_kLocal.m_Rotate.m_pEntry[0][2] + v48 * m_kLocal.m_Rotate.m_pEntry[1][2] +
            v51 * m_kLocal.m_Rotate.m_pEntry[2][2];
        float v43 = 1.0 / m_kLocal.m_fScale;
        float v46 = v43 * v54;
        float v49 = v58 * v43;
        float v52 = v43 * v62;
        if ((m_pkVertex) && (v5)) {
            double v11 = kInvMat.m_pEntry[2][2];
            float* p_z = &m_pkVertex->z;
            double v13 = kInvMat.m_pEntry[2][1];
            int32_t v14 = v5;
            double v15 = kInvMat.m_pEntry[2][0];
            double v16 = kInvMat.m_pEntry[1][2];
            double v17 = kInvMat.m_pEntry[1][1];
            do {
                double v18 = *(p_z - 2);
                p_z += 3;
                --v14;
                float v55 = v18 * kInvMat.m_pEntry[0][0] + *(p_z - 4) * kInvMat.m_pEntry[0][1] +
                    *(p_z - 3) * kInvMat.m_pEntry[0][2];
                float v59 = *(p_z - 5) * kInvMat.m_pEntry[1][0] + *(p_z - 4) * v17 + *(p_z - 3) * v16;
                float v63 = *(p_z - 5) * v15 + *(p_z - 4) * v13 + *(p_z - 3) * v11;
                float v65 = v55 + v46;
                *(p_z - 5) = v65;
                float v69 = v59 + v49;
                *(p_z - 4) = v69;
                float a3 = v63 + v52;
                *(p_z - 3) = a3;
            }
            while (v14);
        }

        if (m_pkNormal) {
            kInvMat.Inverse(inv);
            if (v5) {
                double v19 = inv.m_pEntry[2][2];
                float* v20 = &m_pkNormal->z;
                double v21 = inv.m_pEntry[2][1];
                int32_t v22 = v5;
                double v23 = inv.m_pEntry[2][0];
                double v24 = inv.m_pEntry[1][2];
                double v25 = inv.m_pEntry[1][1];
                do {
                    double v26 = *(v20 - 1);
                    v20 += 3;
                    --v22;
                    float v66 = v26 * inv.m_pEntry[1][0] + *(v20 - 5) * inv.m_pEntry[0][0] + v23 * *(v20 - 3);
                    float v70 = *(v20 - 5) * inv.m_pEntry[0][1] + *(v20 - 4) * v25 + v21 * *(v20 - 3);
                    double v27 = *(v20 - 5) * inv.m_pEntry[0][2] + *(v20 - 4) * v24;
                    double v28 = v19 * *(v20 - 3);
                    *(v20 - 5) = v66;
                    *(v20 - 4) = v70;
                    float a3a = v27 + v28;
                    *(v20 - 3) = a3a;
                }
                while (v22);
            }
        }
    }
    else {
        if ((m_pkVertex) && (v5)) {
            float* v29 = &m_pkVertex->z;
            int32_t v30 = v5;
            do {
                v29 += 3;
                --v30;
                float v67 = kMat.m_pEntry[0][1] * *(v29 - 4) + kMat.m_pEntry[0][0] * *(v29 - 5) +
                    *(v29 - 3) * kMat.m_pEntry[0][2];
                float v71 = kMat.m_pEntry[1][1] * *(v29 - 4) + *(v29 - 5) * kMat.m_pEntry[1][0] +
                    *(v29 - 3) * kMat.m_pEntry[1][2];
                float a3b = kMat.m_pEntry[2][0] * *(v29 - 5) + *(v29 - 4) * kMat.m_pEntry[2][1] +
                    *(v29 - 3) * kMat.m_pEntry[2][2];
                float v56 = kTrn.x + v67;
                float v60 = v71 + kTrn.y;
                double v31 = a3b + kTrn.z;
                *(v29 - 5) = v56;
                *(v29 - 4) = v60;
                float v64 = v31;
                *(v29 - 3) = v64;
            }
            while (v30);
            m_pkNormal = pkData;
        }

        if (m_pkNormal) {
            kMat.Inverse(inv);
            if (v5) {
                double v32 = inv.m_pEntry[2][2];
                float* v33 = &m_pkNormal->z;
                double v34 = inv.m_pEntry[2][1];
                int32_t v35 = v5;
                double v36 = inv.m_pEntry[2][0];
                double v37 = inv.m_pEntry[1][2];
                double v38 = inv.m_pEntry[1][1];
                do {
                    v33 += 3;
                    --v35;
                    float v68 = inv.m_pEntry[1][0] * *(v33 - 4) + inv.m_pEntry[0][0] * *(v33 - 5) + v36 * *(v33 - 3);
                    float v72 = inv.m_pEntry[0][1] * *(v33 - 5) + v38 * *(v33 - 4) + v34 * *(v33 - 3);
                    double v39 = inv.m_pEntry[0][2] * *(v33 - 5) + v37 * *(v33 - 4);
                    double v40 = v32 * *(v33 - 3);
                    *(v33 - 5) = v68;
                    *(v33 - 4) = v72;
                    float a3c = v39 + v40;
                    *(v33 - 3) = a3c;
                }
                while (v35);
            }
        }
    }
    m_spModelData.m_pObject->m_kBound.ComputeFromData(v5, m_pkVertex);
}

// The base update is direct; a skin forces selective updates and clears the rigid flag.
void NiGeometry::SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t bSelectiveUpdateTransforms, bool& bRigid) {
    this->NiAVObject::SetSelectiveUpdateFlags(bSelectiveUpdate, bSelectiveUpdateTransforms, bRigid);
    if (this->m_spSkinInstance.m_pObject) {
        this->m_uFlags = static_cast<NiAVObject::NiFlags>(this->m_uFlags | NiAVObject::SELECTIVE_UPDATE_TRANSFORMS);
        bSelectiveUpdate = true;
        this->m_uFlags = static_cast<NiAVObject::NiFlags>(this->m_uFlags | NiAVObject::SELECTIVE_UPDATE);
        bRigid = false;
    }
}

// The base pushes the parent's state down; then, under the property-state lock, this geometry's state takes the
// parent's and each attached property replaces the slot of its type. Types up to 7 are accepted (a signed compare), one
// past the seven slots, so a type-7 property lands in the word after the state, as the game does. The material is then
// marked for an update.
void NiGeometry::UpdatePropertiesDownward(NiPropertyState* pkParentState) {
    NiAVObject::UpdatePropertiesDownward(pkParentState);
    NiAVObject::kPropertyStateLock.Lock(nullptr);
    m_PropertyState = *pkParentState;
    NiTListItem<NiPointer<NiProperty>>* pkPos = m_kPropertyList.m_pkHead;
    while (pkPos) {
        NiProperty* pkProperty = pkPos->m_element.m_pObject;
        pkPos = pkPos->m_pkNext;
        if (pkProperty && pkProperty->Type() <= 7) {
            NiPointer<NiProperty>* pspSlot = m_PropertyState.m_aspProps + pkProperty->Type();
            *pspSlot = pkProperty;
        }
    }
    NiAVObject::kPropertyStateLock.Unlock();
    SetMaterialNeedsUpdate(true);
}

// Recomputed only when the bound is orphaned and not fixed, or when its radius is exactly 0 (an ordered compare: a NaN
// radius skips). A missing world bound is first allocated, set through SetBound and zeroed. When the world-bound change
// flag is set, it is left set if the new bound equals the old one and cleared otherwise, as the game does.
void NiGeometry::UpdateWorldBound() {
    NiBound* pkWorldBound = m_kWorldBound;
    const NiBound* pkSource = pkWorldBound ? pkWorldBound : &NiAVObject::NullBoundS;
    NiBound kOldBound;
    kOldBound.m_kCenter.x = pkSource->m_kCenter.x;
    kOldBound.m_kCenter.y = pkSource->m_kCenter.y;
    kOldBound.m_kCenter.z = pkSource->m_kCenter.z;
    kOldBound.m_iRadiusAsInt = pkSource->m_iRadiusAsInt;
    uint32_t uFlags = m_uFlags;
    bool bOrphaned = (uFlags & NiAVObject::HAS_ORPHANED_BOUND) && !(uFlags & NiAVObject::FIXED_BOUND);
    if (!bOrphaned && !(kOldBound.m_fRadius == 0.0f)) {
        return;
    }
    if (!pkWorldBound) {
        SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(NiBound))));
        NiBound* pkNewBound = m_kWorldBound;
        pkNewBound->m_kCenter.x = NiPoint3_ZERO.x;
        pkNewBound->m_kCenter.y = NiPoint3_ZERO.y;
        pkNewBound->m_fRadius = 0.0f;
        pkNewBound->m_kCenter.z = NiPoint3_ZERO.z;
    }
    NiSkinInstance* pkSkin = m_spSkinInstance.m_pObject;
    if (pkSkin) {
        pkSkin->UpdateModelBound(*m_kWorldBound);
        m_kWorldBound->Update(m_kWorld);
    }
    else {
        m_kWorldBound->Update(m_spModelData.m_pObject->m_kBound, m_kWorld);
    }
    uFlags = m_uFlags;
    if (uFlags & NiAVObject::WORLD_BOUND_CHANGE) {
        if (*m_kWorldBound == kOldBound) {
            m_uFlags = static_cast<NiAVObject::NiFlags>(uFlags | NiAVObject::WORLD_BOUND_CHANGE);
        }
        else {
            m_uFlags = static_cast<NiAVObject::NiFlags>(uFlags & ~NiAVObject::WORLD_BOUND_CHANGE);
        }
    }
}

void NiGeometry::OnVisible(NiCullingProcess& kCuller) {
    kCuller.AppendVirtual(*this);
}

// The renderer's purge virtuals live in the platform renderer.
void NiGeometry::PurgeRendererData(NiRenderer* apkRenderer) {
    NiAVObject::PurgeRendererData(apkRenderer);
    NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(apkRenderer);
    pkRenderer->PurgeGeometryData(m_spModelData.m_pObject);
    if (m_spSkinInstance.m_pObject) {
        pkRenderer->PurgeSkinInstance(m_spSkinInstance.m_pObject);
        pkRenderer->PurgeSkinPartition(m_spSkinInstance.m_pObject->m_spSkinPartition.m_pObject);
    }
}

void NiGeometry::RenderImmediate(NiRenderer* pkRenderer) {
    pkRenderer->m_pkCurrProp = &this->m_PropertyState;
    NiTimeController* pController = this->m_spControllers;
    while (pController) {
        pController->OnPreDisplay();
        pController = pController->m_spNext;
    }
}

void NiGeometry::OnlyRenderImmediate(NiRenderer* pkRenderer) {
    NiTimeController* i;
    pkRenderer->m_pkCurrProp = &m_PropertyState;
    for (i = m_spControllers; i; i = i->m_spNext.m_pObject) {
        i->OnPreDisplay();
    }
}

// A null model data is ignored. Otherwise a collision object first rebuilds its world data and updates it with a zeroed
// update record (time 0, every flag false), then the model data is replaced.
void NiGeometry::SetModelData(NiGeometryData* pkModelData) {
    if (!pkModelData) {
        return;
    }
    if (m_spCollisionObject.m_pObject) {
        m_spCollisionObject.m_pObject->RecreateWorldData();
        NiUpdateData kUpdateData;
        // EXE-INLINED 0x43D410
        kUpdateData.fTime = 0.0f;
        kUpdateData.bUpdateControllers = false;
        kUpdateData.bParallelUpdate = false;
        kUpdateData.bFoundParticles = false;
        kUpdateData.bFoundMorphController = false;
        kUpdateData.bSceneGraphChange = false;
        m_spCollisionObject.m_pObject->UpdateWorldData(kUpdateData);
    }
    m_spModelData = pkModelData;
}

void NiGeometry::CalculateNormals() {
    m_spModelData.m_pObject->CalculateNormals();
}

// Geometry is static unless a vertex controller animates it, or it is skinned without a partition (or, at run time,
// the renderer lacks hardware skinning: flag 2 of GetFlags).
void NiGeometry::CalculateConsistency(bool bTool) {
    NiGeometryData::Consistency eConsistency = NiGeometryData::STATIC;
    for (NiTimeController* pkCtrl = m_spControllers; pkCtrl; pkCtrl = pkCtrl->m_spNext.m_pObject) {
        if (pkCtrl->IsVertexController()) {
            eConsistency = NiGeometryData::MUTABLE;
            break;
        }
    }
    NiSkinInstance* pkSkin = m_spSkinInstance.m_pObject;
    if (pkSkin) {
        if (bTool) {
            if (!pkSkin->m_spSkinPartition.m_pObject) {
                eConsistency = NiGeometryData::MUTABLE;
            }
        }
        else if (!pkSkin->m_spSkinPartition.m_pObject) {
            eConsistency = NiGeometryData::MUTABLE;
        }
        else {
            NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
            if (pkRenderer && !(pkRenderer->GetFlags() & 2)) {
                eConsistency = NiGeometryData::MUTABLE;
            }
        }
    }
    m_spModelData.m_pObject->SetConsistency(eConsistency);
}

void NiGeometry::CopyMembers(NiGeometry* pkDest, NiCloningProcess& kCloning) {
    pkDest->SetModelData(this->m_spModelData.m_pObject);
    this->NiAVObject::CopyMembers(pkDest, kCloning);
}

NiSkinInstance* NiGeometry::GetSkinInstance() const {
    return m_spSkinInstance;
}

void NiGeometry::SetSkinInstance(NiSkinInstance* pSkinInstance) {
    m_spSkinInstance = pSkinInstance;
}

uint16_t NiGeometry::GetActiveVertexCount() const {
    NiGeometryData* v2 = m_spModelData;
    return v2->GetActiveVertexCount();
}

void NiGeometry::SetAdditionalGeometryData(NiAdditionalGeometryData* pkData) {
    NiGeometryData* v2 = m_spModelData;
    v2->SetAdditionalGeometryData(pkData);
}
