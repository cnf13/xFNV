#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/ActorValue.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BSShaderPPLightingProperty;
class NiCamera;

// The buttons' callbacks. SpecialBookButton::Construct takes them as void*.
void SPECIALBookDoneCallback();

class SPECIALBookMenu : public Menu {
public:
	// The book's state: the open page (a SPECIAL's page, SBMS_STRENGTH through SBMS_LUCK, is that actor value minus 4)
	// or the book's own states. It is signed and compared as a signed value.
	enum SPECIAL_BOOK_MENU_STATE : int32_t {
		SBMS_START = 0,
		SBMS_STRENGTH = 1,
		SBMS_PERCEPTION = 2,
		SBMS_ENDURANCE = 3,
		SBMS_CHARISMA = 4,
		SBMS_INTELLIGENCE = 5,
		SBMS_AGILITY = 6,
		SBMS_LUCK = 7,
		SBMS_SPECIAL = 8,
		SBMS_CLOSED = 9,
	};

	// A clickable 3D button of the book: the two callbacks, the button object found by name and its shader property.
	// Larger buttons extend this with an array of text items.
	class SpecialBookButton {
	public:
		// Builds the button: the two callbacks, the button object found by name in the menu's
		// 3D scene and that object's shader property.
		SpecialBookButton(void* apButtonClick, void* apRefresh, const char* apButtonName);

		void (*pfButtonClick)(SpecialBookButton* apButton, ...);
		void (*pfRefresh)(SpecialBookButton* apButton);
		NiPointer<NiAVObject> spButton;
		// The button's shader property (null when it has none of the lighting shaders, types 8 to 12).
		BSShaderPPLightingProperty* pImage;

		// The base constructor writes neither member.
		union {
			bool bPageForward;
			int16_t sModifyAmount;
		};
	};

	// A number or text shown in the book: holds the refresh callback, up to two objects found by name with their shader
	// properties, and the actor value.
	class SpecialBookTextItem {
	public:
		// Builds the text item.
		SpecialBookTextItem(void (*apRefreshFieldCallback)(SPECIALBookMenu::SpecialBookTextItem*),
			const char* apTextObjectName1, const char* apTextObjectName2, ActorValue::Index aeActorValue);

		void (*pfRefreshField)(SpecialBookTextItem* apItem);
		// The objects' shader properties, kept only when their shader type is 8 to 12 (the lighting shaders).
		BSShaderPPLightingProperty* pText[2];
		NiAVObject* pTextObj[2];
		ActorValue::Index eActorValue;
	};

	// A SPECIAL book button that changes the actor value of the text items it holds.
	class ActorValueButton : public SpecialBookButton {
	public:
		// Variadic constructor: the base button, then the actor value, the number of affected fields and that many text
		// items.
		ActorValueButton(void* apButtonClick, void* apRefresh, const char* apButtonName, int32_t aeActorValue,
			int32_t aiNumAffectedFields, ...);

		BSSimpleArray<SpecialBookTextItem*, 1024> AffectedFields;
		ActorValue::Index eActorValue;
	};

	// One page of the book: its buttons and the text items they refresh. The constructor builds both arrays.
	class SpecialBookPage {
	public:
		SpecialBookPage();
		~SpecialBookPage();

		BSSimpleArray<SpecialBookButton*, 1024> Buttons;
		BSSimpleArray<SpecialBookTextItem*, 1024> TextItems;
	};

	SPECIALBookMenu();
	~SPECIALBookMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoIdle() override;
	bool DoKey(int32_t) override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;

	// The menu's close code, which Interface::CloseAllSupportMenus runs before it removes
	// the menu's tile.
	void Exit();
	ActorValue::Index GetSelectedSPECIAL();
	// Turns the book a page forward or back.
	bool TurnPage(bool bForward);
	// Runs the current page's callbacks.
	void CallPageCallbacks();
	// Builds the book's model, light and the nine pages' buttons and text items, then refreshes every page. Called by
	// Create.
	void InitBook();
	void LoadBBTextures();
	NiCamera* InitCamera();

	// Opens the SPECIAL book menu with aiTotalPoints to spend.
	static void Create(int32_t aiTotalPoints);
	// The menu's class ID, 1060.
	static int32_t GetClassStatic();
	// Draws the menu's 3D scene.
	static void Draw3DElements();
	static void Create35();
	// The next-page button's click callback; apButton is unused.
	static void NextPage(SpecialBookButton* apButton);
	// The previous-page button's click callback; apButton is unused.
	static void PreviousPage(SpecialBookButton* apButton);
	// The SPECIAL value fields' refresh callback: shows the player's permanent value of apTextItem's actor value.
	static void RefreshActorValue(SpecialBookTextItem* apTextItem);
	// The points message items' refresh callback: shows the message for the points left to spend. The refresh callbacks
	// return void.
	static void RefreshTotalPointsText(SpecialBookTextItem* apTextItem);
	// The points-remaining digits' refresh callback: reads apItem's pText (+4, +8) and pTextObj (+0xC, +0x10).
	static void RefreshTotalPointsValue(SpecialBookTextItem* apTextItem);
	// The decrease buttons' refresh callback: reads apButton's eActorValue (+0x24) and culls its spButton (+8).
	static void RefreshLeftAVButton(ActorValueButton* apButton);
	// The increase buttons' refresh callback: reads apButton's eActorValue (+0x24) and culls its spButton (+8).
	static void RefreshRightAVButton(ActorValueButton* apButton);
	// The next-page buttons' refresh callback: sets the button's shader property diffuse texture 0 (SetTexture) to the
	// menu's highlighted next-page image.
	static void RefreshNextPageButton(SpecialBookButton* apButton);
	// The previous-page buttons' refresh callback: as RefreshNextPageButton with the menu's first previous-page image.
	static void RefreshPreviousPageButton(SpecialBookButton* apButton);
	// The all-done button's refresh callback: sets the button's shader property (+0xC) diffuse texture 0 to the
	// menu's open/close image, the second while points remain to spend.
	static void RefreshAllDoneButton(SpecialBookButton* apButton);
	// The increase buttons' click callback: reads apButton's eActorValue (+0x24).
	static void IncreaseActorValue(ActorValueButton* apButton);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// The decrease buttons' click callback (InitBook stores it only in ActorValueButtons): reads apButton's
	// eActorValue (+0x24).
	static void DecreaseActorValue(ActorValueButton* apButton);

	Tile* pTiles[8];
	SPECIAL_BOOK_MENU_STATE eState;
	// The SPECIAL selected with the controller on the SBMS_SPECIAL page.
	SPECIAL_BOOK_MENU_STATE eXBoxIndexState;
	float fFrameAnimTimer;
	bool bAnimFinished;
	uint8_t padding55[3];
	int32_t iAvailablePoints;
	// The book's model, as the model loader returns it (IsNode is checked before attaching nodes to it).
	NiAVObject* pBookGeometry;
	// The book's nine pages; the PC constructor builds them at +0x60 as nine 0x20-byte records.
	SpecialBookPage Pages[9];
	// The digit textures by value.
	NiPointer<NiSourceTexture> spAVValues[11];
	// These arrays are passed to TES::CreateTextureImage as NiPointer references.
	NiPointer<NiSourceTexture> spPointsText[4];
	NiPointer<NiSourceTexture> spNextPageButtonImage[2];
	NiPointer<NiSourceTexture> spPreviousPageButtonImage[2];
	NiPointer<NiSourceTexture> spOpenCloseBookButtonImage[2];
	NiPointer<NiSourceTexture> spAVLeftArrowButtonImage[2];
	NiPointer<NiSourceTexture> spAVRightArrowButtonImage[2];

	static inline SPECIALBookMenu* pSPECIALBookMenu;
	static NiPointer<NiCamera> sp3DCamera;
};

inline char bAnimForward = 1;
// The texts by state while bAnimForward is clear; the eight pointers up to aForwardAnimations.
using ReverseAnimations_t = char* [8];
inline ReverseAnimations_t ReverseAnimations{const_cast<char*>("Attack2"), const_cast<char*>("Attack1"),
	const_cast<char*>("AimUp"), const_cast<char*>("Aim"), const_cast<char*>("Holster"), const_cast<char*>("JumpLand"),
	const_cast<char*>("JumpLoop"), const_cast<char*>("JumpStart")};
// The texts by state while bAnimForward is set; ten pointers.
using aForwardAnimations_t = char* [10];
inline aForwardAnimations_t aForwardAnimations{const_cast<char*>("TurnRight"), const_cast<char*>("Forward"),
	const_cast<char*>("Backward"), const_cast<char*>("Left"), const_cast<char*>("Right"),
	const_cast<char*>("FastForward"), const_cast<char*>("FastBackward"), const_cast<char*>("FastLeft"),
	const_cast<char*>("FastRight"), const_cast<char*>("TurnLeft")};
