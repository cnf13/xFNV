#include "Fallout/Misc/SceneGraph.h"

#include "BSMain/BSSceneGraph.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

NiRTTI SceneGraph::ms_RTTI("SceneGraph", &BSSceneGraph::ms_RTTI);

SceneGraph::SceneGraph(const char* apSceneGraphName, bool abMenuSceneGraph, NiCamera* apCamera)
  : BSSceneGraph(apSceneGraphName, abMenuSceneGraph, apCamera) {}

const NiRTTI* SceneGraph::GetRTTI() const {
    return &SceneGraph::ms_RTTI;
}

float SceneGraph::GetFarDistance() {
    float DataClipDist;
    if (bMenuSceneGraph) {
        return 20480.0f;
    }
    else {
        TESWorldSpace* WorldSpace;
        BGSTerrainManager* TerrainManager;
        if (TES::pInstance &&
            (TES::pInstance->GetWorldSpace() &&
             (WorldSpace = TES::pInstance->GetWorldSpace(),
              (TerrainManager = WorldSpace->GetTerrainManager(), !TerrainManager->HasRootNode())))) {
            Sky* CurrentAIProcess;
            float fClipDistance;
            if (TES::pInstance &&
                (TES::pInstance->GetInterior() &&
                 (CurrentAIProcess = TES::pInstance->GetSky(), CurrentAIProcess->GetMode() == Sky::SM_INTERIOR))) {
                TESObjectCELL* Interior = TES::pInstance->GetInterior();
                fClipDistance = Interior->GetDataClipDist();
                if (fClipDistance <= 0.0 ||
                    BSSceneGraph::fNoLODFarDistanceMax.Float() < static_cast<double>(fClipDistance)) {
                    fClipDistance = BSSceneGraph::fNoLODFarDistanceMax.Float();
                }
            }
            else {
                fClipDistance = BSSceneGraph::fNoLODFarDistanceMax.Float();
            }
            float fNoLODFarDistancePct = BSSceneGraph::fNoLODFarDistancePct.Float();
            float* v7 = &BSSceneGraph::fNoLODFarDistanceMin.Float();
            return FLerp(*v7, fClipDistance, 0.0, 1.0, fNoLODFarDistancePct);
        }
        else {
            Sky* v3;
            if (TES::pInstance &&
                (TES::pInstance->GetInterior() && (v3 = TES::pInstance->GetSky(), v3->GetMode() == Sky::SM_INTERIOR))) {
                TESObjectCELL* v4 = TES::pInstance->GetInterior();
                DataClipDist = v4->GetDataClipDist();
                if (DataClipDist <= 0.0 || DataClipDist > 353840.0f) {
                    return 353840.0f;
                }
            }
            else {
                return 353840.0f;
            }
        }
    }
    return DataClipDist;
}
