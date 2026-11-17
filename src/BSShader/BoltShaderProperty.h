#pragma once

#include "BSShader/BSShaderProperty.h"
#include "BSShader/BoltShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BoltShaderProperty : public BSShaderProperty {
public:
    BoltShaderProperty();
    ~BoltShaderProperty() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bool SetupGeometry(NiGeometry* apGeometry) override;
    BSShaderProperty::RenderPassArray* GetRenderPasses(
        NiGeometry* apGeom,
        int32_t aiEnabledPasses,
        uint16_t& asCount,
        uint32_t auiRenderMode,
        BSShaderAccumulator* apAccumulator,
        int32_t abUpdateList
    ) override;

    // Sets up segment aiSegment of bolt aiBolt from its previous segment aiLastSegment and
    // the segment aiLookBackSegment further back.
    void CreateSegment(int32_t aiBolt, int32_t aiSegment, int32_t aiLastSegment, int32_t aiLookBackSegment);
    // Adds aiNewSegments segments at the end of every bolt, dropping the oldest when the ring is full.
    void ExtendSegments(int32_t aiNewSegments);
    // Drops the newest aiRemoveSegments (modulo the ring size) segments from the end of every bolt.
    void RemoveSegments(int32_t aiRemoveSegments);
    // Drops the oldest aiRemoveSegments (modulo the ring size) segments from the start of every bolt.
    void CleanupSegments(int32_t aiRemoveSegments);
    // Advances every segment by afTime.
    void MoveAllSegments(float afMoveDelta);
    // Drops the segments the bolt's tail passed in afTimeDelta and moves fDistanceShort by afTimeDelta * afSpeed.
    void CleanupBolt(float afTimeDelta, float afSpeed);
    // Extends the bolts for afTimeDelta up to afDesiredBoltDistance.
    void ExtendBolt(float afTimeDelta, float afDesiredBoltDistance);
    // Whether the bolt is gone: not bWrapped and iBoltStartSegment caught up with iBoltEndSegment, compared signed.
    // Called from MagicBoltProjectile's Update and UpdateBranch.
    bool IsFinished();
    // Whether the bolt has reached its full length; always false on PC.
    bool IsAtMaxLength();
    // Cleans the bolt up at afSpeed when abCleanup is set, else extends it towards afDesiredBoltDistance.
    void UpdateBolt(float afTimeDelta, float afDesiredBoltDistance, float afSpeed, bool abCleanup);
    void SetStartFromPoint(bool abStartFromPoint);
    void SetMoving(bool abMoving);
    void SetAbsorbNumBolts(int32_t a2);
    void SetAbsorbCoreColor(float a2, float a3, float a4, float a5);
    void SetAbsorbGlowColor(float a2, float a3, float a4, float a5);
    void SetAbsorbSegmentLength(float a2);
    void SetAbsorbTortuosityVariance(float a2);
    // Stores fBoltsRadius.
    void SetBoltsRadius(float afBoltsRadius);
    // Stores fBoltsRadiusStrength.
    void SetBoltsRadiusStrength(float afStrength);
    // Stores fBoltSmallWidth.
    void SetBoltSmallWidth(float afBoltSmallWidth);
    // Stores fBoltGrowWidth.
    void SetBoltGrowWidth(float afBoltGrowWidth);
    // Stores
    // fSegmentVariance.
    void SetSegmentVariance(float afSegmentVariance);
    void UpdateAndMoveBolt(float afTimeDelta, float afDesiredBoltDistance, float afSpeed, bool abCleanup);
    void InitSegmentData();
    void CopyMembers(BoltShaderProperty* pDest, NiCloningProcess& arCloneData);

    BoltShader::SegmentData* pSegmentData;
    NiPoint3 TargetLocation;
    float fTimer;
    uint32_t iLookBack;
    int32_t iBoltStartSegment;
    int32_t iBoltEndSegment;
    uint32_t iTotalSegmentCount;
    float fDistanceLong;
    // 40 floats, used up to iNumBolts; the constructor fills all 40.
    float fTortuosity[40];
    uint32_t iNumBolts;
    float fBoltLength;
    float fBoltsRadius;
    float fBoltsRadiusStrength;
    float fBoltSmallWidth;
    float fBoltGrowWidth;
    int32_t iSegmentsPerBolt;
    float fSegmentLength;
    float fSegmentVariance;
    uint32_t iSubSegments;
    float fSubSegmentVariance;
    NiColorA CoreColor;
    NiColorA GlowColor;
    uint8_t bWrapped;
    uint8_t bMoving;
    uint8_t bGuided;
    uint8_t bStartFromPoint;
    NiGeometry* pMyGeometry;
    // The bolt's middle segment. No function of the class reads or writes it, and the constructor initialises every
    // neighbouring member but not this one; it is declared because the layout needs a four-byte member before
    // fDistanceShort.
    int32_t iBoltMidSegment;
    float fDistanceShort;
    uint32_t iBoltCurrentSegmentCount;
    uint32_t iTotalAddedSegments;
    float fAlpha;

    static NiRTTI ms_RTTI;
    // The live property count; the shader's shared resources go with the last one.
    static inline int32_t iRefCount;
};
