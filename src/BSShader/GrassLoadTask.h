#pragma once

#include "BSMain/BSStream.h"
#include "BSMain/QueuedFiles.h"

// A queued load of a grass model (.nif) into its own stream.
class GrassLoadTask : public QueuedFileEntry {
public:
	GrassLoadTask(const char* apName);
	~GrassLoadTask() override;

	void Run() override;
	void Finish() override {}
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	uint8_t GetFileIndex() override;

	// The stream's first top-level object, or null; the caller gets a reference.
	NiPointer<NiObject> GetObject();

	BSStream m_kStream;
};
