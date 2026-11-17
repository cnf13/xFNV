#include "Fallout Shared/Pathfinding/RemoveIndex.h"

RemoveIndex::RemoveIndex() : iMasterIndex(-1), iMinorIndex(-1) {}

RemoveIndex::RemoveIndex(int32_t aiMasterIndex, int32_t aiMinorIndex)
	: iMasterIndex(aiMasterIndex), iMinorIndex(aiMinorIndex) {}
