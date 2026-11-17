#include "BSCore/BSSpinLock.h"
#include "BSMain/MessageHandler.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"

uint32_t NiTAbstractPoolAllocator<uint32_t>::ms_uiBlockSize = 2048;
NiTAbstractPoolAllocator<uint32_t>::AllocNode* NiTAbstractPoolAllocator<uint32_t>::ms_pkBlockHeader;
NiTAbstractPoolAllocator<uint32_t>::AllocNode* NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;

BSSpinLock NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock;

// The static message handler (MessageHandler::Construct's abStatic set).
static MessageHandler messageHandler(true);
