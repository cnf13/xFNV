#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

#include <stdint.h>

class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;
class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiD3DRenderState;
class NiD3DShaderProgram;
class NiDX9Renderer;
class NiDynamicEffectState;
class NiExtraData;
class NiGeometry;
class NiGeometryBufferData;
class NiLines;
class NiNode;
class NiParticles;
class NiPropertyState;
class NiSkinInstance;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;

void FillDataArray(float* pfData, const _D3DMATRIX& kMatrix);

// The D3D constant map: its entries, and how each kind of entry (defined, constant, attribute, global, operator,
// object) is set on a shader program per draw. Repeats NiObject's run-time type virtuals.
class NiD3DShaderConstantMap : public NiShaderConstantMap {
public:
    using MappingIntArray = int32_t[4];
    // Scratch data SetObjectConstant fills from the entry's dynamic effect.
    using ObjectDataArray = float[16];
    // Operand scratch for operator entries (the Setup*ConstantValue members fill operand 0 or 1) and the results
    // the PerformOperator* members compute from them.
    using OperatorMatrixArray = D3DXMATRIX[2];
    using OperatorVectorArray = D3DXVECTOR4[2];

    NiD3DShaderConstantMap(NiDX9Renderer* apRenderer, NiGPUProgram::ProgramType aeType);
    ~NiD3DShaderConstantMap() override;

    uint32_t AddEntry(
        const char* pszKey,
        uint32_t uiFlags,
        uint32_t uiExtra,
        uint32_t uiShaderRegister,
        uint32_t uiRegisterCount,
        const char* pszVariableName,
        uint32_t uiDataSize,
        uint32_t uiDataStride,
        const void* pvDataSource,
        bool bCopyData
    ) override;
    uint32_t AddAttributeEntry(
        const char* pszKey,
        uint32_t uiFlags,
        uint32_t uiExtra,
        uint32_t uiShaderRegister,
        uint32_t uiRegisterCount,
        const char* pszVariableName,
        uint32_t uiDataSize,
        uint32_t uiDataStride,
        const void* pvDataSource,
        bool bCopyData
    ) override;
    uint32_t AddConstantEntry(
        const char* pszKey,
        uint32_t auiFlags,
        uint32_t auiExtra,
        uint32_t uiShaderRegister,
        uint32_t uiRegisterCount,
        const char* pszVariableName,
        uint32_t uiDataSize,
        uint32_t uiDataStride,
        const void* pvDataSource,
        bool bCopyData
    ) override;
    uint32_t AddGlobalEntry(
        const char* pszKey,
        uint32_t auiFlags,
        uint32_t uiExtra,
        uint32_t uiShaderRegister,
        uint32_t uiRegisterCount,
        const char* pszVariableName,
        uint32_t uiDataSize,
        uint32_t uiDataStride,
        const void* pvDataSource,
        bool bCopyData
    ) override;
    uint32_t AddOperatorEntry(
        const char* pszKey,
        uint32_t uiFlags,
        uint32_t uiExtra,
        uint32_t uiShaderRegister,
        uint32_t uiRegisterCount,
        const char* pszVariableName
    ) override;
    uint32_t AddObjectEntry(
        const char* pszKey,
        uint32_t uiShaderRegister,
        const char* pszVariableName,
        uint32_t uiObjectIndex,
        NiShaderAttributeDesc::ObjectType eObjectType
    ) override;
    void SortByEnabled() override;
    virtual const NiRTTI* GetRTTI() const {
        return &ms_RTTI;
    }
    // type queries: the object as the named class, or null
    virtual NiNode* IsNode() {
        return nullptr;
    }
    virtual BSFadeNode* IsFadeNode() {
        return nullptr;
    }
    virtual BSMultiBoundNode* IsMultiBoundNode() {
        return nullptr;
    }
    virtual NiGeometry* IsGeometry() {
        return nullptr;
    }
    virtual NiTriBasedGeom* IsTriBasedGeom() {
        return nullptr;
    }
    virtual NiTriStrips* IsTriStrips() {
        return nullptr;
    }
    virtual NiTriShape* IsTriShape() {
        return nullptr;
    }
    virtual BSSegmentedTriShape* IsSegmentedTriShape() {
        return nullptr;
    }
    virtual BSResizableTriShape* IsResizableTriShape() {
        return nullptr;
    }
    virtual NiParticles* IsParticlesGeom() {
        return nullptr;
    }
    virtual NiLines* IsLinesGeom() {
        return nullptr;
    }
    virtual bhkNiCollisionObject* IsBhkNiCollisionObject() {
        return nullptr;
    }
    virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject() {
        return nullptr;
    }
    virtual bhkRigidBody* IsBhkRigidBody() const {
        return nullptr;
    }
    virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() {
        return nullptr;
    }
    virtual uint32_t AddPredefinedEntry(
        const char* pszKey,
        uint32_t uiExtra,
        uint32_t uiShaderRegister,
        const char* pszVariableName,
        uint32_t uiDataSize,
        uint32_t uiDataStride,
        const void* pvDataSource,
        bool bCopyData
    );
    virtual uint32_t RemoveEntry(const char* pszKey);
    virtual NiShaderConstantMapEntry* GetEntry(const char* pszKey);
    virtual NiShaderConstantMapEntry* GetEntryAtIndex(uint32_t uiIndex) {
        return m_aspEntries.m_pBase[uiIndex];
    }
    virtual uint32_t SetShaderConstants(
        NiD3DShaderProgram* pkShaderProgram,
        NiGeometry* pkGeometry,
        const NiSkinInstance* pkSkin,
        const NiSkinPartition::Partition* pkPartition,
        NiGeometryBufferData* pkBuffData,
        const NiPropertyState* pkState,
        const NiDynamicEffectState* pkEffects,
        const NiTransform& kWorld,
        const NiBound& kWorldBound,
        uint32_t uiPass,
        bool bGlobal
    );
    virtual uint32_t GetEntryIndex(const char* pszKey);
    virtual uint32_t InsertEntry(NiShaderConstantMapEntry* pkEntry);
    virtual uint32_t InsertDefinedEntry(NiShaderConstantMapEntry* pkEntry);
    virtual uint32_t SetupPredefinedEntry(NiShaderConstantMapEntry* pkEntry);
    virtual uint32_t SetDefinedConstant(
        NiD3DShaderProgram* pkShaderProgram,
        NiShaderConstantMapEntry* pkEntry,
        NiGeometry* pkGeometry,
        const NiSkinInstance* pkSkin,
        const NiSkinPartition::Partition* pkPartition,
        NiGeometryBufferData* pkBuffData,
        const NiPropertyState* pkState,
        const NiDynamicEffectState* pkEffects,
        const NiTransform& kWorld,
        const NiBound& kWorldBound,
        uint32_t uiPass
    );
    virtual uint32_t
    SetConstantConstant(NiD3DShaderProgram* pkShaderProgram, NiShaderConstantMapEntry* pkEntry, uint32_t uiPass);
    virtual uint32_t SetAttributeConstant(
        NiD3DShaderProgram* pkShaderProgram,
        NiShaderConstantMapEntry* pkEntry,
        NiGeometry* pkGeometry,
        const NiSkinInstance* pkSkin,
        const NiSkinPartition::Partition* pkPartition,
        NiGeometryBufferData* pkBuffData,
        const NiPropertyState* pkState,
        const NiDynamicEffectState* pkEffects,
        const NiTransform& kWorld,
        const NiBound& kWorldBound,
        uint32_t uiPass,
        bool bGlobal,
        NiExtraData* pkExtraData
    );
    virtual uint32_t
    SetGlobalConstant(NiD3DShaderProgram* pkShaderProgram, NiShaderConstantMapEntry* pkEntry, uint32_t uiPass);
    virtual uint32_t SetOperatorConstant(
        NiD3DShaderProgram* pkShaderProgram,
        NiShaderConstantMapEntry* pkEntry,
        NiGeometry* pkGeometry,
        const NiSkinInstance* pkSkin,
        const NiPropertyState* pkState,
        const NiDynamicEffectState* pkEffects,
        const NiTransform& kWorld,
        const NiBound& kWorldBound,
        uint32_t uiPass
    );
    virtual uint32_t SetupObjectEntry(NiShaderConstantMapEntry* pkEntry);
    virtual uint32_t SetObjectConstant(
        NiD3DShaderProgram* pkShaderProgram,
        NiShaderConstantMapEntry* pkEntry,
        NiGeometry* pkGeometry,
        const NiSkinInstance* pkSkin,
        const NiSkinPartition::Partition* pkPartition,
        NiGeometryBufferData* pkBuffData,
        const NiPropertyState* pkState,
        const NiDynamicEffectState* pkEffects,
        const NiTransform& kWorld,
        const NiBound& kWorldBound,
        uint32_t uiPass
    );
    using NiShaderConstantMap::AddPredefinedEntry;

    void SortListByEnabled(NiTObjectArray<NiPointer<NiShaderConstantMapEntry>>* apEntries);
    void SetD3DRenderer(NiDX9Renderer* pkD3DRenderer);
    const void* MapConstantValue(NiShaderConstantMapEntry* pkEntry);
    const void* MapAttributeValue(NiShaderConstantMapEntry* pkEntry, NiExtraData* pkExtra);
    // Computes operand auiOperatorNum of an operator entry from a defined entry.
    uint32_t SetupDefinedConstantValue(
        uint32_t uiOperatorNum,
        NiShaderConstantMapEntry* pkEntry,
        NiGeometry* pkGeometry,
        const NiSkinInstance* pkSkin,
        const NiPropertyState* pkState,
        const NiDynamicEffectState* pkEffects,
        const NiTransform& kWorld,
        const NiBound& kWorldBound
    );
    uint32_t SetupConstantConstantValue(uint32_t uiOperatorNum, NiShaderConstantMapEntry* pkEntry, uint32_t uiPass);
    uint32_t SetupAttributeConstantValue(
        uint32_t uiOperatorNum,
        NiShaderConstantMapEntry* pkEntry,
        NiGeometry* pkGeometry,
        const NiSkinInstance* pkSkin,
        const NiPropertyState* pkState,
        const NiTransform& kWorld,
        const NiBound& kWorldBound,
        uint32_t uiPass
    );
    uint32_t PerformOperatorMultiply(
        NiD3DShaderProgram* pkShaderProgram,
        NiShaderConstantMapEntry* pkEntry,
        NiShaderAttributeDesc::AttributeType eType1,
        NiShaderAttributeDesc::AttributeType eType2,
        bool bInverse,
        bool bTranspose
    );
    // Sets the quotient of the two operands on the program.
    uint32_t PerformOperatorDivide(
        NiD3DShaderProgram* pkShaderProgram,
        NiShaderConstantMapEntry* pkEntry,
        NiShaderAttributeDesc::AttributeType eType1,
        NiShaderAttributeDesc::AttributeType eType2,
        bool bInverse,
        bool bTranspose
    );
    uint32_t PerformOperatorAdd(
        NiD3DShaderProgram* pkShaderProgram,
        NiShaderConstantMapEntry* pkEntry,
        NiShaderAttributeDesc::AttributeType eType1,
        NiShaderAttributeDesc::AttributeType eType2,
        bool bInverse,
        bool bTranspose
    );
    uint32_t PerformOperatorSubtract(
        NiD3DShaderProgram* pkShaderProgram,
        NiShaderConstantMapEntry* pkEntry,
        NiShaderAttributeDesc::AttributeType eType1,
        NiShaderAttributeDesc::AttributeType eType2,
        bool bInverse,
        bool bTranspose
    );

    static void SetupTextureTransformMatrix(
        D3DXMATRIX& kResult,
        const NiMatrix3* pkTexMatrix,
        bool bInv,
        bool bTrans,
        uint32_t uiTexCoordSize
    );
    static bool GetDynamicEffectData(
        void* pvData,
        uint32_t uiDataSize,
        NiShaderConstantMap::ObjectMappings eMapping,
        NiDynamicEffect* pkDynEffect,
        NiGeometry* pkGeometry,
        const NiSkinInstance* pkSkin,
        const NiSkinPartition::Partition* pkPartition,
        NiGeometryBufferData* pkBuffData,
        const NiPropertyState* pkState,
        const NiDynamicEffectState* pkEffects,
        const NiTransform& kWorld,
        const NiBound& kWorldBound,
        uint32_t uiPass
    );

    NiTObjectArray<NiPointer<NiShaderConstantMapEntry>> m_aspEntries;
    NiTObjectArray<NiPointer<NiShaderConstantMapEntry>> m_aspDefinedEntries;
    bool m_bModified;
    NiD3DShaderProgram* m_pkLastShaderProgram;
    uint32_t m_eLastError;
    IDirect3DDevice9* m_pkD3DDevice;
    NiDX9Renderer* m_pkD3DRenderer;
    NiD3DRenderState* m_pkD3DRenderState;

    static NiRTTI ms_RTTI;
    // Taylor series coefficients that SCM_DEF_CONSTS_TAYLOR_SIN/COS entries point at.
    static inline D3DXVECTOR4 ms_vTaylorSin{1.0f, -0.16161616f, 0.0083333f, -0.00019841f};
    static inline D3DXVECTOR4 ms_vTaylorCos{-0.5f, -0.041666664f, -0.0013888889f, 2.4801588e-05f};
    // Scratch values SetGlobalConstant widens a global's data into before handing it to the program.
    static inline D3DXVECTOR4 ms_vMappingValue{};
    static inline D3DXMATRIX ms_mMappingValue{};
    static inline int32_t ms_bMappingValue;
    static inline MappingIntArray ms_aiMappingValue{};
    static inline ObjectDataArray ms_afObjectData{};
    static inline OperatorMatrixArray ms_kD3DMat{};
    static inline OperatorVectorArray ms_kD3DVector4{};
    static inline D3DXMATRIX ms_kD3DMatResult{};
    static inline D3DXVECTOR4 ms_kD3DVector4Result{};
};

NiSmartPointer(NiD3DShaderConstantMap);
