#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

#include <stdint.h>

class IBSAnimNoteReceiver;

// What a BSAnimNoteListener hands the animation notes it hears to.
class IBSAnimNoteReceiver {
public:
    IBSAnimNoteReceiver();
    virtual ~IBSAnimNoteReceiver();

    // Handles one note.
    virtual void Update(BSAnimNote* apNote) = 0;

    // The deleting destructors free through ourFree.
    static void operator delete(void* apMemory);
};

// A controller manager's list of the receivers its sequences' animation notes go to.
class BSAnimNoteListener {
public:
    // A receiver and the note type it listens for.
    struct BSAnimReceiverType {
        BSAnimNote::AnimNoteType eNoteType;
        IBSAnimNoteReceiver* pReceiver;
    };

    BSAnimNoteListener();
    ~BSAnimNoteListener();

    // Adds apReceiver for notes of type aeType unless it is registered for them already; returns false.
    bool Register(BSAnimNote::AnimNoteType aeType, IBSAnimNoteReceiver* apReceiver);
    void Update(BSAnimNote* apNote);
    int32_t Registered(BSAnimNote::AnimNoteType aeType, IBSAnimNoteReceiver* apReceiver);

    // The registered receivers.
    NiTPrimitiveArray<BSAnimReceiverType*> ReceiverA;
};

inline IBSAnimNoteReceiver::~IBSAnimNoteReceiver() {}
