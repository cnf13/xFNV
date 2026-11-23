#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

// A triangle-list mesh split into segments the game switches on and off (dismemberment, damage levels).
class BSSegmentedTriShape : public NiTriShape {
public:
    struct Segment {
        // in index units: the DrawIndexedPrimitive start index
        uint32_t iStartIndex;
        uint32_t iNumPrimitives;
        bool bEnabled;
        // the count the shape draw uses after the enabled segments are merged
        uint32_t iNumPrimitivesMerged;
        bool bEnabledPostMerge;
    };

    BSSegmentedTriShape();
    ~BSSegmentedTriShape() override;

    const NiRTTI* GetRTTI() const override;
    BSSegmentedTriShape* IsSegmentedTriShape() override {
        return this;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void RenderImmediate(NiRenderer* pkRenderer) override;
    void OnlyRenderImmediate(NiRenderer* pkRenderer) override;

    // Recomputes the merged primitive counts of the segments when they changed.
    void MergeEnabledSegments();
    uint32_t GetNumSegments();
    bool GetSegmentEmpty(uint32_t aiSegment);
    void EnableSegment(uint32_t aiSegment);
    void DisableSegment(uint32_t aiSegment);
    void EnableAllSegments();
    // BSResizableTriShape's members, then the segment table.
    void CopyMembers(BSSegmentedTriShape* pkDest, NiCloningProcess& kCloning);

    static void SetSystemDisabled(bool abDisable);
    static NiObject* CreateObject();

    Segment* pSegments;
    uint32_t iNumSegments;
    bool bSegmentsChanged;
    bool bIgnoreSegments;

    static NiRTTI ms_RTTI;
    static inline bool bSystemDisabled;
};

NiSmartPointer(BSSegmentedTriShape);
