#pragma once

#include "BSMain/QueuedFiles.h"

#include <stdint.h>

class BSGameSound;

// A queued file task for a game sound. It has its own header because BSAudioManager.h including QueuedFiles.h makes an
// include cycle back to BSGameSound.h.
class AudioLoadTask : public QueuedFileEntry {
public:
	AudioLoadTask(BSGameSound* apSound);
	~AudioLoadTask() override;

	void Run() override;
	void Finish() override;
	void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	uint8_t GetFileIndex() override;

	static void operator delete(void* apMemory);

	BSGameSound* pSound;
	uint8_t padding34[4];

	static inline int32_t uiLastID;
	static inline int32_t uiLastTime;
};
