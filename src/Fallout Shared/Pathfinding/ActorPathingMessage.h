#pragma once

#include "BSCore/BSTFreeList.h"
#include "BSCore/BSTStaticFreeList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class PathingSolution;

// The kind of a pathing message.
enum PATHING_MESSAGE_TYPE : int32_t {
	PMT_UNDEF = -1,
	PMT_OK = 0,
	PMT_ERROR = 1,
};

// A message from the pathing thread to an actor's mover: the kind of message and the path found.
class ActorPathingMessage {
public:
	// The default message: msgType PMT_UNDEF, spSolution null.
	ActorPathingMessage();
	// A message of kind aType about aspSolution. aType is a PATHING_MESSAGE_TYPE.
	ActorPathingMessage(uint32_t aType, const NiPointer<PathingSolution>& aspSolution);
	// Copies msgType and spSolution.
	ActorPathingMessage(const ActorPathingMessage& aMsg);

	// User-declared copy assignment: copies msgType, then assigns spSolution through NiPointer's assignment. Defined in
	// PathManager.cpp.
	ActorPathingMessage& operator=(const ActorPathingMessage& aMsg);

	// Deleting a message frees it with ourFree.
	static void operator delete(void* apMemory);

	PATHING_MESSAGE_TYPE msgType;
	NiPointer<PathingSolution> spSolution;
};
