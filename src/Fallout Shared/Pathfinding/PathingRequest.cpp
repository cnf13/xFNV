#include "Fallout Shared/Pathfinding/PathingRequest.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestCover.h"
#include "Fallout Shared/Pathfinding/PathingRequestCoveredMove.h"
#include "Fallout Shared/Pathfinding/PathingRequestFlee.h"
#include "Fallout Shared/Pathfinding/PathingRequestHide.h"
#include "Fallout Shared/Pathfinding/PathingRequestLOS.h"
#include "Fallout Shared/Pathfinding/PathingRequestOptimalLocation.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/MSVC/string.h"

ActorLockData::ActorLockData(Actor* apActor)
  : pActorBase(apActor->GetActorBase()), pInventory(apActor->TESObjectREFR::GetExtra()->GetContainerChanges()),
    bIsAlarmed(apActor->IsAlarmed()) {}

PathingRequest::PathingRequest()
  : NiRefObject(), Origin(), Destination(), LockData(), fActorRadius(35.0f), fActorHeight(1.0f), fGoalZDelta(180.0f),
    fTargetRadius(GetDefaultRadius()), fCenterRadius(0.0f), Center(NiPoint3::ZERO), fSmoothingRandomness(0.0f),
    fGoalAngle(0.0f), fInitialPathHeading(0.0f), spAvoidNodeArray(nullptr), bCantOpenDoors(false),
    bFaceTargetAtGoal(false), bDrawDebugGeometry(false), bAllowIncompletePath(true), bSoftGoalRadius(true),
    bCanSwim(true), bCanFly(false), bWaitIfPlayerWaits(false), bFirstTangentUsesHeading(false),
    bGoDirectIfPathingFails(false), bTryToUseCurvedPath(false), bIgnoreSmoothingIfFails(false), bIgnoreLocks(false),
    iRetryCount(3), bFailIfGoalOccupied(false) {}

// Default members, then aRequest's CopyTo onto this, called directly (not virtually).
PathingRequest::PathingRequest(const PathingRequest& aRequest)
  : NiRefObject(), Origin(), Destination(), LockData(), Center(), spAvoidNodeArray(nullptr) {
    aRequest.PathingRequest::CopyTo(this);
}

PathingRequest::~PathingRequest() {}

void PathingRequest::CopyTo(PathingRequest* apDest) const {
    apDest->SetOrigin(Origin);
    apDest->SetDestination(Destination);
    apDest->SetActorHeight(fActorHeight);
    apDest->SetActorRadius(fActorRadius);
    apDest->SetActorLockData(LockData);
    apDest->SetTargetRadius(fTargetRadius);
    NiPoint3 center = GetTargetPoint();
    apDest->SetCenterRadius(center, fCenterRadius);
    apDest->SetGoalZDelta(fGoalZDelta);
    apDest->SetAvoidNodeArray(GetAvoidNodeArray());
    // fGoalZDelta is set a second time here.
    apDest->SetGoalZDelta(fGoalZDelta);
    apDest->SetSmoothingRandomness(fSmoothingRandomness);
    apDest->bCantOpenDoors = bCantOpenDoors;
    apDest->fGoalAngle = fGoalAngle;
    apDest->fInitialPathHeading = fInitialPathHeading;
    apDest->bFaceTargetAtGoal = bFaceTargetAtGoal;
    apDest->bAllowIncompletePath = bAllowIncompletePath;
    apDest->bSoftGoalRadius = bSoftGoalRadius;
    apDest->bCanSwim = bCanSwim;
    apDest->bCanFly = bCanFly;
    apDest->bWaitIfPlayerWaits = bWaitIfPlayerWaits;
    apDest->bFirstTangentUsesHeading = bFirstTangentUsesHeading;
    apDest->bGoDirectIfPathingFails = bGoDirectIfPathingFails;
    apDest->bTryToUseCurvedPath = bTryToUseCurvedPath;
    apDest->bIgnoreSmoothingIfFails = bIgnoreSmoothingIfFails;
    apDest->bIgnoreLocks = bIgnoreLocks;
    apDest->iRetryCount = iRetryCount;
    apDest->bFailIfGoalOccupied = bFailIfGoalOccupied;
}

uint32_t PathingRequest::GetType() const {
    return PATHING_REQUEST_STANDARD;
}

void PathingRequest::PrintDebugText(IDebugText* apDebugText) {
    char cBuffer[260];
    Origin.PrintDebugText(cBuffer, 256);
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Origin: %s", cBuffer);
    Destination.PrintDebugText(cBuffer, 256);
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Destination: %s", cBuffer);
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Actor Radius: %f", fActorRadius);
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Goal Z Delta: %f", fGoalZDelta);
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Target Radius: %f", fTargetRadius);
    const char* pcValue = bAllowIncompletePath ? "true" : "false";
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Allow Incomplete Path: %s", pcValue);
    if (fCenterRadius != 0.0) {
        apDebugText->PrintLine(
            DTL_LEFT,
            DTJ_LEFT,
            "Center Radius: (%.2f,%.2f,%.2f) %f",
            Center.x,
            Center.y,
            Center.z,
            fCenterRadius
        );
    }
    pcValue = bFaceTargetAtGoal ? "true" : "false";
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Face Target at Goal: %s", pcValue);
    if (bFaceTargetAtGoal) {
        apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Goal Angle: %f", fGoalAngle);
    }
    if (bFirstTangentUsesHeading) {
        apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Initial Path Heading: %f", fInitialPathHeading);
    }
    pcValue = bGoDirectIfPathingFails ? "true" : "false";
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Allow Direct Path If Pathing Fails: %s", pcValue);
    pcValue = bTryToUseCurvedPath ? "true" : "false";
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Using Curved path: %s", pcValue);
    pcValue = bIgnoreLocks ? "true" : "false";
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Ignoring Locks: %s", pcValue);
    apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Pathsmoothing retry count: %i", iRetryCount);
}

void PathingRequest::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Origin.SaveGame(apSaveGameBuffer);
    Destination.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fActorRadius)), sizeof(fActorRadius), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fActorHeight)), sizeof(fActorHeight), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fGoalZDelta)), sizeof(fGoalZDelta), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fTargetRadius)), sizeof(fTargetRadius), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fCenterRadius)), sizeof(fCenterRadius), 0);
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&fSmoothingRandomness)),
        sizeof(fSmoothingRandomness),
        0
    );
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fGoalAngle)), sizeof(fGoalAngle), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bFaceTargetAtGoal)), sizeof(bFaceTargetAtGoal), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bCantOpenDoors)), sizeof(bCantOpenDoors), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bDrawDebugGeometry)), sizeof(bDrawDebugGeometry), 0);
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&bAllowIncompletePath)),
        sizeof(bAllowIncompletePath),
        0
    );
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bSoftGoalRadius)), sizeof(bSoftGoalRadius), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bCanSwim)), sizeof(bCanSwim), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bWaitIfPlayerWaits)), sizeof(bWaitIfPlayerWaits), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fInitialPathHeading)), sizeof(fInitialPathHeading), 0);
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&bFirstTangentUsesHeading)),
        sizeof(bFirstTangentUsesHeading),
        0
    );
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&bGoDirectIfPathingFails)),
        sizeof(bGoDirectIfPathingFails),
        0
    );
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&Center)), sizeof(Center), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bTryToUseCurvedPath)), sizeof(bTryToUseCurvedPath), 0);
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&bIgnoreSmoothingIfFails)),
        sizeof(bIgnoreSmoothingIfFails),
        0
    );
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bIgnoreLocks)), sizeof(bIgnoreLocks), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iRetryCount)), sizeof(iRetryCount), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bCanFly)), sizeof(bCanFly), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bFailIfGoalOccupied)), sizeof(bFailIfGoalOccupied), 0);
    uint32_t uiCount = spAvoidNodeArray.m_pObject ? spAvoidNodeArray->iSize : 0;
    apSaveGameBuffer->SaveVariableSizedValue(uiCount);
    for (uint32_t i = 0; i < uiCount; ++i) {
        spAvoidNodeArray->GetAt(i).SaveGame(apSaveGameBuffer);
    }
}

void PathingRequest::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Origin.LoadGame(apLoadGameBuffer);
    Destination.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fActorRadius, sizeof(fActorRadius));
    apLoadGameBuffer->LoadData(&fActorHeight, sizeof(fActorHeight));
    apLoadGameBuffer->LoadData(&fGoalZDelta, sizeof(fGoalZDelta));
    apLoadGameBuffer->LoadData(&fTargetRadius, sizeof(fTargetRadius));
    apLoadGameBuffer->LoadData(&fCenterRadius, sizeof(fCenterRadius));
    apLoadGameBuffer->LoadData(&fSmoothingRandomness, sizeof(fSmoothingRandomness));
    apLoadGameBuffer->LoadData(&fGoalAngle, sizeof(fGoalAngle));
    apLoadGameBuffer->LoadData(&bFaceTargetAtGoal, sizeof(bFaceTargetAtGoal));
    apLoadGameBuffer->LoadData(&bCantOpenDoors, sizeof(bCantOpenDoors));
    apLoadGameBuffer->LoadData(&bDrawDebugGeometry, sizeof(bDrawDebugGeometry));
    apLoadGameBuffer->LoadData(&bAllowIncompletePath, sizeof(bAllowIncompletePath));
    apLoadGameBuffer->LoadData(&bSoftGoalRadius, sizeof(bSoftGoalRadius));
    apLoadGameBuffer->LoadData(&bCanSwim, sizeof(bCanSwim));
    if (apLoadGameBuffer->GetVersion() >= 13u) {
        apLoadGameBuffer->LoadData(&bWaitIfPlayerWaits, sizeof(bWaitIfPlayerWaits));
    }
    if (apLoadGameBuffer->GetVersion() >= 15u) {
        apLoadGameBuffer->LoadData(&fInitialPathHeading, sizeof(fInitialPathHeading));
        apLoadGameBuffer->LoadData(&bFirstTangentUsesHeading, sizeof(bFirstTangentUsesHeading));
        apLoadGameBuffer->LoadData(&bGoDirectIfPathingFails, sizeof(bGoDirectIfPathingFails));
    }
    if (apLoadGameBuffer->GetVersion() >= 18u) {
        apLoadGameBuffer->LoadData(&Center, sizeof(Center));
        apLoadGameBuffer->LoadData(&bTryToUseCurvedPath, sizeof(bTryToUseCurvedPath));
        apLoadGameBuffer->LoadData(&bIgnoreSmoothingIfFails, sizeof(bIgnoreSmoothingIfFails));
        apLoadGameBuffer->LoadData(&bIgnoreLocks, sizeof(bIgnoreLocks));
        apLoadGameBuffer->LoadData(&iRetryCount, sizeof(iRetryCount));
    }
    if (apLoadGameBuffer->GetVersion() >= 20u) {
        apLoadGameBuffer->LoadData(&bCanFly, sizeof(bCanFly));
    }
    if (apLoadGameBuffer->GetVersion() >= 21u) {
        apLoadGameBuffer->LoadData(&bFailIfGoalOccupied, sizeof(bFailIfGoalOccupied));
    }
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (isaveGameCount) {
        spAvoidNodeArray = new PathingAvoidNodeArray;
        spAvoidNodeArray->SetSize(isaveGameCount, true);
        for (uint32_t i = 0; i < isaveGameCount; ++i) {
            PathingAvoidNode* pavoidNode = &spAvoidNodeArray->GetAt(i);
            pavoidNode->LoadGame(apLoadGameBuffer);
        }
    }
}

void PathingRequest::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetVersion() < 14u) {
        bWaitIfPlayerWaits = false;
        Actor* pactor = apLoadGameBuffer->GetActor();
        if (pactor) {
            TESPackage* ppack = pactor->GetCurrentPackage();
            if (ppack &&
                ppack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_ESCORT_ACTOR &&
                ppack->GetEscortTargetForPackage(pactor, false) == pPlayer) {
                bWaitIfPlayerWaits = true;
            }
        }
    }
}

void PathingRequest::SetOrigin(const PathingLocation& aOrigin) {
    Origin = aOrigin;
}

void PathingRequest::SetAllowIncompletePath(bool abAllow) {
    bAllowIncompletePath = abAllow;
}

void PathingRequest::SetRetryCount(int32_t aiRetryCount) {
    iRetryCount = aiRetryCount;
}

void PathingRequest::Initialize(Actor* apActor) {
    Origin = PathingLocation(apActor);
    float fRadius = apActor->GetWidth() * 0.6000000238418579;
    SetActorRadius(fRadius);
    SetActorLockData(apActor);
    bool bDoesSwim = apActor->DoesSwim();
    SetCanSwim(bDoesSwim);
    bool bDoesFly = apActor->DoesFly();
    SetCanFly(bDoesFly);
    if (apActor->IsCreature()) {
        TESActorBaseData* pbaseData = static_cast<TESActorBaseData*>(apActor->GetActorBase());
        SetCantOpenDoors(pbaseData->GetCantOpenDoors());
    }
    float fHeading = apActor->GetHeading(false);
    SetInitialPathHeading(fHeading);
}

void PathingRequest::SetActorRadius(float aRadius) {
    fActorRadius = aRadius;
}

void PathingRequest::SetActorHeight(float aHeight) {
    fActorHeight = aHeight;
}

void PathingRequest::SetGoalZDelta(float aZDelta) {
    fGoalZDelta = aZDelta;
}

void PathingRequest::SetSmoothingRandomness(float afRandomness) {
    fSmoothingRandomness = afRandomness;
}

void PathingRequest::SetGoalAngleToFace(float aTargetAngle) {
    fGoalAngle = aTargetAngle;
    bFaceTargetAtGoal = true;
}

void PathingRequest::SetCantOpenDoors(bool abCantOpenDoors) {
    bCantOpenDoors = abCantOpenDoors;
}

PathingAvoidNodeArray* PathingRequest::GetAvoidNodeArray() const {
    return spAvoidNodeArray;
}

bool PathingRequest::GetCanSwim() const {
    return bCanSwim;
}

bool PathingRequest::GetCanFly() const {
    return bCanFly;
}

bool PathingRequest::GetTryToUseCurvedPath() const {
    return bTryToUseCurvedPath;
}

bool PathingRequest::GetIgnoreLocks() const {
    return bIgnoreLocks;
}

bool PathingRequest::GetGoDirectIfPathingFails() const {
    return bGoDirectIfPathingFails;
}

void PathingRequest::SetCenterRadius(NiPoint3& arCenter, float afOriginRadius) {
    Center = arCenter;
    fCenterRadius = afOriginRadius;
}

void PathingRequest::SetActorLockData(const ActorLockData& aLockData) {
    LockData = aLockData;
}

void PathingRequest::SetActorLockData(Actor* apActor) {
    LockData = ActorLockData(apActor);
}

void PathingRequest::SetCanSwim(bool abCanSwim) {
    bCanSwim = abCanSwim;
}

void PathingRequest::SetCanFly(bool abCanFly) {
    bCanFly = abCanFly;
}

void PathingRequest::SetInitialPathHeading(float afInitialPathHeading) {
    fInitialPathHeading = afInitialPathHeading;
    bFirstTangentUsesHeading = true;
}

void PathingRequest::SetGoDirectIfPathingFails(bool abGoDirect) {
    bGoDirectIfPathingFails = abGoDirect;
}

void PathingRequest::SetWaitIfPlayerWaits(bool abAbort) {
    bWaitIfPlayerWaits = abAbort;
}

void PathingRequest::SetSoftGoalRadius(bool abUseSoft) {
    bSoftGoalRadius = abUseSoft;
}

void PathingRequest::SetIgnoreSmoothingIfFails(bool abIgnoreSmoothingIfFails) {
    bIgnoreSmoothingIfFails = abIgnoreSmoothingIfFails;
}

bool PathingRequest::GetIgnoreSmoothingIfFails() const {
    return bIgnoreSmoothingIfFails;
}

void PathingRequest::ClearInitialPathHeading() {
    bFirstTangentUsesHeading = false;
}

bool PathingRequest::GetWaitIfPlayerWaits() const {
    return bWaitIfPlayerWaits;
}

bool PathingRequest::GetSoftGoalRadius() const {
    return bSoftGoalRadius;
}

NiPoint3 PathingRequest::GetTargetPoint() const {
    return Center;
}

float PathingRequest::GetDefaultRadius() {
    return fDefaultRadius;
}

PathingRequest* PathingRequest::CreatePathingRequest(PATHING_REQUEST_TYPE aeType) {
    PathingRequest* pRequest;
    switch (aeType) {
        case PATHING_REQUEST_STANDARD: {
            pRequest = new PathingRequest;
            break;
        }
        case PATHING_REQUEST_COVER: {
            pRequest = new PathingRequestCover;
            break;
        }
        case PATHING_REQUEST_CLOSE_POINT: {
            pRequest = new PathingRequestClosePoint;
            break;
        }
        case PATHING_REQUEST_FLEE: {
            pRequest = new PathingRequestFlee;
            break;
        }
        case PATHING_REQUEST_HIDE: {
            pRequest = new PathingRequestHide;
            break;
        }
        case PATHING_REQUEST_LOS: {
            pRequest = new PathingRequestLOS;
            break;
        }
        case PATHING_REQUEST_OPTIMAL_LOCATION: {
            pRequest = new PathingRequestOptimalLocation;
            break;
        }
        case PATHING_REQUEST_COVERED_MOVE: {
            pRequest = new PathingRequestCoveredMove;
            break;
        }
        default: {
            Warning("PATHFINDING: Unknown pathing request type %i", aeType);
            pRequest = nullptr;
            break;
        }
    }
    return pRequest;
}

float PathingRequestClosePoint::GetMinDistance() const {
    return fMinDistance;
}
