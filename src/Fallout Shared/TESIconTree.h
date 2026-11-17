#pragma once

#include "Fallout Shared/TESTexture1024.h"

// The icon texture of a tree.
class TESIconTree : public TESTexture1024 {
public:
	TESIconTree();
	~TESIconTree();

	const char* GetDefaultPath() const override;
};
