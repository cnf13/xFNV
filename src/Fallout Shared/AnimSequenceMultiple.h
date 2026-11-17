#pragma once

#include "Fallout Shared/AnimSequenceBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

class AnimSequenceSingle;
class BSAnimGroupSequence;

enum ANIM_GROUP_SECTION : int32_t;

// An animation group that has several sequences to choose from.
class AnimSequenceMultiple : public AnimSequenceBase {
public:
	// Takes over apSingleSeq's sequence and deletes apSingleSeq.
	AnimSequenceMultiple(AnimSequenceSingle* apSingleSeq);
	~AnimSequenceMultiple() override;

	void SetSequence(BSAnimGroupSequence* apSeq, bool abReleaseModelLoader) override;
	bool RemoveSequence(BSAnimGroupSequence* apSeq, bool abReleaseModelLoader) override;
	bool IsSingleSeq() override;
	BSAnimGroupSequence* GetSequenceToPlay(char aiSequenceIndex) override;
	BSAnimGroupSequence* GetSequence(TESAnimGroup* apAnimGroup) override;
	char GetSequenceIndex(BSAnimGroupSequence* apSeq) override;

	// The sequence of this multiple sequence that corresponds to apSeq in aeSection.
	BSAnimGroupSequence* GetCorrespondingSequence(BSAnimGroupSequence* apSeq, ANIM_GROUP_SECTION aeSection);

	// The sequences' references are counted by hand, as AnimSequenceSingle does.
	NiTPointerList<BSAnimGroupSequence*>* pSeqList;
};
