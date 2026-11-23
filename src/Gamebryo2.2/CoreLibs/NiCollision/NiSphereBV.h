#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphere.h"

class NiStream;

// a sphere bounding volume
class NiSphereBV : public NiBoundingVolume {
public:
    NiSphereBV(float fRadius, const NiPoint3& kCenter);
    NiSphereBV(const NiSphereBV& kABV);

    // No destructor of its own; it uses the NiBoundingVolume one.
    void LoadBinary(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    // shared constant stub
    int32_t Type() const override {
        return SPHERE_BV;
    }
    void UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) override;
    NiBoundingVolume* Create() const override;
    NiBoundingVolume* Clone() const override;
    void Copy(const NiBoundingVolume& kABV) override;
    bool operator==(const NiBoundingVolume& kABV) const override;
    // one body shared by every bounding volume: the negated operator==
    bool operator!=(const NiBoundingVolume& kABV) const override;
    void AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const override;

    // The loader of ms_apfnLoaders[SPHERE_BV]: a new unit sphere at the origin (0x14 bytes)
    // that LoadBinary fills from arStream. Hides NiBoundingVolume's, as in the engine.
    static NiBoundingVolume* CreateFromStream(NiStream& kStream);
    static bool SphereTriTestIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiPoint3& kVert0,
        const NiPoint3& kVert1,
        const NiPoint3& kVert2,
        const NiPoint3& kV1
    );
    static bool SphereTriFindIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiPoint3& kVert0,
        const NiPoint3& kVert1,
        const NiPoint3& kVert2,
        const NiPoint3& kV1,
        float& fIntrTime,
        NiPoint3& kIntrPt,
        bool bCalcNormals,
        NiPoint3& kNormal0,
        NiPoint3& kNormal1
    );
    static bool SphereSphereTestIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiBoundingVolume& kABV1,
        const NiPoint3& kV1
    );
    static bool SphereSphereFindIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiBoundingVolume& kABV1,
        const NiPoint3& kV1,
        float& fIntrTime,
        NiPoint3& kIntrPt,
        bool bCalcNormals,
        NiPoint3& kNormal0,
        NiPoint3& kNormal1
    );
    // Reads a lozenge's centre, axes and extents and returns a sphere with its centre and the last float as radius.
    static NiBoundingVolume* ConvertLozengeToSphereBoundingVolume(NiStream& kStream);

    NiSphere m_kSphere;
};
