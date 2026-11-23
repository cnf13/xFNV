#include "Fallout Shared/GlobalFunc.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Xenon/XControls.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkMouseSpringAction.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPCollisionObject.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkUnaryAction.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSDecalPlacementVectorExtraData.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShader.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProcessorSpecificCode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace bhkConvert {} // namespace bhkConvert

// The source mesh FixNormals last ran on, whose vertices a second extrusion from the same source leaves where the first
// moved them.
static NiGeometry* plastSource;

// Four [HAVOK] INI settings of the PC build that no code reads.
static SettingT<INISettingCollection> gs_fSD_HAVOK("fSD:HAVOK", 0.98f);
static SettingT<INISettingCollection> gs_fSE_HAVOK("fSE:HAVOK", 0.3f);
static SettingT<INISettingCollection> gs_fOD_HAVOK("fOD:HAVOK", 0.9f);
static SettingT<INISettingCollection> gs_fRF_HAVOK("fRF:HAVOK", 1000.0f);

// The tile code's map from a vertex index to its distance.
using NiTMapBaseVertexDist = NiTMapBase<DFALL<NiTMapItem<uint32_t, VertexDist>>, uint32_t, VertexDist>;

double dMin(double adA, float afB) {
    if (afB > adA) {
        return adA;
    }
    return afB;
}

float GetZAngleFromVector(const NiPoint3& aDirection) {
    if (aDirection.y != 0.0) {
        float fRatio = aDirection.x / aDirection.y;
        float fAngle = std::atanf(fRatio);
        if (aDirection.y < 0.0) {
            fAngle = static_cast<float>(fAngle + 3.1415927410125732);
        }
        return ClampAngle(fAngle);
    }
    if (aDirection.x > 0.0) {
        return ClampAngle(1.5707963705062866f);
    }
    return ClampAngle(4.71238899230957f);
}

// The larger of two ints: aiB unless aiA is above it. The compare is signed.
int32_t BSMAX(int32_t aiA, int32_t aiB) {
    if (aiA <= aiB) {
        return aiB;
    }

    return aiA;
}

int32_t BSAbs(int32_t alValue) {
    return std::abs(alValue);
}

const char* BSstristr(const char* apString1, const char* apString2) {
    int32_t iLen1 = std::strlen(apString1);
    int32_t iLen2 = std::strlen(apString2);
    for (int32_t i = 0; i < iLen1 - iLen2 + 1; ++i) {
        int32_t j;
        for (j = 0; j < iLen2; ++j) {
            int32_t iChar = std::tolower(apString1[j + i]);
            if (iChar != std::tolower(apString2[j])) {
                break;
            }
        }
        if (j == iLen2) {
            return &apString1[i];
        }
    }
    return nullptr;
}

// Wraps afAngle into [0, 2pi) with fmod; a negative angle gets 2pi (a double constant) added first, rounded to a float
// local. The fmod result is stored over afAngle and returned from there.
float ClampAngle(float afAngle) {
    if (afAngle < 0.0) {
        float fAngle = static_cast<float>(afAngle + 6.2831854820251465);
        afAngle = std::fmod(fAngle, 6.2831855f);
    }
    else if (afAngle >= 6.2831854820251465) {
        afAngle = std::fmod(afAngle, 6.2831855f);
    }
    return afAngle;
}

// Evaluates (value - y1) / (y2 - y1), times (x2 - x1), plus x1, then rounds the result to float.
float FLerp(float x1, float x2, float y1, float y2, float value) {
    float fResult = (value - y1) / (y2 - y1) * (x2 - x1) + x1;
    return fResult;
}

float BSRoundToNearest(float afVal, float afNearest) {
    float fQuotient = afVal / afNearest;
    return static_cast<float>(
        static_cast<double>(
            !(fQuotient - static_cast<double>(static_cast<int32_t>(fQuotient)) < 0.5) + static_cast<int32_t>(fQuotient)
        ) *
        afNearest
    );
}

float RandomFloat0To1() {
    return BSRandom::GetSingleton()->Float0To1();
}

float RandomFloatTwoPi() {
    return BSRandom::GetSingleton()->FloatTwoPi();
}

int32_t RandomInt(int32_t aiMin, int32_t aiMax) {
    return static_cast<int32_t>(
        BSRandom::GetSingleton()->UnsignedInt(static_cast<uint32_t>(aiMin), static_cast<uint32_t>(aiMax))
    );
}

void BSClamp(float& aVal, float aMin, float aMax) {
    if (aMax < static_cast<double>(aVal)) {
        aVal = aMax;
    }
    else if (aMin > static_cast<double>(aVal)) {
        aVal = aMin;
    }
}

float GetAngleDelta(float afStartAngle, float afTargetAngle, float& afOutDir) {
    float fDelta = afTargetAngle - afStartAngle;
    afOutDir = 0.0f;
    if (fDelta == 0.0) {
        return fDelta;
    }
    afOutDir = 1.0f;
    if (fDelta < 0.0) {
        if (fDelta > -3.1415927410125732) {
            afOutDir = -1.0f;
        }
        else {
            return static_cast<float>(fDelta + 6.2831854820251465);
        }
    }
    else if (fDelta > 3.1415927410125732) {
        afOutDir = -1.0f;
        return static_cast<float>(6.2831854820251465 - fDelta);
    }
    return fDelta;
}

NiLines* MakeLine(
    const NiPoint3& aStart,
    const NiColorA& aStartColor,
    const NiPoint3& aEnd,
    const NiColorA& aEndColor,
    bool abWithNoLightingProp
) {
    NiPoint3* pVerts = new NiPoint3[2];
    pVerts[0] = aStart;
    pVerts[1] = aEnd;
    NiColorA* pColors = new NiColorA[2];
    pColors[0] = aStartColor;
    pColors[1] = aEndColor;
    uint8_t* pBools = static_cast<uint8_t*>(ourMalloc(2u));
    pBools[0] = 1;
    pBools[1] = 0;
    NiLines* pLines = new NiLines(2, pVerts, pColors, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, pBools);
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pShProp = new BSShaderNoLightingProperty;
        pLines->AttachProperty(pShProp);
        BSShaderManager::PrepareObject(pLines, 0, 0);
    }
    return pLines;
}

NiTriShape* MakeTriPoint(float afDrawSize, const NiColorA& aColor, bool abWithNoLightingProp) {
    NiPoint3* pVerts = new NiPoint3[6];
    float fTop = afDrawSize * 1.4142135381698608;
    float fNegSize = -afDrawSize;
    float fBottom = -afDrawSize * 1.4142135381698608;
    pVerts[0] = NiPoint3(0.0f, 0.0f, fTop);
    pVerts[1] = NiPoint3(-afDrawSize, afDrawSize, 0.0f);
    pVerts[2] = NiPoint3(afDrawSize, afDrawSize, 0.0f);
    pVerts[3] = NiPoint3(afDrawSize, -afDrawSize, 0.0f);
    pVerts[4] = NiPoint3(fNegSize, fNegSize, 0.0f);
    pVerts[5] = NiPoint3(0.0f, 0.0f, fBottom);
    NiColorA* pColors = new NiColorA[6];
    pColors[0] = aColor;
    pColors[1] = aColor;
    pColors[2] = aColor;
    pColors[3] = aColor;
    pColors[4] = aColor;
    pColors[5] = aColor;
    uint16_t* pTriList = static_cast<uint16_t*>(_NiMalloc(24 * sizeof(uint16_t)));
    pTriList[0] = 0;
    pTriList[1] = 2;
    pTriList[2] = 1;
    pTriList[3] = 0;
    pTriList[4] = 3;
    pTriList[5] = 2;
    pTriList[6] = 0;
    pTriList[7] = 4;
    pTriList[8] = 3;
    pTriList[9] = 0;
    pTriList[10] = 1;
    pTriList[11] = 4;
    pTriList[12] = 1;
    pTriList[13] = 2;
    pTriList[14] = 5;
    pTriList[15] = 2;
    pTriList[16] = 3;
    pTriList[17] = 5;
    pTriList[18] = 3;
    pTriList[19] = 4;
    pTriList[20] = 5;
    pTriList[21] = 4;
    pTriList[22] = 1;
    pTriList[23] = 5;
    NiTriShape*
        pTri = new NiTriShape(6, pVerts, nullptr, pColors, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 8, pTriList);
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pShProp = new BSShaderNoLightingProperty;
        pTri->AttachProperty(pShProp);
        BSShaderManager::PrepareObject(pTri, 0, 0);
    }
    return pTri;
}

NiPoint3* CreateWorldVertices(NiGeometry* apGeom) {
    if (!apGeom || !apGeom->GetVertexCount()) {
        return nullptr;
    }
    uint16_t usVertexCount = apGeom->GetVertexCount();
    NiPoint3* pReturn = new NiPoint3[usVertexCount];
    const NiTransform* pWorldTransform = &apGeom->GetWorldTransform();
    NiPoint3* pVertices = apGeom->GetVertices();
    NiProcessorSpecificCode::TransformPoints(apGeom->GetVertexCount(), &pVertices->x, &pReturn->x, pWorldTransform);
    return pReturn;
}

// Each destination vertex finds the nearest source vertex in world space within afTolerance (any distance when
// afTolerance is negative or NaN); the map keeps per source vertex the nearest destination vertex, and a second one at
// the same distance. Each matched destination normal becomes its source normal turned into apDest's space and unitized;
// abExtrudeMatches also moves the matched vertices 10 units along their normals (the source's only when it differs from
// the last call's). abDisplayMatches is unused.
bool FixNormals(
    NiGeometry* apSource,
    NiGeometry* apDest,
    float afTolerance,
    bool abDisplayMatches,
    bool abExtrudeMatches
) {
    alignas(NiTMapVertexDist) uint8_t aMapStorage[sizeof(NiTMapVertexDist)];
    NiTMapVertexDist* pMap = ::new (aMapStorage) NiTMapVertexDist(37);
    alignas(NiMatrix3) uint8_t aMatrixStorage[sizeof(NiMatrix3)];
    NiMatrix3* pMatrix = ::new (aMatrixStorage) NiMatrix3;
    alignas(NiTransform) uint8_t aInverseStorage[sizeof(NiTransform)];
    NiTransform* pInverse = ::new (aInverseStorage) NiTransform;
    // Constructs a point local that is never used.
    alignas(NiPoint3) uint8_t aPointStorage[sizeof(NiPoint3)];
    ::new (aPointStorage) NiPoint3;
    bool bChanged = false;
    if (!apSource || !apDest || apSource == apDest) {
        pMap->~NiTMapVertexDist();
        return false;
    }
    NiPoint3* pSourceVertices = apSource->GetVertices();
    NiPoint3* pSourceNormals = apSource->GetNormals();
    uint32_t uiSourceCount = apSource->GetVertexCount();
    if (!pSourceVertices || !pSourceNormals || !uiSourceCount) {
        pMap->~NiTMapVertexDist();
        return false;
    }
    NiPoint3* pDestVertices = apDest->GetVertices();
    NiPoint3* pDestNormals = apDest->GetNormals();
    uint32_t uiDestCount = apDest->GetVertexCount();
    if (!pDestVertices || !pDestNormals || !uiDestCount) {
        pMap->~NiTMapVertexDist();
        return false;
    }
    NiPoint3* pSourceWorld = CreateWorldVertices(apSource);
    if (!pSourceWorld) {
        pMap->~NiTMapVertexDist();
        return false;
    }
    NiPoint3* pDestWorld = CreateWorldVertices(apDest);
    if (!pDestWorld) {
        delete[] pSourceWorld;
        pMap->~NiTMapVertexDist();
        return false;
    }
    // The inverse transform is computed but never read.
    apDest->GetWorldTransform().Invert(*pInverse);
    // Gets the source's transform before the destination's.
    const NiTransform& rSourceWorld = apSource->GetWorldTransform();
    NiMatrix3 kDestInverse;
    *pMatrix = *apDest->GetWorldTransform().m_Rotate.Inverse(&kDestInverse) * rSourceWorld.m_Rotate;
    for (uint32_t i = 0; i < uiDestCount; ++i) {
        uint32_t uiNearest = uiSourceCount;
        // A negative or NaN tolerance means any distance.
        float fTolerance = afTolerance >= 0.0 ? afTolerance : std::numeric_limits<float>::max();
        float fNearest = fTolerance;
        for (uint32_t j = 0; j < uiSourceCount; ++j) {
            float fDistance = (pDestWorld[i] - pSourceWorld[j]).Length();
            if (fNearest > fDistance) {
                uiNearest = j;
                fNearest = fDistance;
            }
        }
        if (uiNearest < uiSourceCount) {
            VertexDist kNew;
            kNew.iVertexDest = i;
            kNew.iVertexDest2 = uiDestCount;
            kNew.fDistance = fNearest;
            VertexDist kOld;
            if (pMap->GetAt(uiNearest, kOld)) {
                if (FloatEQ(kNew.fDistance, kOld.fDistance, 0.001f)) {
                    kNew.iVertexDest2 = kOld.iVertexDest;
                    pMap->SetAt(uiNearest, kNew);
                }
                else if (kOld.fDistance > kNew.fDistance) {
                    pMap->SetAt(uiNearest, kNew);
                }
            }
            else {
                pMap->SetAt(uiNearest, kNew);
            }
        }
    }
    NiTMapItem<uint32_t, VertexDist>* pPos = pMap->GetFirstPos();
    while (pPos) {
        uint32_t uiSource;
        VertexDist kMatch;
        pMap->GetNext(pPos, uiSource, kMatch);
        pDestNormals[kMatch.iVertexDest] = *pMatrix * pSourceNormals[uiSource];
        pDestNormals[kMatch.iVertexDest].Unitize();
        if (static_cast<uint32_t>(kMatch.iVertexDest2) < uiDestCount) {
            pDestNormals[kMatch.iVertexDest2] = pDestNormals[kMatch.iVertexDest];
        }
        if (abExtrudeMatches) {
            if (plastSource != apSource) {
                pSourceVertices[uiSource] += 10.0f * pSourceNormals[uiSource];
            }
            pDestVertices[kMatch.iVertexDest] += 10.0f * pDestNormals[kMatch.iVertexDest];
            if (static_cast<uint32_t>(kMatch.iVertexDest2) < uiDestCount) {
                pDestVertices[kMatch.iVertexDest2] = pDestVertices[kMatch.iVertexDest];
            }
        }
        bChanged = true;
    }
    if (bChanged) {
        if (abExtrudeMatches) {
            apSource->GetModelData()->MarkAsChanged(NiGeometryData::NORMAL_MASK | NiGeometryData::VERTEX_MASK);
        }
        apDest->GetModelData()->MarkAsChanged(NiGeometryData::NORMAL_MASK | NiGeometryData::VERTEX_MASK);
    }
    if (pSourceWorld) {
        delete[] pSourceWorld;
    }
    if (pDestWorld) {
        delete[] pDestWorld;
    }
    plastSource = apSource;
    pMap->~NiTMapVertexDist();
    return bChanged;
}

uint32_t BSGetTickCount() {
    return GetTickCount();
}

int32_t GetCollisionObjectCountInSceneGraph(
    NiAVObject* apRoot,
    bool abIgnoreWeaponBone,
    bool abIgnoreFadeNodes,
    bool abIgnoreArrows
) {
    bhkWorldNS::ObjectRecData recData;
    recData.eAction = bhkWorldNS::OA_TOTAL;
    recData.bRecurse = true;
    recData.uDataA[0].i = 0;
    recData.uDataA[3].i = 0;
    if (abIgnoreWeaponBone) {
        recData.uDataA[3].i |= 1u;
    }
    if (abIgnoreFadeNodes) {
        recData.uDataA[3].i |= 2u;
    }
    if (abIgnoreArrows) {
        recData.uDataA[3].i |= 4u;
    }
    bhkWorld::DoObjectRec(apRoot, recData, &GetCollisionObjectCount);
    return recData.uDataA[0].i;
}

void SetReferenceIDOnScenegraph(NiAVObject* apObject, uint32_t auiID) {
    if (apObject) {
        if (apObject->IsGeometry()) {
            NiShadeProperty* pProperty = static_cast<NiShadeProperty*>(apObject->GetProperty(3));
            BSShaderLightingProperty* pShaderProp = nullptr;
            if (BSSM_ShaderPropertyIsLighting(pProperty)) {
                pShaderProp = static_cast<BSShaderLightingProperty*>(pProperty);
            }
            if (pShaderProp) {
                pShaderProp->uiReferenceID = auiID;
            }
        }
        else {
            NiNode* pNode = apObject->IsNode();
            if (pNode) {
                for (uint32_t n = 0; n < pNode->GetArrayCount(); ++n) {
                    SetReferenceIDOnScenegraph(pNode->GetAt(n), auiID);
                }
            }
        }
    }
}

void UnTintScenegraph(NiAVObject* apRoot) {
    NiProperty* pProperty = apRoot->GetProperty(3);
    BSShaderPPLightingProperty* pShaderProp = static_cast<BSShaderPPLightingProperty*>(
        NiObject::DynamicCastStatic(&BSShaderPPLightingProperty::ms_RTTI, pProperty)
    );
    if (pShaderProp) {
        pShaderProp->SetTexEffectData(nullptr);
    }
    NiNode* pNode = apRoot->IsNode();
    if (pNode) {
        for (uint32_t n = 0; n < pNode->GetArrayCount(); ++n) {
            if (pNode->GetAt(n)) {
                UnTintScenegraph(pNode->GetAt(n));
            }
        }
    }
}

BSShaderPPLightingProperty* FindPerPixelLighting(NiNode* apNode) {
    if (!apNode->GetArrayCount()) {
        return nullptr;
    }
    for (uint32_t n = apNode->GetArrayCount() - 1;; --n) {
        NiAVObject* pObj = apNode->GetAt(n);
        if (pObj) {
            NiNode* pNode = pObj->IsNode();
            if (pNode) {
                NiAVObject* pChild = pNode->GetAtChecked(0);
                if (pChild) {
                    int32_t iType = NiShadeProperty::GetType();
                    NiShadeProperty* pProperty = static_cast<NiShadeProperty*>(pChild->GetProperty(iType));
                    if (pProperty) {
                        // Tests the per-pixel lighting pointer for null, then computes it again to return it.
                        int32_t iPPLighting = pProperty->iShaderPropertyType >= 8 &&
                            pProperty->iShaderPropertyType <= 12;
                        if (iPPLighting ? pProperty : nullptr) {
                            return iPPLighting ? static_cast<BSShaderPPLightingProperty*>(pProperty) : nullptr;
                        }
                    }
                }
            }
        }
        if (!n) {
            break;
        }
    }
    return FindPerPixelLighting_Recurse(apNode);
}

bhkCollisionObject* FindFirstCollisionObject(NiAVObject* ap3D) {
    bhkCollisionObject* pCollisionObject = nullptr;
    if (ap3D) {
        pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(ap3D);
        if (!pCollisionObject) {
            NiNode* pNode = ap3D->IsNode();
            if (pNode) {
                uint32_t uiArrayCount = pNode->GetArrayCount();
                for (uint32_t n = 0; n < uiArrayCount; ++n) {
                    pCollisionObject = FindFirstCollisionObject(pNode->GetAt(n));
                    if (pCollisionObject) {
                        break;
                    }
                }
            }
        }
    }
    return pCollisionObject;
}

bool HasParticleSystem(NiObjectNET* apObj) {
    if (!apObj) {
        return false;
    }
    bool bParticles = NiObject::IsKindOf(&NiParticleSystem::ms_RTTI, apObj);
    if (bParticles) {
        return true;
    }
    NiNode* pNode = apObj->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            if (HasParticleSystem(pNode->GetAt(i))) {
                return true;
            }
        }
    }
    return false;
}

bool NodeInFrustum(NiAVObject* apObject, NiCamera* apCamera) {
    if (!apObject) {
        return false;
    }
    const NiBound& worldBound = apObject->GetWorldBound();
    return BoundInFrustum(worldBound, apCamera);
}

bool PointInFrustum(NiPoint3 aTestLocation, NiCamera* apCamera, float afDistance) {
    if (!apCamera) {
        return false;
    }
    NiBound bound;
    bound.SetCenter(aTestLocation);
    bound.SetRadius(afDistance);
    return BoundInFrustum(bound, apCamera);
}

// Clips the segment aStartPt-aEndPt against aTestRect (its top is the larger y) and writes the hit nearest aStartPt to
// aOutputIntersectPt. The floats are compared against the exact integer edges, so the edges widen to double; the branch
// senses are chosen for their NaN handling.
bool LineVsRect(
    const tagRECT& aTestRect,
    const NiPoint3& aStartPt,
    const NiPoint3& aEndPt,
    NiPoint3& aOutputIntersectPt
) {
    bool bResult = false;
    if (aTestRect.left >= aTestRect.right) {
        return false;
    }
    if (aTestRect.bottom >= aTestRect.top) {
        return false;
    }
    if (aStartPt.x < static_cast<double>(aTestRect.left) && aEndPt.x < static_cast<double>(aTestRect.left)) {
        return false;
    }
    if (aStartPt.y < static_cast<double>(aTestRect.bottom) && aEndPt.y < static_cast<double>(aTestRect.bottom)) {
        return false;
    }
    if (aStartPt.x > static_cast<double>(aTestRect.right) && aEndPt.x > static_cast<double>(aTestRect.right)) {
        return false;
    }
    if (aStartPt.y > static_cast<double>(aTestRect.top) && aEndPt.y > static_cast<double>(aTestRect.top)) {
        return false;
    }
    if (aStartPt.x > static_cast<double>(aTestRect.left) &&
        aStartPt.x < static_cast<double>(aTestRect.right) &&
        aStartPt.y > static_cast<double>(aTestRect.bottom) &&
        aStartPt.y < static_cast<double>(aTestRect.top) &&
        aEndPt.x > static_cast<double>(aTestRect.left) &&
        aEndPt.x < static_cast<double>(aTestRect.right) &&
        aEndPt.y > static_cast<double>(aTestRect.bottom) &&
        aEndPt.y < static_cast<double>(aTestRect.top)) {
        return false;
    }
    if (aStartPt.x == aEndPt.x || aStartPt.y == aEndPt.y) {
        if (aStartPt.x == aEndPt.x && aStartPt.y == aEndPt.y) {
            if (aStartPt.x <= static_cast<double>(aTestRect.right) &&
                aStartPt.x >= static_cast<double>(aTestRect.left) &&
                aStartPt.y <= static_cast<double>(aTestRect.top) &&
                aStartPt.y >= static_cast<double>(aTestRect.bottom)) {
                bResult = true;
                aOutputIntersectPt = aStartPt;
            }
        }
        else if (aStartPt.x == aEndPt.x) {
            if (aStartPt.x <= static_cast<double>(aTestRect.right) &&
                aStartPt.x >= static_cast<double>(aTestRect.left)) {
                bResult = true;
                // asm: falls through to the top edge only when aEndPt->y < aStartPt->y (NaN takes the bottom).
                if (aEndPt.y < aStartPt.y) {
                    aOutputIntersectPt = NiPoint3(aStartPt.x, static_cast<float>(aTestRect.top), 0.0f);
                }
                else {
                    aOutputIntersectPt = NiPoint3(aStartPt.x, static_cast<float>(aTestRect.bottom), 0.0f);
                }
            }
        }
        else if (
            aStartPt.y == aEndPt.y &&
            aStartPt.y <= static_cast<double>(aTestRect.top) &&
            aStartPt.y >= static_cast<double>(aTestRect.bottom)
        ) {
            bResult = true;
            // asm: the right edge only when aEndPt->x < aStartPt->x (NaN takes the left).
            if (aEndPt.x < aStartPt.x) {
                aOutputIntersectPt = NiPoint3(static_cast<float>(aTestRect.right), aStartPt.y, 0.0f);
            }
            else {
                aOutputIntersectPt = NiPoint3(static_cast<float>(aTestRect.left), aStartPt.y, 0.0f);
            }
        }
    }
    else {
        int32_t iHits = 0;
        NiPoint3 kHit0(
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        );
        NiPoint3 kHit1(
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        );
        float fSlope = (aEndPt.y - aStartPt.y) / (aEndPt.x - aStartPt.x);
        float fIntercept = aEndPt.y - aEndPt.x * fSlope;
        if (iHits < 2) {
            float fY = static_cast<float>(static_cast<double>(aTestRect.left) * fSlope + fIntercept);
            if (fY <= static_cast<double>(aTestRect.top) && fY >= static_cast<double>(aTestRect.bottom)) {
                if (iHits == 0) {
                    kHit0 = NiPoint3(static_cast<float>(aTestRect.left), fY, 0.0f);
                }
                else {
                    kHit1 = NiPoint3(static_cast<float>(aTestRect.left), fY, 0.0f);
                }
                ++iHits;
            }
        }
        if (iHits < 2) {
            float fX = static_cast<float>((static_cast<double>(aTestRect.top) - fIntercept) / fSlope);
            if (fX <= static_cast<double>(aTestRect.right) && fX >= static_cast<double>(aTestRect.left)) {
                if (iHits == 0) {
                    kHit0 = NiPoint3(fX, static_cast<float>(aTestRect.top), 0.0f);
                }
                else {
                    kHit1 = NiPoint3(fX, static_cast<float>(aTestRect.top), 0.0f);
                }
                ++iHits;
            }
        }
        if (iHits < 2) {
            float fY = static_cast<float>(static_cast<double>(aTestRect.right) * fSlope + fIntercept);
            if (fY <= static_cast<double>(aTestRect.top) && fY >= static_cast<double>(aTestRect.bottom)) {
                if (iHits == 0) {
                    kHit0 = NiPoint3(static_cast<float>(aTestRect.right), fY, 0.0f);
                }
                else {
                    kHit1 = NiPoint3(static_cast<float>(aTestRect.right), fY, 0.0f);
                }
                ++iHits;
            }
        }
        if (iHits < 2) {
            float fX = static_cast<float>((static_cast<double>(aTestRect.bottom) - fIntercept) / fSlope);
            if (fX <= static_cast<double>(aTestRect.right) && fX >= static_cast<double>(aTestRect.left)) {
                if (iHits == 0) {
                    kHit0 = NiPoint3(fX, static_cast<float>(aTestRect.bottom), 0.0f);
                }
                else {
                    kHit1 = NiPoint3(fX, static_cast<float>(aTestRect.bottom), 0.0f);
                }
                ++iHits;
            }
        }
        if (iHits != 0) {
            bResult = true;
            if (iHits == 2) {
                if (aStartPt.x > static_cast<double>(aTestRect.left) &&
                    aStartPt.x < static_cast<double>(aTestRect.right) &&
                    aStartPt.y > static_cast<double>(aTestRect.bottom) &&
                    aStartPt.y < static_cast<double>(aTestRect.top)) {
                    // The start is inside: take the hit on the segment's side of it, flattened to xy.
                    NiPoint3 kDir = aEndPt - aStartPt;
                    NiPoint3 kToHit0 = kHit0 - aStartPt;
                    kDir.z = 0.0f;
                    kToHit0.z = 0.0f;
                    kDir.Unitize();
                    kToHit0.Unitize();
                    // kHit0 is chosen only when the length is greater than 1.0; NaN takes kHit1.
                    if ((kDir + kToHit0).Length() > 1.0) {
                        aOutputIntersectPt = kHit0;
                    }
                    else {
                        aOutputIntersectPt = kHit1;
                    }
                }
                else {
                    double dDist0 = (kHit0 - aStartPt).Length();
                    if ((kHit1 - aStartPt).Length() > dDist0) {
                        aOutputIntersectPt = kHit0;
                    }
                    else {
                        aOutputIntersectPt = kHit1;
                    }
                }
            }
            else {
                aOutputIntersectPt = kHit0;
            }
        }
    }
    return bResult;
}

bool HasMorpherController(NiObjectNET* apObj) {
    if (!apObj) {
        return false;
    }
    if (apObj->GetController(&NiGeomMorpherController::ms_RTTI)) {
        return true;
    }
    NiNode* pNode = apObj->IsNode();
    if (pNode) {
        for (uint32_t n = 0; n < pNode->GetArrayCount(); ++n) {
            NiAVObject* pChild = pNode->GetAt(n);
            if (HasMorpherController(pChild)) {
                return true;
            }
        }
    }
    return false;
}

hkpWorldObject* hkGetWorldObject(const hkpCollidable* collidable) {
    return static_cast<hkpWorldObject*>(collidable->getOwner());
}

bool PickDistance(
    NiPick* apPick,
    const NiPoint3& aOrigin,
    const NiPoint3& aDir,
    float afDistance,
    NiAVObject* apTarget,
    bool abClearResults
) {
    if (apPick && apTarget && afDistance != 0.0) {
        if (abClearResults) {
            apPick->SetTarget(nullptr);
            apPick->PickObjects(NiPoint3::ZERO, NiPoint3::ZERO, false);
        }
        return PickDistanceRecursive(apPick, aOrigin, aDir, afDistance, apTarget);
    }
    return false;
}

bool PickDistanceRecursive(
    NiPick* apPick,
    const NiPoint3& aOrigin,
    const NiPoint3& aDir,
    float afDistance,
    NiAVObject* apTarget
) {
    bool bHitSomething = false;
    if (apPick && apTarget && (!apPick->m_bObserveAppCullFlag || !apTarget->GetAppCulled())) {
        if (!apTarget->IsVisualObject()) {
            return false;
        }
        NiBound kTargetBound = apTarget->GetWorldBound();
        NiPoint3 kPtDiff = kTargetBound.m_kCenter - aOrigin;
        double dDistance = kPtDiff.Length();
        if (kTargetBound.m_fRadius + afDistance > dDistance) {
            NiNode* pTargetNode = apTarget->IsNode();
            if (pTargetNode) {
                int32_t iChildCount = pTargetNode->GetArrayCount();
                for (int32_t i = 0; i < iChildCount; ++i) {
                    if (PickDistanceRecursive(
                            apPick,
                            aOrigin,
                            aDir,
                            afDistance,
                            pTargetNode->GetAt(static_cast<uint32_t>(i))
                        )) {
                        bHitSomething = true;
                    }
                }
            }
            else {
                apPick->SetTarget(apTarget);
                if (apPick->PickObjects(aOrigin, aDir, true)) {
                    return true;
                }
            }
        }
    }
    return bHitSomething;
}

bool BoundInFrustum(const NiBound& aBound, NiCamera* apCamera) {
    if (!apCamera) {
        return false;
    }
    static NiFrustumPlanes cullPlanes;
    if (pLastCamera != apCamera || uiLastTime != appTimer.iLastTime) {
        cullPlanes.Set(*apCamera);
        uiLastTime = appTimer.iLastTime;
        pLastCamera = apCamera;
    }
    for (uint32_t uiPlane = 0; uiPlane < 6; ++uiPlane) {
        NiPlane kPlane = cullPlanes.GetPlane(uiPlane);
        if (aBound.WhichSide(kPlane) == 2) {
            return false;
        }
    }
    return true;
}

bool ReturnTrue() {
    return true;
}

float NiATan2(float fNumerator, float fDenominator) {
    return std::atan2(fNumerator, fDenominator);
}

hkpPhantom* hkpGetPhantom(const hkpCollidable* collidable) {
    if (collidable->getType() == 2) {
        return static_cast<hkpPhantom*>(hkGetWorldObject(collidable));
    }

    return nullptr;
}

hkpRigidBody* hkpGetRigidBody(const hkpCollidable* collidable) {
    if (collidable->getType() == 1) {
        return static_cast<hkpRigidBody*>(hkGetWorldObject(collidable));
    }

    return nullptr;
}

NiAVObject* GetAVObjectForCollidable(const hkpCollidable* aphkCol) {
    NiPoint3 kLocation;
    NiAVObject* p3D = nullptr;
    if (aphkCol) {
        hkpRigidBody* pBody = hkpGetRigidBody(aphkCol);
        hkpPhantom* pPhantom = pBody ? nullptr : hkpGetPhantom(aphkCol);
        hkpWorldObject* pWorldObject = pPhantom;
        if (pBody) {
            bhkCollisionObject* pCol = bhkCollisionObject::Getbhk(pBody);
            if (pCol) {
                return pCol->m_pkSceneObject;
            }

            CFilter filter(aphkCol->getCollisionFilterInfo());
            if (filter.GetGroup() == 1) {
                HK2NI(kLocation, pBody->getTranslation());
                NiAVObject* pLand3D = TES::pInstance->GetLandGeometry(kLocation);
                if (pLand3D && pLand3D->m_pkParent) {
                    return pLand3D->m_pkParent;
                }
            }
        }
        else {
            bhkPCollisionObject* pPCol = bhkUtilFunctions::IsbhkPCollisionObject(pWorldObject);
            if (pPCol) {
                p3D = pPCol->m_pkSceneObject;
            }
            if (!p3D) {
                bhkEntity* pBP = bhkEntity::Getbhk(pWorldObject);
                if (pBP) {
                    return pBP->GetAVObject(0);
                }
            }
        }
    }
    return p3D;
}

void BSmemcpy(void* apDest, int32_t aiDestSize, const void* apSrc, int32_t aiCount) {
    memcpy_s(apDest, aiDestSize, apSrc, aiCount);
}

bool SwapPlatformLanguageTexturePath(const char* apInPath, char* apOutPath, uint32_t aiOutPathSize) {
    if (!apInPath || !apOutPath || !aiOutPathSize) {
        return false;
    }
    bool bRetVal = false;
    *apOutPath = 0;
    uint32_t uiOutLen = 0;
    int32_t iPlatform = GetIsControllerConnected();
    LANGUAGE eLang = GetCurrentTexLanguage();
    const char* pSearch = apInPath;
    for (int32_t i = 0; i < 4; ++i) {
        uint32_t uiLen;
        const char* pFind;
        char cSlash = 0;
        for (pFind = BSstristr(apInPath, PipBoyPlatforms[i]); pFind;
             pFind = BSstristr(&pFind[uiLen], PipBoyPlatforms[i])) {
            uiLen = BSstrlen(PipBoyPlatforms[i]);
            if (pFind[uiLen] == '\\' || pFind[uiLen] == '/') {
                cSlash = pFind[uiLen];
                break;
            }
        }
        if (pFind) {
            uiLen = pFind - apInPath;
            if (pFind != apInPath) {
                BSmemcpy(apOutPath, static_cast<int32_t>(aiOutPathSize), apInPath, static_cast<int32_t>(uiLen));
                uiOutLen = pFind - apInPath;
                apOutPath[uiLen] = 0;
            }
            BSstrcat(apOutPath, static_cast<int32_t>(aiOutPathSize), PipBoyPlatforms[iPlatform]);
            uint32_t uiSlashPos = uiOutLen + std::strlen(PipBoyPlatforms[iPlatform]);
            apOutPath[uiSlashPos] = cSlash;
            uiOutLen = uiSlashPos + 1;
            apOutPath[uiOutLen] = 0;
            pSearch = &pFind[std::strlen(PipBoyPlatforms[i]) + 1];
            bRetVal = true;
            break;
        }
    }
    for (int32_t i = 0; i < 5; ++i) {
        uint32_t uiLen;
        const char* pFind;
        char cSlash = 0;
        for (pFind = BSstristr(pSearch, PipBoyLanguages[i]); pFind;
             pFind = BSstristr(&pFind[uiLen], PipBoyLanguages[i])) {
            uiLen = BSstrlen(PipBoyLanguages[i]);
            if (pFind[uiLen] == '\\' || pFind[uiLen] == '/') {
                cSlash = pFind[uiLen];
                break;
            }
        }
        if (pFind) {
            uiLen = pFind - pSearch;
            if (pFind != pSearch) {
                // The remaining size is computed unsigned and passed as BSmemcpy's int size.
                BSmemcpy(
                    &apOutPath[uiOutLen],
                    static_cast<int32_t>(aiOutPathSize - uiOutLen),
                    pSearch,
                    static_cast<int32_t>(uiLen)
                );
                uiOutLen += uiLen;
                apOutPath[uiOutLen] = 0;
            }
            BSstrcat(apOutPath, static_cast<int32_t>(aiOutPathSize), PipBoyLanguages[eLang]);
            uint32_t uiSlashPos = uiOutLen + std::strlen(PipBoyLanguages[eLang]);
            apOutPath[uiSlashPos] = cSlash;
            apOutPath[uiSlashPos + 1] = 0;
            pSearch = &pFind[std::strlen(PipBoyLanguages[i]) + 1];
            bRetVal = true;
            break;
        }
    }
    BSstrcat(apOutPath, static_cast<int32_t>(aiOutPathSize), pSearch);
    return bRetVal;
}

bool SwapPlatformLanguageTextures(NiAVObject* ap3D) {
    bool bSwapped = false;
    if (!ap3D) {
        return bSwapped;
    }
    NiProperty* pProperty = ap3D->GetProperty(NiProperty::SHADE);
    // Shader property types 8 to 12 are the per-pixel lighting properties.
    bool bIsLighting;
    if (pProperty) {
        bIsLighting = static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() >= 8 &&
            static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() <= 12;
    }
    else {
        bIsLighting = false;
    }
    BSShaderPPLightingProperty* pShaderProperty = bIsLighting ? static_cast<BSShaderPPLightingProperty*>(pProperty)
                                                              : nullptr;
    if (pShaderProperty) {
        uint32_t uiCount = pShaderProperty->GetTextureCount();
        for (uint32_t i = 0; i < uiCount; ++i) {
            for (int32_t j = 0; j < 6; ++j) {
                const BSShaderProperty::TextureTypeEnum eType = static_cast<BSShaderProperty::TextureTypeEnum>(j);
                NiTexture* pTexture = pShaderProperty->GetTexture(eType, i);
                if (!pTexture || !NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, pTexture)) {
                    continue;
                }
                NiSourceTexture* pSourceTexture = static_cast<NiSourceTexture*>(pTexture);
                alignas(NiFixedString) uint8_t aNameStorage[sizeof(NiFixedString)];
                NiFixedString* pName = ::new (aNameStorage) NiFixedString(pSourceTexture->GetFilename());
                char acPath[260];
                acPath[0] = '\0';
                if (SwapPlatformLanguageTexturePath(pName->GetHandleString(), acPath, 260) && acPath[0]) {
                    alignas(NiPointer<NiSourceTexture>) uint8_t aTextureStorage[sizeof(NiPointer<NiSourceTexture>)];
                    NiPointer<NiSourceTexture>* pspTexture = ::new (aTextureStorage)
                        NiPointer<NiSourceTexture>(nullptr);
                    TES::pInstance->CreateTextureImage(acPath, *pspTexture, false, false);
                    if (*pspTexture) {
                        pShaderProperty->SetTexture(eType, i, *pspTexture);
                        bSwapped = true;
                    }
                    pspTexture->~NiPointer<NiSourceTexture>();
                }
                pName->~NiFixedString();
            }
        }
    }
    NiNode* pNode = ap3D->IsNode();
    if (!pNode) {
        return bSwapped;
    }
    uint32_t uiChildCount = pNode->GetArrayCount();
    for (uint32_t k = 0; k < uiChildCount; ++k) {
        if (SwapPlatformLanguageTextures(pNode->GetAt(k))) {
            bSwapped = true;
        }
    }
    return bSwapped;
}

bool RemoveScabard(NiNode* apNode) {
    if (!apNode) {
        return false;
    }
    for (uint32_t n = 0; n < apNode->GetArrayCount(); ++n) {
        NiAVObject* pObj = apNode->GetAt(n);
        if (pObj) {
            const char* pName = pObj->m_kName.GetHandleString();
            if (pName) {
                if (!BSstricmp(pName, "Scb")) {
                    apNode->DetachChildAt(n);
                    return true;
                }
                if (!_strnicmp(pName, "FadeNode ", 9u)) {
                    return RemoveScabard(pObj->IsNode());
                }
            }
        }
    }
    return false;
}

void SetPhase(NiNode* apNode, float afPhase) {
    if (!apNode) {
        return;
    }
    for (NiTimeController* pController = apNode->GetControllers(); pController; pController = pController->GetNext()) {
        pController->SetPhase(afPhase);
    }
    for (uint32_t i = 0; i < apNode->GetArrayCount(); ++i) {
        NiAVObject* pChild = apNode->GetAt(i);
        if (pChild) {
            bool bNode = NiObject::IsKindOf(&NiNode::ms_RTTI, pChild);
            if (bNode) {
                SetPhase(static_cast<NiNode*>(pChild), afPhase);
            }
        }
    }
}

void RecursivelyRemoveAllDecals(NiAVObject* apRoot) {
    if (apRoot) {
        if (apRoot->IsTriBasedGeom()) {
            NiShadeProperty* pProp = static_cast<NiShadeProperty*>(apRoot->GetProperty(3));
            if (pProp) {
                NiShadeProperty* pShaderProperty = BSSM_ShaderPropertyIsLighting(pProp) != 0 ? pProp : nullptr;
                if (pShaderProperty) {}
            }
        }
        else {
            NiNode* pNode = apRoot->IsNode();
            if (pNode) {
                for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                    RecursivelyRemoveAllDecals(pNode->GetAt(i));
                }
            }
        }
    }
}

void AddNoLightingPropertyRecurse(NiAVObject* apObject) {
    if (apObject->IsGeometry() && !apObject->GetProperty(3)) {
        BSShaderNoLightingProperty* pShProp = new BSShaderNoLightingProperty;
        apObject->AttachProperty(pShProp);
    }

    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        for (uint32_t n = 0; n < pNode->GetArrayCount(); ++n) {
            NiAVObject* pChild = pNode->GetAt(n);
            if (pChild) {
                AddNoLightingPropertyRecurse(pChild);
            }
        }
    }
}

void PrepareObjectWithNoLightingRecurse(NiAVObject* apObject) {
    if (!apObject->IsGeometry() || apObject->GetProperty(3)) {
        NiNode* pNode = apObject->IsNode();
        if (pNode && pNode->IsVisualObject()) {
            for (uint32_t n = 0; n < pNode->GetArrayCount(); ++n) {
                NiAVObject* pChild = pNode->GetAt(n);
                if (pChild) {
                    PrepareObjectWithNoLightingRecurse(pChild);
                }
            }
        }
    }
    else {
        BSShaderNoLightingProperty* pShProp = new BSShaderNoLightingProperty;
        pShProp->SetFlag(BSShaderProperty::BSSP_FLAG_WIREFRAME, 1);
        apObject->AttachProperty(pShProp);
        apObject->UpdateProperties();
        BSShaderManager::PrepareObject(apObject, 0, 0);
    }
}

float GetUnclampedZAngleFromVector(const NiPoint3& aDirection) {
    if (aDirection.y == 0.0) {
        if (aDirection.x > 0.0) {
            return 1.5707963705062866f;
        }
        return 4.71238899230957f;
    }
    float fRatio = aDirection.x / aDirection.y;
    float fAngle = std::atanf(fRatio);
    if (aDirection.y < 0.0) {
        return static_cast<float>(fAngle + 3.1415927410125732);
    }
    return fAngle;
}

LANGUAGE GetCurrentTexLanguage() {
    return GetCurrentLanguage();
}

OBJECT_TYPE FormIDToObjectType(uint8_t acFormID) {
    OBJECT_TYPE eType;
    switch (acFormID) {
        case 4: {
            eType = OBJECT_TEXTURESET;
            break;
        }
        case 5: {
            eType = OBJECT_MENUICON;
            break;
        }
        case 7: {
            eType = OBJECT_CLASS;
            break;
        }
        case 8: {
            eType = OBJECT_FACTION;
            break;
        }
        case 9: {
            eType = OBJECT_HEADPART;
            break;
        }
        case 11: {
            eType = OBJECT_EYES;
            break;
        }
        case 12: {
            eType = OBJECT_RACE;
            break;
        }
        case 13: {
            eType = OBJECT_SOUND;
            break;
        }
        case 14: {
            eType = OBJECT_ACOUSTICSPACE;
            break;
        }
        case 16: {
            eType = OBJECT_MAGICEFFECT;
            break;
        }
        case 18: {
            eType = OBJECT_LANDTEXTURE;
            break;
        }
        case 19: {
            eType = OBJECT_ENCHANTING;
            break;
        }
        case 20: {
            eType = OBJECT_SPELLMAKING;
            break;
        }
        case 21: {
            eType = OBJECT_ACTIVATOR;
            break;
        }
        case 22: {
            eType = OBJECT_TALKINGACTIVATOR;
            break;
        }
        case 23: {
            eType = OBJECT_TERMINAL;
            break;
        }
        case 24: {
            eType = OBJECT_ARMOR;
            break;
        }
        case 25: {
            eType = OBJECT_BOOK;
            break;
        }
        case 27: {
            eType = OBJECT_CONTAINER;
            break;
        }
        case 28: {
            eType = OBJECT_DOOR;
            break;
        }
        case 29: {
            eType = OBJECT_INGREDIENT;
            break;
        }
        case 30: {
            eType = OBJECT_LIGHT;
            break;
        }
        case 31: {
            eType = OBJECT_MISCITEM;
            break;
        }
        case 32: {
            eType = OBJECT_STATIC;
            break;
        }
        case 33: {
            eType = OBJECT_STATICCOLLECTION;
            break;
        }
        case 34: {
            eType = OBJECT_MOVABLESTATIC;
            break;
        }
        case 35: {
            eType = OBJECT_PLACEABLEWATER;
            break;
        }
        case 36: {
            eType = OBJECT_GRASS;
            break;
        }
        case 37: {
            eType = OBJECT_TREE;
            break;
        }
        case 38: {
            eType = OBJECT_FLORA;
            break;
        }
        case 39: {
            eType = OBJECT_FURNITURE;
            break;
        }
        case 40: {
            eType = OBJECT_WEAPON;
            break;
        }
        case 41: {
            eType = OBJECT_AMMO;
            break;
        }
        case 42: {
            eType = OBJECT_NPC;
            break;
        }
        case 43: {
            eType = OBJECT_CREATURE;
            break;
        }
        case 44: {
            eType = OBJECT_LEVCREATURE;
            break;
        }
        case 45: {
            eType = OBJECT_LEVCHARACTER;
            break;
        }
        case 46: {
            eType = OBJECT_KEY;
            break;
        }
        case 47: {
            eType = OBJECT_ALCHEMY;
            break;
        }
        case 48: {
            eType = OBJECT_IDLEMARKER;
            break;
        }
        case 49: {
            eType = OBJECT_NOTE;
            break;
        }
        case 50: {
            eType = OBJECT_CONSTRUCTIBLE;
            break;
        }
        case 51: {
            eType = OBJECT_PROJECTILE;
            break;
        }
        case 52: {
            eType = OBJECT_LEVITEM;
            break;
        }
        case 53: {
            eType = OBJECT_WEATHER;
            break;
        }
        case 54: {
            eType = OBJECT_CLIMATE;
            break;
        }
        case 71: {
            eType = OBJECT_QUEST;
            break;
        }
        case 73: {
            eType = OBJECT_PACKAGE;
            break;
        }
        case 74: {
            eType = OBJECT_COMBATSTYLE;
            break;
        }
        case 75: {
            eType = OBJECT_LOADSCREEN;
            break;
        }
        case 76: {
            eType = OBJECT_LEVSPELL;
            break;
        }
        case 77: {
            eType = OBJECT_ANIMOBJECT;
            break;
        }
        case 78: {
            eType = OBJECT_WATER;
            break;
        }
        case 79: {
            eType = OBJECT_EFFECTSHADER;
            break;
        }
        case 81: {
            eType = OBJECT_EXPLOSION;
            break;
        }
        case 82: {
            eType = OBJECT_DEBRIS;
            break;
        }
        case 85: {
            eType = OBJECT_FORMLIST;
            break;
        }
        case 86: {
            eType = OBJECT_PERK;
            break;
        }
        case 87: {
            eType = OBJECT_BODYPARTDATA;
            break;
        }
        case 88: {
            eType = OBJECT_ADDON;
            break;
        }
        case 91: {
            eType = OBJECT_CAMERASHOT;
            break;
        }
        case 93: {
            eType = OBJECT_VOICETYPE;
            break;
        }
        case 94: {
            eType = OBJECT_IMPACTDATA;
            break;
        }
        case 95: {
            eType = OBJECT_IMPACTDATASET;
            break;
        }
        case 97: {
            eType = OBJECT_ENCOUNTERZONE;
            break;
        }
        case 102: {
            eType = OBJECT_MUSICTYPE;
            break;
        }
        case 103: {
            eType = OBJECT_MOD;
            break;
        }
        case 104: {
            eType = OBJECT_REPUTATION;
            break;
        }
        case 106: {
            eType = OBJECT_RECIPE;
            break;
        }
        case 107: {
            eType = OBJECT_RECIPE_CATEGORY;
            break;
        }
        case 108: {
            eType = OBJECT_CHIP;
            break;
        }
        case 109: {
            eType = OBJECT_CASINO;
            break;
        }
        case 110: {
            eType = OBJECT_LOADSCREENTYPE;
            break;
        }
        case 111: {
            eType = OBJECT_MEDIASET;
            break;
        }
        case 112: {
            eType = OBJECT_MEDIALOCATIONCONTROLLER;
            break;
        }
        case 113: {
            eType = OBJECT_CHALLENGE;
            break;
        }
        case 114: {
            eType = OBJECT_AMMO_EFFECT;
            break;
        }
        case 115: {
            eType = OBJECT_CARAVANCARD;
            break;
        }
        case 116: {
            eType = OBJECT_CARAVANMONEY;
            break;
        }
        case 117: {
            eType = OBJECT_CARAVANDECK;
            break;
        }
        default: {
            eType = OBJECT_NONE;
            break;
        }
    }
    return eType;
}

char* FormIDToString(uint8_t acFormID) {
    char* pcName;
    OBJECT_TYPE oType = FormIDToObjectType(acFormID);
    if (oType >= OBJECT_ACTIVATOR) {
        return objectLabel[oType];
    }
    switch (acFormID) {
        case 3: {
            pcName = formString[9];
            break;
        }
        case 6: {
            pcName = formString[7];
            break;
        }
        case 7: {
            pcName = formString[1];
            break;
        }
        case 8: {
            pcName = formString[5];
            break;
        }
        case 9: {
            pcName = formString[16];
            break;
        }
        case 10: {
            pcName = formString[17];
            break;
        }
        case 11: {
            pcName = formString[18];
            break;
        }
        case 12: {
            pcName = formString[0];
            break;
        }
        case 13: {
            pcName = formString[6];
            break;
        }
        case 14: {
            pcName = formString[25];
            break;
        }
        case 15: {
            pcName = formString[14];
            break;
        }
        case 16: {
            pcName = formString[10];
            break;
        }
        case 17: {
            pcName = formString[2];
            break;
        }
        case 49: {
            pcName = formString[22];
            break;
        }
        case 53: {
            pcName = formString[13];
            break;
        }
        case 54: {
            pcName = formString[12];
            break;
        }
        case 55: {
            pcName = formString[8];
            break;
        }
        case 57: {
            pcName = formString[4];
            break;
        }
        case 58: {
            pcName = formString[19];
            break;
        }
        case 65: {
            pcName = formString[11];
            break;
        }
        case 69: {
            pcName = formString[3];
            break;
        }
        case 71: {
            pcName = formString[15];
            break;
        }
        case 72: {
            pcName = formString[20];
            break;
        }
        case 73: {
            pcName = formString[21];
            break;
        }
        case 83: {
            pcName = formString[23];
            break;
        }
        case 84: {
            pcName = formString[24];
            break;
        }
        case 102: {
            pcName = formString[26];
            break;
        }
        case 104: {
            pcName = formString[27];
            break;
        }
        case 106: {
            pcName = formString[28];
            break;
        }
        case 107: {
            pcName = formString[29];
            break;
        }
        case 108: {
            pcName = formString[30];
            break;
        }
        case 109: {
            pcName = formString[31];
            break;
        }
        case 110: {
            pcName = formString[32];
            break;
        }
        case 111: {
            pcName = formString[33];
            break;
        }
        case 112: {
            pcName = formString[34];
            break;
        }
        case 113: {
            pcName = formString[35];
            break;
        }
        case 114: {
            pcName = formString[36];
            break;
        }
        case 115: {
            pcName = formString[37];
            break;
        }
        case 116: {
            pcName = formString[38];
            break;
        }
        case 117: {
            pcName = formString[39];
            break;
        }
        default: {
            if (acFormID >= 121u) {
                Warning("FORMS: Invalid FormID sent to FormIDToString().\r\n", formEnumString[acFormID].pFormString);
                acFormID = 0;
            }
            pcName = formEnumString[acFormID].pFormString;
            break;
        }
    }
    return pcName;
}

NiTriShape* MakeQuadBox(
    const NiPoint3& aPoint1,
    const NiPoint3& aPoint2,
    const NiPoint3& aPoint3,
    const NiPoint3& aPoint4,
    const NiPoint3& aPoint5,
    const NiPoint3& aPoint6,
    const NiPoint3& aPoint7,
    const NiPoint3& aPoint8,
    const NiColorA& aColor,
    bool abWithNoLightingProp
) {
    NiPoint3* pVerts = new NiPoint3[8];
    pVerts[0] = aPoint1;
    pVerts[1] = aPoint2;
    pVerts[2] = aPoint3;
    pVerts[3] = aPoint4;
    pVerts[4] = aPoint5;
    pVerts[5] = aPoint6;
    pVerts[6] = aPoint7;
    pVerts[7] = aPoint8;
    NiColorA* pColors = new NiColorA[8];
    pColors[0] = aColor;
    pColors[1] = aColor;
    pColors[2] = aColor;
    pColors[3] = aColor;
    pColors[4] = aColor;
    pColors[5] = aColor;
    pColors[6] = aColor;
    pColors[7] = aColor;
    uint16_t* pTriList = static_cast<uint16_t*>(_NiMalloc(72 * sizeof(uint16_t)));
    pTriList[0] = 0;
    pTriList[1] = 2;
    pTriList[2] = 1;
    pTriList[3] = 0;
    pTriList[4] = 3;
    pTriList[5] = 2;
    pTriList[6] = 0;
    pTriList[7] = 7;
    pTriList[8] = 3;
    pTriList[9] = 0;
    pTriList[10] = 4;
    pTriList[11] = 7;
    pTriList[12] = 1;
    pTriList[13] = 4;
    pTriList[14] = 0;
    pTriList[15] = 1;
    pTriList[16] = 5;
    pTriList[17] = 4;
    pTriList[18] = 2;
    pTriList[19] = 5;
    pTriList[20] = 1;
    pTriList[21] = 2;
    pTriList[22] = 6;
    pTriList[23] = 5;
    pTriList[24] = 3;
    pTriList[25] = 6;
    pTriList[26] = 2;
    pTriList[27] = 3;
    pTriList[28] = 7;
    pTriList[29] = 6;
    pTriList[30] = 4;
    pTriList[31] = 7;
    pTriList[32] = 6;
    pTriList[33] = 4;
    pTriList[34] = 6;
    pTriList[35] = 5;
    pTriList[36] = 0;
    pTriList[37] = 1;
    pTriList[38] = 2;
    pTriList[39] = 0;
    pTriList[40] = 2;
    pTriList[41] = 3;
    pTriList[42] = 0;
    pTriList[43] = 3;
    pTriList[44] = 7;
    pTriList[45] = 0;
    pTriList[46] = 7;
    pTriList[47] = 4;
    pTriList[48] = 1;
    pTriList[49] = 0;
    pTriList[50] = 4;
    pTriList[51] = 1;
    pTriList[52] = 4;
    pTriList[53] = 5;
    pTriList[54] = 2;
    pTriList[55] = 1;
    pTriList[56] = 5;
    pTriList[57] = 2;
    pTriList[58] = 5;
    pTriList[59] = 6;
    pTriList[60] = 3;
    pTriList[61] = 2;
    pTriList[62] = 6;
    pTriList[63] = 3;
    pTriList[64] = 6;
    pTriList[65] = 7;
    pTriList[66] = 4;
    pTriList[67] = 6;
    pTriList[68] = 7;
    pTriList[69] = 4;
    pTriList[70] = 5;
    pTriList[71] = 6;
    NiTriShape*
        pTri = new NiTriShape(8, pVerts, nullptr, pColors, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 24, pTriList);
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pShProp = new BSShaderNoLightingProperty;
        pTri->AttachProperty(pShProp);
        BSShaderManager::PrepareObject(pTri, 0, 0);
    }
    return pTri;
}

NiTriShape*
MakeTriangle(NiPoint3 aPt1, NiPoint3 aPt2, NiPoint3 aPt3, const NiColorA& aColor, bool abWithNoLightingProp) {
    NiPoint3* pVerts = new NiPoint3[3];
    pVerts[0] = aPt1;
    pVerts[1] = aPt2;
    pVerts[2] = aPt3;
    NiColorA* pColors = new NiColorA[3];
    pColors[0] = aColor;
    pColors[1] = aColor;
    pColors[2] = aColor;
    uint16_t* pTriList = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    pTriList[0] = 0;
    pTriList[1] = 2;
    pTriList[2] = 1;
    pTriList[3] = 0;
    pTriList[4] = 1;
    pTriList[5] = 2;
    NiTriShape*
        pTri = new NiTriShape(3, pVerts, nullptr, pColors, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 2, pTriList);
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pShProp = new BSShaderNoLightingProperty;
        pTri->AttachProperty(pShProp);
        BSShaderManager::PrepareObject(pTri, 0, 0);
    }
    return pTri;
}

BSShaderPPLightingProperty* FindPerPixelLighting_Recurse(NiNode* apNode) {
    BSShaderPPLightingProperty* pRetProp = nullptr;
    if (!apNode->GetArrayCount()) {
        return nullptr;
    }
    uint32_t uiCount = apNode->GetArrayCount();
    for (uint32_t uiIndex = 0; uiIndex < uiCount; ++uiIndex) {
        if (pRetProp) {
            return pRetProp;
        }
        NiNode* pChild = static_cast<NiNode*>(apNode->GetAt(uiIndex));
        if (pChild) {
            if (pChild->IsNode()) {
                pRetProp = FindPerPixelLighting_Recurse(pChild);
            }
            else if (pChild->IsGeometry()) {
                int32_t iType = NiShadeProperty::GetType();
                NiShadeProperty* pProperty = static_cast<NiShadeProperty*>(pChild->GetProperty(iType));
                if (pProperty) {
                    // The per-pixel lighting shader types are 8 to 12.
                    BSShaderPPLightingProperty* pLighting = nullptr;
                    if (pProperty->iShaderPropertyType >= 8 && pProperty->iShaderPropertyType <= 12) {
                        pLighting = static_cast<BSShaderPPLightingProperty*>(pProperty);
                    }
                    if (pLighting) {
                        return pLighting;
                    }
                }
            }
        }
    }
    return pRetProp;
}

bool RemoveEditorMarkers(NiNode* apNode) {
    bool bRetVal = false;
    if (!apNode) {
        return false;
    }
    if (!kMarkerRemover.pRoot) {
        BSXFlags* pBSX = static_cast<BSXFlags*>(apNode->GetExtraData(BSXFlags::Tag()));
        if (pBSX && pBSX->QEditorMarker()) {
            kMarkerRemover.pRoot = apNode;
            NiControllerManager* pManager = static_cast<NiControllerManager*>(
                apNode->GetController(&NiControllerManager::ms_RTTI)
            );
            if (pManager) {
                kMarkerRemover.pPalette = pManager->GetObjectPalette();
            }
        }
    }
    if (!kMarkerRemover.pRoot) {
        return false;
    }
    if (!apNode->m_kName.GetHandleString() || BSstricmp(apNode->m_kName.GetHandleString(), "EditorMarker")) {
        for (uint32_t n = 0; n < apNode->GetArrayCount(); ++n) {
            NiAVObject* pObj = apNode->GetAt(n);
            if (pObj) {
                NiNode* pNode = pObj->IsNode();
                if (pNode) {
                    bRetVal = RemoveEditorMarkers(pNode);
                    if (bRetVal) {
                        if (kMarkerRemover.pRoot == apNode) {
                            kMarkerRemover.pRoot = nullptr;
                            kMarkerRemover.pPalette = nullptr;
                        }
                        return bRetVal;
                    }
                }
            }
        }
        if (kMarkerRemover.pRoot == apNode) {
            kMarkerRemover.pRoot = nullptr;
            kMarkerRemover.pPalette = nullptr;
        }
        return bRetVal;
    }

    if (kMarkerRemover.pPalette) {
        NiFixedString* pMarker = get_g_EditorMarker();
        kMarkerRemover.pPalette->SetAVObject(*pMarker, nullptr);
        pMarker = get_g_EditorMarker0();
        kMarkerRemover.pPalette->SetAVObject(*pMarker, nullptr);
        pMarker = get_g_EditorMarker1();
        kMarkerRemover.pPalette->SetAVObject(*pMarker, nullptr);
        pMarker = get_g_EditorMarker2();
        kMarkerRemover.pPalette->SetAVObject(*pMarker, nullptr);
    }
    if (apNode->m_pkParent) {
        apNode->m_pkParent->DetachChild(apNode);
    }
    if (kMarkerRemover.pRoot == apNode) {
        kMarkerRemover.pRoot = nullptr;
        kMarkerRemover.pPalette = nullptr;
    }
    return true;
}

float FindTimeForNoteKey(NiAVObject* pObject, const char* pcSequenceName, const char* pcKeyName) {
    uint32_t uiNumKeys;
    if (pObject) {
        NiControllerManager* pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pObject->GetControllers())
        );
        if (pManager) {
            NiControllerSequence* pSequence = pManager->GetSequenceByName(pcSequenceName);
            if (pSequence) {
                NiTextKeyExtraData* pText = pSequence->GetTextKeys();
                NiTextKey* pKeys = pText->GetKeys(uiNumKeys);
                uint32_t uiSearchKeyLength = BSstrlen(pcKeyName);
                if (uiSearchKeyLength) {
                    for (uint32_t i = 0; i < uiNumKeys; ++i) {
                        const char* pKeyText = pKeys[i].m_kText.GetHandleString();
                        if (pKeyText && !_strnicmp(pKeyText, pcKeyName, uiSearchKeyLength)) {
                            return pKeys[i].m_fTime;
                        }
                    }
                }
            }
        }
        return 0.0f;
    }

    return 0.0f;
}

void TintScenegraph(NiAVObject* apRoot, NiColor& aColor, void* apTextureEffectData) {
    NiPointer<BSShaderPPLightingProperty::TextureEffectData> spData(
        static_cast<BSShaderPPLightingProperty::TextureEffectData*>(apTextureEffectData)
    );
    if (!apTextureEffectData) {
        spData = new BSShaderPPLightingProperty::TextureEffectData;
        apTextureEffectData = spData.m_pObject;
        spData->FillColor = NiColorA(aColor.r, aColor.g, aColor.b, 0.5f);
        spData->RimColor = NiColorA(aColor.r, aColor.g, aColor.b, 0.5f);
        spData->CurrentFillColor = NiColorA(aColor.r, aColor.g, aColor.b, 0.5f);
        spData->CurrentRimColor = NiColorA(aColor.r, aColor.g, aColor.b, 0.5f);
        spData->eSrcBlendMode = D3DBLEND_DESTCOLOR;
        spData->eDestBlendMode = D3DBLEND_INVDESTCOLOR;
        spData->eBlendOperation = D3DBLENDOP_ADD;
        spData->eZTestFunction = D3DCMP_LESSEQUAL;
    }
    NiProperty* pProperty = apRoot->GetProperty(3);
    BSShaderPPLightingProperty* pSPPLProp = static_cast<BSShaderPPLightingProperty*>(
        NiObject::DynamicCastStatic(&BSShaderPPLightingProperty::ms_RTTI, pProperty)
    );
    if (pSPPLProp) {
        pSPPLProp->SetTexEffectData(spData.m_pObject);
    }
    NiNode* pNode = apRoot->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            if (pNode->GetAt(i)) {
                TintScenegraph(pNode->GetAt(i), aColor, apTextureEffectData);
            }
        }
    }
}

void GetCollisionObjectCount(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    if (((arData.uDataA[3].i & 1) == 0 || !NiObject::IsKindOf_0(&WeaponObject::ms_RTTI, apCollision)) &&
        ((arData.uDataA[3].i & 2) == 0 || !NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, apCollision->m_pkSceneObject))) {
        if ((arData.uDataA[3].i & 2) == 0 ||
            !apCollision->m_pkSceneObject ||
            !apCollision->m_pkSceneObject->m_kName.GetHandleString() ||
            BSstrcmp(apCollision->m_pkSceneObject->m_kName.GetHandleString(), "Arrow")) {
            ++arData.uDataA[0u].i;
        }
    }
}

int32_t GetCollisionObjectIndexInSceneGraph(
    NiAVObject* apRoot,
    bhkNiCollisionObject* apCollision,
    bool abIgnoreWeaponBone,
    bool abIgnoreFadeNodes,
    bool abIgnoreArrows
) {
    bhkWorldNS::ObjectRecData recData;
    recData.eAction = bhkWorldNS::OA_TOTAL;
    recData.bRecurse = true;
    recData.uDataA[0].i = 0;
    recData.uDataA[1].p = apCollision;
    recData.uDataA[2].i = -1;
    recData.uDataA[3].i = 0;
    if (abIgnoreWeaponBone) {
        recData.uDataA[3].i |= 1u;
    }
    if (abIgnoreFadeNodes) {
        recData.uDataA[3].i |= 2u;
    }
    if (abIgnoreArrows) {
        recData.uDataA[3].i |= 4u;
    }
    bhkWorld::DoObjectRec(apRoot, recData, &GetCollisionObjectIndex);
    if (recData.uDataA[2].i == -1) {
        return -1;
    }

    return recData.uDataA[2].i;
}

void GetCollisionObjectIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    if (((arData.uDataA[3].i & 1) == 0 || !NiObject::IsKindOf_0(&WeaponObject::ms_RTTI, apCollision)) &&
        ((arData.uDataA[3].i & 2) == 0 || !NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, apCollision->m_pkSceneObject))) {
        if ((arData.uDataA[3].i & 2) == 0 ||
            !apCollision->m_pkSceneObject ||
            !apCollision->m_pkSceneObject->m_kName.GetHandleString() ||
            BSstrcmp(apCollision->m_pkSceneObject->m_kName.GetHandleString(), "Arrow")) {
            if (apCollision == static_cast<bhkNiCollisionObject*>(arData.uDataA[1].p)) {
                arData.uDataA[2].i = arData.uDataA[0u].i;
            }
            ++arData.uDataA[0u].i;
        }
    }
}

bhkNiCollisionObject* GetCollisionObjectByIndexInSceneGraph(
    NiAVObject* apRoot,
    int32_t aiIndex,
    bool abIgnoreWeaponBone,
    bool abIgnoreFadeNodes,
    bool abIgnoreArrows
) {
    bhkWorldNS::ObjectRecData recData;
    recData.eAction = bhkWorldNS::OA_TOTAL;
    recData.bRecurse = true;
    recData.uDataA[0].i = 0;
    recData.uDataA[1].i = 0;
    recData.uDataA[2].i = aiIndex;
    recData.uDataA[3].i = 0;
    if (abIgnoreWeaponBone) {
        recData.uDataA[3].i |= 1u;
    }
    if (abIgnoreFadeNodes) {
        recData.uDataA[3].i |= 2u;
    }
    if (abIgnoreArrows) {
        recData.uDataA[3].i |= 4u;
    }
    bhkWorld::DoObjectRec(apRoot, recData, &GetCollisionObjectByIndex);
    return static_cast<bhkNiCollisionObject*>(recData.uDataA[1].p);
}

int32_t GetHavokWorldObjectCountInSceneGraph(NiAVObject* apRoot) {
    bhkWorldNS::ObjectRecData recData;
    recData.eAction = bhkWorldNS::OA_TOTAL;
    recData.bRecurse = true;
    recData.uDataA[0].i = 0;
    bhkWorld::DoObjectRec(apRoot, recData, &GetHavokWorldObjectCount);
    return recData.uDataA[0].i;
}

void GetHavokWorldObjectCount(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pWorldObject = apCollision->GetWorldObject();
    if (pWorldObject && pWorldObject->GethkObject()) {
        ++arData.uDataA[0u].i;
    }
}

int32_t GetHavokWorldObjectIndexInSceneGraph(NiAVObject* apRoot, hkpWorldObject* apWorldObject) {
    bhkWorldNS::ObjectRecData recData;
    recData.eAction = bhkWorldNS::OA_TOTAL;
    recData.bRecurse = true;
    recData.uDataA[0].i = 0;
    recData.uDataA[1].p = apWorldObject;
    recData.uDataA[2].i = -1;
    bhkWorld::DoObjectRec(apRoot, recData, &GetHavokWorldObjectIndex);
    if (recData.uDataA[2].i == -1) {
        return -1;
    }

    return recData.uDataA[2].i;
}

void GetHavokWorldObjectIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pWorldObject = apCollision->GetWorldObject();
    if (pWorldObject) {
        hkReferencedObject* phkObject = pWorldObject->GethkObject();
        if (phkObject) {
            if (phkObject == static_cast<hkReferencedObject*>(arData.uDataA[1].p)) {
                arData.uDataA[2].i = arData.uDataA[0u].i;
            }
            ++arData.uDataA[0u].i;
        }
    }
}

hkpWorldObject* GetHavokWorldObjectByIndexInSceneGraph(NiAVObject* apRoot, int32_t aiIndex) {
    bhkWorldNS::ObjectRecData recData;
    recData.eAction = bhkWorldNS::OA_TOTAL;
    recData.bRecurse = true;
    recData.uDataA[0].i = 0;
    recData.uDataA[1].i = 0;
    recData.uDataA[2].i = aiIndex;
    bhkWorld::DoObjectRec(apRoot, recData, &GetHavokWorldObjectByIndex);
    return static_cast<hkpWorldObject*>(recData.uDataA[1].p);
}

void GetHavokWorldObjectByIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pWorldObject = apCollision->GetWorldObject();
    if (pWorldObject) {
        hkpWorldObject* phkObject = static_cast<hkpWorldObject*>(pWorldObject->GethkObject());
        if (phkObject) {
            if (arData.uDataA[0u].i == arData.uDataA[2].i) {
                arData.uDataA[1].p = phkObject;
            }
            ++arData.uDataA[0u].i;
        }
    }
    ++arData.uDataA[0u].i;
}

const char* GetLanguageString(LANGUAGE aeLanguage) {
    return PipBoyLanguages[aeLanguage];
}

int32_t IsStringInteger(char* apString) {
    char cChar = *apString++;
    if ((cChar < '0' || cChar > '9') && cChar != '-') {
        return 0;
    }
    if (!*apString && cChar == '-') {
        return 0;
    }
    cChar = *apString++;
    while (cChar) {
        if (cChar < '0' || cChar > '9') {
            return 0;
        }
        cChar = *apString++;
    }
    return 1;
}

int32_t IsStringFloat(char* apString) {
    char cDecimal = 0;
    char cChar = *apString++;
    if (!cChar || ((cChar < '0' || cChar > '9') && cChar != '-' && cChar != '.')) {
        return 0;
    }
    if (!*apString && (cChar == '-' || cChar == '.')) {
        return 0;
    }
    if (cChar == '.') {
        cDecimal = 1;
    }
    cChar = *apString++;
    while (cChar) {
        if ((cChar < '0' || cChar > '9') && (cChar != '.' || cDecimal)) {
            return 0;
        }
        if (cChar == '.') {
            cDecimal = 1;
        }
        cChar = *apString++;
    }
    return 1;
}

// When the user name is unknown or does not fit in 260 bytes with its NUL, userNameG is left as it is.
void FillUserNameGlobal(const char* apAppName) {
    const std::string kUserName = platform::UserName();
    if (!kUserName.empty() && kUserName.size() < 260) {
        std::strcpy(userNameG, kUserName.c_str());
        char acMessage[264];
        InitializationFlow("%s", apAppName);
        std::sprintf(acMessage, "Computer Name: %s", userNameG);
        InitializationFlow(acMessage);
    }
}

NiFixedString* get_g_EditorMarker() {
    return FixedStrings::EditorMarker;
}

NiFixedString* get_g_EditorMarker0() {
    return FixedStrings::EditorMarker0;
}

NiFixedString* get_g_EditorMarker1() {
    return FixedStrings::EditorMarker1;
}

NiFixedString* get_g_EditorMarker2() {
    return FixedStrings::EditorMarker2;
}

// A bhkWorld::RecFunction, passed to bhkWorld::DoObjectRec, which takes the record data by reference.
void GetCollisionObjectByIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    if (((arData.uDataA[3].i & 1) == 0 || !NiObject::IsKindOf_0(&WeaponObject::ms_RTTI, apCollision)) &&
        ((arData.uDataA[3].i & 2) == 0 || !NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, apCollision->m_pkSceneObject))) {
        if ((arData.uDataA[3].i & 2) == 0 ||
            !apCollision->m_pkSceneObject ||
            !apCollision->m_pkSceneObject->m_kName.GetHandleString() ||
            BSstrcmp(apCollision->m_pkSceneObject->m_kName.GetHandleString(), "Arrow")) {
            if (arData.uDataA[0u].i == arData.uDataA[2].i) {
                arData.uDataA[1].p = apCollision;
            }
            ++arData.uDataA[0u].i;
        }
    }
}

bhkVector4::bhkVector4() {
    setZero4();
}

void bhkVector4::set(float afX, float afY, float afZ, float afW) {
    fX = afX;
    fY = afY;
    fZ = afZ;
    fW = afW;
}

void bhkVector4::setZero4() {
    set(0.0f, 0.0f, 0.0f, 0.0f);
}

void bhkVector4::setAll3(float afVal) {
    fX = afVal;
    fY = afVal;
    fZ = afVal;
}

hkVector4& bhkVector4::ToHK(hkVector4& arDst) const {
    arDst.set(fX, fY, fZ, fW);
    return arDst;
}

bhkVector4& bhkVector4::NI2HK(const NiPoint3& arSrc) {
    fX = bhkConvert::fBS2HkScaleSC * arSrc.x;
    fY = bhkConvert::fBS2HkScaleSC * arSrc.y;
    fZ = bhkConvert::fBS2HkScaleSC * arSrc.z;
    fW = 0.0;
    return *this;
}

bhkVector4& bhkVector4::operator=(const hkVector4& arSrc) {
    fX = arSrc.getSimdAt(0);
    fY = arSrc.getSimdAt(1);
    fZ = arSrc.getSimdAt(2);
    fW = arSrc.getSimdAt(3);
    return *this;
}

bhkCollisionObject* bhkCollisionObject::Getbhk(const hkpRigidBody* apBody) {
    bhkNiCollisionObject* pCollisionObject = bhkNiCollisionObject::Getbhk(apBody);
    return static_cast<bhkCollisionObject*>(
        NiObject::DynamicCastStatic(&bhkCollisionObject::ms_RTTI, pCollisionObject)
    );
}

// bhkVector4's constructor zeroes the vectors.
hkMouseSpringActionCinfo::hkMouseSpringActionCinfo()
  : hkUnaryActionCinfo(), m_springDamping(0.5f), m_springElasticity(0.3f), m_maxRelativeForce(250.0f),
    m_objectDamping(0.95f) {}

// The mass is rounded to a float local and returned from there.
float bhkRigidBody::GetMass() {
    hkpRigidBody* pBody = **this;
    float fMass = pBody ? pBody->getMass() : 0.0f;
    return fMass;
}

bhkUnaryAction::bhkUnaryAction() {
    ++iCount;
}

bhkUnaryAction::~bhkUnaryAction() {
    bhkSerializable::Kill();
    --iCount;
}

bhkWorldObject* bhkWorldObject::Getbhk(const hkpWorldObject* apObj) {
    if (apObj) {
        return apObj->getUserData();
    }

    return nullptr;
}

void FixedStrings::InitSDM() {
    FixedStrings::pBip01 = new NiFixedString("Bip01");
    FixedStrings::pBip = new NiFixedString("Bip");
    FixedStrings::pBip01_Head = new NiFixedString("Bip01 Head");
    FixedStrings::pBip01_Neck = new NiFixedString("Bip01 Neck");
    FixedStrings::Bip01_Spine = new NiFixedString("Bip01 Spine");
    FixedStrings::Bip01_Spine1 = new NiFixedString("Bip01 Spine1");
    FixedStrings::pMagicNode = new NiFixedString("magicNode");
    FixedStrings::SpecialIdle_Cast = new NiFixedString("SpecialIdle_Cast");
    FixedStrings::SpecialIdle_AreaEffect = new NiFixedString("SpecialIdle_AreaEffect");
    FixedStrings::AttachSound = new NiFixedString("AttachSound");
    FixedStrings::Sound_Marker = new NiFixedString("Sound Marker");
    FixedStrings::Skinned_Decal_Node = new NiFixedString("Skinned Decal Node");
    FixedStrings::Decal_Node = new NiFixedString("Decal Node");
    FixedStrings::ModelSwapNode = new NiFixedString("ModelSwapNode");
    FixedStrings::Open = new NiFixedString("Open");
    FixedStrings::Close = new NiFixedString("Close");
    BSDecalPlacementVectorExtraData::pTag = new NiFixedString("DVPG");
    FixedStrings::pPrn = new NiFixedString("Prn");
    FixedStrings::Weapon = new NiFixedString("Weapon");
    FixedStrings::EditorMarker = new NiFixedString("EditorMarker");
    FixedStrings::EditorMarker0 = new NiFixedString("EditorMarker:0");
    FixedStrings::EditorMarker1 = new NiFixedString("EditorMarker:1");
    FixedStrings::EditorMarker2 = new NiFixedString("EditorMarker:2");
    FixedStrings::ArrowQuiver = new NiFixedString("ArrowQuiver");
    FixedStrings::MarkerSource = new NiFixedString("MarkerSource");
    FixedStrings::MarkerTarget = new NiFixedString("MarkerTarget");
    FixedStrings::pAttachLight = new NiFixedString("AttachLight");
    FixedStrings::pSkin = new NiFixedString("skin");
    FixedStrings::pFaceGenEars = new NiFixedString("FaceGenEars");
    FixedStrings::pUnequip = new NiFixedString("Unequip");
    FixedStrings::pLaserSight = new NiFixedString("LaserSight");
    FixedStrings::pAimSight = new NiFixedString("AimSight");
    FixedStrings::pGrass = new NiFixedString("Grass");
    FixedStrings::pDecal = new NiFixedString(pcString);
    FixedStrings::pPermanentDecal = new NiFixedString("Permanent Decal");
    FixedStrings::pGrabLeft = new NiFixedString("grabLeft");
    FixedStrings::pGrabRight = new NiFixedString("grabRight");
    FixedStrings::pArrow0 = new NiFixedString("Arrow:0");
    FixedStrings::pArrowBone = new NiFixedString("ArrowBone");
    FixedStrings::pRaceHeadBiped = new NiFixedString("BSFaceGenNiNodeBiped");
    FixedStrings::pRaceHeadSkinned = new NiFixedString("BSFaceGenNiNodeSkinned");
    FixedStrings::pHeadAnims = new NiFixedString("HeadAnims:0");
    FixedStrings::pEntryPoint = new NiFixedString("EntryPoint");
    FixedStrings::pBip01LUpperArm = new NiFixedString("Bip01 L UpperArm");
    FixedStrings::pBip01LForearm = new NiFixedString("Bip01 L Forearm");
    FixedStrings::pBip01RUpperArm = new NiFixedString("Bip01 R UpperArm");
    FixedStrings::pBip01Yaw = new NiFixedString("Bip01 Yaw");
    FixedStrings::pBip01Pitch = new NiFixedString("Bip01 Pitch");
    FixedStrings::pBip01Looking = new NiFixedString("Bip01 Looking");
    FixedStrings::pTalking = new NiFixedString("Talking");
    FixedStrings::pCamera1st = new NiFixedString("Camera1st");
    FixedStrings::pCamera3rd = new NiFixedString("Camera3rd");
    FixedStrings::pHeadMeshExp = new NiFixedString("HeadMeshForExport_FaceGenAndMorphs:0");
    FixedStrings::pPinnedLimb = new NiFixedString("PinnedLimb");
}

void FixedStrings::KillSDM() {
    if (FixedStrings::pBip01) {
        delete FixedStrings::pBip01;
    }
    if (FixedStrings::pBip) {
        delete FixedStrings::pBip;
    }
    if (FixedStrings::pBip01_Head) {
        delete FixedStrings::pBip01_Head;
    }
    if (FixedStrings::pBip01_Neck) {
        delete FixedStrings::pBip01_Neck;
    }
    if (FixedStrings::pMagicNode) {
        delete FixedStrings::pMagicNode;
    }
    if (FixedStrings::SpecialIdle_Cast) {
        delete FixedStrings::SpecialIdle_Cast;
    }
    if (FixedStrings::SpecialIdle_AreaEffect) {
        delete FixedStrings::SpecialIdle_AreaEffect;
    }
    if (FixedStrings::AttachSound) {
        delete FixedStrings::AttachSound;
    }
    if (FixedStrings::Sound_Marker) {
        delete FixedStrings::Sound_Marker;
    }
    if (FixedStrings::Skinned_Decal_Node) {
        delete FixedStrings::Skinned_Decal_Node;
    }
    if (FixedStrings::Decal_Node) {
        delete FixedStrings::Decal_Node;
    }
    if (FixedStrings::ModelSwapNode) {
        delete FixedStrings::ModelSwapNode;
    }
    if (FixedStrings::Open) {
        delete FixedStrings::Open;
    }
    if (FixedStrings::Close) {
        delete FixedStrings::Close;
    }
    if (BSDecalPlacementVectorExtraData::pTag) {
        delete BSDecalPlacementVectorExtraData::pTag;
    }
    if (FixedStrings::pPrn) {
        delete FixedStrings::pPrn;
    }
    if (FixedStrings::Weapon) {
        delete FixedStrings::Weapon;
    }
    if (FixedStrings::EditorMarker) {
        delete FixedStrings::EditorMarker;
    }
    if (FixedStrings::EditorMarker0) {
        delete FixedStrings::EditorMarker0;
    }
    if (FixedStrings::EditorMarker1) {
        delete FixedStrings::EditorMarker1;
    }
    if (FixedStrings::EditorMarker2) {
        delete FixedStrings::EditorMarker2;
    }
    if (FixedStrings::ArrowQuiver) {
        delete FixedStrings::ArrowQuiver;
    }
    if (FixedStrings::MarkerSource) {
        delete FixedStrings::MarkerSource;
    }
    if (FixedStrings::MarkerTarget) {
        delete FixedStrings::MarkerTarget;
    }
    if (FixedStrings::pAttachLight) {
        delete FixedStrings::pAttachLight;
    }
    if (FixedStrings::pSkin) {
        delete FixedStrings::pSkin;
    }
    if (FixedStrings::pFaceGenEars) {
        delete FixedStrings::pFaceGenEars;
    }
    if (FixedStrings::pUnequip) {
        delete FixedStrings::pUnequip;
    }
    if (FixedStrings::pLaserSight) {
        delete FixedStrings::pLaserSight;
    }
    if (FixedStrings::pAimSight) {
        delete FixedStrings::pAimSight;
    }
    if (FixedStrings::pGrass) {
        delete FixedStrings::pGrass;
    }
    if (FixedStrings::pDecal) {
        delete FixedStrings::pDecal;
    }
    if (FixedStrings::pPermanentDecal) {
        delete FixedStrings::pPermanentDecal;
    }
    if (FixedStrings::pGrabLeft) {
        delete FixedStrings::pGrabLeft;
    }
    if (FixedStrings::pGrabRight) {
        delete FixedStrings::pGrabRight;
    }
    if (FixedStrings::pArrow0) {
        delete FixedStrings::pArrow0;
    }
    if (FixedStrings::pArrowBone) {
        delete FixedStrings::pArrowBone;
    }
    if (FixedStrings::pRaceHeadBiped) {
        delete FixedStrings::pRaceHeadBiped;
    }
    if (FixedStrings::pRaceHeadSkinned) {
        delete FixedStrings::pRaceHeadSkinned;
    }
    if (FixedStrings::pHeadAnims) {
        delete FixedStrings::pHeadAnims;
    }
    if (FixedStrings::pEntryPoint) {
        delete FixedStrings::pEntryPoint;
    }
    if (FixedStrings::pBip01LUpperArm) {
        delete FixedStrings::pBip01LUpperArm;
    }
    if (FixedStrings::pBip01LForearm) {
        delete FixedStrings::pBip01LForearm;
    }
    if (FixedStrings::pBip01RUpperArm) {
        delete FixedStrings::pBip01RUpperArm;
    }
    if (FixedStrings::pBip01Yaw) {
        delete FixedStrings::pBip01Yaw;
    }
    if (FixedStrings::pBip01Pitch) {
        delete FixedStrings::pBip01Pitch;
    }
    if (FixedStrings::pBip01Looking) {
        delete FixedStrings::pBip01Looking;
    }
    if (FixedStrings::pTalking) {
        delete FixedStrings::pTalking;
    }
    if (FixedStrings::pCamera1st) {
        delete FixedStrings::pCamera1st;
    }
    if (FixedStrings::pCamera3rd) {
        delete FixedStrings::pCamera3rd;
    }
    if (FixedStrings::pHeadMeshExp) {
        delete FixedStrings::pHeadMeshExp;
    }
    if (FixedStrings::pPinnedLimb) {
        delete FixedStrings::pPinnedLimb;
    }
}

void NiBound::SetCenter(float x, float y, float z) {
    m_kCenter = NiPoint3(x, y, z);
}

NiAVObject* NiCollisionObject::GetSceneGraphObject() {
    return m_pkSceneObject;
}

float NiASin(float fValue) {
    if (fValue > -1.0) {
        if (fValue < 1.0) {
            return std::asin(fValue);
        }
        return NI_HALF_PI;
    }
    return -NI_HALF_PI;
}

NiMatrix3* NiMatrix3::Inverse(NiMatrix3* apResult) const {
    NiMatrix3 kInverse;
    if (!Inverse(kInverse)) {
        kInverse.MakeZero();
    }
    *apResult = kInverse;
    return apResult;
}

void NiTransform::Invert(NiTransform& kDest) const {
    kDest.m_Rotate = m_Rotate.Transpose();
    kDest.m_fScale = 1.0f / m_fScale;
    NiPoint3 kNegated = -m_Translate;
    NiPoint3 kRotated = kDest.m_Rotate * kNegated;
    kDest.m_Translate = kDest.m_fScale * kRotated;
}
