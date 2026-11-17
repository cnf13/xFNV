#pragma once

#include "Reconstruction.hpp"

class BSAnimGroupSequence;
class TESAnimGroup;

// What an animation plays for one animation group: a single sequence or several to choose from. Every slot is pure
// here; the exe's parameter types come from AnimSequenceSingle and AnimSequenceMultiple's bodies.
class AnimSequenceBase {
public:
	AnimSequenceBase();
	virtual ~AnimSequenceBase();

	// Takes apSequence as the sequence to play; abUnloadPrevious also unloads the one it replaces.
	virtual void SetSequence(BSAnimGroupSequence* apSequence, bool abUnloadPrevious) = 0;
	// true if apSequence was held and has been removed.
	virtual bool RemoveSequence(BSAnimGroupSequence* apSequence, bool abUnload) = 0;
	virtual bool IsSingleSeq() = 0;
	// The sequence to play; acIndex picks one of several, -1 picks at random.
	virtual BSAnimGroupSequence* GetSequenceToPlay(char acIndex) = 0;
	// The sequence of animation group apGroup.
	virtual BSAnimGroupSequence* GetSequence(TESAnimGroup* apGroup) = 0;
	// The position of apSequence among those held, -1 when it is not held.
	virtual char GetSequenceIndex(BSAnimGroupSequence* apSequence) = 0;

	static void operator delete(void* apMemory) { ourFree(apMemory); }
};
inline constexpr fnvd::Vtable AnimSequenceBase_vftable{};
