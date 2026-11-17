#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"

#include "BSCore/BSTFreeList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

ActorPathingMessage::ActorPathingMessage() : msgType(PMT_UNDEF), spSolution(nullptr) {}

ActorPathingMessage::ActorPathingMessage(uint32_t aType, const NiPointer<PathingSolution>& aspSolution)
	: msgType(static_cast<PATHING_MESSAGE_TYPE>(aType)), spSolution(aspSolution) {}

ActorPathingMessage::ActorPathingMessage(const ActorPathingMessage& aMsg)
	: msgType(aMsg.msgType), spSolution(aMsg.spSolution) {}

void ActorPathingMessage::operator delete(void* apMemory) {
	ourFree(apMemory);
}
