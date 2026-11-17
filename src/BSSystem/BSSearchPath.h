#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSearchPath.h"
#include "Reconstruction.hpp"

// The search path of the engine's files: walks the search directories and tests whether a file exists in them.
class BSSearchPath : public NiSearchPath {
public:
	BSSearchPath();
	// The type of NiSearchPath's create callback.
	~BSSearchPath() override;

	bool GetNextSearchPath(char* apPath, uint32_t uiStringLen) override;
	virtual bool Access(char* apFile);

	static void InitCallback();
	static void operator delete(void* apMemory);
	static NiSearchPath* GetBSSearchPath();
};
inline constexpr fnvd::Vtable BSSearchPath_vftable{};
