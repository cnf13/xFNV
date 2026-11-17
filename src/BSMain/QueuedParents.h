#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class QueuedFile;

// Parent file-load tasks waiting for a queued file.
class QueuedParents : public BSSimpleArray<NiPointer<QueuedFile>, 1024> {
public:
	QueuedParents();
	~QueuedParents() override;

	void RemoveFastReleasing(int32_t a2, int32_t a3);

	static void operator delete(void* apMemory);
};
