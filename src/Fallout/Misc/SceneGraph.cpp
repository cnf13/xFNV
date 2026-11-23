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
    float fDataClipDist;
    if (bMenuSceneGraph) {
        return 20480.0f;
    }
    else {
        if (TES::pInstance &&
            TES::pInstance->GetWorldSpace() &&
            !TES::pInstance->GetWorldSpace()->GetTerrainManager()->HasRootNode()) {
            float fClipDistance;
            if (TES::pInstance &&
                TES::pInstance->GetInterior() &&
                TES::pInstance->GetSky()->GetMode() == Sky::SM_INTERIOR) {
                fClipDistance = TES::pInstance->GetInterior()->GetDataClipDist();
                if (fClipDistance <= 0.0 ||
                    BSSceneGraph::fNoLODFarDistanceMax.Float() < static_cast<double>(fClipDistance)) {
                    fClipDistance = BSSceneGraph::fNoLODFarDistanceMax.Float();
                }
            }
            else {
                fClipDistance = BSSceneGraph::fNoLODFarDistanceMax.Float();
            }
            float fNoLODFarDistancePct = BSSceneGraph::fNoLODFarDistancePct.Float();
            return FLerp(BSSceneGraph::fNoLODFarDistanceMin.Float(), fClipDistance, 0.0, 1.0, fNoLODFarDistancePct);
        }

        if (TES::pInstance &&
            TES::pInstance->GetInterior() &&
            TES::pInstance->GetSky()->GetMode() == Sky::SM_INTERIOR) {
            fDataClipDist = TES::pInstance->GetInterior()->GetDataClipDist();
            if (fDataClipDist <= 0.0 || fDataClipDist > 353840.0f) {
                return 353840.0f;
            }
        }
        else {
            return 353840.0f;
        }
    }
    return fDataClipDist;
}
