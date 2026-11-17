#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialLibrary.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdlib.h"

NiTPrimitiveSet<NiMaterialLibrary*> NiMaterialLibrary::ms_kMaterialLibraries;

// The name takes a reference on its handle, then the library joins the engine's set.
NiMaterialLibrary::NiMaterialLibrary(const NiFixedString& kName) {
	m_kName = kName;
	ms_kMaterialLibraries.Add(this);
}

// The library leaves the set (the last entry takes its place); the name's reference is released by its NiFixedString
// member.
NiMaterialLibrary::~NiMaterialLibrary() {
	ms_kMaterialLibraries.Remove(this);
}

NiMaterial* NiMaterialLibrary::GetMaterial(const NiFixedString&) {
	return nullptr;
}

const char* NiMaterialLibrary::GetMaterialName(uint32_t) {
	return nullptr;
}

uint32_t NiMaterialLibrary::GetMaterialCount() const {
	return 0;
}

// The exe inlines RemoveAll as the used-count store, then frees the storage with Realloc(0).
void NiMaterialLibrary::_SDMShutdown() {
	ms_kMaterialLibraries.RemoveAll();
	ms_kMaterialLibraries.Realloc(0);
}
