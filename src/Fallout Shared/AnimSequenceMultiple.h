#pragma once

#include "Fallout Shared/AnimSequenceBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Reconstruction.hpp"

enum ANIM_GROUP_SECTION : int32_t;
class AnimSequenceSingle;
class BSAnimGroupSequence;

// An animation group that has several sequences to choose from (IDA's pSequences; the PDB's pSeqList). The PDB's
// source module is Animation.cpp.
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

	// The exe counts the sequences' references by hand, as AnimSequenceSingle does.
	NiTPointerList<BSAnimGroupSequence*>* pSeqList;
};
inline constexpr fnvd::Vtable AnimSequenceMultiple_vftable{};
