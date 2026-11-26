#include "Fallout Shared/Sky/SkyTextureManager.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSShaderManager.h"
#include "BSSystem/Archive.h"
#include "Fallout Shared/Sky/SkyTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

void SkyTextureManager::LoadTextureInBackground(NiPointer<NiTexture>* apspTexture, const char* apFilename) {
    if (apFilename) {
        char cPath[268];
        StandardizeTexturePath(apFilename, cPath, 260u);
    }
    SkyTask* pTask = new SkyTask(apspTexture, apFilename);
    pIOManager->AddTask(pTask);
}

SkyTask::SkyTask(NiPointer<NiTexture>* apspTexture, const char* apFilename)
  : QueuedFileEntry(IO_TASK_PRIORITY_MID), pspTexture(apspTexture) {
    SetFileName(apFilename);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_TEXTURES);
}

SkyTask::~SkyTask() {}

void SkyTask::Run() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\sky\\SkyTextureManager.cpp",
        50
    );
    BSShaderManager::GetTexture(pFileName, true, *pspTexture, true, false);
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
