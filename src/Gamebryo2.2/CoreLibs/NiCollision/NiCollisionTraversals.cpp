#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionTraversals.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/NiGeometryDataIteratorNiColorA.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionUtils.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSwitchNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"

#include <cmath>

bool NiCollisionTraversals::FindIntersections(
    const NiPoint3& kOrigin,
    const NiPoint3& kDir,
    NiPick& kPick,
    NiAVObject* pkRoot
) {
    NiPoint3 distance;
    if (!pkRoot || (kPick.m_bObserveAppCullFlag && (pkRoot->m_uFlags & NiAVObject::APP_CULLED) != 0)) {
        return false;
    }
    NiNode* pnode = pkRoot->IsNode();
    if (pnode && (pnode->m_uFlags & NiAVObject::IGNORES_PICKING) != 0) {
        return false;
    }
    if (NiCollisionUtils::FindBoundIntersect(pkRoot, kOrigin, kDir) ||
        (pnode && (pnode->m_uFlags & NiAVObject::FIXED_BOUND) != 0)) {
        if (pnode) {
            return NiCollisionTraversals::FindIntersectionsNode(kOrigin, kDir, kPick, static_cast<NiNode*>(pkRoot));
        }
        if (pkRoot->IsTriBasedGeom()) {
            return NiCollisionTraversals::FindIntersectionsTriBasedGeom(
                kOrigin,
                kDir,
                kPick,
                static_cast<NiTriBasedGeom*>(pkRoot)
            );
        }
        if (NiCollisionTraversals::sbPickLines && NiObject::IsKindOf_0(&NiLines::ms_RTTI, pkRoot)) {
            distance = pkRoot->m_kWorld.m_Translate - kOrigin;
            if (!(kDir.Dot(distance) < 0.0f)) {
                float pkRootb = distance.Length();
                kPick.m_pickResults.AddNew(pkRoot)->m_fDistance = pkRootb;
                return false;
            }
        }
        else {
            if (NiCollisionTraversals::sbPickParticles && NiObject::IsKindOf_0(&NiParticles::ms_RTTI, pkRoot)) {
                distance = pkRoot->m_kWorld.m_Translate - kOrigin;
                if (!(kDir.Dot(distance) < 0.0f)) {
                    NiPick::Record* v8 = kPick.m_pickResults.AddNew(pkRoot);
                    float pkRoota = distance.Length();
                    v8->m_fDistance = pkRoota;
                    if (kPick.m_eIntersectType == NiPick::TRIANGLE_INTERSECT) {
                        NiPoint3* v9 = &(distance = kDir * pkRoota);
                        v8->m_intersect = kOrigin + *v9;
                        return false;
                    }
                }
            }
        }
    }
    return false;
}

bool NiCollisionTraversals::FindIntersectionsSwitchNode(
    const NiPoint3& kOrigin,
    const NiPoint3& kDir,
    NiPick& kPick,
    NiSwitchNode* pkSwitch
) {
    int32_t iIndex = pkSwitch->m_iIndex;
    bool bFound = false;
    if (iIndex >= 0) {
        if (kPick.m_ePickType == NiPick::FIND_FIRST &&
            kPick.m_eSortType == NiPick::NO_SORT &&
            kPick.m_pickResults.m_uiValidResults) {
            return true;
        }
        if (iIndex < pkSwitch->m_kChildren.m_usSize) {
            NiAVObject* pkChild = pkSwitch->m_kChildren.m_pBase[iIndex].m_pObject;
            if (pkChild && FindIntersections(kOrigin, kDir, kPick, pkChild)) {
                return true;
            }
        }
    }
    return bFound;
}

bool NiCollisionTraversals::FindIntersectionsNode(
    const NiPoint3& kOrigin,
    const NiPoint3& kDir,
    NiPick& kPick,
    NiNode* pkNode
) {
    if (kPick.m_eIntersectType || kPick.m_ePickType != NiPick::FIND_FIRST) {
        if (pkNode && pkNode->IsKindOf(&NiSwitchNode::ms_RTTI)) {
            return NiCollisionTraversals::FindIntersectionsSwitchNode(
                kOrigin,
                kDir,
                kPick,
                static_cast<NiSwitchNode*>(pkNode)
            );
        }

        int32_t v7 = 0;
        bool bFound = false;
        if (pkNode->m_kChildren.m_usSize) {
            while (kPick.m_ePickType != NiPick::FIND_FIRST ||
                   (kPick.m_eSortType != NiPick::NO_SORT || !kPick.m_pickResults.m_uiValidResults)) {
                NiPointer<NiAVObject>* m_pBase = pkNode->m_kChildren.m_pBase;
                if (m_pBase[v7].m_pObject &&
                    NiCollisionTraversals::FindIntersections(kOrigin, kDir, kPick, m_pBase[v7].m_pObject)) {
                    bFound = true;
                }

                if (++v7 >= static_cast<uint32_t>(pkNode->m_kChildren.m_usSize)) {
                    return bFound;
                }
            }
            return true;
        }

        return bFound;
    }

    kPick.m_pickResults.AddNew(pkNode);
    return true;
}

// Gamebryo's general picking of a triangle-based geometry: a one-strip NiTriStrips and an NiTriShape take their fast
// paths; a skinned geometry, or a bound pick, records the hit at the world bound's center when the bound lies ahead of
// the ray. Otherwise the ray is taken into model space and tested against each active triangle (GetTriangleIndices);
// each hit adds a pick record with the intersection, its distance, the indices and the texture coordinates, normal and
// colour interpolated at the hit, as kPick asks. With no vertex array the vertices come from the packed vertex buffer,
// locked for reading; a geometry with neither records one hit at the distance of its world translation. Unlike the
// fast paths, no colour array leaves the record's colour as it was.
bool NiCollisionTraversals::FindIntersectionsTriBasedGeom(
    const NiPoint3& kOrigin,
    const NiPoint3& kDir,
    NiPick& kPick,
    NiTriBasedGeom* pkTBG
) {
    if (kPick.m_eIntersectType != NiPick::BOUND_INTERSECT && !pkTBG->m_spSkinInstance.m_pObject) {
        const NiRTTI* pkRTTI = pkTBG->GetRTTI();
        bool bStrips = false;
        while (pkRTTI) {
            if (pkRTTI == &NiTriStrips::ms_RTTI) {
                bStrips = true;
                break;
            }
            pkRTTI = pkRTTI->GetBaseRTTI();
        }
        if (bStrips) {
            if (static_cast<NiTriStripsData*>(pkTBG->m_spModelData.m_pObject)->GetStripCount() == 1) {
                return FindIntersectionsOneStripFastPath(kOrigin, kDir, kPick, static_cast<NiTriStrips*>(pkTBG));
            }
        }
        else if (NiObject::IsKindOf_0(&NiTriShape::ms_RTTI, pkTBG)) {
            return FindIntersectionsTriShapeFastPath(kOrigin, kDir, kPick, static_cast<NiTriShape*>(pkTBG));
        }

        const NiTransform& kWorld = pkTBG->m_kWorld;
        NiPoint3 kDiff;
        kDiff.x = kOrigin.x - kWorld.m_Translate.x;
        kDiff.y = kOrigin.y - kWorld.m_Translate.y;
        kDiff.z = kOrigin.z - kWorld.m_Translate.z;
        float fInvWorldScale = 1.0f / kWorld.m_fScale;
        NiPoint3 kRotated = kDiff * kWorld.m_Rotate;
        NiPoint3 kModelOrigin;
        kModelOrigin.x = kRotated.x * fInvWorldScale;
        kModelOrigin.y = kRotated.y * fInvWorldScale;
        kModelOrigin.z = kRotated.z * fInvWorldScale;
        kRotated = kDir * kWorld.m_Rotate;
        NiPoint3 kModelDir;
        kModelDir.x = kRotated.x * fInvWorldScale;
        kModelDir.y = kRotated.y * fInvWorldScale;
        kModelDir.z = kRotated.z * fInvWorldScale;

        bool bFound = false;
        uint16_t usTris = static_cast<NiTriBasedGeomData*>(pkTBG->m_spModelData.m_pObject)->GetActiveTriangleCount();
        NiGeometryData* pkData = pkTBG->m_spModelData.m_pObject;
        NiGeometryDataIteratorNiPoint3 kVertexIter;
        kVertexIter.m_pvCurrentPosition = nullptr;
        kVertexIter.m_iStride = 0;
        kVertexIter.m_bCompressed = false;
        bool bLocked = false;
        NiPoint3* pkVertex = pkData->m_pkVertex;
        if (!pkVertex) {
            NiAdditionalGeometryData* pkAGD = pkData->m_spAdditionalGeomData.m_pObject;
            if (pkAGD && pkAGD->GetVBReady()) {
                bLocked = pkData->LockPackedVertexData(true);
                pkData->GetVerticesIterator(kVertexIter);
            }
            if (!kVertexIter.m_pvCurrentPosition) {
                NiPick::Record* pkRecord = kPick.m_pickResults.AddNew(pkTBG);
                NiPoint3 kDelta = pkTBG->m_kWorld.m_Translate - kOrigin;
                pkRecord->m_fDistance = kDelta.Length();
                return true;
            }
        }

        for (uint16_t i = 0; i < usTris; i++) {
            uint16_t i0;
            uint16_t i1;
            uint16_t i2;
            static_cast<NiTriBasedGeomData*>(pkTBG->m_spModelData.m_pObject)->GetTriangleIndices(i, i0, i1, i2);

            NiPoint3 kV0;
            NiPoint3 kV1;
            NiPoint3 kV2;
            if (pkVertex) {
                kV0 = pkVertex[i0];
                kV1 = pkVertex[i1];
                kV2 = pkVertex[i2];
            }
            else {
                // Stride iterator indexing: element n lies n strides past the current position. The stream is raw
                // vertex memory (void*), read untyped.
                const char* pcVertices = static_cast<const char*>(kVertexIter.m_pvCurrentPosition);
                kV0 = *reinterpret_cast<const NiPoint3*>(pcVertices + i0 * kVertexIter.m_iStride);
                kV1 = *reinterpret_cast<const NiPoint3*>(pcVertices + i1 * kVertexIter.m_iStride);
                kV2 = *reinterpret_cast<const NiPoint3*>(pcVertices + i2 * kVertexIter.m_iStride);
            }

            NiPoint3 kIntersect;
            float fLineParam;
            float fTriParam1;
            float fTriParam2;
            // IntersectTriangle reads the third vertex through a float pointer.
            if (!NiCollisionUtils::IntersectTriangle(
                    &kModelOrigin,
                    &kModelDir,
                    &kV0,
                    &kV1,
                    &kV2.x,
                    kPick.m_bFrontOnly,
                    &kIntersect,
                    &fLineParam,
                    &fTriParam1,
                    &fTriParam2
                )) {
                continue;
            }

            bFound = true;
            NiPick::Record* pkRecord = kPick.m_pickResults.AddNew(pkTBG);
            if (kPick.m_eCoordinateType == NiPick::WORLD_COORDINATES) {
                NiPoint3 kRotatedIntersect = kWorld.m_Rotate * kIntersect;
                float fScale = kWorld.m_fScale;
                float fScaledX = fScale * kRotatedIntersect.x;
                float fScaledY = kRotatedIntersect.y * fScale;
                float fScaledZ = fScale * kRotatedIntersect.z;
                kIntersect.x = fScaledX + kWorld.m_Translate.x;
                kIntersect.y = kWorld.m_Translate.y + fScaledY;
                kIntersect.z = kWorld.m_Translate.z + fScaledZ;
            }
            pkRecord->m_intersect = kIntersect;
            pkRecord->m_fDistance = fLineParam;
            pkRecord->m_usTriangleIndex = i;
            pkRecord->m_usVertexIndex[0] = i0;
            pkRecord->m_usVertexIndex[1] = i1;
            pkRecord->m_usVertexIndex[2] = i2;
            // The barycentric weight of i0; fTriParam1 weighs i1 and fTriParam2 i2.
            float fTriParam0 = 1.0f - (fTriParam1 + fTriParam2);

            if (kPick.m_bReturnTexture) {
                NiGeometryDataIteratorNiPoint2 kTextureIter;
                kTextureIter.m_pvCurrentPosition = nullptr;
                kTextureIter.m_iStride = 0;
                kTextureIter.m_bCompressed = false;
                pkData->GetTextureSetIterator(0, kTextureIter);
                if (kTextureIter.m_pvCurrentPosition) {
                    // Stride iterator indexing over the raw texture coordinate stream (void*), read untyped.
                    const char* pcTextures = static_cast<const char*>(kTextureIter.m_pvCurrentPosition);
                    NiPoint2 kT2 = *reinterpret_cast<const NiPoint2*>(pcTextures + i2 * kTextureIter.m_iStride);
                    NiPoint2 kT1 = *reinterpret_cast<const NiPoint2*>(pcTextures + i1 * kTextureIter.m_iStride);
                    NiPoint2 kT0 = *reinterpret_cast<const NiPoint2*>(pcTextures + i0 * kTextureIter.m_iStride);
                    float fT2X = kT2.x * fTriParam2;
                    float fT2Y = fTriParam2 * kT2.y;
                    float fT1X = kT1.x * fTriParam1;
                    float fT1Y = fTriParam1 * kT1.y;
                    float fT0X = kT0.x * fTriParam0;
                    float fT0Y = fTriParam0 * kT0.y;
                    float fSumX = fT0X + fT1X;
                    float fSumY = fT0Y + fT1Y;
                    pkRecord->m_texture.x = fSumX + fT2X;
                    pkRecord->m_texture.y = fSumY + fT2Y;
                }
                else {
                    NiPoint2* pkTexture = pkTBG->m_spModelData->m_pkTexture;
                    if (pkTexture) {
                        NiPoint2 kScaled2;
                        NiPoint2 kScaled1;
                        NiPoint2 kScaled0;
                        NiPoint2* pkScaled2 = NiPoint2::Scale_(&kScaled2, fTriParam2, &pkTexture[i2]);
                        NiPoint2* pkScaled1 = NiPoint2::Scale_(&kScaled1, fTriParam1, &pkTexture[i1]);
                        NiPoint2* pkScaled0 = NiPoint2::Scale_(&kScaled0, fTriParam0, &pkTexture[i0]);
                        NiPoint2 kTexture = *pkScaled0 + *pkScaled1 + *pkScaled2;
                        pkRecord->m_texture = kTexture;
                    }
                    else {
                        pkRecord->m_texture = NiPoint2::ZERO;
                    }
                }
            }
            else {
                pkRecord->m_texture = NiPoint2::ZERO;
            }

            if (kPick.m_bReturnNormal) {
                NiPoint3 kNormal;
                NiGeometryDataIteratorNiPoint3 kNormalIter;
                kNormalIter.m_pvCurrentPosition = nullptr;
                kNormalIter.m_iStride = 0;
                kNormalIter.m_bCompressed = false;
                pkData->GetNormalsIterator(kNormalIter);
                NiPoint3* pkNormal;
                if (kNormalIter.m_pvCurrentPosition) {
                    // Stride iterator indexing over the raw normal stream (void*), read untyped.
                    const char* pcNormals = static_cast<const char*>(kNormalIter.m_pvCurrentPosition);
                    NiPoint3 kN0 = *reinterpret_cast<const NiPoint3*>(pcNormals + i0 * kNormalIter.m_iStride);
                    NiPoint3 kN1 = *reinterpret_cast<const NiPoint3*>(pcNormals + i1 * kNormalIter.m_iStride);
                    NiPoint3 kN2 = *reinterpret_cast<const NiPoint3*>(pcNormals + i2 * kNormalIter.m_iStride);
                    float fN2X = kN2.x * fTriParam2;
                    float fN2Y = kN2.y * fTriParam2;
                    float fN2Z = fTriParam2 * kN2.z;
                    float fN1X = kN1.x * fTriParam1;
                    float fN1Y = kN1.y * fTriParam1;
                    float fN1Z = fTriParam1 * kN1.z;
                    float fN0X = kN0.x * fTriParam0;
                    float fN0Y = kN0.y * fTriParam0;
                    float fN0Z = fTriParam0 * kN0.z;
                    float fSumX = fN0X + fN1X;
                    float fSumY = fN0Y + fN1Y;
                    float fSumZ = fN0Z + fN1Z;
                    kNormal.x = fSumX + fN2X;
                    kNormal.y = fSumY + fN2Y;
                    kNormal.z = fSumZ + fN2Z;
                }
                else if (kPick.m_bReturnSmoothNormal && (pkNormal = pkTBG->m_spModelData->m_pkNormal) != nullptr) {
                    NiPoint3 kScaled2 = fTriParam2 * pkNormal[i2];
                    NiPoint3 kScaled1 = fTriParam1 * pkNormal[i1];
                    kNormal = fTriParam0 * pkNormal[i0] + kScaled1 + kScaled2;
                }
                else {
                    NiPoint3 kEdge1;
                    kEdge1.x = kV1.x - kV0.x;
                    kEdge1.y = kV1.y - kV0.y;
                    kEdge1.z = kV1.z - kV0.z;
                    NiPoint3 kEdge2;
                    kEdge2.x = kV2.x - kV0.x;
                    kEdge2.y = kV2.y - kV0.y;
                    kEdge2.z = kV2.z - kV0.z;
                    kNormal = kEdge1.Cross(kEdge2);
                }
                kNormal.Unitize();
                if (kPick.m_eCoordinateType == NiPick::WORLD_COORDINATES) {
                    pkRecord->m_normal = kWorld.m_Rotate * kNormal;
                }
                else {
                    pkRecord->m_normal = kNormal;
                }
            }
            else {
                pkRecord->m_normal = NiPoint3::ZERO;
            }

            if (kPick.m_bReturnColor) {
                NiGeometryDataIteratorNiColorA kColorIter;
                kColorIter.m_pvCurrentPosition = nullptr;
                kColorIter.m_iStride = 0;
                kColorIter.m_bCompressed = false;
                pkData->GetColorsIterator(kColorIter);
                if (kColorIter.m_pvCurrentPosition) {
                    NiColorA kC0;
                    NiColorA kC1;
                    NiColorA kC2;
                    kColorIter.Get(i0, kC0);
                    kColorIter.Get(i1, kC1);
                    kColorIter.Get(i2, kC2);
                    pkRecord->m_color = fTriParam0 * kC0 + fTriParam1 * kC1 + fTriParam2 * kC2;
                }
                else {
                    NiColorA* pkColor = pkTBG->m_spModelData->m_pkColor;
                    if (pkColor) {
                        pkRecord->m_color = fTriParam0 * pkColor[i0] + fTriParam1 * pkColor[i1] +
                            fTriParam2 * pkColor[i2];
                    }
                }
            }
            else {
                pkRecord->m_color = NiColorA::WHITE;
            }

            if (kPick.m_ePickType == NiPick::FIND_FIRST && kPick.m_eSortType == NiPick::NO_SORT) {
                if (bLocked) {
                    pkData->UnlockPackedVertexData();
                }
                return true;
            }
        }
        if (bLocked) {
            pkData->UnlockPackedVertexData();
        }
        return bFound;
    }

    const NiBound* pkBound = pkTBG->m_kWorldBound;
    if (!pkBound) {
        pkBound = &NiAVObject::NullBoundS;
    }
    NiBound kBound = *pkBound;
    NiPoint3 kDiff;
    kDiff.x = kBound.m_kCenter.x - kOrigin.x;
    kDiff.y = kBound.m_kCenter.y - kOrigin.y;
    kDiff.z = kBound.m_kCenter.z - kOrigin.z;
    float fDot = kDir.y * kDiff.y + kDiff.x * kDir.x + kDir.z * kDiff.z;
    if (fDot < 0.0f) {
        return false;
    }
    NiPick::Record* pkRecord = kPick.m_pickResults.AddNew(pkTBG);
    float fSqrLength = kDiff.z * kDiff.z + (kDiff.y * kDiff.y + kDiff.x * kDiff.x);
    float fLength = std::sqrt(static_cast<double>(fSqrLength));
    pkRecord->m_fDistance = fLength;
    if (kPick.m_eIntersectType != NiPick::TRIANGLE_INTERSECT) {
        return true;
    }
    float fScaledX = fLength * kDir.x;
    float fScaledY = kDir.y * fLength;
    float fScaledZ = fLength * kDir.z;
    float fIntersectX = kOrigin.x + fScaledX;
    float fIntersectY = kOrigin.y + fScaledY;
    float fIntersectZ = fScaledZ + kOrigin.z;
    pkRecord->m_intersect.x = fIntersectX;
    pkRecord->m_intersect.y = fIntersectY;
    pkRecord->m_intersect.z = fIntersectZ;
    return true;
}

// Picks the triangles of a strip geometry (one strip list; Gamebryo's FindIntersectionsTriBasedGeom, read per
// strip triangle with every odd triangle's first two indices swapped). Without strip lists or vertices it records
// a hit at the geometry's origin and returns true.
// The asm differs from Gamebryo's source in the vertex source (the packed-vertex iterators when m_pkVertex is null,
// locked while picking), in the no-geometry case and in the texture, normal and colour iterators tried first.
bool NiCollisionTraversals::FindIntersectionsOneStripFastPath(
    const NiPoint3& kOrigin,
    const NiPoint3& kDir,
    NiPick& kPick,
    NiTriStrips* pkTBG
) {
    const NiPoint3* apOrigin = &kOrigin;
    const NiPoint3* apDir = &kDir;
    NiPick* apPick = &kPick;
    NiTriStrips* apGeom = pkTBG;
    const NiTransform& kWorld = apGeom->m_kWorld;
    NiPoint3 kDiff = *apOrigin - kWorld.m_Translate;
    float fInvWorldScale = 1.0f / kWorld.m_fScale;
    const NiMatrix3& kRotate = kWorld.m_Rotate;
    NiPoint3 kModelOrigin = (kDiff * kRotate) * fInvWorldScale;
    NiPoint3 kModelDir = (*apDir * kRotate) * fInvWorldScale;

    bool bFound = false;
    uint16_t usTris = static_cast<NiTriBasedGeomData*>(apGeom->m_spModelData.m_pObject)->GetActiveTriangleCount();
    NiTriStripsData* pkData = static_cast<NiTriStripsData*>(apGeom->m_spModelData.m_pObject);
    NiGeometryDataIteratorNiPoint3 kVertexIter;
    kVertexIter.m_pvCurrentPosition = nullptr;
    kVertexIter.m_iStride = 0;
    kVertexIter.m_bCompressed = false;
    bool bLocked = false;
    NiPoint3* pkVertex = pkData->m_pkVertex;
    if (!pkVertex) {
        NiAdditionalGeometryData* pkAGD = pkData->m_spAdditionalGeomData.m_pObject;
        if (pkAGD && pkAGD->GetVBReady()) {
            bLocked = pkData->LockPackedVertexData(true);
            pkData->GetVerticesIterator(kVertexIter);
        }
    }
    uint16_t* pusStrip = pkData->GetStripLists();
    if (!pusStrip || (!pkVertex && !kVertexIter.m_pvCurrentPosition)) {
        NiPick::Record* pkRecord = apPick->m_pickResults.AddNew(apGeom);
        NiPoint3 kDelta = kWorld.m_Translate - *apOrigin;
        pkRecord->m_fDistance = kDelta.Length();
        return true;
    }

    for (uint16_t i = 0; i < usTris; i++) {
        uint16_t i0;
        uint16_t i1;
        if (i & 1) {
            i0 = pusStrip[i + 1];
            i1 = pusStrip[i];
        }
        else {
            i0 = pusStrip[i];
            i1 = pusStrip[i + 1];
        }
        uint16_t i2 = pusStrip[i + 2];
        if (i0 == i1 || i1 == i2) {
            continue;
        }

        NiPoint3 kV0;
        NiPoint3 kV1;
        NiPoint3 kV2;
        if (pkVertex) {
            kV0 = pkVertex[i0];
            kV1 = pkVertex[i1];
            kV2 = pkVertex[i2];
        }
        else {
            // The iterators' streams are raw memory, elements m_iStride bytes apart, read inline as Gamebryo's
            // NiTConvertingStrideIterator does.
            uint8_t* pucVertices = static_cast<uint8_t*>(kVertexIter.m_pvCurrentPosition);
            kV0 = *reinterpret_cast<NiPoint3*>(pucVertices + i0 * kVertexIter.m_iStride);
            kV1 = *reinterpret_cast<NiPoint3*>(pucVertices + i1 * kVertexIter.m_iStride);
            kV2 = *reinterpret_cast<NiPoint3*>(pucVertices + i2 * kVertexIter.m_iStride);
        }

        NiPoint3 kIntersect;
        float fLineParam;
        float fTriParam1;
        float fTriParam2;
        // IntersectTriangle reads the third vertex through a float pointer.
        if (!NiCollisionUtils::IntersectTriangle(
                &kModelOrigin,
                &kModelDir,
                &kV0,
                &kV1,
                &kV2.x,
                apPick->m_bFrontOnly,
                &kIntersect,
                &fLineParam,
                &fTriParam1,
                &fTriParam2
            )) {
            continue;
        }

        bFound = true;
        NiPick::Record* pkRecord = apPick->m_pickResults.AddNew(apGeom);
        if (apPick->m_eCoordinateType == NiPick::WORLD_COORDINATES) {
            NiPoint3 kRotatedIntersect = kWorld.m_Rotate * kIntersect;
            kIntersect = kWorld.m_fScale * kRotatedIntersect + kWorld.m_Translate;
        }
        pkRecord->m_intersect = kIntersect;
        pkRecord->m_fDistance = fLineParam;
        pkRecord->m_usTriangleIndex = i;
        pkRecord->m_usVertexIndex[0] = i0;
        pkRecord->m_usVertexIndex[1] = i1;
        pkRecord->m_usVertexIndex[2] = i2;
        float fTriParam0 = 1.0f - (fTriParam1 + fTriParam2);

        if (apPick->m_bReturnTexture) {
            NiGeometryDataIteratorNiPoint2 kTextureIter;
            kTextureIter.m_pvCurrentPosition = nullptr;
            kTextureIter.m_iStride = 0;
            kTextureIter.m_bCompressed = false;
            pkData->GetTextureSetIterator(0, kTextureIter);
            if (kTextureIter.m_pvCurrentPosition) {
                // Raw stream memory, as for the vertices above.
                uint8_t* pucTextures = static_cast<uint8_t*>(kTextureIter.m_pvCurrentPosition);
                NiPoint2 kT0 = *reinterpret_cast<NiPoint2*>(pucTextures + i0 * kTextureIter.m_iStride);
                NiPoint2 kT1 = *reinterpret_cast<NiPoint2*>(pucTextures + i1 * kTextureIter.m_iStride);
                NiPoint2 kT2 = *reinterpret_cast<NiPoint2*>(pucTextures + i2 * kTextureIter.m_iStride);
                float fT2X = kT2.x * fTriParam2;
                float fT2Y = fTriParam2 * kT2.y;
                float fT1X = kT1.x * fTriParam1;
                float fT1Y = fTriParam1 * kT1.y;
                float fT0X = kT0.x * fTriParam0;
                float fT0Y = fTriParam0 * kT0.y;
                float fSumX = fT0X + fT1X;
                float fSumY = fT0Y + fT1Y;
                pkRecord->m_texture.x = fSumX + fT2X;
                pkRecord->m_texture.y = fSumY + fT2Y;
            }
            else {
                NiPoint2* pkTexture = apGeom->m_spModelData->m_pkTexture;
                if (pkTexture) {
                    NiPoint2 kScaled2;
                    NiPoint2 kScaled1;
                    NiPoint2 kScaled0;
                    NiPoint2* pkScaled2 = NiPoint2::Scale_(&kScaled2, fTriParam2, &pkTexture[i2]);
                    NiPoint2* pkScaled1 = NiPoint2::Scale_(&kScaled1, fTriParam1, &pkTexture[i1]);
                    NiPoint2* pkScaled0 = NiPoint2::Scale_(&kScaled0, fTriParam0, &pkTexture[i0]);
                    NiPoint2 kTexture = *pkScaled0 + *pkScaled1 + *pkScaled2;
                    pkRecord->m_texture = kTexture;
                }
                else {
                    pkRecord->m_texture = NiPoint2::ZERO;
                }
            }
        }
        else {
            pkRecord->m_texture = NiPoint2::ZERO;
        }

        if (apPick->m_bReturnNormal) {
            NiPoint3 kNormal;
            NiGeometryDataIteratorNiPoint3 kNormalIter;
            kNormalIter.m_pvCurrentPosition = nullptr;
            kNormalIter.m_iStride = 0;
            kNormalIter.m_bCompressed = false;
            pkData->GetNormalsIterator(kNormalIter);
            if (kNormalIter.m_pvCurrentPosition) {
                // Raw stream memory, as for the vertices above.
                uint8_t* pucNormals = static_cast<uint8_t*>(kNormalIter.m_pvCurrentPosition);
                NiPoint3 kN0 = *reinterpret_cast<NiPoint3*>(pucNormals + i0 * kNormalIter.m_iStride);
                NiPoint3 kN1 = *reinterpret_cast<NiPoint3*>(pucNormals + i1 * kNormalIter.m_iStride);
                NiPoint3 kN2 = *reinterpret_cast<NiPoint3*>(pucNormals + i2 * kNormalIter.m_iStride);
                float fN2X = kN2.x * fTriParam2;
                float fN2Y = kN2.y * fTriParam2;
                float fN2Z = fTriParam2 * kN2.z;
                float fN1X = kN1.x * fTriParam1;
                float fN1Y = kN1.y * fTriParam1;
                float fN1Z = fTriParam1 * kN1.z;
                float fN0X = kN0.x * fTriParam0;
                float fN0Y = kN0.y * fTriParam0;
                float fN0Z = fTriParam0 * kN0.z;
                float fSumX = fN0X + fN1X;
                float fSumY = fN0Y + fN1Y;
                float fSumZ = fN0Z + fN1Z;
                kNormal.x = fSumX + fN2X;
                kNormal.y = fSumY + fN2Y;
                kNormal.z = fSumZ + fN2Z;
            }
            else {
                NiPoint3* pkNormal;
                if (apPick->m_bReturnSmoothNormal && (pkNormal = apGeom->m_spModelData->m_pkNormal)) {
                    NiPoint3 kScaled2 = fTriParam2 * pkNormal[i2];
                    NiPoint3 kScaled1 = fTriParam1 * pkNormal[i1];
                    kNormal = fTriParam0 * pkNormal[i0] + kScaled1 + kScaled2;
                }
                else {
                    NiPoint3 kEdge1 = kV1 - kV0;
                    NiPoint3 kEdge2 = kV2 - kV0;
                    kNormal = kEdge1.Cross(kEdge2);
                }
            }
            kNormal.Unitize();
            if (apPick->m_eCoordinateType == NiPick::WORLD_COORDINATES) {
                pkRecord->m_normal = kWorld.m_Rotate * kNormal;
            }
            else {
                pkRecord->m_normal = kNormal;
            }
        }
        else {
            pkRecord->m_normal = NiPoint3::ZERO;
        }

        if (apPick->m_bReturnColor) {
            NiGeometryDataIteratorNiColorA kColorIter;
            kColorIter.m_pvCurrentPosition = nullptr;
            kColorIter.m_iStride = 0;
            kColorIter.m_bCompressed = false;
            pkData->GetColorsIterator(kColorIter);
            if (kColorIter.m_pvCurrentPosition) {
                NiColorA kC0;
                NiColorA kC1;
                NiColorA kC2;
                kColorIter.Get(i0, kC0);
                kColorIter.Get(i1, kC1);
                kColorIter.Get(i2, kC2);
                pkRecord->m_color = fTriParam0 * kC0 + fTriParam1 * kC1 + fTriParam2 * kC2;
            }
            else {
                NiColorA* pkColor = apGeom->m_spModelData->m_pkColor;
                if (pkColor) {
                    pkRecord->m_color = fTriParam0 * pkColor[i0] + fTriParam1 * pkColor[i1] + fTriParam2 * pkColor[i2];
                }
                else {
                    pkRecord->m_color = NiColorA::WHITE;
                }
            }
        }
        else {
            pkRecord->m_color = NiColorA::WHITE;
        }

        if (apPick->m_ePickType == NiPick::FIND_FIRST && apPick->m_eSortType == NiPick::NO_SORT) {
            if (bLocked) {
                pkData->UnlockPackedVertexData();
            }
            return true;
        }
    }

    if (bLocked) {
        pkData->UnlockPackedVertexData();
    }
    return bFound;
}

// Bethesda's triangle-list form of Gamebryo's FindIntersectionsTriBasedGeom: the ray is taken into the shape's model
// space and tested against each active triangle; each hit adds a pick record with the intersection, its distance, the
// indices and the texture coordinates, normal and colour interpolated at the hit, as kPick asks. With no vertex array
// the vertices come from the packed vertex buffer, locked for reading. A shape with no triangle list or no vertices
// adds one record at the distance of its world translation and counts as a hit. Each product and sum of the NiPoint3,
// NiPoint2, NiColorA and matrix arithmetic is rounded to a float.
bool NiCollisionTraversals::FindIntersectionsTriShapeFastPath(
    const NiPoint3& kOrigin,
    const NiPoint3& kDir,
    NiPick& kPick,
    NiTriShape* pkTBG
) {
    const NiTransform& kWorld = pkTBG->m_kWorld;
    const NiMatrix3& kRotate = kWorld.m_Rotate;
    NiPoint3 kDiff;
    kDiff.x = kOrigin.x - kWorld.m_Translate.x;
    kDiff.y = kOrigin.y - kWorld.m_Translate.y;
    kDiff.z = kOrigin.z - kWorld.m_Translate.z;
    float fInvWorldScale = 1.0f / kWorld.m_fScale;
    // kDiff * kRotate, then the scale: the model-space ray origin.
    NiPoint3 kRotated;
    kRotated.x = kRotate.m_pEntry[0][0] * kDiff.x + kDiff.y * kRotate.m_pEntry[1][0] + kDiff.z * kRotate.m_pEntry[2][0];
    kRotated.y = kDiff.x * kRotate.m_pEntry[0][1] + kDiff.y * kRotate.m_pEntry[1][1] + kDiff.z * kRotate.m_pEntry[2][1];
    kRotated.z = kDiff.x * kRotate.m_pEntry[0][2] + kDiff.y * kRotate.m_pEntry[1][2] + kDiff.z * kRotate.m_pEntry[2][2];
    NiPoint3 kModelOrigin;
    kModelOrigin.x = kRotated.x * fInvWorldScale;
    kModelOrigin.y = kRotated.y * fInvWorldScale;
    kModelOrigin.z = kRotated.z * fInvWorldScale;
    // kDir * kRotate, then the scale: the model-space ray direction.
    kRotated.x = kDir.y * kRotate.m_pEntry[1][0] + kRotate.m_pEntry[0][0] * kDir.x + kRotate.m_pEntry[2][0] * kDir.z;
    kRotated.y = kDir.x * kRotate.m_pEntry[0][1] + kDir.y * kRotate.m_pEntry[1][1] + kRotate.m_pEntry[2][1] * kDir.z;
    kRotated.z = kRotate.m_pEntry[1][2] * kDir.y + kRotate.m_pEntry[0][2] * kDir.x + kRotate.m_pEntry[2][2] * kDir.z;
    NiPoint3 kModelDir;
    kModelDir.x = kRotated.x * fInvWorldScale;
    kModelDir.y = kRotated.y * fInvWorldScale;
    kModelDir.z = kRotated.z * fInvWorldScale;

    bool bFound = false;
    uint16_t usTris = static_cast<NiTriBasedGeomData*>(pkTBG->m_spModelData.m_pObject)->GetActiveTriangleCount();
    NiTriShapeData* pkData = static_cast<NiTriShapeData*>(pkTBG->m_spModelData.m_pObject);
    NiGeometryDataIteratorNiPoint3 kVertexIter;
    NiPoint3* pkVertex = pkData->m_pkVertex;
    bool bLocked = false;
    if (!pkVertex) {
        NiAdditionalGeometryData* pkAddData = pkData->m_spAdditionalGeomData.m_pObject;
        if (pkAddData && pkAddData->GetVBReady()) {
            bLocked = pkData->LockPackedVertexData(true);
            pkData->GetVerticesIterator(kVertexIter);
        }
    }
    uint16_t* pusTriList = pkData->m_pusTriList;
    if (!pusTriList || (!pkVertex && !kVertexIter.m_pvCurrentPosition)) {
        NiPick::Record* pkRecord = kPick.m_pickResults.AddNew(pkTBG);
        float fDiffX = pkTBG->m_kWorld.m_Translate.x - kOrigin.x;
        float fDiffY = pkTBG->m_kWorld.m_Translate.y - kOrigin.y;
        float fDiffZ = pkTBG->m_kWorld.m_Translate.z - kOrigin.z;
        float fLengthSq = fDiffY * fDiffY + fDiffX * fDiffX + fDiffZ * fDiffZ;
        float fLength = std::sqrt(static_cast<double>(fLengthSq));
        pkRecord->m_fDistance = fLength;
        return true;
    }

    for (uint16_t i = 0; i < usTris; i++) {
        uint16_t i0 = pusTriList[3 * i];
        uint16_t i1 = pusTriList[3 * i + 1];
        uint16_t i2 = pusTriList[3 * i + 2];
        NiPoint3 kV0;
        NiPoint3 kV1;
        NiPoint3 kV2;
        // Tests bLocked, not pkVertex, to pick the source of the vertices.
        if (bLocked) {
            // Stride iterator indexing: element n lies n strides past the current position. The stream is raw vertex
            // memory (void*), read untyped.
            const char* pcVertices = static_cast<const char*>(kVertexIter.m_pvCurrentPosition);
            kV0 = *reinterpret_cast<const NiPoint3*>(pcVertices + i0 * kVertexIter.m_iStride);
            kV1 = *reinterpret_cast<const NiPoint3*>(pcVertices + i1 * kVertexIter.m_iStride);
            kV2 = *reinterpret_cast<const NiPoint3*>(pcVertices + i2 * kVertexIter.m_iStride);
        }
        else {
            kV0 = pkVertex[i0];
            kV1 = pkVertex[i1];
            kV2 = pkVertex[i2];
        }
        NiPoint3 kIntersect;
        float fLineParam;
        float fTriParam1;
        float fTriParam2;
        if (!NiCollisionUtils::IntersectTriangle(
                &kModelOrigin,
                &kModelDir,
                &kV0,
                &kV1,
                &kV2.x,
                kPick.m_bFrontOnly,
                &kIntersect,
                &fLineParam,
                &fTriParam1,
                &fTriParam2
            )) {
            continue;
        }
        bFound = true;
        NiPick::Record* pkRecord = kPick.m_pickResults.AddNew(pkTBG);
        if (kPick.m_eCoordinateType == NiPick::WORLD_COORDINATES) {
            // kWorld * kIntersect.
            float fRotatedX = kRotate.m_pEntry[0][0] * kIntersect.x + kIntersect.y * kRotate.m_pEntry[0][1] +
                kIntersect.z * kRotate.m_pEntry[0][2];
            float fRotatedY = kIntersect.x * kRotate.m_pEntry[1][0] + kIntersect.y * kRotate.m_pEntry[1][1] +
                kIntersect.z * kRotate.m_pEntry[1][2];
            float fRotatedZ = kIntersect.x * kRotate.m_pEntry[2][0] + kIntersect.y * kRotate.m_pEntry[2][1] +
                kIntersect.z * kRotate.m_pEntry[2][2];
            float fScale = kWorld.m_fScale;
            float fScaledX = fRotatedX * fScale;
            float fScaledY = fRotatedY * fScale;
            float fScaledZ = fScale * fRotatedZ;
            kIntersect.x = kWorld.m_Translate.x + fScaledX;
            kIntersect.y = kWorld.m_Translate.y + fScaledY;
            kIntersect.z = kWorld.m_Translate.z + fScaledZ;
        }
        pkRecord->m_intersect = kIntersect;
        pkRecord->m_fDistance = fLineParam;
        pkRecord->m_usTriangleIndex = i;
        pkRecord->m_usVertexIndex[0] = i0;
        pkRecord->m_usVertexIndex[1] = i1;
        pkRecord->m_usVertexIndex[2] = i2;
        // The barycentric weight of i0; fTriParam1 weighs i1 and fTriParam2 i2.
        float fTriParam0 = 1.0f - (fTriParam1 + fTriParam2);

        if (kPick.m_bReturnTexture) {
            NiGeometryDataIteratorNiPoint2 kTextureIter;
            pkData->GetTextureSetIterator(0, kTextureIter);
            NiPoint2* pkTexture;
            if (kTextureIter.m_pvCurrentPosition) {
                // Stride iterator indexing over the raw texture coordinate stream (void*), read untyped.
                const char* pcTextures = static_cast<const char*>(kTextureIter.m_pvCurrentPosition);
                NiPoint2 kT2 = *reinterpret_cast<const NiPoint2*>(pcTextures + i2 * kTextureIter.m_iStride);
                NiPoint2 kT1 = *reinterpret_cast<const NiPoint2*>(pcTextures + i1 * kTextureIter.m_iStride);
                NiPoint2 kT0 = *reinterpret_cast<const NiPoint2*>(pcTextures + i0 * kTextureIter.m_iStride);
                float fT2X = kT2.x * fTriParam2;
                float fT2Y = fTriParam2 * kT2.y;
                float fT1X = kT1.x * fTriParam1;
                float fT1Y = fTriParam1 * kT1.y;
                float fT0X = kT0.x * fTriParam0;
                float fT0Y = fTriParam0 * kT0.y;
                float fSumX = fT0X + fT1X;
                float fSumY = fT0Y + fT1Y;
                pkRecord->m_texture.x = fSumX + fT2X;
                pkRecord->m_texture.y = fSumY + fT2Y;
            }
            else if ((pkTexture = pkTBG->m_spModelData->m_pkTexture) != nullptr) {
                float fT2X = pkTexture[i2].x * fTriParam2;
                float fT2Y = fTriParam2 * pkTexture[i2].y;
                float fT1X = pkTexture[i1].x * fTriParam1;
                float fT1Y = fTriParam1 * pkTexture[i1].y;
                float fT0X = pkTexture[i0].x * fTriParam0;
                float fT0Y = fTriParam0 * pkTexture[i0].y;
                float fSumX = fT0X + fT1X;
                float fSumY = fT0Y + fT1Y;
                pkRecord->m_texture.x = fSumX + fT2X;
                pkRecord->m_texture.y = fSumY + fT2Y;
            }
            else {
                pkRecord->m_texture = NiPoint2::ZERO;
            }
        }
        else {
            pkRecord->m_texture = NiPoint2::ZERO;
        }

        if (kPick.m_bReturnNormal) {
            NiPoint3 kNormal;
            NiGeometryDataIteratorNiPoint3 kNormalIter;
            pkData->GetNormalsIterator(kNormalIter);
            NiPoint3* pkNormals;
            if (kNormalIter.m_pvCurrentPosition) {
                // Stride iterator indexing over the raw normal stream (void*), read untyped.
                const char* pcNormals = static_cast<const char*>(kNormalIter.m_pvCurrentPosition);
                NiPoint3 kN0 = *reinterpret_cast<const NiPoint3*>(pcNormals + i0 * kNormalIter.m_iStride);
                NiPoint3 kN1 = *reinterpret_cast<const NiPoint3*>(pcNormals + i1 * kNormalIter.m_iStride);
                NiPoint3 kN2 = *reinterpret_cast<const NiPoint3*>(pcNormals + i2 * kNormalIter.m_iStride);
                float fN2X = kN2.x * fTriParam2;
                float fN2Y = kN2.y * fTriParam2;
                float fN2Z = fTriParam2 * kN2.z;
                float fN1X = kN1.x * fTriParam1;
                float fN1Y = kN1.y * fTriParam1;
                float fN1Z = fTriParam1 * kN1.z;
                float fN0X = kN0.x * fTriParam0;
                float fN0Y = kN0.y * fTriParam0;
                float fN0Z = fTriParam0 * kN0.z;
                float fSumX = fN0X + fN1X;
                float fSumY = fN0Y + fN1Y;
                float fSumZ = fN0Z + fN1Z;
                kNormal.x = fSumX + fN2X;
                kNormal.y = fSumY + fN2Y;
                kNormal.z = fSumZ + fN2Z;
            }
            else if (kPick.m_bReturnSmoothNormal && (pkNormals = pkTBG->m_spModelData->m_pkNormal) != nullptr) {
                float fN2X = pkNormals[i2].x * fTriParam2;
                float fN2Y = pkNormals[i2].y * fTriParam2;
                float fN2Z = fTriParam2 * pkNormals[i2].z;
                float fN1X = pkNormals[i1].x * fTriParam1;
                float fN1Y = pkNormals[i1].y * fTriParam1;
                float fN1Z = fTriParam1 * pkNormals[i1].z;
                float fN0X = pkNormals[i0].x * fTriParam0;
                float fN0Y = pkNormals[i0].y * fTriParam0;
                float fN0Z = fTriParam0 * pkNormals[i0].z;
                float fSumX = fN0X + fN1X;
                float fSumY = fN0Y + fN1Y;
                float fSumZ = fN0Z + fN1Z;
                kNormal.x = fSumX + fN2X;
                kNormal.y = fSumY + fN2Y;
                kNormal.z = fSumZ + fN2Z;
            }
            else {
                // The face normal: (kV1 - kV0).Cross(kV2 - kV0).
                float fE1X = kV1.x - kV0.x;
                float fE1Y = kV1.y - kV0.y;
                float fE1Z = kV1.z - kV0.z;
                float fE2X = kV2.x - kV0.x;
                float fE2Y = kV2.y - kV0.y;
                float fE2Z = kV2.z - kV0.z;
                kNormal.x = fE1Y * fE2Z - fE2Y * fE1Z;
                kNormal.y = fE1Z * fE2X - fE2Z * fE1X;
                kNormal.z = fE1X * fE2Y - fE2X * fE1Y;
            }
            kNormal.Unitize();
            if (kPick.m_eCoordinateType == NiPick::WORLD_COORDINATES) {
                pkRecord->m_normal = kRotate * kNormal;
            }
            else {
                pkRecord->m_normal = kNormal;
            }
        }
        else {
            pkRecord->m_normal = NiPoint3::ZERO;
        }

        if (kPick.m_bReturnColor) {
            NiGeometryDataIteratorNiColorA kColorIter;
            pkData->GetColorsIterator(kColorIter);
            if (kColorIter.m_pvCurrentPosition) {
                NiColorA kC0;
                NiColorA kC1;
                NiColorA kC2;
                kColorIter.Get(i0, kC0);
                kColorIter.Get(i1, kC1);
                kColorIter.Get(i2, kC2);
                float fC2R = kC2.r * fTriParam2;
                float fC2G = kC2.g * fTriParam2;
                float fC2B = kC2.b * fTriParam2;
                float fC2A = fTriParam2 * kC2.a;
                float fC1R = kC1.r * fTriParam1;
                float fC1G = kC1.g * fTriParam1;
                float fC1B = kC1.b * fTriParam1;
                float fC1A = fTriParam1 * kC1.a;
                float fC0R = kC0.r * fTriParam0;
                float fC0G = kC0.g * fTriParam0;
                float fC0B = kC0.b * fTriParam0;
                float fC0A = fTriParam0 * kC0.a;
                float fSumR = fC0R + fC1R;
                float fSumG = fC0G + fC1G;
                float fSumB = fC0B + fC1B;
                float fSumA = fC0A + fC1A;
                pkRecord->m_color.r = fSumR + fC2R;
                pkRecord->m_color.g = fSumG + fC2G;
                pkRecord->m_color.b = fSumB + fC2B;
                pkRecord->m_color.a = fSumA + fC2A;
            }
            else {
                NiColorA* pkColors = pkTBG->m_spModelData->m_pkColor;
                if (pkColors) {
                    // The three colours scaled by their weights, summed as (c0 + c1) + c2.
                    pkRecord->m_color = fTriParam0 * pkColors[i0] + fTriParam1 * pkColors[i1] +
                        fTriParam2 * pkColors[i2];
                }
                else {
                    pkRecord->m_color = NiColorA::WHITE;
                }
            }
        }
        else {
            pkRecord->m_color = NiColorA::WHITE;
        }

        if (kPick.m_ePickType == NiPick::FIND_FIRST && kPick.m_eSortType == NiPick::NO_SORT) {
            if (bLocked) {
                pkData->UnlockPackedVertexData();
            }
            return true;
        }
    }
    if (bLocked) {
        pkData->UnlockPackedVertexData();
    }
    return bFound;
}

NiPick::Record* NiPick::Results::AddNew(NiAVObject* pObject) {
    uint32_t m_uiValidResults = this->m_uiValidResults;
    if (m_uiValidResults >= this->m_usSize) {
        NiPick::Record* v6 = static_cast<NiPick::Record*>(MemoryManager::Instance().Allocate(sizeof(NiPick::Record)));
        NiPick::Record* v7 = v6;
        if (v6) {
            v6->m_spObject.m_pObject = nullptr;
            v6->m_spProxyParent.m_pObject = nullptr;
            v6->m_color.r = 0.0f;
            v6->m_color.g = 0.0f;
            v6->m_color.b = 0.0f;
            v6->m_color.a = 0.0f;
            v6->Initialize(pObject);
        }
        else {
            v7 = nullptr;
        }
        uint32_t m_usMaxSize = this->m_usMaxSize;
        uint32_t m_usSize = this->m_usSize;
        NiPick::Record* v11 = v7;
        if (m_usSize >= m_usMaxSize) {
            SetSize(m_usSize + m_usGrowBy);
        }
        SetAt(m_usSize, v11);
        ++this->m_uiValidResults;
        return v7;
    }

    NiPick::Record* v4 = m_pBase[m_uiValidResults];
    v4->Initialize(pObject);
    ++this->m_uiValidResults;
    return v4;
}
