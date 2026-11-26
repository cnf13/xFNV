#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"

class BGSMenuIcon;

// One button of a message box: the text it shows and the conditions that show it.
struct MESSAGEBOX_BUTTON {
    MESSAGEBOX_BUTTON();
    ~MESSAGEBOX_BUTTON();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSStringT<char> Text;
    TESCondition Conditions;
};

// A message form (MESG).
class BGSMessage : public TESForm, public TESFullName, public TESDescription {
public:
    enum MESSAGE_FLAGS : uint32_t {
        FLAG_MESSAGE_BOX = 0x1,
        FLAG_AUTO_DISPLAY = 0x2,
    };

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
    // Stores pIcon.
    void SetIcon(BGSMenuIcon* apIcon);
    BGSMenuIcon* GetIcon();
    // Returns iDisplayTime.
    uint32_t GetDisplayTime();
    BSSimpleList<MESSAGEBOX_BUTTON*>* GetButtonList();
    BGSMessage* SetIsMessageBox(uint8_t abVal);
    BGSMessage* SetAutoDisplay(uint8_t abVal);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BGSMenuIcon* pIcon;
    BSSimpleList<MESSAGEBOX_BUTTON*> ButtonList;
    uint32_t iFlags;
    uint32_t iDisplayTime;
};

extern SettingT<GameSettingCollection> iMessageBoxMaxItems;
