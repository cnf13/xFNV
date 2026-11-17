#include "Fallout Shared/Pathfinding/PathingRequest.h"

PathingRequestClosePoint::PathingRequestClosePoint() : PathingRequest(), fMinDistance(128.0f), fMaxDistance(2048.0f) {}

PathingRequestClosePoint::PathingRequestClosePoint(const PathingRequestClosePoint& aRequest)
  : PathingRequest(aRequest), fMinDistance(aRequest.fMinDistance), fMaxDistance(aRequest.fMaxDistance) {}

PathingRequestClosePoint::~PathingRequestClosePoint() {}

void PathingRequestClosePoint::CopyTo(PathingRequest* apDest) const {
    PathingRequest::CopyTo(apDest);
    static_cast<PathingRequestClosePoint*>(apDest)->SetMinDistance(fMinDistance);
    static_cast<PathingRequestClosePoint*>(apDest)->SetMaxDistance(fMaxDistance);
}

uint32_t PathingRequestClosePoint::GetType() const {
    return PATHING_REQUEST_CLOSE_POINT;
}

void PathingRequestClosePoint::SetMinDistance(float afDistance) {
    fMinDistance = afDistance;
}

void PathingRequestClosePoint::SetMaxDistance(float afDistance) {
    fMaxDistance = afDistance;
}
