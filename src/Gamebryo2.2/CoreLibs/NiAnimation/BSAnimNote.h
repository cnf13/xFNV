#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiTextKey;

// A note an animation sequence carries for one of its text keys; the subclasses drive IK while the sequence plays.
class BSAnimNote : public NiObject {
public:
	// The kind of note.
	enum AnimNoteType {
		BS_ANT_INVALID,
		BS_ANT_GRABIK,
		BS_ANT_LOOKIK,
	};

protected:
	// The base construction the exe inlines into the subclasses' constructors (BSGrabIKNote, BSLookIKNote): the
	// note's type and time.
	BSAnimNote(AnimNoteType aeType, float afTime) : eType(aeType), fTime(afTime) {}

public:
	// the exe's vtable holds _purecall for the destructor
	~BSAnimNote() override = 0;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& arStream) override;
	// LinkObject and RegisterStreamables fold into jumps to the NiObject bodies
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	// Reads the note's settings from its text key; the base takes the key's time.
	virtual bool ParseTextKey(const NiTextKey* apTextKey);

protected:
	void CopyMembers(BSAnimNote* pDest, NiCloningProcess& arCloneData);

public:
	AnimNoteType eType;
	float fTime;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSAnimNote);

// The notes of an animation sequence; it owns them and destroys them with itself.
class BSAnimNotes : public NiObject {
public:
	BSAnimNotes();
	~BSAnimNotes() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	// IsEqual and GetViewerStrings fold into jumps to the NiObject bodies
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	static NiObject* CreateObject();

	BSAnimNote** NoteA;
	uint16_t usNumNotes;

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable BSAnimNote_vftable{};
inline constexpr fnvd::Vtable BSAnimNotes_vftable{};
