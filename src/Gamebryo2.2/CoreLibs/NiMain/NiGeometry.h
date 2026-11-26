#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

class BSShader;
class NiPoint2;
class NiRenderer;
class NiSkinInstance;

// A renderable leaf of the scene graph: shared model data, an optional skin, the properties in effect and the shader.
class NiGeometry : public NiAVObject {
public:
    NiGeometry();
    NiGeometry(NiGeometryData* pkModelData);
    ~NiGeometry() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiGeometry* IsGeometry() override {
        return this;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    void PostLinkObject(NiStream& kStream) override;
    // This override, shared by NiGeometry, NiNode and their derived classes, just calls NiAVObject::SetBound.
    void SetBound(BSNiBound* apBound) override;
    void ApplyTransform(const NiMatrix3& kMat, const NiPoint3& kTrn, bool bOnLeft) override;
    void SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t bSelectiveUpdateTransforms, bool& bRigid) override;
    void UpdatePropertiesDownward(NiPropertyState* pkParentState) override;
    void UpdateWorldBound() override;
    void OnVisible(NiCullingProcess& kCuller) override;
    void PurgeRendererData(NiRenderer* apkRenderer) override;
    virtual void RenderImmediate(NiRenderer* pkRenderer);
    virtual void OnlyRenderImmediate(NiRenderer* pkRenderer);
    virtual void SetModelData(NiGeometryData* pkModelData);
    virtual void CalculateNormals();
    virtual void CalculateConsistency(bool bTool);

    void CopyMembers(NiGeometry* pkDest, NiCloningProcess& kCloning);
    void SetConsistency(NiGeometryData::Consistency eConsistency);
    void SetShader(BSShader* pkShader);
    uint16_t GetVertexCount();
    NiPoint3* GetVertices();
    NiBound& GetModelBound();
    NiPoint3* GetNormals();
    NiColorA* GetColors();
    NiAdditionalGeometryData* GetAdditionalGeometryData();
    // m_spModelData's object.
    NiGeometryData* GetModelData() const;
    // Sets the model data's bound to kBound.
    void SetModelBound(const NiBound& kBound);
    NiSkinInstance* GetSkinInstance() const;
    void SetSkinInstance(NiSkinInstance* pSkinInstance);
    BSShader* GetShader() const {
        return m_spShader;
    }
    uint16_t GetActiveVertexCount() const;
    void SetAdditionalGeometryData(NiAdditionalGeometryData* pkData);
    NiPoint2* GetTextures();

    NiPropertyState m_PropertyState;
    NiPointer<NiGeometryData> m_spModelData;
    NiPointer<NiSkinInstance> m_spSkinInstance;
    BSShader* m_spShader;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiGeometry);

inline NiGeometryData* NiGeometry::GetModelData() const {
    return m_spModelData;
}

inline NiPoint3* NiGeometry::GetVertices() {
    return this->m_spModelData->m_pkVertex;
}

inline void NiGeometry::SetModelBound(const NiBound& kBound) {
    m_spModelData->SetBound(kBound);
}

inline NiPoint3* NiGeometry::GetNormals() {
    return m_spModelData->GetNormals();
}

inline NiColorA* NiGeometry::GetColors() {
    return m_spModelData->GetColors();
}

inline NiPoint2* NiGeometry::GetTextures() {
    NiGeometryData* pModelData = m_spModelData;
    return pModelData->m_pkTexture;
}

inline void NiGeometry::SetShader(BSShader* pkShader) {
    this->m_spShader = pkShader;
}

inline void NiGeometry::SetConsistency(NiGeometryData::Consistency eConsistency) {
    NiGeometryData* pModelData = this->m_spModelData;
    pModelData->SetConsistency(eConsistency);
}

// The shared model-data getter's class (TESActorBaseData) has its field at another offset than m_usVertices with
// 8-byte pointers, so the field is read directly.
inline uint16_t NiGeometry::GetVertexCount() {
    return this->m_spModelData->m_usVertices;
}

// The m_kBound field is read directly from the model data (the exe's shared getter has another class's offset, which
// differs with 8-byte pointers).
inline NiBound& NiGeometry::GetModelBound() {
    return this->m_spModelData->m_kBound;
}

// The additional geometry data of the model data, null without model data.
inline NiAdditionalGeometryData* NiGeometry::GetAdditionalGeometryData() {
    if (!m_spModelData) {
        return nullptr;
    }
    return m_spModelData->GetAdditionalGeometryData();
}
