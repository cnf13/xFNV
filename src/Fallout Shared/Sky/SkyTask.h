#pragma once

#include "BSMain/QueuedFiles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// A queued load of a sky texture by archive path.
class SkyTask : public QueuedFileEntry {
public:
    SkyTask(NiPointer<NiTexture>* apspTexture, const char* apFilename);
    ~SkyTask() override;

    void Run() override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    uint8_t GetFileIndex() override;

    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);

    // The caller's texture slot: the constructor stores the slot it is given and Run has BSShaderManager::GetTexture
    // fill it; the destructor releases nothing.
    NiPointer<NiTexture>* pspTexture;
};
