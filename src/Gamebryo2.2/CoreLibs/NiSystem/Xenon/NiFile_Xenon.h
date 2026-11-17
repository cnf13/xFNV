#pragma once

#include "BSCore/BSList.h"
#include "BSCore/BSSemaphore.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/ehvec.h"

inline BSThread* s_thread_0;

inline BSList<BSSemaphore*>* s_wait_handle_queue;

inline BSSemaphore* s_wait_handles;
