#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryGroupManager.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

NiGeometryGroupManager::NiGeometryGroupManager() {
    ms_pkManager = this;
}

NiGeometryGroupManager::~NiGeometryGroupManager() {
    ms_pkManager = nullptr;
}
