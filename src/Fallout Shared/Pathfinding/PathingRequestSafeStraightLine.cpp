#include "Fallout Shared/Pathfinding/PathingRequestSafeStraightLine.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

PathingRequestSafeStraightLine::PathingRequestSafeStraightLine() : PathingRequestClosePoint(), pTarget(nullptr) {}

PathingRequestSafeStraightLine::PathingRequestSafeStraightLine(const PathingRequestSafeStraightLine& aRequest)
	: PathingRequestClosePoint(aRequest), pTarget(aRequest.pTarget) {}

PathingRequestSafeStraightLine::~PathingRequestSafeStraightLine() {}

// The exe calls PathingRequest::CopyTo (0x6E2750), not PathingRequestClosePoint's, then hands the target on.
void PathingRequestSafeStraightLine::CopyTo(PathingRequest* apDest) const {
	PathingRequest::CopyTo(apDest);
	static_cast<PathingRequestSafeStraightLine*>(apDest)->SetTarget(pTarget);
}

uint32_t PathingRequestSafeStraightLine::GetType() const {
	return PATHING_REQUEST_STRAIGHT_LINE;
}

void PathingRequestSafeStraightLine::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	PathingRequestClosePoint::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveFormID(pTarget, 0);
}

void PathingRequestSafeStraightLine::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	PathingRequestClosePoint::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadFormID(pTarget);
}

void PathingRequestSafeStraightLine::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESObjectREFR* v3;
	if (pTarget) {
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
		v3 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
	} else {
		v3 = 0;
	}
	pTarget = v3;
}

void PathingRequestSafeStraightLine::SetTarget(TESObjectREFR* apTarget) {
	pTarget = pTarget;
}
