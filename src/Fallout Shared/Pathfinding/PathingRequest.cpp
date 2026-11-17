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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

ActorLockData::ActorLockData(Actor* apActor)
	: pActorBase(apActor->GetActorBase()), pInventory(apActor->TESObjectREFR::GetExtra()->GetContainerChanges()),
	  bIsAlarmed(apActor->IsAlarmed()) {}

PathingRequest::PathingRequest()
	: NiRefObject(), Origin(), Destination(), LockData(), fActorRadius(35.0f), fActorHeight(1.0f), fGoalZDelta(180.0f),
	  fTargetRadius(GetDefaultRadius()), fCenterRadius(0.0f), Center(NiPoint3_ZERO), fSmoothingRandomness(0.0f),
	  fGoalAngle(0.0f), fInitialPathHeading(0.0f), spAvoidNodeArray(nullptr), bCantOpenDoors(false),
	  bFaceTargetAtGoal(false), bDrawDebugGeometry(false), bAllowIncompletePath(true), bSoftGoalRadius(true),
	  bCanSwim(true), bCanFly(false), bWaitIfPlayerWaits(false), bFirstTangentUsesHeading(false),
	  bGoDirectIfPathingFails(false), bTryToUseCurvedPath(false), bIgnoreSmoothingIfFails(false), bIgnoreLocks(false),
	  iRetryCount(3), bFailIfGoalOccupied(false) {}

// Default members, then aRequest's CopyTo onto this, called directly (not virtually) as the game does.
PathingRequest::PathingRequest(const PathingRequest& aRequest)
	: NiRefObject(), Origin(), Destination(), LockData(), Center(), spAvoidNodeArray(nullptr) {
	aRequest.PathingRequest::CopyTo(this);
}

PathingRequest::~PathingRequest() {}

void PathingRequest::CopyTo(PathingRequest* apDest) const {
	NiPoint3 center;
	apDest->SetOrigin(Origin);
	apDest->SetDestination(Destination);
	apDest->SetActorHeight(fActorHeight);
	apDest->SetActorRadius(fActorRadius);
	apDest->SetActorLockData(LockData);
	apDest->SetTargetRadius(fTargetRadius);
	GetTargetPoint(&center);
	apDest->SetCenterRadius(center, fCenterRadius);
	apDest->SetGoalZDelta(fGoalZDelta);
	PathingAvoidNodeArray* AvoidNodeArray = GetAvoidNodeArray();
	apDest->SetAvoidNodeArray(AvoidNodeArray);
	// fGoalZDelta is set a second time here, as the game does.
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
	const char* v2;
	const char* v3;
	const char* v4;
	const char* v5;
	const char* v6;
	char apBuffer[260];
	Origin.PrintDebugText(apBuffer, 256);
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Origin: %s", apBuffer);
	Destination.PrintDebugText(apBuffer, 256);
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Destination: %s", apBuffer);
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Actor Radius: %f", fActorRadius);
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Goal Z Delta: %f", fGoalZDelta);
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Target Radius: %f", fTargetRadius);
	if (bAllowIncompletePath) {
		v6 = "true";
	} else {
		v6 = "false";
	}
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Allow Incomplete Path: %s", v6);
	if (fCenterRadius != 0.0) {
		apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Center Radius: (%.2f,%.2f,%.2f) %f", Center.x, Center.y, Center.z,
			fCenterRadius);
	}
	if (bFaceTargetAtGoal) {
		v5 = "true";
	} else {
		v5 = "false";
	}
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Face Target at Goal: %s", v5);
	if (bFaceTargetAtGoal) {
		apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Goal Angle: %f", fGoalAngle);
	}
	if (bFirstTangentUsesHeading) {
		apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Initial Path Heading: %f", fInitialPathHeading);
	}
	if (bGoDirectIfPathingFails) {
		v4 = "true";
	} else {
		v4 = "false";
	}
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Allow Direct Path If Pathing Fails: %s", v4);
	if (bTryToUseCurvedPath) {
		v3 = "true";
	} else {
		v3 = "false";
	}
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Using Curved path: %s", v3);
	if (bIgnoreLocks) {
		v2 = "true";
	} else {
		v2 = "false";
	}
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Ignoring Locks: %s", v2);
	apDebugText->PrintLine(DTL_LEFT, DTJ_LEFT, "Pathsmoothing retry count: %i", iRetryCount);
}

void PathingRequest::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	PathingAvoidNodeArray* v2;
	PathingAvoidNodeArray* v3;
	uint32_t formFlags;
	PathingAvoidNode* v6;
	uint32_t auiIndex;
	(&Origin)->SaveGame(apSaveGameBuffer);
	(&Destination)->SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fActorRadius)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fActorHeight)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fGoalZDelta)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fTargetRadius)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fCenterRadius)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fSmoothingRandomness)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fGoalAngle)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bFaceTargetAtGoal)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bCantOpenDoors)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bDrawDebugGeometry)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bAllowIncompletePath)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bSoftGoalRadius)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bCanSwim)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bWaitIfPlayerWaits)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fInitialPathHeading)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bFirstTangentUsesHeading)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bGoDirectIfPathingFails)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&Center)), 0xCu, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bTryToUseCurvedPath)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bIgnoreSmoothingIfFails)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bIgnoreLocks)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iRetryCount)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bCanFly)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bFailIfGoalOccupied)), 1u, 0);
	if ((&spAvoidNodeArray)->m_pObject) {
		v2 = (&spAvoidNodeArray)->m_pObject;
		formFlags = v2->iSize;
	} else {
		formFlags = 0;
	}
	apSaveGameBuffer->SaveVariableSizedValue(formFlags);
	for (auiIndex = 0; auiIndex < formFlags; ++auiIndex) {
		v3 = (&spAvoidNodeArray)->m_pObject;
		v6 = &v3->GetAt(auiIndex);
		v6->SaveGame(apSaveGameBuffer);
	}
}

void PathingRequest::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	Origin.LoadGame(apLoadGameBuffer);
	Destination.LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(&fActorRadius, 4u);
	apLoadGameBuffer->LoadData(&fActorHeight, 4u);
	apLoadGameBuffer->LoadData(&fGoalZDelta, 4u);
	apLoadGameBuffer->LoadData(&fTargetRadius, 4u);
	apLoadGameBuffer->LoadData(&fCenterRadius, 4u);
	apLoadGameBuffer->LoadData(&fSmoothingRandomness, 4u);
	apLoadGameBuffer->LoadData(&fGoalAngle, 4u);
	apLoadGameBuffer->LoadData(&bFaceTargetAtGoal, 1u);
	apLoadGameBuffer->LoadData(&bCantOpenDoors, 1u);
	apLoadGameBuffer->LoadData(&bDrawDebugGeometry, 1u);
	apLoadGameBuffer->LoadData(&bAllowIncompletePath, 1u);
	apLoadGameBuffer->LoadData(&bSoftGoalRadius, 1u);
	apLoadGameBuffer->LoadData(&bCanSwim, 1u);
	if (apLoadGameBuffer->GetVersion() >= 13u) {
		apLoadGameBuffer->LoadData(&bWaitIfPlayerWaits, 1u);
	}
	if (apLoadGameBuffer->GetVersion() >= 15u) {
		apLoadGameBuffer->LoadData(&fInitialPathHeading, 4u);
		apLoadGameBuffer->LoadData(&bFirstTangentUsesHeading, 1u);
		apLoadGameBuffer->LoadData(&bGoDirectIfPathingFails, 1u);
	}
	if (apLoadGameBuffer->GetVersion() >= 18u) {
		apLoadGameBuffer->LoadData(&Center, 12u);
		apLoadGameBuffer->LoadData(&bTryToUseCurvedPath, 1u);
		apLoadGameBuffer->LoadData(&bIgnoreSmoothingIfFails, 1u);
		apLoadGameBuffer->LoadData(&bIgnoreLocks, 1u);
		apLoadGameBuffer->LoadData(&iRetryCount, 4u);
	}
	if (apLoadGameBuffer->GetVersion() >= 20u) {
		apLoadGameBuffer->LoadData(&bCanFly, 1u);
	}
	if (apLoadGameBuffer->GetVersion() >= 21u) {
		apLoadGameBuffer->LoadData(&bFailIfGoalOccupied, 1u);
	}
	uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
	if (isaveGameCount) {
		PathingAvoidNodeArray* v2;
		PathingAvoidNodeArray* apMemory =
			static_cast<PathingAvoidNodeArray*>(NiMemObject::operator new(sizeof(PathingAvoidNodeArray)));
		if (apMemory) {
			v2 = ::new (apMemory) PathingAvoidNodeArray;
		} else {
			v2 = 0;
		}
		spAvoidNodeArray.operator=(v2);
		spAvoidNodeArray.m_pObject->SetSize(isaveGameCount, 1);
		for (uint32_t i = 0; i < isaveGameCount; ++i) {
			PathingAvoidNode* avoidNode = &spAvoidNodeArray.m_pObject->GetAt(i);
			avoidNode->LoadGame(apLoadGameBuffer);
		}
	}
}

void PathingRequest::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (apLoadGameBuffer->GetVersion() < 14u) {
		bWaitIfPlayerWaits = 0;
		Actor* pactor = apLoadGameBuffer->GetActor();
		if (pactor) {
			TESPackage* ppack = pactor->GetCurrentPackage();
			if (ppack) {
				if (ppack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_ESCORT_ACTOR &&
					ppack->GetEscortTargetForPackage(pactor, 0) == pPlayer) {
					bWaitIfPlayerWaits = 1;
				}
			}
		}
	}
}

void PathingRequest::SetOrigin(const PathingLocation& aOrigin) {
	Origin = aOrigin;
}

void PathingRequest::SetAllowIncompletePath(bool abAllow) {
	bAllowIncompletePath = (bool)((uint8_t)abAllow);
}

void PathingRequest::SetRetryCount(int32_t aiRetryCount) {
	iRetryCount = aiRetryCount;
}

void PathingRequest::Initialize(Actor* apActor) {
	FNVD_INLINED_LOCAL(PathingLocation, apLocation, std::nullopt, std::nullopt);
	PathingLocation* aLoc = ::new (&apLocation) PathingLocation(apActor);
	Origin.operator=(*aLoc);
	apLocation.~PathingLocation();
	float aRadius = apActor->GetWidth() * 0.6000000238418579;
	SetActorRadius(aRadius);
	SetActorLockData(apActor);
	bool DoesSwim = apActor->DoesSwim();
	SetCanSwim(DoesSwim);
	bool DoesFly = apActor->DoesFly();
	SetCanFly(DoesFly);
	if (apActor->IsCreature()) {
		TESActorBaseData* v5 = static_cast<TESActorBaseData*>(apActor->GetActorBase());
		SetCantOpenDoors(v5->GetCantOpenDoors());
	}
	float a2 = apActor->GetHeading(0);
	SetInitialPathHeading(a2);
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
	bFaceTargetAtGoal = (bool)1;
}

void PathingRequest::SetCantOpenDoors(bool abCantOpenDoors) {
	bCantOpenDoors = abCantOpenDoors;
}

PathingAvoidNodeArray* PathingRequest::GetAvoidNodeArray() const {
	return (&spAvoidNodeArray)->operator->();
}

bool PathingRequest::GetCanSwim() const {
	return (bool)((uint8_t)bCanSwim);
}

bool PathingRequest::GetCanFly() const {
	return (bool)((uint8_t)bCanFly);
}

bool PathingRequest::GetTryToUseCurvedPath() const {
	return (bool)((uint8_t)bTryToUseCurvedPath);
}

bool PathingRequest::GetIgnoreLocks() const {
	return (bool)((uint8_t)bIgnoreLocks);
}

bool PathingRequest::GetGoDirectIfPathingFails() const {
	return (bool)((uint8_t)bGoDirectIfPathingFails);
}

void PathingRequest::SetCenterRadius(NiPoint3& arCenter, float afOriginRadius) {
	Center = arCenter;
	fCenterRadius = afOriginRadius;
}

void PathingRequest::SetActorLockData(const ActorLockData& aLockData) {
	LockData = aLockData;
}

void PathingRequest::SetActorLockData(Actor* apActor) {
	ActorLockData kLockData;
	LockData = *FNVD_RECONSTRUCT(ActorLockData)(&kLockData, apActor);
}

void PathingRequest::SetCanSwim(bool abCanSwim) {
	bCanSwim = (bool)((uint8_t)abCanSwim);
}

void PathingRequest::SetCanFly(bool abCanFly) {
	bCanFly = (bool)((uint8_t)abCanFly);
}

void PathingRequest::SetInitialPathHeading(float afInitialPathHeading) {
	fInitialPathHeading = afInitialPathHeading;
	bFirstTangentUsesHeading = (bool)1;
}

void PathingRequest::SetGoDirectIfPathingFails(bool abGoDirect) {
	bGoDirectIfPathingFails = (bool)((uint8_t)abGoDirect);
}

void PathingRequest::SetWaitIfPlayerWaits(bool abAbort) {
	bWaitIfPlayerWaits = (bool)((uint8_t)abAbort);
}

void PathingRequest::SetSoftGoalRadius(bool abUseSoft) {
	bSoftGoalRadius = (bool)((uint8_t)abUseSoft);
}

void PathingRequest::SetIgnoreSmoothingIfFails(bool abIgnoreSmoothingIfFails) {
	bIgnoreSmoothingIfFails = (bool)((uint8_t)abIgnoreSmoothingIfFails);
}

bool PathingRequest::GetIgnoreSmoothingIfFails() const {
	return bIgnoreSmoothingIfFails;
}

void PathingRequest::ClearInitialPathHeading() {
	bFirstTangentUsesHeading = (bool)0;
}

bool PathingRequest::GetWaitIfPlayerWaits() const {
	return (bool)((uint8_t)bWaitIfPlayerWaits);
}

bool PathingRequest::GetSoftGoalRadius() const {
	return (bool)((uint8_t)bSoftGoalRadius);
}

NiPoint3* PathingRequest::GetTargetPoint(NiPoint3* a2) const {
	*a2 = Center;
	return a2;
}

float PathingRequest::GetDefaultRadius() {
	return fDefaultRadius;
}

PathingRequest* PathingRequest::CreatePathingRequest(PATHING_REQUEST_TYPE aeType) {
	PathingRequest* result;
	switch (aeType) {
	case PATHING_REQUEST_STANDARD: {
		PathingRequest* v9;
		PathingRequest* v17 = static_cast<PathingRequest*>(NiMemObject::operator new(sizeof(PathingRequest)));
		if (v17) {
			v9 = ::new (v17) PathingRequest;
		} else {
			v9 = 0;
		}
		result = v9;
		break;
	}
	case PATHING_REQUEST_COVER: {
		PathingRequestCover* v8;
		PathingRequestCover* v16 =
			static_cast<PathingRequestCover*>(NiMemObject::operator new(sizeof(PathingRequestCover)));
		if (v16) {
			v8 = ::new (v16) PathingRequestCover;
		} else {
			v8 = 0;
		}
		result = v8;
		break;
	}
	case PATHING_REQUEST_CLOSE_POINT: {
		PathingRequestClosePoint* v7;
		PathingRequestClosePoint* apMemory =
			static_cast<PathingRequestClosePoint*>(NiMemObject::operator new(sizeof(PathingRequestClosePoint)));
		if (apMemory) {
			v7 = ::new (apMemory) PathingRequestClosePoint;
		} else {
			v7 = 0;
		}
		result = v7;
		break;
	}
	case PATHING_REQUEST_FLEE: {
		PathingRequestFlee* v6;
		PathingRequestFlee* v14 =
			static_cast<PathingRequestFlee*>(NiMemObject::operator new(sizeof(PathingRequestFlee)));
		if (v14) {
			v6 = ::new (v14) PathingRequestFlee;
		} else {
			v6 = 0;
		}
		result = v6;
		break;
	}
	case PATHING_REQUEST_HIDE: {
		PathingRequestHide* v5;
		PathingRequestHide* v13 =
			static_cast<PathingRequestHide*>(NiMemObject::operator new(sizeof(PathingRequestHide)));
		if (v13) {
			v5 = ::new (v13) PathingRequestHide;
		} else {
			v5 = 0;
		}
		result = v5;
		break;
	}
	case PATHING_REQUEST_LOS: {
		PathingRequestLOS* v4;
		PathingRequestLOS* v12 = static_cast<PathingRequestLOS*>(NiMemObject::operator new(sizeof(PathingRequestLOS)));
		if (v12) {
			v4 = ::new (v12) PathingRequestLOS;
		} else {
			v4 = 0;
		}
		result = v4;
		break;
	}
	case PATHING_REQUEST_OPTIMAL_LOCATION: {
		PathingRequestOptimalLocation* v3;
		PathingRequestOptimalLocation* v11 = static_cast<
			PathingRequestOptimalLocation*>(NiMemObject::operator new(sizeof(PathingRequestOptimalLocation)));
		if (v11) {
			v3 = ::new (v11) PathingRequestOptimalLocation;
		} else {
			v3 = 0;
		}
		result = v3;
		break;
	}
	case PATHING_REQUEST_COVERED_MOVE: {
		PathingRequestCoveredMove* v2;
		PathingRequestCoveredMove* v10 =
			static_cast<PathingRequestCoveredMove*>(NiMemObject::operator new(sizeof(PathingRequestCoveredMove)));
		if (v10) {
			v2 = ::new (v10) PathingRequestCoveredMove;
		} else {
			v2 = 0;
		}
		result = v2;
		break;
	}
	default: {
		Warning("PATHFINDING: Unknown pathing request type %i", aeType);
		result = 0;
		break;
	}
	}
	return result;
}

float PathingRequestClosePoint::GetMinDistance() const {
	return fMinDistance;
}
