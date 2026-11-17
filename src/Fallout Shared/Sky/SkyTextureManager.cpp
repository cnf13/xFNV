#include "Fallout Shared/Sky/SkyTextureManager.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSShaderManager.h"
#include "BSSystem/Archive.h"
#include "Fallout Shared/Sky/SkyTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

void SkyTextureManager::LoadTextureInBackground(NiPointer<NiTexture>* apspTexture, const char* apFilename) {
	SkyTask* v2;
	if (apFilename) {
		char apOutPath[268];
		StandardizeTexturePath(apFilename, apOutPath, 260u);
	}
	SkyTask* apMemory = static_cast<SkyTask*>(ourMalloc(sizeof(SkyTask)));
	if (apMemory) {
		v2 = ::new (apMemory) SkyTask(apspTexture, apFilename);
	} else {
		v2 = 0;
	}
	pIOManager->AddTask(v2);
}

SkyTask::SkyTask(NiPointer<NiTexture>* apspTexture, const char* apFilename)
	: QueuedFileEntry(IO_TASK_PRIORITY_MID), pspTexture(apspTexture) {
	SetFileName(apFilename);
	LookupFileEntry(ARCHIVE_TYPE_INDEX_TEXTURES);
}

SkyTask::~SkyTask() {}

void SkyTask::Run() {
	AutoMemContext tempMemContext(MEM_CONTEXT_SKY, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\sky\\SkyTextureManager.cpp", 50);
	BSShaderManager::GetTexture(pFileName, 1, *pspTexture, 1, 0);
}

bool SkyTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
	return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("skytask"));
}

// The sky texture is read from no particular file.
uint8_t SkyTask::GetFileIndex() {
	return 0;
}

void SkyTask::operator delete(void* apMemory) {
	ourFree(apMemory);
}
