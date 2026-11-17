#pragma once

#include "Fallout Shared/AnimSequenceBase.h"

class BSAnimGroupSequence;

// An animation group that plays one sequence.
class AnimSequenceSingle : public AnimSequenceBase {
public:
	AnimSequenceSingle();
	~AnimSequenceSingle() override;

	void SetSequence(BSAnimGroupSequence* apSeq, bool abReleaseModelLoader) override;
	bool RemoveSequence(BSAnimGroupSequence* apSeq, bool abReleaseModelLoader) override;
	bool IsSingleSeq() override;
	BSAnimGroupSequence* GetSequenceToPlay(char aiSequenceIndex) override;
	BSAnimGroupSequence* GetSequence(TESAnimGroup* apAnimGroup) override;
	char GetSequenceIndex(BSAnimGroupSequence* apSeq) override;

	// A plain pointer whose references are counted by hand: SetSequence releases the old sequence before it adds a
	// reference to the new one, and the destructor releases it before unloading the KF model.
	BSAnimGroupSequence* pSeq;
};
