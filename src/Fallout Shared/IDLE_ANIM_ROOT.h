#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/NiFormArray.h"

// The idle manager's root animation arrays, directory and per-group keyframe file lists.
class IDLE_ANIM_ROOT {
public:
    IDLE_ANIM_ROOT(const char* apIdleDir);
    ~IDLE_ANIM_ROOT();

    void AddRootFilenameList(BSSimpleList<const char*>* apList);

    static void operator delete(void* apMemory);

    NiFormArray IdleArray;
    NiFormArray LooseIdleArray;
    BSStringT<char> IdleDir;
    BSSimpleList<char const*>* pKFFileList[12];
};
