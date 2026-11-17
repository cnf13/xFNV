#include "Fallout Shared/ActorValueInfo.h"

#include "Fallout Shared/ActorValue.h"
#include "Reconstruction.hpp"

uint32_t ActorValueInfo::GetActorValueFormID(ActorValue::Index aiWhich) {
	ActorValue::Section eSection = ActorValue::GetSection(static_cast<ActorValue::Index>(aiWhich));
	// each section occupies a block of 100 form IDs starting at 1000
	int32_t iResult = 100 * eSection + 1000;
	// The exe passes aiWhich's whole dword on to ToArrayIndex.
	iResult += ActorValue::ToArrayIndex(eSection, aiWhich);
	return iResult;
}
