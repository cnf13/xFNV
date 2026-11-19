#include "Fallout Shared/GlobalFunc.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
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
    if (!(afB > adA)) {
        return afB;
    }

    return adA;
}

float GetZAngleFromVector(const NiPoint3& aDirection) {
    float v4;
    float v5;
    if ((&aDirection)->y != 0.0) {
        v4 = (&aDirection)->x / (&aDirection)->y;
        v5 = std::atanf(v4);
        if ((&aDirection)->y < 0.0) {
            v5 = (float)(v5 + 3.1415927410125732);
        }
        return ClampAngle(v5);
    }
    if ((&aDirection)->x > 0.0) {
        return ClampAngle(1.5707963705062866f);
    }
    v5 = 4.71238899230957f;
    return ClampAngle(v5);
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
    int32_t ilen1 = std::strlen(apString1);
    int32_t ilen2 = std::strlen(apString2);
    for (int32_t i = 0; i < ilen1 - ilen2 + 1; ++i) {
        int32_t j;
        for (j = 0; j < ilen2; ++j) {
            int32_t v2 = std::tolower(apString1[j + i]);
            if (v2 != std::tolower(apString2[j])) {
                break;
            }
        }
        if (j == ilen2) {
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
    float v5 = afVal / afNearest;
    return (float)(((double)(((uint8_t)(!((v5 - ((double)((int32_t)v5))) < 0.5))) + ((int32_t)v5))) * afNearest);
}

float RandomFloat0To1() {
    BSRandom* singleton = BSRandom::GetSingleton();
    return singleton->Float0To1();
}

float RandomFloatTwoPi() {
    BSRandom* singleton = BSRandom::GetSingleton();
    return singleton->FloatTwoPi();
}

int32_t RandomInt(int32_t aiMin, int32_t aiMax) {
    BSRandom* singleton = BSRandom::GetSingleton();
    return (int32_t)singleton->UnsignedInt((uint32_t)aiMin, (uint32_t)aiMax);
}

void BSClamp(float& aVal, float aMin, float aMax) {
    if (!(aMax < ((double)aVal))) {
        if (aMin > ((double)aVal)) {
            aVal = aMin;
        }
    }
    else {
        aVal = aMax;
    }
}

float GetAngleDelta(float afStartAngle, float afTargetAngle, float& afOutDir) {
    float fDelta = afTargetAngle - afStartAngle;
    afOutDir = (float)0.0;
    if (fDelta == 0.0) {
        return fDelta;
    }
    afOutDir = (float)1.0;
    if (fDelta < 0.0) {
        if (!(fDelta > (-3.1415927410125732))) {
            return (float)(fDelta + 6.2831854820251465);
        }

        afOutDir = (-0x1p+0f);
    }
    else {
        if (fDelta > 3.1415927410125732) {
            afOutDir = (-0x1p+0f);
            return (float)(6.2831854820251465 - fDelta);
        }
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
    NiLines* plines;
    NiColorA* pcolors_2;
    NiPoint3* pverts_2;
    NiPoint3* pverts = static_cast<NiPoint3*>(ourMalloc(2 * sizeof(NiPoint3)));
    if (pverts) {
        rt::eh_vector_constructor_iterator(pverts, sizeof(NiPoint3), 2, &emptyPointConstructor);
        pverts_2 = pverts;
    }
    else {
        pverts_2 = nullptr;
    }
    *pverts_2 = aStart;
    pverts_2[1] = aEnd;
    NiColorA* pcolors = static_cast<NiColorA*>(ourMalloc(2 * sizeof(NiColorA)));
    if (pcolors) {
        ::new (static_cast<void*>(pcolors)) NiColorA[2];
        pcolors_2 = pcolors;
    }
    else {
        pcolors_2 = nullptr;
    }
    *pcolors_2 = aStartColor;
    pcolors_2[1] = aEndColor;
    uint8_t* pbools = static_cast<uint8_t*>(ourMalloc(2u));
    *pbools = 1;
    pbools[1] = 0;
    NiLines* v11 = static_cast<NiLines*>(NiMemObject::operator new(sizeof(NiLines)));
    if (v11) {
        plines = ::new (v11) NiLines(2, pverts_2, pcolors_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, pbools);
    }
    else {
        plines = nullptr;
    }
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pshprop;
        BSShaderNoLightingProperty* apMemory =
            static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
        if (apMemory) {
            pshprop = ::new (apMemory) BSShaderNoLightingProperty;
        }
        else {
            pshprop = nullptr;
        }
        plines->AttachProperty(pshprop);
        BSShaderManager::PrepareObject(plines, 0, 0);
    }
    return plines;
}

NiTriShape* MakeTriPoint(float afDrawSize, const NiColorA& aColor, bool abWithNoLightingProp) {
    NiTriShape* ptri;
    NiColorA* pcolors_2;
    NiPoint3* pverts;
    NiPoint3 v22;
    NiPoint3 v23;
    NiPoint3 v24;
    NiPoint3 v25;
    NiPoint3 v26;
    NiPoint3 dst;
    void* ptr = ourMalloc(6 * sizeof(NiPoint3));
    if (ptr) {
        rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 6, &emptyPointConstructor);
        pverts = static_cast<NiPoint3*>(ptr);
    }
    else {
        pverts = nullptr;
    }
    float v17 = afDrawSize * 1.4142135381698608;
    NiPoint3* v3 = ::new (&dst) NiPoint3(0.0f, 0.0f, v17);
    *pverts = *v3;
    NiPoint3* v4 = ::new (&v26) NiPoint3(-afDrawSize, afDrawSize, 0.0f);
    pverts[1] = *v4;
    NiPoint3* v5 = ::new (&v25) NiPoint3(afDrawSize, afDrawSize, 0.0f);
    pverts[2] = *v5;
    NiPoint3* v6 = ::new (&v24) NiPoint3(afDrawSize, -afDrawSize, 0.0f);
    pverts[3] = *v6;
    float ya = -afDrawSize;
    NiPoint3* v7 = ::new (&v23) NiPoint3(ya, ya, 0.0f);
    pverts[4] = *v7;
    float v16 = -afDrawSize * 1.4142135381698608;
    NiPoint3* v8 = ::new (&v22) NiPoint3(0.0f, 0.0f, v16);
    pverts[5] = *v8;
    NiColorA* pcolors = static_cast<NiColorA*>(ourMalloc(6 * sizeof(NiColorA)));
    if (pcolors) {
        ::new (static_cast<void*>(pcolors)) NiColorA[6];
        pcolors_2 = pcolors;
    }
    else {
        pcolors_2 = nullptr;
    }
    *pcolors_2 = aColor;
    pcolors_2[1] = aColor;
    pcolors_2[2] = aColor;
    pcolors_2[3] = aColor;
    pcolors_2[4] = aColor;
    pcolors_2[5] = aColor;
    uint16_t* ptrilist = static_cast<uint16_t*>(_NiMalloc(24 * sizeof(uint16_t)));
    *ptrilist = 0;
    ptrilist[1] = 2;
    ptrilist[2] = 1;
    ptrilist[3] = 0;
    ptrilist[4] = 3;
    ptrilist[5] = 2;
    ptrilist[6] = 0;
    ptrilist[7] = 4;
    ptrilist[8] = 3;
    ptrilist[9] = 0;
    ptrilist[10] = 1;
    ptrilist[11] = 4;
    ptrilist[12] = 1;
    ptrilist[13] = 2;
    ptrilist[14] = 5;
    ptrilist[15] = 2;
    ptrilist[16] = 3;
    ptrilist[17] = 5;
    ptrilist[18] = 3;
    ptrilist[19] = 4;
    ptrilist[20] = 5;
    ptrilist[21] = 4;
    ptrilist[22] = 1;
    ptrilist[23] = 5;
    NiTriShape* v20 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v20) {
        ptri = ::new (v20)
            NiTriShape(6, pverts, nullptr, pcolors_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 8, ptrilist);
    }
    else {
        ptri = nullptr;
    }
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pshprop;
        BSShaderNoLightingProperty* apMemory =
            static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
        if (apMemory) {
            pshprop = ::new (apMemory) BSShaderNoLightingProperty;
        }
        else {
            pshprop = nullptr;
        }
        NiProperty* pkProperty = pshprop;
        ptri->AttachProperty(pshprop);
        BSShaderManager::PrepareObject(ptri, 0, 0);
    }
    return ptri;
}

NiPoint3* CreateWorldVertices(NiGeometry* apGeom) {
    NiPoint3* preturn_2;
    if (!apGeom || !apGeom->GetVertexCount()) {
        return nullptr;
    }
    uint16_t VertexCount = apGeom->GetVertexCount();
    int32_t numObjs = VertexCount;
    NiPoint3* preturn = static_cast<NiPoint3*>(
        ourMalloc(VertexCount <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * VertexCount : SIZE_MAX)
    );
    if (preturn) {
        rt::eh_vector_constructor_iterator(preturn, sizeof(NiPoint3), numObjs, &emptyPointConstructor);
        preturn_2 = preturn;
    }
    else {
        preturn_2 = nullptr;
    }
    const NiTransform* WorldTransform = &apGeom->GetWorldTransform();
    NiPoint3* Vertices = apGeom->GetVertices();
    uint16_t v3 = apGeom->GetVertexCount();
    NiProcessorSpecificCode::TransformPoints(v3, &Vertices->x, &preturn_2->x, WorldTransform);
    return preturn_2;
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
    // Constructs a point local that is never used, as the game does.
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
        ourFree(pSourceWorld);
        pMap->~NiTMapVertexDist();
        return false;
    }
    // The inverse transform is computed but never read.
    apDest->GetWorldTransform().Invert(*pInverse);
    // Gets the source's transform before the destination's.
    const NiTransform& rSourceWorld = apSource->GetWorldTransform();
    NiMatrix3 kDestInverse;
    *pMatrix = apDest->GetWorldTransform().m_Rotate.Inverse(&kDestInverse)->operator*(rSourceWorld.m_Rotate);
    for (uint32_t i = 0; i < uiDestCount; ++i) {
        uint32_t uiNearest = uiSourceCount;
        // A negative or NaN tolerance means any distance.
        float fTolerance = afTolerance >= 0.0 ? afTolerance : 3.4028235e+38f;
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
        ourFree(pSourceWorld);
    }
    if (pDestWorld) {
        ourFree(pDestWorld);
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
    bhkWorld::DoObjectRec(apRoot, recData, &GetCollisionObjectByIndex);
    return recData.uDataA[0].i;
}

void SetReferenceIDOnScenegraph(NiAVObject* apObject, uint32_t auiID) {
    if (apObject) {
        if (apObject->IsGeometry()) {
            NiShadeProperty* Property = static_cast<NiShadeProperty*>(apObject->GetProperty(3));
            BSShaderLightingProperty* pShaderProp = BSSM_ShaderPropertyIsLighting(Property) != 0
                ? static_cast<BSShaderLightingProperty*>(Property)
                : nullptr;
            if (pShaderProp) {
                pShaderProp->uiReferenceID = auiID;
            }
        }
        else {
            NiNode* pnode = apObject->IsNode();
            if (pnode) {
                for (uint32_t n = 0; n < pnode->GetArrayCount(); ++n) {
                    NiAVObject* v2 = pnode->GetAt(n);
                    SetReferenceIDOnScenegraph(v2, auiID);
                }
            }
        }
    }
}

void UnTintScenegraph(NiAVObject* apRoot) {
    NiProperty* Property = apRoot->GetProperty(3);
    BSShaderPPLightingProperty* v5 = static_cast<BSShaderPPLightingProperty*>(
        NiObject::DynamicCastStatic(&BSShaderPPLightingProperty::ms_RTTI, Property)
    );
    if (v5) {
        v5->SetTexEffectData(nullptr);
    }
    NiNode* v4 = apRoot->IsNode();
    if (v4) {
        for (uint32_t a1 = 0; a1 < v4->GetArrayCount(); ++a1) {
            if (v4->GetAt(a1)) {
                NiAVObject* v2 = v4->GetAt(a1);
                UnTintScenegraph(v2);
            }
        }
    }
}

BSShaderPPLightingProperty* FindPerPixelLighting(NiNode* apNode) {
    if (!apNode->GetArrayCount()) {
        return nullptr;
    }
    for (uint32_t n = apNode->GetArrayCount() - 1;; --n) {
        NiAVObject* pobj = apNode->GetAt(n);
        if (pobj) {
            NiNode* pnode = pobj->IsNode();
            if (pnode) {
                NiAVObject* pchild = pnode->GetAtChecked(0);
                if (pchild) {
                    int32_t Type = NiShadeProperty::GetType();
                    NiShadeProperty* pproperty = static_cast<NiShadeProperty*>(pchild->GetProperty(Type));
                    if (pproperty) {
                        // Tests the per-pixel lighting pointer for null, then computes it again to return it.
                        int32_t v3 = pproperty->iShaderPropertyType >= 8 && pproperty->iShaderPropertyType <= 12;
                        if (v3 ? pproperty : nullptr) {
                            return v3 ? static_cast<BSShaderPPLightingProperty*>(pproperty) : nullptr;
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
    NiAVObject* v1;
    uint32_t a1;
    uint32_t arrayCount;
    NiNode* v5;
    bhkCollisionObject* pCollisionObject = nullptr;
    if (ap3D) {
        pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(ap3D);
        if (!pCollisionObject) {
            v5 = ap3D->IsNode();
            if (v5) {
                arrayCount = v5->GetArrayCount();
                for (a1 = 0; a1 < arrayCount; ++a1) {
                    v1 = v5->GetAt(a1);
                    pCollisionObject = FindFirstCollisionObject(v1);
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
    bool bParticles;
    bParticles = NiObject::IsKindOf(&NiParticleSystem::ms_RTTI, apObj);
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
    return (bool)((int8_t)BoundInFrustum(worldBound, apCamera));
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
    if (aStartPt.x < (double)aTestRect.left && aEndPt.x < (double)aTestRect.left) {
        return false;
    }
    if (aStartPt.y < (double)aTestRect.bottom && aEndPt.y < (double)aTestRect.bottom) {
        return false;
    }
    if (aStartPt.x > (double)aTestRect.right && aEndPt.x > (double)aTestRect.right) {
        return false;
    }
    if (aStartPt.y > (double)aTestRect.top && aEndPt.y > (double)aTestRect.top) {
        return false;
    }
    if (aStartPt.x > (double)aTestRect.left && aStartPt.x < (double)aTestRect.right &&
        aStartPt.y > (double)aTestRect.bottom && aStartPt.y < (double)aTestRect.top &&
        aEndPt.x > (double)aTestRect.left && aEndPt.x < (double)aTestRect.right &&
        aEndPt.y > (double)aTestRect.bottom && aEndPt.y < (double)aTestRect.top) {
        return false;
    }
    if (aStartPt.x == aEndPt.x || aStartPt.y == aEndPt.y) {
        if (aStartPt.x == aEndPt.x && aStartPt.y == aEndPt.y) {
            if (aStartPt.x <= (double)aTestRect.right && aStartPt.x >= (double)aTestRect.left &&
                aStartPt.y <= (double)aTestRect.top && aStartPt.y >= (double)aTestRect.bottom) {
                bResult = true;
                aOutputIntersectPt = aStartPt;
            }
        }
        else if (aStartPt.x == aEndPt.x) {
            if (aStartPt.x <= (double)aTestRect.right && aStartPt.x >= (double)aTestRect.left) {
                bResult = true;
                // asm: falls through to the top edge only when aEndPt->y < aStartPt->y (NaN takes the bottom).
                if (aEndPt.y < aStartPt.y) {
                    aOutputIntersectPt = NiPoint3(aStartPt.x, (float)aTestRect.top, 0.0f);
                }
                else {
                    aOutputIntersectPt = NiPoint3(aStartPt.x, (float)aTestRect.bottom, 0.0f);
                }
            }
        }
        else if (
            aStartPt.y == aEndPt.y && aStartPt.y <= (double)aTestRect.top && aStartPt.y >= (double)aTestRect.bottom
        ) {
            bResult = true;
            // asm: the right edge only when aEndPt->x < aStartPt->x (NaN takes the left).
            if (aEndPt.x < aStartPt.x) {
                aOutputIntersectPt = NiPoint3((float)aTestRect.right, aStartPt.y, 0.0f);
            }
            else {
                aOutputIntersectPt = NiPoint3((float)aTestRect.left, aStartPt.y, 0.0f);
            }
        }
    }
    else {
        int32_t iHits = 0;
        NiPoint3 kHit0(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
        NiPoint3 kHit1(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
        float fSlope = (aEndPt.y - aStartPt.y) / (aEndPt.x - aStartPt.x);
        float fIntercept = aEndPt.y - aEndPt.x * fSlope;
        if (iHits < 2) {
            float fY = (float)((double)aTestRect.left * fSlope + fIntercept);
            if (fY <= (double)aTestRect.top && fY >= (double)aTestRect.bottom) {
                if (iHits == 0) {
                    kHit0 = NiPoint3((float)aTestRect.left, fY, 0.0f);
                }
                else {
                    kHit1 = NiPoint3((float)aTestRect.left, fY, 0.0f);
                }
                ++iHits;
            }
        }
        if (iHits < 2) {
            float fX = (float)(((double)aTestRect.top - fIntercept) / fSlope);
            if (fX <= (double)aTestRect.right && fX >= (double)aTestRect.left) {
                if (iHits == 0) {
                    kHit0 = NiPoint3(fX, (float)aTestRect.top, 0.0f);
                }
                else {
                    kHit1 = NiPoint3(fX, (float)aTestRect.top, 0.0f);
                }
                ++iHits;
            }
        }
        if (iHits < 2) {
            float fY = (float)((double)aTestRect.right * fSlope + fIntercept);
            if (fY <= (double)aTestRect.top && fY >= (double)aTestRect.bottom) {
                if (iHits == 0) {
                    kHit0 = NiPoint3((float)aTestRect.right, fY, 0.0f);
                }
                else {
                    kHit1 = NiPoint3((float)aTestRect.right, fY, 0.0f);
                }
                ++iHits;
            }
        }
        if (iHits < 2) {
            float fX = (float)(((double)aTestRect.bottom - fIntercept) / fSlope);
            if (fX <= (double)aTestRect.right && fX >= (double)aTestRect.left) {
                if (iHits == 0) {
                    kHit0 = NiPoint3(fX, (float)aTestRect.bottom, 0.0f);
                }
                else {
                    kHit1 = NiPoint3(fX, (float)aTestRect.bottom, 0.0f);
                }
                ++iHits;
            }
        }
        if (iHits != 0) {
            bResult = true;
            if (iHits == 2) {
                if (aStartPt.x > (double)aTestRect.left && aStartPt.x < (double)aTestRect.right &&
                    aStartPt.y > (double)aTestRect.bottom && aStartPt.y < (double)aTestRect.top) {
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
    NiNode* v4 = apObj->IsNode();
    if (v4) {
        for (uint32_t a1 = 0; a1 < v4->GetArrayCount(); ++a1) {
            NiNode* apObja = static_cast<NiNode*>(v4->GetAt(a1));
            if (HasMorpherController(apObja)) {
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
    bool v7 = false;
    if ((apPick) && (apTarget) && (afDistance != 0.0)) {
        if (abClearResults) {
            apPick->SetTarget(nullptr);
            apPick->PickObjects(NiPoint3_ZERO, NiPoint3_ZERO, false);
        }
        return PickDistanceRecursive(apPick, aOrigin, aDir, afDistance, apTarget);
    }

    return v7;
}

bool PickDistanceRecursive(
    NiPick* apPick,
    const NiPoint3& aOrigin,
    const NiPoint3& aDir,
    float afDistance,
    NiAVObject* apTarget
) {
    bool bhitsomething = false;
    if (apPick && (apTarget && (!apPick->m_bObserveAppCullFlag || !apTarget->GetAppCulled()))) {
        if (!apTarget->IsVisualObject()) {
            return false;
        }
        NiBound targetbound = apTarget->GetWorldBound();
        NiPoint3 ptdiff = targetbound.m_kCenter - aOrigin;
        double v7 = ptdiff.Length();
        if (targetbound.m_fRadius + afDistance > v7) {
            NiNode* ptargetnode = apTarget->IsNode();
            if (ptargetnode) {
                int32_t ichildcount = ptargetnode->GetArrayCount();
                for (int32_t i = 0; i < ichildcount; ++i) {
                    NiAVObject* v6 = ptargetnode->GetAt(static_cast<uint32_t>(i));
                    if (PickDistanceRecursive(apPick, aOrigin, aDir, afDistance, v6)) {
                        bhitsomething = true;
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
    return bhitsomething;
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
    for (uint32_t uiplane = 0; uiplane < 6; ++uiplane) {
        NiPlane plane = cullPlanes.GetPlane(uiplane);
        if (aBound.WhichSide(plane) == 2) {
            return false;
        }
    }
    return true;
}

bool ReturnTrue() {
    return true;
}

const char* convert_to_dword(NiFixedString* apString) {
    return apString->GetHandle();
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
    NiPoint3 location;
    NiAVObject* p3D = nullptr;
    if (aphkCol) {
        hkpPhantom* pphantom;
        hkpRigidBody* pbody = hkpGetRigidBody(aphkCol);
        if (pbody) {
            pphantom = nullptr;
        }
        else {
            pphantom = hkpGetPhantom(aphkCol);
        }
        hkpWorldObject* apBody = (hkpWorldObject*)pphantom;
        if (pbody) {
            bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(pbody);
            if (pcol) {
                return pcol->m_pkSceneObject;
            }

            uint32_t CollisionFilterInfo = aphkCol->getCollisionFilterInfo();
            CFilter filter(CollisionFilterInfo);
            if (filter.GetGroup() == 1) {
                const hkVector4* Translation = &pbody->getTranslation();
                HK2NI(location, *Translation);
                NiAVObject* pland3D = (NiAVObject*)TES::pInstance->GetLandGeometry(location);
                if ((pland3D) && (pland3D->m_pkParent)) {
                    return (NiAVObject*)pland3D->m_pkParent;
                }
            }
        }
        else {
            bhkPCollisionObject* pcol_2 = bhkUtilFunctions::IsbhkPCollisionObject(apBody);
            if (pcol_2) {
                p3D = pcol_2->m_pkSceneObject;
            }
            if (!p3D) {
                bhkEntity* pbp = bhkEntity::Getbhk(apBody);
                if (pbp) {
                    return pbp->GetAVObject(0);
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
    if (!apInPath || (!apOutPath || !aiOutPathSize)) {
        return false;
    }
    bool bretval = false;
    *apOutPath = 0;
    uint32_t ioutlen = 0;
    int32_t eplatform = GetIsControllerConnected();
    LANGUAGE elang = GetCurrentTexLanguage();
    const char* psearch = apInPath;
    for (int32_t i = 0; i < 4; ++i) {
        uint32_t ilen;
        const char* pfind;
        char cslash = 0;
        for (pfind = BSstristr(apInPath, PipBoyPlatforms[i]); pfind;
             pfind = BSstristr(&pfind[ilen], PipBoyPlatforms[i])) {
            ilen = BSstrlen(PipBoyPlatforms[i]);
            if (pfind[ilen] == 92 || pfind[ilen] == 47) {
                cslash = pfind[ilen];
                break;
            }
        }
        if (pfind) {
            uint32_t ilen_2 = pfind - apInPath;
            if (pfind != apInPath) {
                BSmemcpy(apOutPath, static_cast<int32_t>(aiOutPathSize), apInPath, static_cast<int32_t>(ilen_2));
                ioutlen = pfind - apInPath;
                apOutPath[ilen_2] = 0;
            }
            BSstrcat(apOutPath, static_cast<int32_t>(aiOutPathSize), PipBoyPlatforms[eplatform]);
            uint32_t v16 = ioutlen + std::strlen(PipBoyPlatforms[eplatform]);
            apOutPath[v16] = cslash;
            ioutlen = v16 + 1;
            apOutPath[ioutlen] = 0;
            psearch = &pfind[std::strlen(PipBoyPlatforms[i]) + 1];
            bretval = true;
            break;
        }
    }
    for (int32_t i_2 = 0; i_2 < 5; ++i_2) {
        uint32_t ilen_3;
        const char* pfind_2;
        char cslash_2 = 0;
        for (pfind_2 = BSstristr(psearch, PipBoyLanguages[i_2]); pfind_2;
             pfind_2 = BSstristr(&pfind_2[ilen_3], PipBoyLanguages[i_2])) {
            ilen_3 = BSstrlen(PipBoyLanguages[i_2]);
            if (pfind_2[ilen_3] == 92 || pfind_2[ilen_3] == 47) {
                cslash_2 = pfind_2[ilen_3];
                break;
            }
        }
        if (pfind_2) {
            uint32_t ilen_4 = pfind_2 - psearch;
            if (pfind_2 != psearch) {
                // The remaining size is computed unsigned and passed as BSmemcpy's int size.
                BSmemcpy(
                    &apOutPath[ioutlen], static_cast<int32_t>(aiOutPathSize - ioutlen), psearch,
                    static_cast<int32_t>(ilen_4)
                );
                ioutlen += ilen_4;
                apOutPath[ioutlen] = 0;
            }
            BSstrcat(apOutPath, static_cast<int32_t>(aiOutPathSize), PipBoyLanguages[elang]);
            uint32_t v17 = ioutlen + std::strlen(PipBoyLanguages[elang]);
            apOutPath[v17] = cslash_2;
            apOutPath[v17 + 1] = 0;
            psearch = &pfind_2[std::strlen(PipBoyLanguages[i_2]) + 1];
            bretval = true;
            break;
        }
    }
    BSstrcat(apOutPath, static_cast<int32_t>(aiOutPathSize), psearch);
    return bretval;
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
    BSShaderPPLightingProperty* pShaderProperty =
        bIsLighting ? static_cast<BSShaderPPLightingProperty*>(pProperty) : nullptr;
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
                    NiPointer<NiSourceTexture>* pspTexture =
                        ::new (aTextureStorage) NiPointer<NiSourceTexture>(nullptr);
                    TES::pInstance->CreateTextureImage(acPath, *pspTexture, false, false);
                    if (pspTexture->operator->()) {
                        pShaderProperty->SetTexture(eType, i, pspTexture->operator->());
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
        NiAVObject* pobj = apNode->GetAt(n);
        if (pobj) {
            const char* pname = (&pobj->m_kName)->GetHandleString();
            if (pname) {
                if (!BSstricmp(pname, "Scb")) {
                    apNode->DetachChildAt(n);
                    return true;
                }
                if (!_strnicmp(pname, "FadeNode ", 9u)) {
                    NiNode* v2 = pobj->IsNode();
                    return RemoveScabard(v2);
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
            bool bNode;
            bNode = NiObject::IsKindOf(&NiNode::ms_RTTI, pChild);
            if (bNode) {
                SetPhase(static_cast<NiNode*>(pChild), afPhase);
            }
        }
    }
}

void RecursivelyRemoveAllDecals(NiAVObject* apRoot) {
    if (apRoot) {
        if (apRoot->IsTriBasedGeom()) {
            NiShadeProperty* pprop = static_cast<NiShadeProperty*>(apRoot->GetProperty(3));
            if (pprop) {
                NiShadeProperty* pShaderProperty = BSSM_ShaderPropertyIsLighting(pprop) != 0 ? pprop : nullptr;
                if (pShaderProperty) {}
            }
        }
        else {
            NiNode* pnode = apRoot->IsNode();
            if (pnode) {
                for (uint32_t i = 0; i < pnode->GetArrayCount(); ++i) {
                    NiAVObject* v1 = pnode->GetAt(i);
                    RecursivelyRemoveAllDecals(v1);
                }
            }
        }
    }
}

void AddNoLightingPropertyRecurse(NiAVObject* apObject) {
    if ((apObject->IsGeometry()) && (!apObject->GetProperty(3))) {
        BSShaderNoLightingProperty* v1;
        BSShaderNoLightingProperty* apMemory =
            static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
        if (apMemory) {
            v1 = ::new (apMemory) BSShaderNoLightingProperty;
        }
        else {
            v1 = nullptr;
        }
        apObject->AttachProperty(v1);
    }

    NiNode* v5 = apObject->IsNode();
    if (v5) {
        for (uint32_t a1 = 0; a1 < v5->GetArrayCount(); ++a1) {
            NiGeometry* v3 = static_cast<NiGeometry*>(v5->GetAt(a1));
            if (v3) {
                AddNoLightingPropertyRecurse(v3);
            }
        }
    }
}

void PrepareObjectWithNoLightingRecurse(NiAVObject* apObject) {
    if (!apObject->IsGeometry() || apObject->GetProperty(3)) {
        NiNode* v5 = apObject->IsNode();
        if ((v5) && (v5->IsVisualObject())) {
            for (uint32_t auiIndex = 0; auiIndex < v5->GetArrayCount(); ++auiIndex) {
                NiAVObject* apObjecta = v5->GetAt(auiIndex);
                if (apObjecta) {
                    PrepareObjectWithNoLightingRecurse(apObjecta);
                }
            }
        }
    }
    else {
        BSShaderNoLightingProperty* v1;
        BSShaderNoLightingProperty* apMemory =
            static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
        if (apMemory) {
            v1 = ::new (apMemory) BSShaderNoLightingProperty;
        }
        else {
            v1 = nullptr;
        }
        v1->SetFlag(48u, 1);
        apObject->AttachProperty(v1);
        apObject->UpdateProperties();
        BSShaderManager::PrepareObject(apObject, 0, 0);
    }
}

float GetUnclampedZAngleFromVector(const NiPoint3& aDirection) {
    float v2;
    float v3;
    if ((&aDirection)->y == 0.0) {
        if (!((&aDirection)->x > 0.0)) {
            return 4.71238899230957f;
        }

        return 1.5707963705062866f;
    }

    v2 = (&aDirection)->x / (&aDirection)->y;
    v3 = std::atanf(v2);
    if ((&aDirection)->y < 0.0) {
        return (float)(v3 + 3.1415927410125732);
    }

    return v3;
}

LANGUAGE GetCurrentTexLanguage() {
    return GetCurrentLanguage();
}

OBJECT_TYPE FormIDToObjectType(uint8_t acFormID) {
    OBJECT_TYPE result;
    switch (acFormID) {
        case 4: {
            result = OBJECT_TEXTURESET;
            break;
        }
        case 5: {
            result = OBJECT_MENUICON;
            break;
        }
        case 7: {
            result = OBJECT_CLASS;
            break;
        }
        case 8: {
            result = OBJECT_FACTION;
            break;
        }
        case 9: {
            result = OBJECT_HEADPART;
            break;
        }
        case 11: {
            result = OBJECT_EYES;
            break;
        }
        case 12: {
            result = OBJECT_RACE;
            break;
        }
        case 13: {
            result = OBJECT_SOUND;
            break;
        }
        case 14: {
            result = OBJECT_ACOUSTICSPACE;
            break;
        }
        case 16: {
            result = OBJECT_MAGICEFFECT;
            break;
        }
        case 18: {
            result = OBJECT_LANDTEXTURE;
            break;
        }
        case 19: {
            result = OBJECT_ENCHANTING;
            break;
        }
        case 20: {
            result = OBJECT_SPELLMAKING;
            break;
        }
        case 21: {
            result = OBJECT_ACTIVATOR;
            break;
        }
        case 22: {
            result = OBJECT_TALKINGACTIVATOR;
            break;
        }
        case 23: {
            result = OBJECT_TERMINAL;
            break;
        }
        case 24: {
            result = OBJECT_ARMOR;
            break;
        }
        case 25: {
            result = OBJECT_BOOK;
            break;
        }
        case 27: {
            result = OBJECT_CONTAINER;
            break;
        }
        case 28: {
            result = OBJECT_DOOR;
            break;
        }
        case 29: {
            result = OBJECT_INGREDIENT;
            break;
        }
        case 30: {
            result = OBJECT_LIGHT;
            break;
        }
        case 31: {
            result = OBJECT_MISCITEM;
            break;
        }
        case 32: {
            result = OBJECT_STATIC;
            break;
        }
        case 33: {
            result = OBJECT_STATICCOLLECTION;
            break;
        }
        case 34: {
            result = OBJECT_MOVABLESTATIC;
            break;
        }
        case 35: {
            result = OBJECT_PLACEABLEWATER;
            break;
        }
        case 36: {
            result = OBJECT_GRASS;
            break;
        }
        case 37: {
            result = OBJECT_TREE;
            break;
        }
        case 38: {
            result = OBJECT_FLORA;
            break;
        }
        case 39: {
            result = OBJECT_FURNITURE;
            break;
        }
        case 40: {
            result = OBJECT_WEAPON;
            break;
        }
        case 41: {
            result = OBJECT_AMMO;
            break;
        }
        case 42: {
            result = OBJECT_NPC;
            break;
        }
        case 43: {
            result = OBJECT_CREATURE;
            break;
        }
        case 44: {
            result = OBJECT_LEVCREATURE;
            break;
        }
        case 45: {
            result = OBJECT_LEVCHARACTER;
            break;
        }
        case 46: {
            result = OBJECT_KEY;
            break;
        }
        case 47: {
            result = OBJECT_ALCHEMY;
            break;
        }
        case 48: {
            result = OBJECT_IDLEMARKER;
            break;
        }
        case 49: {
            result = OBJECT_NOTE;
            break;
        }
        case 50: {
            result = OBJECT_CONSTRUCTIBLE;
            break;
        }
        case 51: {
            result = OBJECT_PROJECTILE;
            break;
        }
        case 52: {
            result = OBJECT_LEVITEM;
            break;
        }
        case 53: {
            result = OBJECT_WEATHER;
            break;
        }
        case 54: {
            result = OBJECT_CLIMATE;
            break;
        }
        case 71: {
            result = OBJECT_QUEST;
            break;
        }
        case 73: {
            result = OBJECT_PACKAGE;
            break;
        }
        case 74: {
            result = OBJECT_COMBATSTYLE;
            break;
        }
        case 75: {
            result = OBJECT_LOADSCREEN;
            break;
        }
        case 76: {
            result = OBJECT_LEVSPELL;
            break;
        }
        case 77: {
            result = OBJECT_ANIMOBJECT;
            break;
        }
        case 78: {
            result = OBJECT_WATER;
            break;
        }
        case 79: {
            result = OBJECT_EFFECTSHADER;
            break;
        }
        case 81: {
            result = OBJECT_EXPLOSION;
            break;
        }
        case 82: {
            result = OBJECT_DEBRIS;
            break;
        }
        case 85: {
            result = OBJECT_FORMLIST;
            break;
        }
        case 86: {
            result = OBJECT_PERK;
            break;
        }
        case 87: {
            result = OBJECT_BODYPARTDATA;
            break;
        }
        case 88: {
            result = OBJECT_ADDON;
            break;
        }
        case 91: {
            result = OBJECT_CAMERASHOT;
            break;
        }
        case 93: {
            result = OBJECT_VOICETYPE;
            break;
        }
        case 94: {
            result = OBJECT_IMPACTDATA;
            break;
        }
        case 95: {
            result = OBJECT_IMPACTDATASET;
            break;
        }
        case 97: {
            result = OBJECT_ENCOUNTERZONE;
            break;
        }
        case 102: {
            result = OBJECT_MUSICTYPE;
            break;
        }
        case 103: {
            result = OBJECT_MOD;
            break;
        }
        case 104: {
            result = OBJECT_REPUTATION;
            break;
        }
        case 106: {
            result = OBJECT_RECIPE;
            break;
        }
        case 107: {
            result = OBJECT_RECIPE_CATEGORY;
            break;
        }
        case 108: {
            result = OBJECT_CHIP;
            break;
        }
        case 109: {
            result = OBJECT_CASINO;
            break;
        }
        case 110: {
            result = OBJECT_LOADSCREENTYPE;
            break;
        }
        case 111: {
            result = OBJECT_MEDIASET;
            break;
        }
        case 112: {
            result = OBJECT_MEDIALOCATIONCONTROLLER;
            break;
        }
        case 113: {
            result = OBJECT_CHALLENGE;
            break;
        }
        case 114: {
            result = OBJECT_AMMO_EFFECT;
            break;
        }
        case 115: {
            result = OBJECT_CARAVANCARD;
            break;
        }
        case 116: {
            result = OBJECT_CARAVANMONEY;
            break;
        }
        case 117: {
            result = OBJECT_CARAVANDECK;
            break;
        }
        default: {
            result = OBJECT_NONE;
            break;
        }
    }
    return result;
}

char* FormIDToString(uint8_t acFormID) {
    char* result;
    OBJECT_TYPE oType = FormIDToObjectType(acFormID);
    if (oType >= OBJECT_ACTIVATOR) {
        return objectLabel[oType];
    }
    switch (acFormID) {
        case 3: {
            result = formString[9];
            break;
        }
        case 6: {
            result = formString[7];
            break;
        }
        case 7: {
            result = formString[1];
            break;
        }
        case 8: {
            result = formString[5];
            break;
        }
        case 9: {
            result = formString[16];
            break;
        }
        case 10: {
            result = formString[17];
            break;
        }
        case 11: {
            result = formString[18];
            break;
        }
        case 12: {
            result = formString[0];
            break;
        }
        case 13: {
            result = formString[6];
            break;
        }
        case 14: {
            result = formString[25];
            break;
        }
        case 15: {
            result = formString[14];
            break;
        }
        case 16: {
            result = formString[10];
            break;
        }
        case 17: {
            result = formString[2];
            break;
        }
        case 49: {
            result = formString[22];
            break;
        }
        case 53: {
            result = formString[13];
            break;
        }
        case 54: {
            result = formString[12];
            break;
        }
        case 55: {
            result = formString[8];
            break;
        }
        case 57: {
            result = formString[4];
            break;
        }
        case 58: {
            result = formString[19];
            break;
        }
        case 65: {
            result = formString[11];
            break;
        }
        case 69: {
            result = formString[3];
            break;
        }
        case 71: {
            result = formString[15];
            break;
        }
        case 72: {
            result = formString[20];
            break;
        }
        case 73: {
            result = formString[21];
            break;
        }
        case 83: {
            result = formString[23];
            break;
        }
        case 84: {
            result = formString[24];
            break;
        }
        case 102: {
            result = formString[26];
            break;
        }
        case 104: {
            result = formString[27];
            break;
        }
        case 106: {
            result = formString[28];
            break;
        }
        case 107: {
            result = formString[29];
            break;
        }
        case 108: {
            result = formString[30];
            break;
        }
        case 109: {
            result = formString[31];
            break;
        }
        case 110: {
            result = formString[32];
            break;
        }
        case 111: {
            result = formString[33];
            break;
        }
        case 112: {
            result = formString[34];
            break;
        }
        case 113: {
            result = formString[35];
            break;
        }
        case 114: {
            result = formString[36];
            break;
        }
        case 115: {
            result = formString[37];
            break;
        }
        case 116: {
            result = formString[38];
            break;
        }
        case 117: {
            result = formString[39];
            break;
        }
        default: {
            if (acFormID >= 121u) {
                Warning("FORMS: Invalid FormID sent to FormIDToString().\r\n", formEnumString[acFormID].pFormString);
                acFormID = 0;
            }
            result = formEnumString[acFormID].pFormString;
            break;
        }
    }
    return result;
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
    NiTriShape* ptri;
    NiColorA* pcolors_2;
    NiPoint3* pverts_2;
    NiPoint3* pverts = static_cast<NiPoint3*>(ourMalloc(8 * sizeof(NiPoint3)));
    if (pverts) {
        rt::eh_vector_constructor_iterator(pverts, sizeof(NiPoint3), 8, &emptyPointConstructor);
        pverts_2 = pverts;
    }
    else {
        pverts_2 = nullptr;
    }
    *pverts_2 = aPoint1;
    pverts_2[1] = aPoint2;
    pverts_2[2] = aPoint3;
    pverts_2[3] = aPoint4;
    pverts_2[4] = aPoint5;
    pverts_2[5] = aPoint6;
    pverts_2[6] = aPoint7;
    pverts_2[7] = aPoint8;
    NiColorA* pcolors = static_cast<NiColorA*>(ourMalloc(8 * sizeof(NiColorA)));
    if (pcolors) {
        ::new (static_cast<void*>(pcolors)) NiColorA[8];
        pcolors_2 = pcolors;
    }
    else {
        pcolors_2 = nullptr;
    }
    *pcolors_2 = aColor;
    pcolors_2[1] = aColor;
    pcolors_2[2] = aColor;
    pcolors_2[3] = aColor;
    pcolors_2[4] = aColor;
    pcolors_2[5] = aColor;
    pcolors_2[6] = aColor;
    pcolors_2[7] = aColor;
    uint16_t* ptrilist = static_cast<uint16_t*>(_NiMalloc(72 * sizeof(uint16_t)));
    *ptrilist = 0;
    ptrilist[1] = 2;
    ptrilist[2] = 1;
    ptrilist[3] = 0;
    ptrilist[4] = 3;
    ptrilist[5] = 2;
    ptrilist[6] = 0;
    ptrilist[7] = 7;
    ptrilist[8] = 3;
    ptrilist[9] = 0;
    ptrilist[10] = 4;
    ptrilist[11] = 7;
    ptrilist[12] = 1;
    ptrilist[13] = 4;
    ptrilist[14] = 0;
    ptrilist[15] = 1;
    ptrilist[16] = 5;
    ptrilist[17] = 4;
    ptrilist[18] = 2;
    ptrilist[19] = 5;
    ptrilist[20] = 1;
    ptrilist[21] = 2;
    ptrilist[22] = 6;
    ptrilist[23] = 5;
    ptrilist[24] = 3;
    ptrilist[25] = 6;
    ptrilist[26] = 2;
    ptrilist[27] = 3;
    ptrilist[28] = 7;
    ptrilist[29] = 6;
    ptrilist[30] = 4;
    ptrilist[31] = 7;
    ptrilist[32] = 6;
    ptrilist[33] = 4;
    ptrilist[34] = 6;
    ptrilist[35] = 5;
    ptrilist[36] = 0;
    ptrilist[37] = 1;
    ptrilist[38] = 2;
    ptrilist[39] = 0;
    ptrilist[40] = 2;
    ptrilist[41] = 3;
    ptrilist[42] = 0;
    ptrilist[43] = 3;
    ptrilist[44] = 7;
    ptrilist[45] = 0;
    ptrilist[46] = 7;
    ptrilist[47] = 4;
    ptrilist[48] = 1;
    ptrilist[49] = 0;
    ptrilist[50] = 4;
    ptrilist[51] = 1;
    ptrilist[52] = 4;
    ptrilist[53] = 5;
    ptrilist[54] = 2;
    ptrilist[55] = 1;
    ptrilist[56] = 5;
    ptrilist[57] = 2;
    ptrilist[58] = 5;
    ptrilist[59] = 6;
    ptrilist[60] = 3;
    ptrilist[61] = 2;
    ptrilist[62] = 6;
    ptrilist[63] = 3;
    ptrilist[64] = 6;
    ptrilist[65] = 7;
    ptrilist[66] = 4;
    ptrilist[67] = 6;
    ptrilist[68] = 7;
    ptrilist[69] = 4;
    ptrilist[70] = 5;
    ptrilist[71] = 6;
    NiTriShape* apMemory = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (apMemory) {
        ptri = ::new (apMemory)
            NiTriShape(8, pverts_2, nullptr, pcolors_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 24, ptrilist);
    }
    else {
        ptri = nullptr;
    }
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pshprop;
        BSShaderNoLightingProperty* pShader =
            static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
        if (pShader) {
            pshprop = ::new (pShader) BSShaderNoLightingProperty;
        }
        else {
            pshprop = nullptr;
        }
        ptri->AttachProperty(pshprop);
        BSShaderManager::PrepareObject(ptri, 0, 0);
    }
    return ptri;
}

NiTriShape*
MakeTriangle(NiPoint3 aPt1, NiPoint3 aPt2, NiPoint3 aPt3, const NiColorA& aColor, bool abWithNoLightingProp) {
    NiTriShape* ptri;
    NiColorA* pcolors_2;
    NiPoint3* pverts_2;
    NiPoint3* pverts = static_cast<NiPoint3*>(ourMalloc(3 * sizeof(NiPoint3)));
    if (pverts) {
        rt::eh_vector_constructor_iterator(pverts, sizeof(NiPoint3), 3, &emptyPointConstructor);
        pverts_2 = pverts;
    }
    else {
        pverts_2 = nullptr;
    }
    *pverts_2 = aPt1;
    pverts_2[1] = aPt2;
    pverts_2[2] = aPt3;
    NiColorA* pcolors = static_cast<NiColorA*>(ourMalloc(3 * sizeof(NiColorA)));
    if (pcolors) {
        ::new (static_cast<void*>(pcolors)) NiColorA[3];
        pcolors_2 = pcolors;
    }
    else {
        pcolors_2 = nullptr;
    }
    *pcolors_2 = aColor;
    pcolors_2[1] = aColor;
    pcolors_2[2] = aColor;
    uint16_t* ptrilist = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    *ptrilist = 0;
    ptrilist[1] = 2;
    ptrilist[2] = 1;
    ptrilist[3] = 0;
    ptrilist[4] = 1;
    ptrilist[5] = 2;
    NiTriShape* v11 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v11) {
        ptri = ::new (v11)
            NiTriShape(3, pverts_2, nullptr, pcolors_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 2, ptrilist);
    }
    else {
        ptri = nullptr;
    }
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pshprop;
        BSShaderNoLightingProperty* apMemory =
            static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
        if (apMemory) {
            pshprop = ::new (apMemory) BSShaderNoLightingProperty;
        }
        else {
            pshprop = nullptr;
        }
        ptri->AttachProperty(pshprop);
        BSShaderManager::PrepareObject(ptri, 0, 0);
    }
    return ptri;
}

BSShaderPPLightingProperty* FindPerPixelLighting_Recurse(NiNode* apNode) {
    BSShaderPPLightingProperty* pretprop = nullptr;
    if (!apNode->GetArrayCount()) {
        return nullptr;
    }
    uint32_t icount = apNode->GetArrayCount();
    for (uint32_t iindex = 0; iindex < icount; ++iindex) {
        if (pretprop) {
            return pretprop;
        }
        NiNode* pnode__pchildgeom = static_cast<NiNode*>(apNode->GetAt(iindex));
        if (pnode__pchildgeom) {
            if (pnode__pchildgeom->IsNode()) {
                pretprop = FindPerPixelLighting_Recurse(pnode__pchildgeom);
            }
            else {
                if (pnode__pchildgeom->IsGeometry()) {
                    int32_t Type = NiShadeProperty::GetType();
                    NiShadeProperty* pproperty = static_cast<NiShadeProperty*>(pnode__pchildgeom->GetProperty(Type));
                    if (pproperty) {
                        // The per-pixel lighting shader types are 8 to 12.
                        BSShaderPPLightingProperty* plighting =
                            pproperty->iShaderPropertyType >= 8 && pproperty->iShaderPropertyType <= 12
                            ? static_cast<BSShaderPPLightingProperty*>(pproperty)
                            : nullptr;
                        if (plighting) {
                            return plighting;
                        }
                    }
                }
            }
        }
    }
    return pretprop;
}

bool RemoveEditorMarkers(NiNode* apNode) {
    const char* HandleString;
    bool bretVal = false;
    if (!apNode) {
        return false;
    }
    if (!kMarkerRemover.pRoot) {
        const NiFixedString* v2 = &BSXFlags::Tag();
        BSXFlags* pbsx = static_cast<BSXFlags*>(apNode->GetExtraData(*v2));
        if ((pbsx) && (pbsx->QEditorMarker())) {
            kMarkerRemover.pRoot = apNode;
            NiControllerManager* pmanager =
                static_cast<NiControllerManager*>(apNode->GetController(&NiControllerManager::ms_RTTI));
            if (pmanager) {
                kMarkerRemover.pPalette = pmanager->GetObjectPalette();
            }
        }
    }
    if (!kMarkerRemover.pRoot) {
        return false;
    }
    if (!(&apNode->m_kName)->GetHandleString() ||
        (HandleString = (&apNode->m_kName)->GetHandleString(), BSstricmp(HandleString, "EditorMarker"))) {
        for (uint32_t n = 0; n < apNode->GetArrayCount(); ++n) {
            NiAVObject* pobj = apNode->GetAt(n);
            if (pobj) {
                NiNode* pnode = pobj->IsNode();
                if (pnode) {
                    bretVal = RemoveEditorMarkers(pnode);
                    if (bretVal) {
                        if (kMarkerRemover.pRoot == apNode) {
                            kMarkerRemover.pRoot = nullptr;
                            kMarkerRemover.pPalette = nullptr;
                        }
                        return bretVal;
                    }
                }
            }
        }
        if (kMarkerRemover.pRoot == apNode) {
            kMarkerRemover.pRoot = nullptr;
            kMarkerRemover.pPalette = nullptr;
        }
        return bretVal;
    }

    if (kMarkerRemover.pPalette) {
        NiFixedString* g_EditorMarker = get_g_EditorMarker();
        kMarkerRemover.pPalette->SetAVObject(*g_EditorMarker, nullptr);
        NiFixedString* g_EditorMarker0 = get_g_EditorMarker0();
        kMarkerRemover.pPalette->SetAVObject(*g_EditorMarker0, nullptr);
        NiFixedString* g_EditorMarker1 = get_g_EditorMarker1();
        kMarkerRemover.pPalette->SetAVObject(*g_EditorMarker1, nullptr);
        NiFixedString* g_EditorMarker2 = get_g_EditorMarker2();
        kMarkerRemover.pPalette->SetAVObject(*g_EditorMarker2, nullptr);
    }
    if (apNode->m_pkParent) {
        NiNode* Parent = apNode->m_pkParent;
        Parent->DetachChild(apNode);
    }
    if (kMarkerRemover.pRoot == apNode) {
        kMarkerRemover.pRoot = nullptr;
        kMarkerRemover.pPalette = nullptr;
    }
    return true;
}

float FindTimeForNoteKey(NiAVObject* pObject, const char* pcSequenceName, const char* pcKeyName) {
    uint32_t inumKeys;
    if (pObject) {
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pObject->GetControllers())
        );
        if (pmanager) {
            NiControllerSequence* psequence = pmanager->GetSequenceByName(pcSequenceName);
            if (psequence) {
                NiTextKeyExtraData* ptext = psequence->GetTextKeys();
                NiTextKey* pkeys = ptext->GetKeys(inumKeys);
                uint32_t isearchKeyLength = BSstrlen(pcKeyName);
                if (isearchKeyLength) {
                    for (uint32_t i = 0; i < inumKeys; ++i) {
                        NiFixedString* ActorPackageThatIsRunning = &pkeys[i].m_kText;
                        const char* pkeyText = ActorPackageThatIsRunning->GetHandleString();
                        if (pkeyText && !_strnicmp(pkeyText, pcKeyName, isearchKeyLength)) {
                            return pkeys[i].m_fTime;
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
    NiPointer<BSShaderPPLightingProperty::TextureEffectData> spdata(
        static_cast<BSShaderPPLightingProperty::TextureEffectData*>(apTextureEffectData)
    );
    if (!apTextureEffectData) {
        BSShaderPPLightingProperty::TextureEffectData* v11;
        void* apMemory = NiMemObject::operator new(sizeof(BSShaderPPLightingProperty::TextureEffectData));
        if (apMemory) {
            v11 = ::new (apMemory) BSShaderPPLightingProperty::TextureEffectData;
        }
        else {
            v11 = nullptr;
        }
        spdata.operator=(v11);
        apTextureEffectData = (void*)spdata.m_pObject;
        NiColorA v15(aColor.r, aColor.g, aColor.b, 0.5f);
        NiColorA* v3 = &v15;
        spdata.m_pObject->FillColor = *v3;
        NiColorA v14(aColor.r, aColor.g, aColor.b, 0.5f);
        NiColorA* v4 = &v14;
        spdata.m_pObject->RimColor = *v4;
        NiColorA v13(aColor.r, aColor.g, aColor.b, 0.5f);
        NiColorA* v5 = &v13;
        spdata.m_pObject->CurrentFillColor = *v5;
        NiColorA v12(aColor.r, aColor.g, aColor.b, 0.5f);
        NiColorA* v6 = &v12;
        spdata.m_pObject->CurrentRimColor = *v6;
        spdata.m_pObject->eSrcBlendMode = D3DBLEND_DESTCOLOR;
        spdata.m_pObject->eDestBlendMode = D3DBLEND_INVDESTCOLOR;
        spdata.m_pObject->eBlendOperation = D3DBLENDOP_ADD;
        spdata.m_pObject->eZTestFunction = D3DCMP_LESSEQUAL;
    }
    NiProperty* Property = apRoot->GetProperty(3);
    BSShaderPPLightingProperty* pspplprop = static_cast<BSShaderPPLightingProperty*>(
        NiObject::DynamicCastStatic(&BSShaderPPLightingProperty::ms_RTTI, Property)
    );
    if (pspplprop) {
        BSShaderPPLightingProperty::TextureEffectData* fA = spdata.m_pObject;
        pspplprop->SetTexEffectData(fA);
    }
    NiNode* pnode = apRoot->IsNode();
    if (pnode) {
        for (uint32_t i = 0;; ++i) {
            uint32_t ArrayCount = pnode->GetArrayCount();
            if (i >= ArrayCount) {
                break;
            }
            if (pnode->GetAt(i)) {
                NiAVObject* v9 = pnode->GetAt(i);
                TintScenegraph(v9, aColor, apTextureEffectData);
            }
        }
    }
}

void GetCollisionObjectByIndex(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    if (((arData.uDataA[3].i & 1) == 0 || !NiObject::IsKindOf_0(&WeaponObject::ms_RTTI, apCollision)) &&
        ((arData.uDataA[3].i & 2) == 0 || !NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, apCollision->m_pkSceneObject))) {
        const char* HandleString;
        if ((arData.uDataA[3].i & 2) == 0 ||
            (!apCollision->m_pkSceneObject ||
             (!(&apCollision->m_pkSceneObject->m_kName)->GetHandleString() ||
              (HandleString = (&apCollision->m_pkSceneObject->m_kName)->GetHandleString(),
               BSstrcmp(HandleString, "Arrow"))))) {
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
        const char* HandleString;
        if ((arData.uDataA[3].i & 2) == 0 ||
            (!apCollision->m_pkSceneObject ||
             (!(&apCollision->m_pkSceneObject->m_kName)->GetHandleString() ||
              (HandleString = (&apCollision->m_pkSceneObject->m_kName)->GetHandleString(),
               BSstrcmp(HandleString, "Arrow"))))) {
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
    bhkWorld::DoObjectRec(apRoot, recData, &GetCollisionObjectByIndex_0);
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
    bhkWorldObject* WorldObject = apCollision->GetWorldObject();
    if ((WorldObject) && (WorldObject->GethkObject())) {
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
    bhkWorldObject* WorldObject = apCollision->GetWorldObject();
    if (WorldObject) {
        hkReferencedObject* v2 = WorldObject->GethkObject();
        if (v2) {
            if (v2 == static_cast<hkReferencedObject*>(arData.uDataA[1].p)) {
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
    bhkWorldObject* WorldObject = apCollision->GetWorldObject();
    if (WorldObject) {
        hkpWorldObject* phkObject = static_cast<hkpWorldObject*>(WorldObject->GethkObject());
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

int32_t IsStringInteger(char* a1) {
    char v2 = *a1;
    char* a1a = a1 + 1;
    if ((v2 < 48 || v2 > 57) && v2 != 45) {
        return 0;
    }
    if (!*a1a && v2 == 45) {
        return 0;
    }
    char v3 = *a1a;
    char* a1b = a1a + 1;
    while (v3) {
        if (v3 < 48 || v3 > 57) {
            return 0;
        }
        v3 = *a1b++;
    }
    return 1;
}

int32_t IsStringFloat(char* a1) {
    char v4 = 0;
    char v2 = *a1;
    char* a1a = a1 + 1;
    if (!v2 || ((v2 < 48 || v2 > 57) && (v2 != 45 && v2 != 46))) {
        return 0;
    }
    if (!*a1a && (v2 == 45 || v2 == 46)) {
        return 0;
    }
    if (v2 == 46) {
        v4 = 1;
    }
    char v3 = *a1a;
    char* a1b = a1a + 1;
    while (v3) {
        if ((v3 < 48 || v3 > 57) && (v3 != 46 || v4)) {
            return 0;
        }
        if (v3 == 46) {
            v4 = 1;
        }
        v3 = *a1b++;
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
void GetCollisionObjectByIndex_0(bhkNiCollisionObject* a1, bhkWorldNS::ObjectRecData& arData) {
    if (((arData.uDataA[3].i & 1) == 0 || !NiObject::IsKindOf_0(&WeaponObject::ms_RTTI, a1)) &&
        ((arData.uDataA[3].i & 2) == 0 || !NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, a1->m_pkSceneObject))) {
        const char* HandleString;
        if ((arData.uDataA[3].i & 2) == 0 ||
            (!a1->m_pkSceneObject ||
             (!(&a1->m_pkSceneObject->m_kName)->GetHandleString() ||
              (HandleString = (&a1->m_pkSceneObject->m_kName)->GetHandleString(), BSstrcmp(HandleString, "Arrow"))))) {
            if (arData.uDataA[0u].i == arData.uDataA[2].i) {
                arData.uDataA[1].p = a1;
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
    fX = arSrc.getSimdAt(0).operator float();
    fY = arSrc.getSimdAt(1).operator float();
    fZ = arSrc.getSimdAt(2).operator float();
    fW = arSrc.getSimdAt(3).operator float();
    return *this;
}

bhkCollisionObject* bhkCollisionObject::Getbhk(const hkpRigidBody* apBody) {
    bhkNiCollisionObject* pCollisionObject = bhkNiCollisionObject::Getbhk(apBody);
    return static_cast<bhkCollisionObject*>(
        NiObject::DynamicCastStatic(&bhkCollisionObject::ms_RTTI, pCollisionObject)
    );
}

// Zero vectors, then the spring's default damping, elasticity, force limit and object damping.
hkMouseSpringActionCinfo::hkMouseSpringActionCinfo()
  : m_springDamping(0.5f), m_springElasticity(0.3f), m_maxRelativeForce(250.0f), m_objectDamping(0.95f) {}

// The mass is rounded to a float local and returned from there.
float bhkRigidBody::GetMass() {
    float fMass;
    hkpRigidBody* v4 = operator*();
    if (v4) {
        fMass = v4->getMass();
    }
    else {
        fMass = 0.0f;
    }
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
    NiFixedString* v0;
    NiFixedString* v1;
    NiFixedString* v2;
    NiFixedString* v3;
    NiFixedString* v4;
    NiFixedString* v5;
    NiFixedString* v6;
    NiFixedString* v7;
    NiFixedString* v8;
    NiFixedString* v9;
    NiFixedString* v10;
    NiFixedString* v11;
    NiFixedString* v12;
    NiFixedString* v13;
    NiFixedString* v14;
    NiFixedString* v15;
    NiFixedString* v16;
    NiFixedString* v17;
    NiFixedString* v18;
    NiFixedString* v19;
    NiFixedString* v20;
    NiFixedString* v21;
    NiFixedString* v22;
    NiFixedString* v23;
    NiFixedString* v24;
    NiFixedString* v25;
    NiFixedString* v26;
    NiFixedString* v27;
    NiFixedString* v28;
    NiFixedString* v29;
    NiFixedString* v30;
    NiFixedString* v31;
    NiFixedString* v32;
    NiFixedString* v33;
    NiFixedString* v34;
    NiFixedString* v35;
    NiFixedString* v36;
    NiFixedString* v37;
    NiFixedString* v38;
    NiFixedString* v39;
    NiFixedString* v40;
    NiFixedString* v41;
    NiFixedString* v42;
    NiFixedString* v43;
    NiFixedString* v44;
    NiFixedString* v45;
    NiFixedString* v46;
    NiFixedString* v47;
    NiFixedString* v48;
    NiFixedString* v49;
    NiFixedString* v50;
    NiFixedString* v51;
    NiFixedString* v52;
    NiFixedString* v53;
    NiFixedString* v107 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v107) {
        v53 = ::new (static_cast<void*>(v107)) NiFixedString("Bip01");
    }
    else {
        v53 = nullptr;
    }
    FixedStrings::pBip01 = v53;
    NiFixedString* v106 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v106) {
        v52 = ::new (static_cast<void*>(v106)) NiFixedString("Bip");
    }
    else {
        v52 = nullptr;
    }
    FixedStrings::pBip = v52;
    NiFixedString* v105 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v105) {
        v51 = ::new (static_cast<void*>(v105)) NiFixedString("Bip01 Head");
    }
    else {
        v51 = nullptr;
    }
    FixedStrings::pBip01_Head = v51;
    NiFixedString* v104 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v104) {
        v50 = ::new (static_cast<void*>(v104)) NiFixedString("Bip01 Neck");
    }
    else {
        v50 = nullptr;
    }
    FixedStrings::pBip01_Neck = v50;
    NiFixedString* v103 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v103) {
        v49 = ::new (static_cast<void*>(v103)) NiFixedString("Bip01 Spine");
    }
    else {
        v49 = nullptr;
    }
    FixedStrings::Bip01_Spine = v49;
    NiFixedString* v102 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v102) {
        v48 = ::new (static_cast<void*>(v102)) NiFixedString("Bip01 Spine1");
    }
    else {
        v48 = nullptr;
    }
    FixedStrings::Bip01_Spine1 = v48;
    NiFixedString* v101 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v101) {
        v47 = ::new (static_cast<void*>(v101)) NiFixedString("magicNode");
    }
    else {
        v47 = nullptr;
    }
    FixedStrings::pMagicNode = v47;
    NiFixedString* v100 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v100) {
        v46 = ::new (static_cast<void*>(v100)) NiFixedString("SpecialIdle_Cast");
    }
    else {
        v46 = nullptr;
    }
    FixedStrings::SpecialIdle_Cast = v46;
    NiFixedString* v99 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v99) {
        v45 = ::new (static_cast<void*>(v99)) NiFixedString("SpecialIdle_AreaEffect");
    }
    else {
        v45 = nullptr;
    }
    FixedStrings::SpecialIdle_AreaEffect = v45;
    NiFixedString* v98 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v98) {
        v44 = ::new (static_cast<void*>(v98)) NiFixedString("AttachSound");
    }
    else {
        v44 = nullptr;
    }
    FixedStrings::AttachSound = v44;
    NiFixedString* v97 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v97) {
        v43 = ::new (static_cast<void*>(v97)) NiFixedString("Sound Marker");
    }
    else {
        v43 = nullptr;
    }
    FixedStrings::Sound_Marker = v43;
    NiFixedString* v96 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v96) {
        v42 = ::new (static_cast<void*>(v96)) NiFixedString("Skinned Decal Node");
    }
    else {
        v42 = nullptr;
    }
    FixedStrings::Skinned_Decal_Node = v42;
    NiFixedString* v95 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v95) {
        v41 = ::new (static_cast<void*>(v95)) NiFixedString("Decal Node");
    }
    else {
        v41 = nullptr;
    }
    FixedStrings::Decal_Node = v41;
    NiFixedString* v94 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v94) {
        v40 = ::new (static_cast<void*>(v94)) NiFixedString("ModelSwapNode");
    }
    else {
        v40 = nullptr;
    }
    FixedStrings::ModelSwapNode = v40;
    NiFixedString* v93 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v93) {
        v39 = ::new (static_cast<void*>(v93)) NiFixedString("Open");
    }
    else {
        v39 = nullptr;
    }
    FixedStrings::Open = v39;
    NiFixedString* v92 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v92) {
        v38 = ::new (static_cast<void*>(v92)) NiFixedString("Close");
    }
    else {
        v38 = nullptr;
    }
    FixedStrings::Close = v38;
    NiFixedString* v91 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v91) {
        v37 = ::new (static_cast<void*>(v91)) NiFixedString("DVPG");
    }
    else {
        v37 = nullptr;
    }
    BSDecalPlacementVectorExtraData::pTag = v37;
    NiFixedString* v90 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v90) {
        v36 = ::new (static_cast<void*>(v90)) NiFixedString("Prn");
    }
    else {
        v36 = nullptr;
    }
    FixedStrings::pPrn = v36;
    NiFixedString* v89 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v89) {
        v35 = ::new (static_cast<void*>(v89)) NiFixedString("Weapon");
    }
    else {
        v35 = nullptr;
    }
    FixedStrings::Weapon = v35;
    NiFixedString* v88 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v88) {
        v34 = ::new (static_cast<void*>(v88)) NiFixedString("EditorMarker");
    }
    else {
        v34 = nullptr;
    }
    FixedStrings::EditorMarker = v34;
    NiFixedString* v87 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v87) {
        v33 = ::new (static_cast<void*>(v87)) NiFixedString("EditorMarker:0");
    }
    else {
        v33 = nullptr;
    }
    FixedStrings::EditorMarker0 = v33;
    NiFixedString* v86 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v86) {
        v32 = ::new (static_cast<void*>(v86)) NiFixedString("EditorMarker:1");
    }
    else {
        v32 = nullptr;
    }
    FixedStrings::EditorMarker1 = v32;
    NiFixedString* v85 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v85) {
        v31 = ::new (static_cast<void*>(v85)) NiFixedString("EditorMarker:2");
    }
    else {
        v31 = nullptr;
    }
    FixedStrings::EditorMarker2 = v31;
    NiFixedString* v84 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v84) {
        v30 = ::new (static_cast<void*>(v84)) NiFixedString("ArrowQuiver");
    }
    else {
        v30 = nullptr;
    }
    FixedStrings::ArrowQuiver = v30;
    NiFixedString* v83 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v83) {
        v29 = ::new (static_cast<void*>(v83)) NiFixedString("MarkerSource");
    }
    else {
        v29 = nullptr;
    }
    FixedStrings::MarkerSource = v29;
    NiFixedString* v82 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v82) {
        v28 = ::new (static_cast<void*>(v82)) NiFixedString("MarkerTarget");
    }
    else {
        v28 = nullptr;
    }
    FixedStrings::MarkerTarget = v28;
    NiFixedString* v81 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v81) {
        v27 = ::new (static_cast<void*>(v81)) NiFixedString("AttachLight");
    }
    else {
        v27 = nullptr;
    }
    FixedStrings::pAttachLight = v27;
    NiFixedString* v80 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v80) {
        v26 = ::new (static_cast<void*>(v80)) NiFixedString("skin");
    }
    else {
        v26 = nullptr;
    }
    FixedStrings::pSkin = v26;
    NiFixedString* v79 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v79) {
        v25 = ::new (static_cast<void*>(v79)) NiFixedString("FaceGenEars");
    }
    else {
        v25 = nullptr;
    }
    FixedStrings::pFaceGenEars = v25;
    NiFixedString* v78 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v78) {
        v24 = ::new (static_cast<void*>(v78)) NiFixedString("Unequip");
    }
    else {
        v24 = nullptr;
    }
    FixedStrings::pUnequip = v24;
    NiFixedString* v77 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v77) {
        v23 = ::new (static_cast<void*>(v77)) NiFixedString("LaserSight");
    }
    else {
        v23 = nullptr;
    }
    FixedStrings::pLaserSight = v23;
    NiFixedString* v76 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v76) {
        v22 = ::new (static_cast<void*>(v76)) NiFixedString("AimSight");
    }
    else {
        v22 = nullptr;
    }
    FixedStrings::pAimSight = v22;
    NiFixedString* v75 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v75) {
        v21 = ::new (static_cast<void*>(v75)) NiFixedString("Grass");
    }
    else {
        v21 = nullptr;
    }
    FixedStrings::pGrass = v21;
    NiFixedString* v74 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v74) {
        v20 = ::new (static_cast<void*>(v74)) NiFixedString(pcString);
    }
    else {
        v20 = nullptr;
    }
    FixedStrings::pDecal = v20;
    NiFixedString* v73 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v73) {
        v19 = ::new (static_cast<void*>(v73)) NiFixedString("Permanent Decal");
    }
    else {
        v19 = nullptr;
    }
    FixedStrings::pPermanentDecal = v19;
    NiFixedString* v72 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v72) {
        v18 = ::new (static_cast<void*>(v72)) NiFixedString("grabLeft");
    }
    else {
        v18 = nullptr;
    }
    FixedStrings::pGrabLeft = v18;
    NiFixedString* v71 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v71) {
        v17 = ::new (static_cast<void*>(v71)) NiFixedString("grabRight");
    }
    else {
        v17 = nullptr;
    }
    FixedStrings::pGrabRight = v17;
    NiFixedString* v70 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v70) {
        v16 = ::new (static_cast<void*>(v70)) NiFixedString("Arrow:0");
    }
    else {
        v16 = nullptr;
    }
    FixedStrings::pArrow0 = v16;
    NiFixedString* v69 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v69) {
        v15 = ::new (static_cast<void*>(v69)) NiFixedString("ArrowBone");
    }
    else {
        v15 = nullptr;
    }
    FixedStrings::pArrowBone = v15;
    NiFixedString* v68 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v68) {
        v14 = ::new (static_cast<void*>(v68)) NiFixedString("BSFaceGenNiNodeBiped");
    }
    else {
        v14 = nullptr;
    }
    FixedStrings::pRaceHeadBiped = v14;
    NiFixedString* v67 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v67) {
        v13 = ::new (static_cast<void*>(v67)) NiFixedString("BSFaceGenNiNodeSkinned");
    }
    else {
        v13 = nullptr;
    }
    FixedStrings::pRaceHeadSkinned = v13;
    NiFixedString* v66 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v66) {
        v12 = ::new (static_cast<void*>(v66)) NiFixedString("HeadAnims:0");
    }
    else {
        v12 = nullptr;
    }
    FixedStrings::pHeadAnims = v12;
    NiFixedString* v65 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v65) {
        v11 = ::new (static_cast<void*>(v65)) NiFixedString("EntryPoint");
    }
    else {
        v11 = nullptr;
    }
    FixedStrings::pEntryPoint = v11;
    NiFixedString* v64 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v64) {
        v10 = ::new (static_cast<void*>(v64)) NiFixedString("Bip01 L UpperArm");
    }
    else {
        v10 = nullptr;
    }
    FixedStrings::pBip01LUpperArm = v10;
    NiFixedString* v63 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v63) {
        v9 = ::new (static_cast<void*>(v63)) NiFixedString("Bip01 L Forearm");
    }
    else {
        v9 = nullptr;
    }
    FixedStrings::pBip01LForearm = v9;
    NiFixedString* v62 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v62) {
        v8 = ::new (static_cast<void*>(v62)) NiFixedString("Bip01 R UpperArm");
    }
    else {
        v8 = nullptr;
    }
    FixedStrings::pBip01RUpperArm = v8;
    NiFixedString* v61 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v61) {
        v7 = ::new (static_cast<void*>(v61)) NiFixedString("Bip01 Yaw");
    }
    else {
        v7 = nullptr;
    }
    FixedStrings::pBip01Yaw = v7;
    NiFixedString* v60 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v60) {
        v6 = ::new (static_cast<void*>(v60)) NiFixedString("Bip01 Pitch");
    }
    else {
        v6 = nullptr;
    }
    FixedStrings::pBip01Pitch = v6;
    NiFixedString* v59 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v59) {
        v5 = ::new (static_cast<void*>(v59)) NiFixedString("Bip01 Looking");
    }
    else {
        v5 = nullptr;
    }
    FixedStrings::pBip01Looking = v5;
    NiFixedString* v58 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v58) {
        v4 = ::new (static_cast<void*>(v58)) NiFixedString("Talking");
    }
    else {
        v4 = nullptr;
    }
    FixedStrings::pTalking = v4;
    NiFixedString* v57 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v57) {
        v3 = ::new (static_cast<void*>(v57)) NiFixedString("Camera1st");
    }
    else {
        v3 = nullptr;
    }
    FixedStrings::pCamera1st = v3;
    NiFixedString* v56 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v56) {
        v2 = ::new (static_cast<void*>(v56)) NiFixedString("Camera3rd");
    }
    else {
        v2 = nullptr;
    }
    FixedStrings::pCamera3rd = v2;
    NiFixedString* v55 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v55) {
        v1 = ::new (static_cast<void*>(v55)) NiFixedString("HeadMeshForExport_FaceGenAndMorphs:0");
    }
    else {
        v1 = nullptr;
    }
    FixedStrings::pHeadMeshExp = v1;
    NiFixedString* v54 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v54) {
        v0 = ::new (static_cast<void*>(v54)) NiFixedString("PinnedLimb");
    }
    else {
        v0 = nullptr;
    }
    FixedStrings::pPinnedLimb = v0;
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
    NiPoint3 dst;
    m_kCenter = *::new (&dst) NiPoint3(x, y, z);
}

NiAVObject* NiCollisionObject::GetSceneGraphObject() {
    return m_pkSceneObject;
}

float NiASin(float fValue) {
    if (!(fValue > (-1.0))) {
        return -NI_HALF_PI;
    }

    if (!(fValue < 1.0)) {
        return NI_HALF_PI;
    }

    return std::asin(fValue);
}

NiMatrix3* NiMatrix3::Inverse(NiMatrix3* apResult) const {
    NiMatrix3 ecx0;
    if (!Inverse(ecx0)) {
        ecx0.MakeZero();
    }
    *apResult = ecx0;
    return apResult;
}

void NiTransform::Invert(NiTransform& kDest) const {
    kDest.m_Rotate = m_Rotate.Transpose();
    kDest.m_fScale = 1.0f / m_fScale;
    NiPoint3 kNegated = -m_Translate;
    NiPoint3 kRotated = kDest.m_Rotate * kNegated;
    kDest.m_Translate = kDest.m_fScale * kRotated;
}
