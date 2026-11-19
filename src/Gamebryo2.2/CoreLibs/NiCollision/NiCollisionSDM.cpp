#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionSDM.h"

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"

// The first instance registers the library's Init and Shutdown.
NiCollisionSDM::NiCollisionSDM() {
    if (!siCounter++) {
        NiStaticDataManager::AddLibrary(NiCollisionSDM::Init, NiCollisionSDM::Shutdown);
    }
}

// Registers the collision data loader and the bounding volume loaders, and allocates and fills the type-pair
// dispatch tables (an entry per ordered pair of types, null where the pair is tested the other way round or not at
// all) and the conversions.
void NiCollisionSDM::Init() {
    using enum NiBoundingVolume::BoundType;
    if (!ms_bInitialized) {
        ms_bInitialized = true;
        NiStream::RegisterLoader("NiCollisionData", &NiCollisionData::CreateObject);
        NiBoundingVolume::ms_apfnLoaders[BOX_BV] = &NiBoxBV::CreateFromStream;
        NiBoundingVolume::ms_apfnLoaders[CAPSULE_BV] = &NiCapsuleBV::CreateFromStream;
        NiBoundingVolume::ms_apfnLoaders[HALFSPACE_BV] = &NiHalfSpaceBV::CreateFromStream;
        NiBoundingVolume::ms_apfnLoaders[LOZENGE_BV] = &NiSphereBV::ConvertLozengeToSphereBoundingVolume;
        NiBoundingVolume::ms_apfnLoaders[SPHERE_BV] = &NiSphereBV::CreateFromStream;
        NiBoundingVolume::ms_apfnLoaders[UNION_BV] = &NiUnionBV::CreateFromStream;

        NiBVTestIntersectFunction* pfnTestTable = static_cast<NiBVTestIntersectFunction*>(
            _NiMalloc(sizeof(NiBVTestIntersectFunction) * MAXTYPE_BV * MAXTYPE_BV)
        );
        pfnTestTable[SPHERE_BV * MAXTYPE_BV + SPHERE_BV] = &NiSphereBV::SphereSphereTestIntersect;
        pfnTestTable[SPHERE_BV * MAXTYPE_BV + BOX_BV] = nullptr;
        pfnTestTable[SPHERE_BV * MAXTYPE_BV + CAPSULE_BV] = nullptr;
        pfnTestTable[SPHERE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnTestTable[SPHERE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnTestTable[SPHERE_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnTestTable[BOX_BV * MAXTYPE_BV + SPHERE_BV] = &NiBoxBV::BoxSphereTestIntersect;
        pfnTestTable[BOX_BV * MAXTYPE_BV + BOX_BV] = &NiBoxBV::BoxBoxTestIntersect;
        pfnTestTable[BOX_BV * MAXTYPE_BV + CAPSULE_BV] = &NiBoxBV::BoxCapsuleTestIntersect;
        pfnTestTable[BOX_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnTestTable[BOX_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnTestTable[BOX_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnTestTable[CAPSULE_BV * MAXTYPE_BV + SPHERE_BV] = &NiCapsuleBV::CapsuleSphereTestIntersect;
        pfnTestTable[CAPSULE_BV * MAXTYPE_BV + BOX_BV] = nullptr;
        pfnTestTable[CAPSULE_BV * MAXTYPE_BV + CAPSULE_BV] = &NiCapsuleBV::CapsuleCapsuleTestIntersect;
        pfnTestTable[CAPSULE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnTestTable[CAPSULE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnTestTable[CAPSULE_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnTestTable[LOZENGE_BV * MAXTYPE_BV + SPHERE_BV] = nullptr;
        pfnTestTable[LOZENGE_BV * MAXTYPE_BV + BOX_BV] = nullptr;
        pfnTestTable[LOZENGE_BV * MAXTYPE_BV + CAPSULE_BV] = nullptr;
        pfnTestTable[LOZENGE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnTestTable[LOZENGE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnTestTable[LOZENGE_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnTestTable[UNION_BV * MAXTYPE_BV + SPHERE_BV] = &NiUnionBV::UnionOtherTestIntersect;
        pfnTestTable[UNION_BV * MAXTYPE_BV + BOX_BV] = &NiUnionBV::UnionOtherTestIntersect;
        pfnTestTable[UNION_BV * MAXTYPE_BV + CAPSULE_BV] = &NiUnionBV::UnionOtherTestIntersect;
        pfnTestTable[UNION_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnTestTable[UNION_BV * MAXTYPE_BV + UNION_BV] = &NiUnionBV::UnionUnionTestIntersect;
        pfnTestTable[UNION_BV * MAXTYPE_BV + HALFSPACE_BV] = &NiUnionBV::UnionOtherTestIntersect;
        pfnTestTable[HALFSPACE_BV * MAXTYPE_BV + SPHERE_BV] = &NiHalfSpaceBV::HalfSpaceSphereTestIntersect;
        pfnTestTable[HALFSPACE_BV * MAXTYPE_BV + BOX_BV] = &NiHalfSpaceBV::HalfSpaceBoxTestIntersect;
        pfnTestTable[HALFSPACE_BV * MAXTYPE_BV + CAPSULE_BV] = &NiHalfSpaceBV::HalfSpaceCapsuleTestIntersect;
        pfnTestTable[HALFSPACE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnTestTable[HALFSPACE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnTestTable[HALFSPACE_BV * MAXTYPE_BV + HALFSPACE_BV] = &NiHalfSpaceBV::HalfSpaceHalfSpaceTestIntersect;

        NiBVFindIntersectFunction* pfnFindTable = static_cast<NiBVFindIntersectFunction*>(
            _NiMalloc(sizeof(NiBVFindIntersectFunction) * MAXTYPE_BV * MAXTYPE_BV)
        );
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + SPHERE_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiSphereBV::SphereSphereFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + BOX_BV] = nullptr;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + CAPSULE_BV] = nullptr;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnFindTable[BOX_BV * MAXTYPE_BV + SPHERE_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiBoxBV::BoxSphereFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };
        pfnFindTable[BOX_BV * MAXTYPE_BV + BOX_BV] = [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
                                                        const NiBoundingVolume& kABV1, const NiPoint3& kV1,
                                                        float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals,
                                                        NiPoint3& kNormal0, NiPoint3& kNormal1) {
            return NiBoxBV::BoxBoxFindIntersect(
                fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
            );
        };
        pfnFindTable[BOX_BV * MAXTYPE_BV + CAPSULE_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiBoxBV::BoxCapsuleFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };
        pfnFindTable[BOX_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnFindTable[BOX_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnFindTable[BOX_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + SPHERE_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiCapsuleBV::CapsuleSphereFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + BOX_BV] = nullptr;
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + CAPSULE_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiCapsuleBV::CapsuleCapsuleFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnFindTable[LOZENGE_BV * MAXTYPE_BV + SPHERE_BV] = nullptr;
        pfnFindTable[LOZENGE_BV * MAXTYPE_BV + BOX_BV] = nullptr;
        pfnFindTable[LOZENGE_BV * MAXTYPE_BV + CAPSULE_BV] = nullptr;
        pfnFindTable[LOZENGE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnFindTable[LOZENGE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnFindTable[LOZENGE_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        // Unlike the test table, the find table has a union-lozenge entry.
        pfnFindTable[UNION_BV * MAXTYPE_BV + SPHERE_BV] = &NiUnionBV::UnionOtherFindIntersect;
        pfnFindTable[UNION_BV * MAXTYPE_BV + BOX_BV] = &NiUnionBV::UnionOtherFindIntersect;
        pfnFindTable[UNION_BV * MAXTYPE_BV + CAPSULE_BV] = &NiUnionBV::UnionOtherFindIntersect;
        pfnFindTable[UNION_BV * MAXTYPE_BV + LOZENGE_BV] = &NiUnionBV::UnionOtherFindIntersect;
        pfnFindTable[UNION_BV * MAXTYPE_BV + UNION_BV] = &NiUnionBV::UnionUnionFindIntersect;
        pfnFindTable[UNION_BV * MAXTYPE_BV + HALFSPACE_BV] = &NiUnionBV::UnionOtherFindIntersect;
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + SPHERE_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiHalfSpaceBV::HalfSpaceSphereFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + BOX_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiHalfSpaceBV::HalfSpaceBoxFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + CAPSULE_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiHalfSpaceBV::HalfSpaceCapsuleFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + HALFSPACE_BV] =
            [](float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0, const NiBoundingVolume& kABV1,
               const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0,
               NiPoint3& kNormal1) {
                return NiHalfSpaceBV::HalfSpaceHalfSpaceFindIntersect(
                    fTime, kABV0, kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
                );
            };

        NiBVTriTestIntersectFunction* pfnTriTestTable =
            static_cast<NiBVTriTestIntersectFunction*>(_NiMalloc(sizeof(NiBVTriTestIntersectFunction) * MAXTYPE_BV));
        pfnTriTestTable[SPHERE_BV] = &NiSphereBV::SphereTriTestIntersect;
        pfnTriTestTable[BOX_BV] = &NiBoxBV::BoxTriTestIntersect;
        pfnTriTestTable[CAPSULE_BV] = &NiCapsuleBV::CapsuleTriTestIntersect;
        pfnTriTestTable[LOZENGE_BV] = nullptr;
        pfnTriTestTable[UNION_BV] = &NiUnionBV::UnionTriTestIntersect;
        pfnTriTestTable[HALFSPACE_BV] = &NiHalfSpaceBV::HalfSpaceTriTestIntersect;

        NiBVTriFindIntersectFunction* pfnTriFindTable =
            static_cast<NiBVTriFindIntersectFunction*>(_NiMalloc(sizeof(NiBVTriFindIntersectFunction) * MAXTYPE_BV));
        pfnTriFindTable[SPHERE_BV] = [](float fTime, const NiBoundingVolume& kABV, const NiPoint3& kV0,
                                        const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2,
                                        const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals,
                                        NiPoint3& kNormal0, NiPoint3& kNormal1) {
            return NiSphereBV::SphereTriFindIntersect(
                fTime, kABV, kV0, kVert0, kVert1, kVert2, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
            );
        };
        pfnTriFindTable[BOX_BV] = [](float fTime, const NiBoundingVolume& kABV, const NiPoint3& kV0,
                                     const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2,
                                     const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals,
                                     NiPoint3& kNormal0, NiPoint3& kNormal1) {
            return NiBoxBV::BoxTriFindIntersect(
                fTime, kABV, kV0, kVert0, kVert1, kVert2, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
            );
        };
        pfnTriFindTable[CAPSULE_BV] = [](float fTime, const NiBoundingVolume& kABV, const NiPoint3& kV0,
                                         const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2,
                                         const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals,
                                         NiPoint3& kNormal0, NiPoint3& kNormal1) {
            return NiCapsuleBV::CapsuleTriFindIntersect(
                fTime, kABV, kV0, kVert0, kVert1, kVert2, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
            );
        };
        pfnTriFindTable[LOZENGE_BV] = nullptr;
        pfnTriFindTable[UNION_BV] = &NiUnionBV::UnionTriFindIntersect;
        pfnTriFindTable[HALFSPACE_BV] = [](float fTime, const NiBoundingVolume& kABV, const NiPoint3& kV0,
                                           const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2,
                                           const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt,
                                           int32_t bCalcNormals, NiPoint3& kNormal0, NiPoint3& kNormal1) {
            return NiHalfSpaceBV::HalfSpaceTriFindIntersect(
                fTime, kABV, kV0, kVert0, kVert1, kVert2, kV1, fIntrTime, kIntrPt, bCalcNormals != 0, kNormal0, kNormal1
            );
        };

        NiBoundingVolume::ms_pfnFindFuncTable = pfnFindTable;
        NiBoundingVolume::ms_pfnTestFuncTable = pfnTestTable;
        NiBoundingVolume::ms_pfnTriTestFuncTable = pfnTriTestTable;
        NiBoundingVolume::ms_pfnTriFindFuncTable = pfnTriFindTable;
        NiBoundingVolume::ms_pfnConvertToSphereBV_Fast = &NiCollisionConvert::ConvertToSphereBV_Fast;
        NiBoundingVolume::ms_pfnConvertToSphereBV_Safe = &NiCollisionConvert::ConvertToSphereBV_Safe;
        NiBoundingVolume::ms_pfnConvertToBoxBV = &NiCollisionConvert::ConvertToBoxBV;
    }
}

void NiCollisionSDM::Shutdown() {
    if (NiCollisionSDM::ms_bInitialized) {
        NiCollisionSDM::ms_bInitialized = false;
        NiStream::UnregisterLoader("NiCollisionData");
        NiBoundingVolume::ms_apfnLoaders[1] = nullptr;
        NiBoundingVolume::ms_apfnLoaders[2] = nullptr;
        NiBoundingVolume::ms_apfnLoaders[5] = nullptr;
        NiBoundingVolume::ms_apfnLoaders[3] = nullptr;
        NiBoundingVolume::ms_apfnLoaders[0] = nullptr;
        NiBoundingVolume::ms_apfnLoaders[4] = nullptr;
        _NiFree(NiBoundingVolume::ms_pfnTestFuncTable);
        _NiFree(NiBoundingVolume::ms_pfnFindFuncTable);
        _NiFree(NiBoundingVolume::ms_pfnTriTestFuncTable);
        _NiFree(NiBoundingVolume::ms_pfnTriFindFuncTable);
        NiBoundingVolume::ms_pfnTestFuncTable = nullptr;
        NiBoundingVolume::ms_pfnFindFuncTable = nullptr;
        NiBoundingVolume::ms_pfnTriTestFuncTable = nullptr;
        NiBoundingVolume::ms_pfnTriFindFuncTable = nullptr;
        NiBoundingVolume::ms_pfnConvertToSphereBV_Fast = nullptr;
        NiBoundingVolume::ms_pfnConvertToSphereBV_Safe = nullptr;
        NiBoundingVolume::ms_pfnConvertToBoxBV = nullptr;
    }
}
