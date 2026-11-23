#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/BaseFormComponent.h"

// A sound file name.
class TESSoundFile : public BaseFormComponent {
public:
    TESSoundFile();
    ~TESSoundFile();

    void InitializeDataComponent() override;
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;
    virtual void SetSoundFile(const char* apSound);

    // Saves the sound file name as a chunk.
    void Save();
    const char* GetSoundFile();
    uint32_t GetSoundFileLength();

    BSStringT<char> cSoundFile;
};
