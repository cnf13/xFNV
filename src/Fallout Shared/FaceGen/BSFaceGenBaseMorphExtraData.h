#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NiGeometry;

// The "FOD" extra data of a FaceGen mesh: a copy of the mesh's base vertices (plus bonus vertices after them) that the
// morphs are applied to.
class BSFaceGenBaseMorphExtraData : public NiExtraData {
public:
    // Copies the mesh's vertices out and appends apAdditionalVertices.
    BSFaceGenBaseMorphExtraData(
        NiGeometry* apMesh,
        NiPoint3* apAdditionalVertices,
        uint32_t auiAdditionalVerticesCount
    );
    ~BSFaceGenBaseMorphExtraData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }

    NiPoint3* GetVertices() const;
    // iVertsModelCount when abMeshOnly, else iVertsCount.
    uint32_t GetVertexCount(bool abMeshOnly) const;

    // The tag the extra data is named by.
    static const NiFixedString& Tag();
    // Creates the tag "FOD".
    static void InitSDM();
    static void KillSDM();

    NiPoint3* pVerts;
    uint32_t iVertsCount;
    uint32_t iVertsModelCount;

    static inline NiFixedString* pTag;
    static NiRTTI ms_RTTI;
};
