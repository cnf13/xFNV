#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Reconstruction.hpp"

class BGSMenuIcon;

// One button of a message box: the text it shows and the conditions that show it.
struct MESSAGEBOX_BUTTON {
	MESSAGEBOX_BUTTON();
	~MESSAGEBOX_BUTTON();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSStringT<char> Text;
	TESCondition Conditions;
};

// A message form (MESG).
class BGSMessage : public TESForm, public TESFullName, public TESDescription {
public:
	BGSMessage();
	~BGSMessage() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// Empties the message's button list.
	void ClearButtonList();
	bool GetIsMessageBox();
	bool IsMsgFlag2Set();
	// Adds a button to the message's button list.
	void AddButton(MESSAGEBOX_BUTTON* apItem);
	// Not named in the PDB or IDA; name chosen here. Stores pIcon; the exe's shared setter of
	// the dword at +0x2C (BGSLoadFormBuffer::SetCurrentFormChange, thiscall, ret 4, 0x86CF00).
	void SetIcon(BGSMenuIcon* apIcon);
	// pIcon; the exe's body is a shared one-field getter (thiscall, ret 0, 0x55B980).
	BGSMenuIcon* GetIcon();
	// iDisplayTime; the exe's body is a shared one-field getter (thiscall, ret 0, 0x639B40).
	uint32_t GetDisplayTime();
	// &ButtonList; the exe's body is a shared field-address getter (thiscall, ret 0, 0x436AA0).
	BSSimpleList<MESSAGEBOX_BUTTON*>* GetButtonList();
	BGSMessage* SetIsMessageBox(uint8_t abVal);
	BGSMessage* SetAutoDisplay(uint8_t abVal);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BGSMenuIcon* pIcon;
	BSSimpleList<MESSAGEBOX_BUTTON*> ButtonList;
	uint32_t iFlags;
	uint32_t iDisplayTime;
};
inline constexpr fnvd::Vtable BGSMessage_vftable{};
inline constexpr fnvd::Vtable BGSMessage_vftable_TESDescription{};
inline constexpr fnvd::Vtable BGSMessage_vftable_TESFullName{};

// The game settings BGSMessage.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> iMessageBoxMaxItems;
