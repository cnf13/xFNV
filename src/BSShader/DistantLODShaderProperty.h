#pragma once

#include "BSMain/Setting.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

#include <stdint.h>

class DistantLODGroup;
class NiStencilProperty;
class NiTriBasedGeomData;

void ClearDistantLODMap();

// The property that selects the distant LOD shader for the distant landscape.
class DistantLODShaderProperty : public BSShaderLightingProperty {
public:
    // What a distant LOD object is built from.
    class DistantLODParam {
    public:
        // The constructor clears the three members.
        DistantLODParam();

        const char* pcGeomFilename;
        // The model's root node (GetCachedGeometry stores IsNode of a loaded stream's top object); a NiBillboardNode
        // only for tree billboards, which is tested with a DynamicCast.
        NiNode* p3DData;
        uint32_t iBaseObjectID;
    };

    // The shared geometry, texture and properties of one distant LOD model.
    class CachedGeometry;
    // One distant LOD object's cache entry: its groups, its geometry and the cells that use it.
    class CachedDistantLOD;
    // The types of GetCachedGeometryMap's function-local static pmCachedGeometryMap and of
    // DistantLODShaderProperty::GetDistantLODMap's pmDistantLODMap: each base object's form ID to its cached geometry,
    // and to its CachedDistantLOD (GetDistantLODGroup stores them).
    using pmCachedGeometryMap_t = NiTPointerMap<uint32_t, NiPointer<DistantLODShaderProperty::CachedGeometry>>;
    using pmDistantLODMap_t = NiTPointerMap<uint32_t, DistantLODShaderProperty::CachedDistantLOD*>;

    DistantLODShaderProperty();
    ~DistantLODShaderProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    NiTexture* GetBaseTexture() override {
        return spBaseTexture;
    }
    virtual D3DTEXTUREADDRESS GetClampD3D() const {
        return D3DTADDRESS_WRAP;
    }
    virtual void SetClampD3D(D3DTEXTUREADDRESS aClampMode);
    virtual NiTexturingProperty::ClampMode GetClampNI() const {
        return TextureClampMode;
    }
    virtual void SetClampNI(NiTexturingProperty::ClampMode aClampMode) {
        TextureClampMode = aClampMode;
    }

    // Copies this property into pDest while cloning.
    void CopyMembers(DistantLODShaderProperty* pDest, NiCloningProcess& arCloneData);

    static void SetLODTreeParent(NiNode* apLODTreeParent);
    // The cached instancing geometry of the distant LOD object apDistantLODParam names, built on first use; null
    // without shaders, without a form ID or without a node to build it from.
    static CachedGeometry* GetCachedGeometry(DistantLODParam* apDistantLODParam);
    // The distant LOD geometry cache pmCachedGeometryMap: a map from a base object's form ID to its CachedGeometry,
    // built on first use (a guarded static destroyed at exit).
    static NiTPointerMap<uint32_t, NiPointer<DistantLODShaderProperty::CachedGeometry>>* GetCachedGeometryMap();
    static bool SetDistantLODCulled(bool abDistLODStatus);
    static bool HasCachedGeometry(uint32_t aiFormID);
    static uint16_t GetDistantLODGroup(
        DistantLODShaderProperty::DistantLODParam* apDistantLODParam,
        uint32_t aiCellChunk,
        DistantLODGroup** apDistantLODGroup,
        NiNode* apDistantLODNode
    );
    static void ClearStaticMaps();
    static void AddDistantLOD(
        uint32_t aiCellChunk,
        uint32_t aiCellKey,
        NiNode* apDistantLODNode,
        DistantLODShaderProperty::DistantLODParam* apDistantLODParam,
        NiPoint3* apPositions,
        float* afColors,
        uint16_t aiNumPositions
    );
    // The distant LOD map, built on first use.
    static NiTPointerMap<uint32_t, DistantLODShaderProperty::CachedDistantLOD*>* GetDistantLODMap();

    DistantLODGroup* pDistantLODGroup;
    NiPointer<NiTexture> spBaseTexture;
    NiTexturingProperty::ClampMode TextureClampMode;
    DistantLODParam* pDistantLODParam;

    // The node the distant LOD draws against: SetLODTreeParent stores a NiNode, and GetDistantLODGroup attaches groups
    // to it through NiNode::AttachChild.
    static inline NiNode* pLODTreeParent;
    static NiRTTI ms_RTTI;
    static inline int32_t uiLODCount;
    static inline int32_t uiLODGeometryCount;
};

NiSmartPointer(DistantLODShaderProperty);

// The destructor releases the five smart pointers in reverse order.
class DistantLODShaderProperty::CachedGeometry : public NiRefObject {
public:
    CachedGeometry();
    ~CachedGeometry() override;

    NiPointer<NiTriBasedGeomData> spTriGeomData;
    bool bTristrip;
    uint8_t padding0D[3];
    float* pInstanceIndex;
    int32_t iVertices;
    NiPointer<NiTexture> spBaseTexture;
    NiPointer<NiAlphaProperty> spAlphaBlend;
    NiPointer<NiAlphaProperty> spAlphaNoBlend;
    NiPointer<NiStencilProperty> spStencil;
    float fBoundRadius;
    int32_t iMaxInstanceCount;
    bool bBillboard;
    uint8_t padding31;
    uint16_t usInstanceTriCount;
};

// The value DistantLODShaderProperty::GetDistantLODMap's map holds per base object: GetDistantLODGroup allocates one,
// zeroes it and fills it in; ClearDistantLODMap destroys each and frees it.
class DistantLODShaderProperty::CachedDistantLOD {
public:
    ~CachedDistantLOD();

    DistantLODParam kDistantLODParam;
    NiTPointerList<DistantLODGroup*> pGroupList;
    NiPointer<CachedGeometry> spCachedGeometry;
    NiTList<uint32_t> pCellKeyList;
};

// The game settings DistantLODShaderProperty.cpp defines.
extern SettingT<INISettingCollection> iDistantLODGroupWidth;
inline int32_t apMap_guard;
