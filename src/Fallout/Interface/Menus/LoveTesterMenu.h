#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMenu/Menu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Fallout Shared/ActorValue.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BSShaderPPLightingProperty;

class LoveTesterMenu : public Menu {
public:
	// The machine's state: the page shown (a SPECIAL's page, LTMS_STRENGTH through LTMS_LUCK, is that actor value
	// minus 4; it also indexes Pages) or the summary. Signed: the exe compares eState and eXBoxIndexState signed (jle
	// at 0x791B88, jg at 0x79223E, jge at 0x794A39 and 0x7926B5).
	enum LOVE_TESTER_MENU_STATE : int32_t {
		LTMS_START = 0,
		LTMS_STRENGTH = 1,
		LTMS_PERCEPTION = 2,
		LTMS_ENDURANCE = 3,
		LTMS_CHARISMA = 4,
		LTMS_INTELLIGENCE = 5,
		LTMS_AGILITY = 6,
		LTMS_LUCK = 7,
		LTMS_SUMMARY = 8,
		LTMS_NUM_STATES = 9,
	};

	// A clickable 3D button of the cabinet: 0x14 bytes (InitMeshes allocates 0x14 and builds it at 0x794E70: the two
	// callbacks, the button object found by name and its shader property). The 0x28-byte buttons InitMeshes also
	// allocates (0x794FF0) extend this with an array of text items at +0x14 and an amount at +0x24.
	class LoveTesterButton {
	public:
		// The PDB's callback types: a click callback takes the button and, by its type, varargs (the exe's callbacks
		// read at most the button); a refresh callback takes the button.
		using ButtonClickFunc = void (*)(LoveTesterButton* apButton, ...);
		using RefreshFunc = void (*)(LoveTesterButton* apButton);

		LoveTesterButton(ButtonClickFunc apfnButtonClick, RefreshFunc apfnRefresh, const char* apName);
		~LoveTesterButton();

		static void operator delete(void* apMemory) { ourFree(apMemory); }

		ButtonClickFunc pfButtonClick;
		RefreshFunc pfRefresh;
		NiPointer<NiAVObject> spButton;
		// The button's shader property (null when it has none of the lighting shaders, types 8 to 12).
		BSShaderPPLightingProperty* pImage;

		// The PDB places both at +0x10; the base constructor writes neither.
		union {
			bool bPageForward;
			int16_t sModifyAmount;
		};
	};

	class LoveTesterTextItem;

	// A button that changes one actor value (the PDB's LoveTesterMenu::ActorValueButton): the base button plus the
	// text items it refreshes and the actor value it changes. 0x28 bytes.
	class ActorValueButton : public LoveTesterButton {
	public:
		ActorValueButton(ButtonClickFunc apfnButtonClick, RefreshFunc apfnRefresh, const char* apName,
			int32_t aiActorValue, int32_t aiCount, ...);

		BSSimpleArray<LoveTesterTextItem*> AffectedFields;
		ActorValue::Index eActorValue;
	};

	// A number shown on the cabinet: 0x1C bytes (InitMeshes allocates 0x1C and builds it at 0x7950B0: the refresh
	// callback, up to two digit objects found by name with their shader properties, the actor value and the
	// threshold).
	class LoveTesterTextItem {
	public:
		// The PDB's refresh callback type: takes the text item.
		using RefreshFieldFunc = void (*)(LoveTesterTextItem* apItem);

		LoveTesterTextItem(RefreshFieldFunc apRefreshFieldCallback, const char* apTextObjectName1,
			const char* apTextObjectName2, int32_t aeActorValue, bool bElementOnDynamicGeometry,
			int32_t anThresholdToLightAt);

		RefreshFieldFunc pfRefreshField;
		// The digit objects' shader properties, kept only when their shader type is 8 to 12 (the lighting shaders).
		BSShaderPPLightingProperty* pText[2];
		NiAVObject* pTextObj[2];
		ActorValue::Index eActorValue;
		int32_t nThreshold;
	};

	// One point light of the cabinet: 8 bytes (InitMeshes allocates 8 per "P1_PointVal_0%i_GLOW:%i" object found).
	class LoveTesterLightBulbGlows {
	public:
		LoveTesterLightBulbGlows(NiAVObject* a_GlowCard, int32_t a_nValueToGlowAt);

		NiAVObject* m_GlowCard;
		int32_t m_nValueToGlowAt;
	};

	// One page of the menu: its buttons and its text items.
	struct LoveTesterPage {
		LoveTesterPage();

		BSSimpleArray<LoveTesterButton*> Buttons;
		BSSimpleArray<LoveTesterTextItem*> TextItems;
	};

	LoveTesterMenu();
	~LoveTesterMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoIdle() override;
	// Inferred type: a1: int32_t: signed uses from 0x7927b3
	bool DoKey(int32_t) override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;

	bool TurnPage(bool bForward);
	// IDA name RefreshPage; not in the PDB (thiscall, ret 0, 0x795E20). Lights the bulbs up to the selected SPECIAL's
	// value and runs the refresh callbacks of the current page's and the global page's text items and buttons.
	void RefreshPage();
	// IDA name VigorTester::InitTextures; the PDB's LoadButtonAndTextImages by inference (the PDB has no addresses;
	// the body loads the menu's textures) (thiscall, ret 0, 0x796100).
	void LoadButtonAndTextImages();
	// IDA name InitMeshes; the PDB's InitMachine by inference (the PDB has no addresses). Loads the cabinet's two
	// models into the 3D menu scene, starts its "Forward" animation, lights it, and builds the light bulbs, buttons
	// and text items of every page.
	void InitMachine();
	// Inferred type: return: ActorValue::Index: passed to PlayerCharacter::SetActorBaseValue 0 at 0x79202d
	ActorValue::Index GetSelectedSPECIAL();
	char CheckTilesExist(int32_t* a1);
	NiCamera* SetupCamera();

	// IDA name sub_795C60; the PDB's NextPage(apButton) by inference: the next-page button's
	// callback, which once the open instance's animation has finished runs TurnPage(1) and RefreshPage on it
	// (cdecl, ret 0, 0x795C60). Typed as the PDB's ButtonClickFunc, which InitMachine stores it as.
	static void NextPage(LoveTesterButton* apButton, ...);
	// Draws the menu's 3D scene.
	static void Draw3DElements();
	// The menu's ID, without an instance (IDA's GetMenuID_; the PDB's static GetClassStatic, called from Create).
	static int32_t GetClassStatic();
	// IDA name (Close); not in the PDB. The menu's close code, which Interface::CloseAllSupportMenus runs before
	// it removes the menu's tile (ret 0, 0x7917A0). Static: it reads only pInstance, never ecx (DoClick, TurnPage and
	// 0x795CE0 call it with whatever ecx holds).
	static void Close();
	// IDA name (RefreshActorValue); not in the PDB. The text items' refresh callback (InitMeshes stores it as their
	// pfRefreshField): the player's permanent actor value picks the digit texture, which goes to the item's first
	// text property (cdecl, ret 0, 0x795390).
	static void RefreshActorValue(LoveTesterTextItem* apTextItem);
	// IDA name; not in the PDB. Once the open instance's animation has finished, runs TurnPage(0) and
	// RefreshPage on it. Typed as the PDB's ButtonClickFunc, which InitMachine stores it as; the exe never reads
	// the button.
	static void PreviousPage(LoveTesterButton* apButton, ...);
	// Opens the menu with the default 35 points.
	static void Create();
	// Opens the menu with aiTotalPoints to spend.
	static void Create(int32_t aiTotalPoints);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// Not named in the PDB or IDA; name chosen here. The points text item's refresh callback
	// (InitMachine's LoveTesterTextItem::Construct): reads apItem's pText (+4, +8) and pTextObj (+0xC, +0x10). The PDB's
	// refresh callbacks (RefreshTotalPointsText ...) take a LoveTesterTextItem* and return void.
	static void RefreshTotalPointsValue(LoveTesterMenu::LoveTesterTextItem* apItem);
	// Not named in the PDB or IDA; name chosen here. A decrease button's refresh callback (the PDB's
	// RefreshLeftAVButton(LoveTesterButton*) by its calls): culls the button at its actor value's minimum.
	static void RefreshLeftAVButton(LoveTesterMenu::LoveTesterButton* apButton);
	// Not named in the PDB or IDA; name chosen here. An increase button's refresh callback (the PDB's
	// RefreshRightAVButton(LoveTesterButton*) by its calls): culls the button at the maximum or with no points left.
	static void RefreshRightAVButton(LoveTesterMenu::LoveTesterButton* apButton);
	// Not named in the PDB or IDA; name chosen here. A page button's refresh callback (InitMachine's
	// LoveTesterButton::Construct): calls apButton's pImage (+0xC). The PDB's Refresh*Button take a LoveTesterButton*
	// and return void.
	static void RefreshAllDoneButton(LoveTesterMenu::LoveTesterButton* apButton);
	// The previous-page button's refresh callback:
	// calls apButton's pImage (+0xC) with spPreviousPageButtonImage. Typed as LoveTesterMenu::RefreshAllDoneButton.
	static void RefreshPreviousPageButton(LoveTesterMenu::LoveTesterButton* apButton);
	// Not named in the PDB or IDA; name chosen here. The decrease buttons' click callback
	// (InitMachine stores it only in ActorValueButtons): reads apButton's eActorValue (+0x24). Typed as the PDB's
	// ButtonClickFunc, which InitMachine stores it as.
	static void DecreaseActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...);
	// Not named in the PDB or IDA; name chosen here. A click callback, typed as the PDB's ButtonClickFunc, which
	// InitMachine stores it as; the exe never reads the button.
	static void DecreaseCurrentActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...);
	// Not named in the PDB or IDA; name chosen here. The increase buttons' click callback (the PDB's
	// IncreaseActorValue(LoveTesterButton*) by its calls): reads the ActorValueButton's eActorValue (+0x24). Typed as
	// the PDB's ButtonClickFunc, which InitMachine stores it as.
	static void IncreaseActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...);
	// Not named in the PDB or IDA; name chosen here. A click callback, typed as the PDB's ButtonClickFunc, which
	// InitMachine stores it as; the exe never reads the button.
	static void IncreaseCurrentActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...);
	// Not named in the PDB or IDA; name chosen here. The all-done button's click callback: closes the menu when every
	// point is spent. Typed as the PDB's ButtonClickFunc, which InitMachine stores it as; the exe never reads the
	// button.
	static void AllDoneButtonClick(LoveTesterMenu::LoveTesterButton* apButton, ...);

	Tile* pTiles[8];
	LOVE_TESTER_MENU_STATE eState;
	// The SPECIAL selected with the controller on the LTMS_SUMMARY page.
	LOVE_TESTER_MENU_STATE eXBoxIndexState;
	float fFrameAnimTimer;
	bool bAnimFinished;
	uint8_t padding55[3];
	int32_t iAvailablePoints;
	// The machine's models, as the model loader returns them (the exe asks IsNode before attaching nodes to them).
	NiAVObject* pDynamicGeometry;
	NiAVObject* pCabinetGeometry;
	// PDB names and types; InitMachine fills LightBulbGlows. LightBulbs is the text-item array instance: the
	// constructor 0x720550 and destructor 0x720860 build and destroy it with that instance's bodies (0x723FA0,
	// 0x723FD0, its vtable), and 0x795E20 reads each element's nThreshold (+0x18).
	BSSimpleArray<LoveTesterTextItem*> LightBulbs;
	BSSimpleArray<LoveTesterLightBulbGlows*> LightBulbGlows;
	// PDB names: the destructor 0x720860 destroys GlobalPage (+0x1A4), then the nine Pages from +0x84 (0x20 bytes
	// each).
	LoveTesterPage Pages[9];
	LoveTesterPage GlobalPage;
	// The PDB's texture smart pointers (the same element type as spAVValues); the exe reads each through
	// NiPointer's out-of-line operator->, 0x559450, and LoadButtonAndTextImages (0x796100) passes them by reference.
	NiPointer<NiSourceTexture> spAVValues[11];
	NiPointer<NiSourceTexture> spPointsText[4];
	NiPointer<NiSourceTexture> spNextPageButtonImage[2];
	NiPointer<NiSourceTexture> spPreviousPageButtonImage[2];
	NiPointer<NiSourceTexture> spOpenCloseBookButtonImage[2];
	NiPointer<NiSourceTexture> spAVLeftArrowButtonImage[2];
	NiPointer<NiSourceTexture> spAVRightArrowButtonImage[2];
	NiPointer<NiSourceTexture> spOnOffImage[2];

	static inline LoveTesterMenu* pInstance;
	static NiPointer<NiCamera> sp3DCamera;
};

// The actor-value buttons' text-item array: its constructor (0x723FA0, reconstructed in MenuManager.cpp) and the Add
// the pointer instantiations share (0x7CB2E0, not reconstructed yet: forwards to the exe's own body).
using LoveTesterTextItemArray = BSSimpleArray<LoveTesterMenu::LoveTesterTextItem*>;
// The button and light-bulb arrays' Add: the same shared pointer instantiation body (0x7CB2E0, not reconstructed
// yet).
using LoveTesterButtonArray = BSSimpleArray<LoveTesterMenu::LoveTesterButton*>;
using LoveTesterLightBulbGlowsArray = BSSimpleArray<LoveTesterMenu::LoveTesterLightBulbGlows*>;

// IDA name; not in the PDB. The backward page-turn sequence names ("AimUp" ... "TurnLeft"), one per state from
// LTMS_START on: DoIdle and TurnPage index it by eState ([eax*4+0x11A03C0]).
using LoveTesterAnimations_t = char* [8];
inline LoveTesterAnimations_t LoveTesterAnimations{const_cast<char*>("AimUp"), const_cast<char*>("Aim"),
	const_cast<char*>("Holster"), const_cast<char*>("JumpLand"), const_cast<char*>("JumpLoop"),
	const_cast<char*>("JumpStart"), const_cast<char*>("TurnRight"), const_cast<char*>("TurnLeft")};
inline constexpr fnvd::Vtable LoveTesterMenu_vftable{};

// Not named in the PDB or IDA; name chosen here. Which sequence table DoIdle and TurnPage read: TurnPage stores 1
// when turning forward and 0 when turning back (byte stores, byte reads; the exe initialises it to 1).
inline bool bLoveTesterAnimForward = true;

// Not named in the PDB or IDA; name chosen here. The forward page-turn sequence names ("Forward" ... "FastRight"),
// one per state from LTMS_STRENGTH on: the exe indexes it by eState from 0x11A039C, one entry below its start.
using LoveTesterReverseAnimations_t = const char* [8];
inline LoveTesterReverseAnimations_t LoveTesterReverseAnimations{"Forward", "Backward", "Left", "Right", "FastForward",
	"FastBackward", "FastLeft", "FastRight"};
