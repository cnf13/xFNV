#pragma once

#include "Fallout Shared/ModelLoader.h"
#include "Reconstruction.hpp"

class Animation;
class QueuedReplacementKFList;

// The load of one replacement animation file for apAnimation, reporting to the list that owns it. The PDB's source
// module is ModelLoader.cpp.
class QueuedReplacementKF : public QueuedKF {
public:
	QueuedReplacementKF(const char* apFileName, IO_TASK_PRIORITY aePriority, Animation* apAnim,
		QueuedReplacementKFList* apOwner);
	~QueuedReplacementKF() override;

	void Finish() override;
	void PostProcess() override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;

	// IDA's pAnimation.
	Animation* pAnim;
	QueuedReplacementKFList* pOwner;
};
inline constexpr fnvd::Vtable QueuedReplacementKF_vftable{};
