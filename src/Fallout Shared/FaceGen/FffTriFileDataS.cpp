#include "Fallout Shared/FaceGen/FffTriFileDataS.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"

#include <memory>
#include <vector>

// The morph delta vector's at(): the exe's copy is vtxListAt's (folded).
const FR2Vect3TC<float>&
VectorSubscript(const std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>* apThis, uint32_t _Pos) {
	if (_Pos >= apThis->size()) {
		rt::_invalid_parameter_noinfo();
	}
	return (*apThis)[_Pos];
}

FffTriFileDataS::FffTriFileDataS() {}

// size() of vtxList, inline: the byte span divided by the 12-byte element with a signed idiv.
uint32_t FffTriFileDataS::vtxListSize() const {
	return static_cast<uint32_t>(vtxList.size());
}

const FR2Vect3TC<float>& FffTriFileDataS::vtxListAt(uint32_t i) const {
	if (i >= vtxListSize()) {
		rt::_invalid_parameter_noinfo();
	}
	return vtxList.data()[i];
}

void FffTriFileDataS::operator delete(void* apMemory) {
	ourFree(apMemory);
}
