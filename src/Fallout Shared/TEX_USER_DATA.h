#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiTexture;
class TESForm;

// A texture's memory size and the forms that use it.
class TEX_USER_DATA {
public:
	TEX_USER_DATA();
	~TEX_USER_DATA();

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	NiPointer<NiTexture> spTexture;
	uint32_t ulTextureSize;
	BSSimpleList<TESForm*> UserList;
};
