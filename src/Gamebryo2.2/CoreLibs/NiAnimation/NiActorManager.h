#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyMatch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"

// Gamebryo's actor animation manager: plays and blends the sequences of one actor. Only the static data manager's
// shutdown and the two text-key statics are used, so the instance members are not declared yet.
class NiActorManager : public NiRefObject {
public:
    // The kinds of timeline events (4 bytes in TimelineData).
    enum EventType : uint32_t {
        ANIM_ACTIVATED,
        ANIM_DEACTIVATED,
        TEXT_KEY_EVENT,
        END_OF_SEQUENCE,
    };

    // One pending event of the timeline. The destructor releases the pointer and the string it holds. The timeline's
    // list nodes come from the pool NiTAbstractPoolAllocator<TimelineData>.
    class TimelineData : public NiMemObject {
    public:
        // Clears m_spMatchObject, then the members' destructors release m_kTextKeyMatched and m_spMatchObject again.
        ~TimelineData();

        float m_fEventTime;
        EventType m_eEventType;
        uint32_t m_eSequenceID;
        NiPointer<NiTextKeyMatch> m_spMatchObject;
        NiFixedString m_kTextKeyMatched;
    };

    // Sets the two text keys. NiAnimationSDM::Init calls it last.
    static void _SDMInit();

    // Releases the two text keys. NiAnimationSDM::Shutdown calls it.
    static void _SDMShutdown();

    // The default end and start text keys, which _SDMInit sets and _SDMShutdown releases.
    static NiFixedString ms_kEndTextKey;
    static NiFixedString ms_kStartTextKey;
};
