#pragma once

#include "BSCore/BSTFreeList.h"
#include "BSCore/BSTStaticFreeList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class PathingSolution;

// The kind of a pathing message (values from the Xbox PDB).
enum PATHING_MESSAGE_TYPE : int32_t {
	PMT_UNDEF = -1,
	PMT_OK = 0,
	PMT_ERROR = 1,
};

// A message from the pathing thread to an actor's mover: the kind of message and the path found. The PDB's source
// module is PathManager.cpp. IDA names msgType unk00 and spSolution spPathingSolution.
class ActorPathingMessage {
public:
	// The default message: msgType PMT_UNDEF, spSolution null.
	ActorPathingMessage();
	// A message of kind aType about aspSolution. PDB: aType is a PATHING_MESSAGE_TYPE.
	ActorPathingMessage(uint32_t aType, const NiPointer<PathingSolution>& aspSolution);
	// Copies msgType and spSolution.
	ActorPathingMessage(const ActorPathingMessage& aMsg);

	// The PDB's user-declared copy assignment: copies msgType, then assigns spSolution
	// through NiPointer's assignment (0x6E5CC0); returns this (thiscall, ret 4, 0x6E9C60). Defined in PathManager.cpp.
	ActorPathingMessage& operator=(const ActorPathingMessage& aMsg);

	// The exe's deleting destructor frees the message with ourFree.
	static void operator delete(void* apMemory);

	PATHING_MESSAGE_TYPE msgType;
	NiPointer<PathingSolution> spSolution;
};
