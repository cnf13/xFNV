#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiGeomMorpherController;
class NiGeomMorpherUpdateTask;

// A pooled task that runs one geomorpher controller's vertex update on the parallel update thread.
class NiGeomMorpherUpdateTask : public NiTask {
public:
	NiGeomMorpherUpdateTask();
	~NiGeomMorpherUpdateTask() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void DoTask() override;
	// Drops the controller and target and returns the task to the pool; true.
	bool Clear() override;

	// Holds the controller and the target.
	void SetController(NiGeomMorpherController* pkController, NiGeometry* pkTarget);

	// A task from the pool, which grows when empty.
	static NiGeomMorpherUpdateTask* GetFreeObject();
	// Purges ms_kPool. NiAnimationSDM::Shutdown calls it last.
	static void _SDMShutdown();

	NiPointer<NiGeomMorpherController> m_spMorphController;
	NiPointer<NiObjectNET> m_spTarget;

	static NiRTTI ms_RTTI;
	static NiTObjectPool<NiGeomMorpherUpdateTask> ms_kPool;
	static NiCriticalSection ms_kPoolCriticalSection;
};
