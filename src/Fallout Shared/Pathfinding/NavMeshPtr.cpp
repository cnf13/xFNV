#include "Fallout Shared/Pathfinding/NavMeshPtr.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

NavMeshPtr::NavMeshPtr() : NiPointer<NavMesh>(nullptr) {}

NavMeshPtr::NavMeshPtr(NavMesh* apNavMesh) : NiPointer<NavMesh>(apNavMesh) {}

NavMeshPtr::~NavMeshPtr() {}

void NavMeshPtr::DecRefCount() {
    if (m_pObject) {
        static_cast<NiRefObject*>(m_pObject)->DecRefCount();
    }
}

bool NavMeshPtr::operator!=(const NavMeshPtr& ptr) const {
    return (bool)((int32_t)NiPointer<NavMesh>::operator!=(ptr));
}

bool NavMeshPtr::operator==(const NavMeshPtr& ptr) const {
    return NiPointer<NavMesh>::operator==(ptr);
}

NavMeshPtr::operator NavMesh*() const {
    return NiPointer<NavMesh>::operator->();
}

NavMeshPtr* NavMeshPtr::MakeNavMeshPtr(NavMeshPtr* apResult, NavMesh* apNavMesh) {
    ::new (apResult) NavMeshPtr(apNavMesh);
    return apResult;
}

void NavMeshPtr::MakeNavMeshPtr(NavMeshPtr& arPtr, NavMesh* apNavMesh) {
    NavMeshPtr apMemory(apNavMesh);
    arPtr.operator=(apMemory);
}
