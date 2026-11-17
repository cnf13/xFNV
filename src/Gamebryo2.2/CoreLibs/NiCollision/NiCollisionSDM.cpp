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
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + SPHERE_BV] = &fnvd::callFunction<&NiSphereBV::SphereSphereFindIntersect>;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + BOX_BV] = nullptr;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + CAPSULE_BV] = nullptr;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnFindTable[SPHERE_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnFindTable[BOX_BV * MAXTYPE_BV + SPHERE_BV] = &fnvd::callFunction<&NiBoxBV::BoxSphereFindIntersect>;
        pfnFindTable[BOX_BV * MAXTYPE_BV + BOX_BV] = &fnvd::callFunction<&NiBoxBV::BoxBoxFindIntersect>;
        pfnFindTable[BOX_BV * MAXTYPE_BV + CAPSULE_BV] = &fnvd::callFunction<&NiBoxBV::BoxCapsuleFindIntersect>;
        pfnFindTable[BOX_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnFindTable[BOX_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnFindTable[BOX_BV * MAXTYPE_BV + HALFSPACE_BV] = nullptr;
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + SPHERE_BV] =
            &fnvd::callFunction<&NiCapsuleBV::CapsuleSphereFindIntersect>;
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + BOX_BV] = nullptr;
        pfnFindTable[CAPSULE_BV * MAXTYPE_BV + CAPSULE_BV] =
            &fnvd::callFunction<&NiCapsuleBV::CapsuleCapsuleFindIntersect>;
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
            &fnvd::callFunction<&NiHalfSpaceBV::HalfSpaceSphereFindIntersect>;
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + BOX_BV] =
            &fnvd::callFunction<&NiHalfSpaceBV::HalfSpaceBoxFindIntersect>;
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + CAPSULE_BV] =
            &fnvd::callFunction<&NiHalfSpaceBV::HalfSpaceCapsuleFindIntersect>;
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + LOZENGE_BV] = nullptr;
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + UNION_BV] = nullptr;
        pfnFindTable[HALFSPACE_BV * MAXTYPE_BV + HALFSPACE_BV] =
            &fnvd::callFunction<&NiHalfSpaceBV::HalfSpaceHalfSpaceFindIntersect>;

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
        pfnTriFindTable[SPHERE_BV] = &fnvd::callFunction<&NiSphereBV::SphereTriFindIntersect>;
        pfnTriFindTable[BOX_BV] = &fnvd::callFunction<&NiBoxBV::BoxTriFindIntersect>;
        pfnTriFindTable[CAPSULE_BV] = &fnvd::callFunction<&NiCapsuleBV::CapsuleTriFindIntersect>;
        pfnTriFindTable[LOZENGE_BV] = nullptr;
        pfnTriFindTable[UNION_BV] = &NiUnionBV::UnionTriFindIntersect;
        pfnTriFindTable[HALFSPACE_BV] = &fnvd::callFunction<&NiHalfSpaceBV::HalfSpaceTriFindIntersect>;

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
        NiCollisionSDM::ms_bInitialized = 0;
        NiStream::UnregisterLoader("NiCollisionData");
        NiBoundingVolume::ms_apfnLoaders[1] = 0;
        NiBoundingVolume::ms_apfnLoaders[2] = 0;
        NiBoundingVolume::ms_apfnLoaders[5] = 0;
        NiBoundingVolume::ms_apfnLoaders[3] = 0;
        NiBoundingVolume::ms_apfnLoaders[0] = 0;
        NiBoundingVolume::ms_apfnLoaders[4] = 0;
        _NiFree(NiBoundingVolume::ms_pfnTestFuncTable);
        _NiFree(NiBoundingVolume::ms_pfnFindFuncTable);
        _NiFree(NiBoundingVolume::ms_pfnTriTestFuncTable);
        _NiFree(NiBoundingVolume::ms_pfnTriFindFuncTable);
        NiBoundingVolume::ms_pfnTestFuncTable = 0;
        NiBoundingVolume::ms_pfnFindFuncTable = 0;
        NiBoundingVolume::ms_pfnTriTestFuncTable = 0;
        NiBoundingVolume::ms_pfnTriFindFuncTable = 0;
        NiBoundingVolume::ms_pfnConvertToSphereBV_Fast = 0;
        NiBoundingVolume::ms_pfnConvertToSphereBV_Safe = 0;
        NiBoundingVolume::ms_pfnConvertToBoxBV = 0;
    }
}
