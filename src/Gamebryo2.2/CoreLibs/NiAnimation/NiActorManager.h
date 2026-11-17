#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyMatch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Reconstruction.hpp"

// Gamebryo's actor animation manager (Gamebryo2.2\CoreLibs\NiAnimation\NiActorManager.h): plays and blends the
// sequences of one actor. The exe has only the static data manager's shutdown and the two text-key statics, so the
// instance members (the PDB's m_spKFMTool at +0x8 to m_kExtraSequences at +0x9C, sizeof 0xA8) are not declared yet.
class NiActorManager : public NiRefObject {
public:
	// The kinds of timeline events (the PDB's enum; 4 bytes in TimelineData).
	enum EventType : uint32_t {
		ANIM_ACTIVATED,
		ANIM_DEACTIVATED,
		TEXT_KEY_EVENT,
		END_OF_SEQUENCE,
	};

	// One pending event of the timeline (the PDB's members and offsets; the exe's destructor 0xA2DD10 releases the
	// pointer at +0xC and the string at +0x10). The timeline's list nodes come from the pool
	// NiTAbstractPoolAllocator<TimelineData>, whose node holds it at +8.
	class TimelineData : public NiMemObject {
	public:
		// The PDB's ~TimelineData: clears m_spMatchObject, then the members' destructors
		// release m_kTextKeyMatched and m_spMatchObject again (thiscall, ret 0, 0xA2DD10).
		~TimelineData();

		float m_fEventTime;
		EventType m_eEventType;
		uint32_t m_eSequenceID;
		NiPointer<NiTextKeyMatch> m_spMatchObject;
		NiFixedString m_kTextKeyMatched;
	};

	// Sets the two text keys (cdecl, ret 0, 0xA421C0; the PDB's public static _SDMInit; IDA
	// counts it into .text_A420E0). The exe's NiAnimationSDM::Init (0xA2D8A0) jumps to it last.
	static void _SDMInit();

	// Releases the two text keys (cdecl, ret 0, 0xA42230; the PDB's public static
	// _SDMShutdown). The exe's NiAnimationSDM::Shutdown (0xA2DE70) calls it with no arguments.
	static void _SDMShutdown();

	// The default end and start text keys, which _SDMInit sets and _SDMShutdown releases.
	static NiFixedString ms_kEndTextKey;
	static NiFixedString ms_kStartTextKey;
};
