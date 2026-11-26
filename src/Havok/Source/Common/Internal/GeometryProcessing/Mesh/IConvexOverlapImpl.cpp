#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/IConvexOverlapImpl.h"

#include "BSCore/NewOverloads.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkgpMesh.h"

IConvexOverlapImpl::~IConvexOverlapImpl() {}

void IConvexOverlapImpl::operator delete(void* apMemory) {
    ourFree(apMemory);
}
