#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"

#include <stdint.h>

struct BGSDebrisData;

// One model of a debris form. Only some of its members are declared.
struct BGSDebrisData {
    BGSDebrisData();
    ~BGSDebrisData();

    // Whether bit 0 of cFlags is set.
    bool GetHasCollisionData();
    void SetFileName(const char* apFileName);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    int8_t cPercentage;
    // Bit 0: the model has collision data.
    int8_t cFlags;
    uint8_t padding02[2];
    char* pFileName;
    TESTextureList TextureList;
};

// Ranks a1 before a2 by the higher percentage, then by file name.
int32_t DebrisSortFunction(BGSDebrisData* a1, BGSDebrisData* a2);

// A debris form (DEBR): the models a destroyed object scatters.
class BGSDebris : public TESForm, public BGSPreloadable {
public:
    BGSDebris();
    ~BGSDebris() override;

    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) override;
    using TESForm::Save;

    // Adds a debris model and returns its data.
    BGSDebrisData* AddDebrisData(int8_t acPercentage, const char* apFileName, int8_t acFlags);
    // Returns &DataList.
    BSSimpleList<BGSDebrisData*>* GetData();
    // Scatters aiNumParticles pieces of debris at arLocation in apCell, scaled by afScale.
    void SpawnDebris(TESObjectCELL* apCell, const NiPoint3& arLocation, uint32_t aiNumParticles, float afScale);
    // Inserts apDebrisData into DataList, sorted, when it has a percentage and a non-empty file name; false otherwise.
    bool AddDebrisData(BGSDebrisData* apDebrisData);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSSimpleList<BGSDebrisData*> DataList;
};
