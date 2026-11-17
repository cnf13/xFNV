#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class QueuedFile;

// Parent file-load tasks waiting for a queued file. The PDB's base; the exe's constructor sets that instantiation's
// vtable (0x10C14FC) and calls its _InitialSetup (0xC3D780).
class QueuedParents : public BSSimpleArray<NiPointer<QueuedFile>, 1024> {
public:
	QueuedParents();
	~QueuedParents() override;

	void RemoveFastReleasing(int32_t a2, int32_t a3);

	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable QueuedParents_vftable{};
