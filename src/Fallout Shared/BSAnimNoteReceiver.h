#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class TESObjectREFR;

// What a BSAnimNoteListener hands the animation notes it hears to.
class IBSAnimNoteReceiver {
public:
	IBSAnimNoteReceiver();
	virtual ~IBSAnimNoteReceiver();

	// Handles one note (slot 1, pure in the exe).
	virtual void Update(BSAnimNote* apNote) = 0;

	// The exe's deleting destructors (0x4B0550, 0x4B0580) free through ourFree.
	static void operator delete(void* apMemory);
};

// A reference's receiver: calls the function registered for each note type with the reference and the note.
class BSAnimNoteReceiver : public IBSAnimNoteReceiver {
public:
	struct BSAnimNoteReceiverType;

	// Constructs an empty receiver in apMemory.
	BSAnimNoteReceiver();
	~BSAnimNoteReceiver() override;

	// Calls the registered function of each type that matches apNote's (slot 1; thiscall, ret 4, 0x4B0730).
	void Update(BSAnimNote* apNote) override;

	// The index of the registered type aeType with callback apCallbackFunc (the last match), -1 when there is none.
	int32_t Registered(BSAnimNote::AnimNoteType aeType, void (*apCallbackFunc)(TESObjectREFR*, BSAnimNote*));
	// Registers apCallbackFunc for notes of type aeType.
	bool Register(BSAnimNote::AnimNoteType aeType, void (*apCallbackFunc)(TESObjectREFR*, BSAnimNote*));
	// Stores pObjRefr; the exe's body is a shared one-field setter (thiscall, ret 4,
	// 0x6ECD40).
	void SetObjectRefr(TESObjectREFR* apObjRefr);

	TESObjectREFR* pObjRefr;
	NiTPrimitiveArray<BSAnimNoteReceiverType*> CallTypeA;
};

// One registered note type and the function it calls.
struct BSAnimNoteReceiver::BSAnimNoteReceiverType {
	static BSAnimNoteReceiverType* Construct(void* apMemory, BSAnimNote::AnimNoteType aeType,
		void (*apCallback)(TESObjectREFR*, BSAnimNote*));

	BSAnimNote::AnimNoteType eType;
	void (*pCallback)(TESObjectREFR*, BSAnimNote*);
};

// The note-type array the receiver holds: the exe's derived constructor, the base array's constructor it calls and the
// derived vtable (defined in Fallout Shared/TESObjectREFR.cpp).
using NiTArrayAnimNoteType = NiTArray<BSAnimNoteReceiver::BSAnimNoteReceiverType*,
	NiTMallocInterface<BSAnimNoteReceiver::BSAnimNoteReceiverType*>>;
using NiTPrimitiveArrayAnimNoteType = NiTPrimitiveArray<BSAnimNoteReceiver::BSAnimNoteReceiverType*>;

inline constexpr fnvd::Vtable IBSAnimNoteReceiver_vftable{};
inline constexpr fnvd::Vtable BSAnimNoteReceiver_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_BSAnimNoteType_vftable{};
