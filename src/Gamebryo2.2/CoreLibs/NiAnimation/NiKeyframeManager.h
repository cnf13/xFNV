#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

// Holds named keyframe sequences.
class NiKeyframeManager : public NiTimeController {
public:
	// The links LoadBinary reads for LinkObject: each sequence's text key reference, name, key count and object
	// name (the PDB's members; LoadBinary 0xA4DB80 allocates 0x40 bytes for it).
	class LinkRecord {
	public:
		// Constructs the four empty arrays, growing by 1 (thiscall, ret 0, 0xA4D780: the
		// vtables 0x101A3AC and 0x1021024 at +0x0, +0x10, +0x20, +0x30).
		LinkRecord();

		NiTPrimitiveArray<uint32_t> m_kTextKeyReferences;
		NiTPrimitiveArray<char*> m_kSequenceNames;
		NiTPrimitiveArray<uint32_t> m_kNumSequenceKeys;
		NiTPrimitiveArray<char*> m_kObjectName;
	};

	// The exe's constructor: the base, the vtable and the empty sequence map of 37 buckets (thiscall, ret 0,
	// 0xA4DA20; CreateObject 0xA4DB60 jumps to it).
	NiKeyframeManager();
	~NiKeyframeManager() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;

protected:
	bool TargetIsRequiredType() const override;
	void Update(NiUpdateData&) override {}

public:
	bool Add(const char* pcSequenceFile, const char* pcSequenceName, uint32_t uiIndex);

	static NiObject* CreateObject();

	NiTStringPointerMap<NiPointer<NiSequence>> m_kMap;

	static NiRTTI ms_RTTI;
	// The record LoadBinary fills for files before 4.1.0.3 and LinkObject consumes and deletes.
	static inline LinkRecord* ms_pkLinkRecord;
};
inline constexpr fnvd::Vtable NiKeyframeManager_vftable{};
