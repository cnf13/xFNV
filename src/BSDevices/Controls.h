#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/DirectX/xinput.h"
#include "SDK/Windows/dinput.h"

#include <stdint.h>

class Controls;
struct XRumbleControl;
struct _RUMBLE_EFFECT;

enum GAMEPAD_BUTTON : int32_t {
	GP_DPAD_Y = 0,
	GP_DPAD_UP = 1,
	GP_DPAD_DOWN = 2,
	GP_DPAD_X = 3,
	GP_DPAD_RIGHT = 4,
	GP_DPAD_LEFT = 5,
	GP_START = 6,
	GP_BACK = 7,
	GP_LTHUMB_BUTTON = 8,
	GP_RTHUMB_BUTTON = 9,
	GP_A = 10,
	GP_B = 11,
	GP_X = 12,
	GP_Y = 13,
	GP_RBUTTON = 14,
	GP_LBUTTON = 15,
	GP_LTRIGGER = 16,
	GP_RTRIGGER = 17,
	GP_LTHUMB_Y = 18,
	GP_LTHUMB_UP = 19,
	GP_LTHUMB_DOWN = 20,
	GP_LTHUMB_X = 21,
	GP_LTHUMB_RIGHT = 22,
	GP_LTHUMB_LEFT = 23,
	GP_RTHUMB_Y = 24,
	GP_RTHUMB_UP = 25,
	GP_RTHUMB_DOWN = 26,
	GP_RTHUMB_X = 27,
	GP_RTHUMB_RIGHT = 28,
	GP_RTHUMB_LEFT = 29,
	GP_NONE = 30,
	MAX_BUTTONS = 30,
	FIRST_BUTTON = 0,
};

// A buffered keyboard event's kind, as GetNextKeyEvent returns it.
enum KEY_EVENT_TYPE : int32_t {
	KEY_EVENT_NONE = 0,
	KEY_EVENT_DOWN = 1,
	KEY_EVENT_UP = 2,
};

// The PC build's keyboard scan codes (IDA's enum; not in the PDB, whose Xbox build has no keyboard), the SDK's scan
// code set 1 values. Controls::GetKeyboardButton (0xA24180) takes one.
enum Scancodes : int32_t {
	_Escape = 0x1,
	One = 0x2,
	Two = 0x3,
	Three = 0x4,
	Four = 0x5,
	Five = 0x6,
	Six = 0x7,
	Seven = 0x8,
	Eight = 0x9,
	Nine = 0xA,
	Zero = 0xB,
	Minus = 0xC,
	Equals = 0xD,
	Backspace = 0xE,
	Tab = 0xF,
	Q = 0x10,
	W = 0x11,
	E = 0x12,
	R = 0x13,
	T = 0x14,
	Y = 0x15,
	U = 0x16,
	I = 0x17,
	O = 0x18,
	P = 0x19,
	LeftBracket = 0x1A,
	RightBracket = 0x1B,
	Enter = 0x1C,
	LeftControl = 0x1D,
	A = 0x1E,
	S = 0x1F,
	D = 0x20,
	F = 0x21,
	G = 0x22,
	H = 0x23,
	J = 0x24,
	K = 0x25,
	L = 0x26,
	SemiColon = 0x27,
	Apostrophe = 0x28,
	Tilde = 0x29,
	LeftShift = 0x2A,
	BackSlash = 0x2B,
	Z = 0x2C,
	X = 0x2D,
	C = 0x2E,
	V = 0x2F,
	B = 0x30,
	N = 0x31,
	M = 0x32,
	Comma = 0x33,
	Period = 0x34,
	ForwardSlash = 0x35,
	RightShift = 0x36,
	NumStar = 0x37,
	LeftAlt = 0x38,
	SpaceBar = 0x39,
	CapsLock = 0x3A,
	F1 = 0x3B,
	F2 = 0x3C,
	F3 = 0x3D,
	F4 = 0x3E,
	F5 = 0x3F,
	F6 = 0x40,
	F7 = 0x41,
	F8 = 0x42,
	F9 = 0x43,
	F10 = 0x44,
	NumLock = 0x45,
	ScrollLock = 0x46,
	Num7 = 0x47,
	Num8 = 0x48,
	Num9 = 0x49,
	NumMinus = 0x4A,
	Num4 = 0x4B,
	Num5 = 0x4C,
	Num6 = 0x4D,
	NumPlus = 0x4E,
	Num1 = 0x4F,
	Num2 = 0x50,
	Num3 = 0x51,
	Num0 = 0x52,
	NumPeriod = 0x53,
	F11 = 0x57,
	F12 = 0x58,
	NumEnter = 0x9C,
	RightControl = 0x9D,
	NumSlash = 0xB5,
	PrintScrn = 0xB7,
	RightAlt = 0xB8,
	Home = 0xC7,
	UpArrow = 0xC8,
	PageUp = 0xC9,
	LeftArrow = 0xCB,
	RightArrow = 0xCD,
	End_ = 0xCF,
	DownArrow = 0xD0,
	PageDown = 0xD1,
	Delete_ = 0xD3,
};

// The rumble patterns the PDB lists; IDA's enum has no value 4, which the PDB's DEFAULT sits at in its order.
enum RUMBLE_PATTERN : int32_t {
	CONSTANT = 0x0,
	PERIODIC_SQUARE = 0x1,
	PERIODIC_TRIANGLE = 0x2,
	PERIODIC_SAWTOOTH = 0x3,
	PERIODIC_SINE = 0x5,
	PERIODIC_TANGENT = 0x6,
};

// The gamepad motor a rumble is applied to: XRumbleControl::Rumble (0xA26520) drives the right motor when its side
// is nonzero and the left otherwise, and InitRumble (0xA26310) is called with 1 and then 0. PDB: enum RUMBLE_SIDE.
enum RUMBLE_SIDE : int32_t {
	LEFT = 0x0,
	RIGHT = 0x1,
};

// One motor's rumble: its speed and pattern, and the ramps up to and down from it.
struct _RUMBLE_EFFECT {
	// The PDB's unnamed RampUp/RampDown type (IDA: _RUMBLE_EFFECT::<unnamed_type_RampUp>); named here.
	struct Ramp {
		uint32_t uiDuration;
		uint16_t wStartSpeed;
		uint16_t wEndSpeed;
	};

	uint32_t uiSpeed;
	RUMBLE_PATTERN pattern;
	uint32_t uiDuration;
	uint32_t uiWavelength;
	bool bAlternate;
	Ramp RampUp;
	Ramp RampDown;
};

// The running time of one motor's rumble and ramps.
struct _RUMBLE_TIMER {
	uint32_t uiRumbleDuration;
	uint32_t uiRampUpDuration;
	uint32_t uiRampDownDuration;
	uint32_t uiRumbleCounter;
	uint32_t uiRampCounter;
};

// The gamepad's rumble state: one effect and timer per motor. IDA types iDevice const.
struct XRumbleControl {
	XRumbleControl(int32_t aiNewDevice);
	~XRumbleControl();

	// Makes apMotor the left (aeLeftOrRight 0) or right motor's effect and starts that motor's timers: the effect's
	// wavelength defaults to its duration and its ramps end at its speed.
	void InitRumble(RUMBLE_SIDE aeLeftOrRight, _RUMBLE_EFFECT* apMotor);
	// Advances the rumble by iTime milliseconds. thiscall, ret 4, 0xA269F0.
	void UpdateRumble(int32_t iTime);
	// The motor speed of apMotorEffect's main rumble at aiRumbleCounter: its speed capped at 0xFFFF for a constant
	// pattern, the square wave for PERIODIC_SQUARE, else 0 (thiscall, ret 8, the word in ax, 0xA26410).
	uint16_t MainRumble(_RUMBLE_EFFECT* apMotorEffect, uint32_t aiRumbleCounter);
	// The square-wave pattern's speed (thiscall, ret 8, the word in ax, 0xA26A50).
	uint16_t PatternPeriodicSquare(_RUMBLE_EFFECT* apMotorEffect, uint32_t aiRumbleCounter);
	uint16_t RampUp(_RUMBLE_EFFECT* MotorEffect, uint32_t iRumbleCounter);
	uint16_t RampDown(_RUMBLE_EFFECT* MotorEffect, uint32_t iRumbleCounter);
	void Rumble(RUMBLE_SIDE aeLeftOrRight, _XINPUT_VIBRATION* XInput, uint32_t iTime);

	int32_t iDevice;
	_RUMBLE_EFFECT* pLeftEffect;
	_RUMBLE_EFFECT* pRightEffect;
	_RUMBLE_TIMER LeftTimer;
	_RUMBLE_TIMER RightTimer;
	uint32_t dwNewTime;
	uint32_t dwOldTime;
	uint32_t dwStartTime;
};

// The input devices: keyboard, mouse and gamepad state, and the key bindings. The PC build's Controls (0x1C04 bytes)
// wraps DirectInput and shares nothing with the Xbox PDB's layout, so the field names are IDA's (not in the PDB).
class Controls {
public:
	// The user actions a control can be mapped to (the PDB's USERACTIONS). The PC drops the Xbox's MENUNEXT and
	// MENUPREV (16, 17) and numbers VATS, the hotkeys, quick save and quick load from 16; the PDB's LOOKUP, LOOKDOWN,
	// TURNLEFT, TURNRIGHT, AMMOSWAP, OPTIONSMENU and SYSTEMWINDOW have no PC value seen in the exe and are left out.
	enum USERACTIONS : int32_t {
		UACTN_FORWARD = 0,
		UACTN_BACK = 1,
		UACTN_SLDLEFT = 2,
		UACTN_SLDRIGHT = 3,
		UACTN_USE = 4,
		UACTN_ACTIVATE = 5,
		UACTN_BLOCK = 6,
		UACTN_READY_RELOAD_WEAPON = 7,
		UACTN_CROUCH = 8,
		UACTN_RUN = 9,
		UACTN_TOGGLERUN = 10,
		UACTN_AUTOMOVE = 11,
		UACTN_JUMP = 12,
		UACTN_TOGGLEPOV = 13,
		UACTN_MENUMODE = 14,
		UACTN_RESTMENU = 15,
		// The VATS key. The PC's value; the PDB's Xbox enum numbers it 18.
		UACTN_VATSMENU = 16,
		// The eight hotkeys. The PC's values; the PDB's Xbox enum numbers them 19 to 26.
		UACTN_HOTKEY1 = 17,
		UACTN_HOTKEY2 = 18,
		UACTN_HOTKEY3 = 19,
		UACTN_HOTKEY4 = 20,
		UACTN_HOTKEY5 = 21,
		UACTN_HOTKEY6 = 22,
		UACTN_HOTKEY7 = 23,
		UACTN_HOTKEY8 = 24,
		// The PC's quick save and quick load follow the hotkeys (InterfaceManager::Idle shows sCantQuickSave and
		// sCantQuickLoad for them); the Xbox enum numbers them 0x25 and 0x26.
		UACTN_QUICKSAVE = 25,
		UACTN_QUICKLOAD = 26,
		// PlayerCharacter::HandlePhysicsGrab tests it.
		UACTN_GRAB = 27,
		UACTN_UP = 28,
		UACTN_DOWN = 29,
		// The PC's value: Main::Swap (0x86FF70) takes a screenshot on it; the PDB's Xbox enum numbers it 0x27.
		UACTN_SCREENSHOT = 30,
	};

	// Which transition of a control GetMouseButton, GetKeyboardButton and GetUserAction test (IDA's names; the
	// PDB's TOGGLE_FLAGS numbers them differently).
	enum CONTROL_TOGGLE_FLAGS : int32_t {
		isHeld = 0,
		isPressed = 1,
		isDepressed = 2,
		isChanged = 3,
		// Mouse buttons only.
		isDoubleClicked = 4,
	};

	Controls(HINSTANCE__* a1);
	~Controls();

	// Rumbles the gamepad.
	void Rumble(float leftMotor, float rightMotor, int32_t timeInMillis, int32_t iWavelength,
		RUMBLE_PATTERN aRumblePattern, bool abAlternate, bool bOverride);
	// Whether the device aiDevice has input interfaces.
	uint32_t HasInputInterfaces(int32_t aiDevice);
	// IDA name SyncLastButtonState; not in the PDB. Copies the current gamepad state of the buttons mapped to aiControl to
	// the last gamepad state.
	void SyncLastButtonState(int32_t aiControl);
	// IDA name A23690; not in the PDB. The axis count of joystick aiJoystick (kJoyStickCaps' dwAxes).
	int32_t GetJoystickAxisCount(int32_t aiJoystick);
	// Resets the key binds of device aiDevice (all devices from 4
	// on) to unbound.
	// Inferred type: aiDevice: int32_t: signed uses from 0xa24b95
	void InitDefaultKeybinds(int32_t aiDevice);
	// Clears the state of the keys bound to control aiControl.
	void ResetControlState(int32_t aiControl);
	// The XInput button bit of a gamepad button, or 0. IDA name; not in the PDB.
	int32_t MapToXInput(GAMEPAD_BUTTON aeButton);
	// Makes the user action aeAction read as pressed on the next poll.
	void ForceUserAction(USERACTIONS aeAction);
	// The key bound to user action eAction on device uiIndex.
	uint8_t RemapAction(USERACTIONS eAction, uint32_t uiIndex);
	// Forgets the buffered keystrokes.
	void ClearKeystrokes();
	// Sets the axis range of joystick aiJoystick. thiscall, ret 0xC. IDA name; not in the PDB.
	HRESULT SetDeviceRange(int32_t aiJoystick, int32_t aiMin, int32_t aiMax);
	// The next buffered keyboard event: its scan code in Key. thiscall, ret 4.
	KEY_EVENT_TYPE GetNextKeyEvent(int32_t& Key);
	// The state of gamepad button aeButton for the transition aeFlags. thiscall, ret 8. IDA name; not in the PDB.
	int32_t GetControllerButton(GAMEPAD_BUTTON aeButton, CONTROL_TOGGLE_FLAGS aeFlags);
	// The state of key aucKey of device aiDevice (0 keyboard, 1 mouse, 2+ joysticks, 3 gamepad, 255 none).
	// thiscall, ret 0xC.
	int32_t GetButtonState(int32_t aiDevice, uint8_t aucKey, CONTROL_TOGGLE_FLAGS aeFlags);
	// The character a scan code types, with (1) or without (0) shift; 0xFF translates through
	// the raw table without the keyboard language (the PDB's int shiftToggle).
	int32_t TranslateKeystroke(int32_t keystrokeValue, int32_t shiftToggle);
	// The state of a gamepad control (aiKey) of controller aiController. thiscall, ret 8.
	int32_t GetControllerValue(int32_t aiController, int32_t aiKey);
	// The mouse movement on an axis (1 x, 2 y, 3 wheel) since the last poll, or 0.
	int32_t GetMouseAxis(int32_t aiAxis);
	// True while the mouse belongs to another window: 0 or 1 in the whole eax (0x711E0D
	// and eax, 8; neg; sbb; neg), which callers test whole (0x86A718 test eax, eax).
	int32_t IsMouseInBackground();
	// The mouse sensitivity preference.
	int8_t GetMouseSensitivity();
	// The fForegroundMouse* settings (IDA: Controls::fForegroundMouse*::Get), in ST(0).
	// Inferred type: return: float: used as one at 0x711a5f
	float GetForegroundMouseBase();
	// Inferred type: return: float: used as one at 0x711a9e
	float GetForegroundMouseMult();
	// Inferred type: return: float: used as one at 0x711aaa
	float GetForegroundMouseAccelTop();
	// Inferred type: return: float: used as one at 0x711af7
	float GetForegroundMouseAccelBase();
	// The state of keyboard key aiButton for the transition aiFlags. thiscall, ret 8, 0xA24180.
	int32_t GetKeyboardButton(int32_t aiButton, int32_t aiFlags);
	// The state of mouse button buttonNumber for the transition toggleFlags. thiscall, ret 8, 0xA23A50.
	int32_t GetMouseButton(int32_t buttonNumber, int32_t toggleFlags);
	// The number of mouse buttons. thiscall, 0xA23A30 (IDA's name).
	// Inferred type: return: int32_t: signed uses from 0x7d54c2
	int32_t GetNumMouseButtons();
	// The state of a user action for the transition toggleFlags. thiscall, ret 8, 0xA24660.
	int32_t GetUserAction(USERACTIONS eAction, int32_t toggleFlags);
	// Maps the action to key aucKey of device auiDevice, unless the key is locked; true when
	// mapped. thiscall, ret 0xC, 0xA24FC0 (IDA's name).
	bool SetControl(uint32_t auiAction, uint32_t auiDevice, uint8_t aucKey);
	// Stores the action's key for device auiDevice. thiscall, ret 0xC, 0xA25180 (IDA's name).
	void SetActionMapping(uint32_t auiAction, uint32_t auiDevice, uint8_t aucKey);
	// Inferred type: return: int32_t: signed uses from 0x7cff9b
	int32_t GetButtonCount(int32_t aiJoystick);
	// The state of button aiButton of joystick aiJoystick for the transition aiFlags.
	// thiscall, ret 0xC, 0xA234C0 (IDA's name).
	int32_t GetJoyStickButton(int32_t aiJoystick, int32_t aiButton, CONTROL_TOGGLE_FLAGS aiFlags);
	// Inferred type: return: int32_t: signed uses from 0x7d5608
	int32_t GetHatCount(int32_t aiJoystick);
	// The direction key of hat aiHat of joystick aiJoystick, or -1 when centred. thiscall,
	// ret 8, 0xA235F0 (IDA's name).
	int32_t GetJoystickHat(int32_t aiJoystick, int32_t aiHat);
	// Reads the device states. thiscall, 0xA23010 (IDA's name).
	void UpdateValues();
	// Updates the gamepad rumble for the frame time fTime. thiscall, ret 4, 0xA257C0 (the PC build takes only the
	// time).
	void Poll(float fTime);
	void ClearUserActions();
	uint32_t HasJoySticks() const;
	// Both set all of eax to 0 or 1 (setne into a zeroed register); HasInputInterfaces returns that eax.
	uint32_t HasKeyboard() const;
	uint32_t HasMouse() const;
	void RemoveKeybind(int32_t a2, int32_t a3);
	void RemoveDuplicateControls(int32_t device, int32_t button);
	// Inferred type: return: int32_t: signed uses from 0x7ed17e
	int32_t GetKeybind(int32_t a2);
	void SetKeybind(int32_t a2, int32_t a3);
	void sub_A232C0();
	void SaveControls();
	// Reads the 28 action keys' bindings from the app's ini (thiscall, ret 0, 0xA22EF0). IDA name; not in the PDB.
	void InitControlsFromIni();
	int32_t A23650(int32_t a2);
	int32_t A232D0(int32_t a2, int32_t a3);
	int32_t A23490(int32_t a2, char a3);
	// Inferred type: a2: float: used as one at 0xa23782
	int32_t SetDeviceDeadzone(int32_t a1, float a2);
	int32_t GetNextKeystroke(int32_t& nextKeystroke);
	void SetMouseBackgroundStatus(bool abBackground);
	bool SetAction(int32_t action);
	void EnableJoysticks(bool abEnable);
	void KillRumble();
	// Takes the gamepad bind of an action: its switch cases are the
	// triggers and left stick directions, the rest goes to MapToXInput.
	void ForceControllerButton(GAMEPAD_BUTTON aeButton);

	// Turns the gamepad rumble on or off (when the controls exist).
	static void EnableRumble(bool bTF);
	// The controls singleton.
	static Controls* GetInstance();
	// Destroys and frees the controls singleton and clears it.
	static void DeleteInstance();
	// Controls::DeletingDestruct frees through ourFree.
	static void operator delete(void* apMemory);
	// Inferred type: hinst: HINSTANCE__* from 0x86c595 (parameter 2 of Main::Construct)
	static void InitializeInstance(HINSTANCE__* hinst);
	// Not named in the PDB or IDA; name chosen here. Whether the controls list shows aeAction for a keyboard or mouse
	// device (StartMenu::PopulateControlsListBox, 0x7D01B0, passes its USERACTIONS loop variable).
	static bool Remappable(Controls::USERACTIONS aeAction);

	bool bIsControllerDisabled;
	uint8_t padding01[0x3];
	// OSInputGlobals::Flags bits.
	uint32_t uiFlags;
	IDirectInput8A* pDirectInput;
	IDirectInputDevice8A* pDevices[8];
	IDirectInputDevice8A* pKeyboard;
	IDirectInputDevice8A* pMouse;
	DIJOYSTATE kJoyStickState[8][2];
	DIDEVICEINSTANCEA kJoyStickInstances[8];
	DIDEVCAPS kJoyStickCaps[8];
	uint32_t uiJoyStickFlags[8][2];
	uint32_t uiJoyStickCount;
	uint8_t ucKeyboardState[2][256];
	DIDEVCAPS kMouseCaps;
	DIMOUSESTATE2 kMouseState[2];
	int32_t bSwapLeftRightMouseButtons;
	uint8_t cMouseSensitivity;
	uint8_t padding1B51[0x3];
	uint32_t uiDoubleClickTime;
	bool bDoubleClicked[8];
	uint32_t uiLastClickedTime[8];
	XRumbleControl* pControllerVibration;
	_RUMBLE_EFFECT* pVibrationProperties;
	bool bIsControllerEnabled;
	uint8_t padding1B89[0x3];
	uint32_t eActionToSet;
	bool bSettingAControl;
	bool bUpdated;
	uint16_t usOverrideFlags;
	uint8_t ucKeyBinds[4][28];

	static inline Controls* pInstance;
	// The gamepad state of this and the last update, and whether a gamepad is connected and enabled. IDA names;
	// not in the PDB.
	static inline _XINPUT_STATE kCurrentGamepadState{};
	static inline _XINPUT_STATE kLastGamepadState{};
	static inline bool bUseController;
};

// DirectInput's enumeration callbacks (the SDK's LPDIENUMDEVICESCALLBACKA and LPDIENUMDEVICEOBJECTSCALLBACKA): the
// reference is the Controls (EnumDevices) or the joystick's axis flags (EnumObjects) the caller passes.
int32_t CALLBACK DIEnumDevicesCallback(const DIDEVICEINSTANCEA* lpddi, void* apRef);
int32_t CALLBACK DIEnumDeviceObjectsCallback(const DIDEVICEOBJECTINSTANCEW* a1, void* apRef);
// Steps the action to the next one; returns arAction (cdecl, 0x7DC0A0, no PDB name).
Controls::USERACTIONS& operator++(Controls::USERACTIONS& arAction);
int32_t return_1(int32_t a1, int32_t a2);

inline GUID CLSID_IDirectInput8A{0xBF798030, 18490, 19874, {170, 153, 93, 100, 237, 54, 151}};
inline GUID CLSID_GUID_SysMouse{0x6F1D2B60, 54688, 4559, {191, 199, 68, 69, 83, 84}};
inline GUID CLSID_GUID_SysKeyboard{0x6F1D2B61, 54688, 4559, {191, 199, 68, 69, 83, 84}};

// The [Controls] INI key names, by user action; Controls::SaveControls and
// Controls::InitControlsFromIni read them by index.
using userActionKeys_t = const char* [28];
inline userActionKeys_t userActionKeys{"Forward", "Back", "Slide Left", "Slide Right", "Use", "Activate", "Block",
	"Ready Item", "Crouch/Sneak", "Run", "Always Run", "Auto Move", "Jump", "Toggle POV", "Menu Mode", "Rest", "Vats",
	"Hotkey1", "Ammo Swap", "Hotkey3", "Hotkey4", "Hotkey5", "Hotkey6", "Hotkey7", "Hotkey8", "QuickSave", "QuickLoad",
	"Grab"};
inline char* pAppPrefININame = const_cast<char*>("FalloutPrefs.ini");

// Not named in the PDB or IDA; name chosen here. The SDK's LPDIENUMDEVICEOBJECTSCALLBACKA: DIEnumDevicesCallback
// passes the exe's callback to IDirectInputDevice8A::EnumObjects.
inline GUID CLSID_GUID_XAxis{0xA36D02E0, 51699, 4559, {191, 199, 68, 69, 83, 84}};
inline GUID CLSID_GUID_YAxis{0xA36D02E1, 51699, 4559, {191, 199, 68, 69, 83, 84}};
inline GUID CLSID_GUID_ZAxis{0xA36D02E2, 51699, 4559, {191, 199, 68, 69, 83, 84}};
inline GUID CLSID_GUID_RxAxis{0xA36D02F4, 51699, 4559, {191, 199, 68, 69, 83, 84}};
inline GUID CLSID_GUID_RyAxis{0xA36D02F5, 51699, 4559, {191, 199, 68, 69, 83, 84}};
inline GUID CLSID_GUID_RzAxis{0xA36D02E3, 51699, 4559, {191, 199, 68, 69, 83, 84}};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
// The exe reads these bytes zero-extended (movzx, 0xA238CC), so they are unsigned.
using translateNormal_English_t = uint8_t[256];
inline translateNormal_English_t translateNormal_English{0, 27, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 45, 61, 8, 9,
	113, 119, 101, 114, 116, 121, 117, 105, 111, 112, 91, 93, 13, 0, 97, 115, 100, 102, 103, 104, 106, 107, 108, 59, 39,
	96, 0, 92, 122, 120, 99, 118, 98, 110, 109, 44, 46, 47, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 45, 0, 0, 0, 43, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58,
	95, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
using translateShifted_English_t = uint8_t[256];
inline translateShifted_English_t translateShifted_English{0, 27, 33, 64, 35, 36, 37, 94, 38, 42, 40, 41, 95, 43, 8, 9,
	81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 123, 125, 13, 0, 65, 83, 68, 70, 71, 72, 74, 75, 76, 58, 34, 126, 0, 124,
	90, 88, 67, 86, 66, 78, 77, 60, 62, 63, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 0, 0, 0,
	43, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58, 95, 0, 0, 0, 0, 0,
	0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
using translateNormal_German_t = char[256];
inline translateNormal_German_t translateNormal_German{0, 27, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, -33, -76, 8, 9,
	113, 119, 101, 114, 116, 122, 117, 105, 111, 112, -4, 43, 13, 0, 97, 115, 100, 102, 103, 104, 106, 107, 108, -10,
	-28, 96, 0, 35, 121, 120, 99, 118, 98, 110, 109, 44, 46, 45, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 45, 0, 0, 0, 43, 0, 0, 0, 0, 127, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58,
	95, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
using translateShifted_German_t = char[256];
inline translateShifted_German_t translateShifted_German{0, 27, 33, 34, -89, 36, 37, 38, 47, 40, 41, 61, 63, 96, 8, 9,
	81, 87, 69, 82, 84, 90, 85, 73, 79, 80, -36, 42, 13, 0, 65, 83, 68, 70, 71, 72, 74, 75, 76, -42, -60, -80, 0, 39,
	89, 88, 67, 86, 66, 78, 77, 59, 58, 95, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 0, 0, 0,
	43, 0, 0, 0, 0, 127, 0, 0, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58, 95, 0, 0, 0, 0,
	0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
using translateNormal_French_t = char[256];
inline translateNormal_French_t translateNormal_French{0, 27, 38, -23, 34, 39, 40, 45, -24, 95, -25, -32, 41, 61, 8, 9,
	97, 122, 101, 114, 116, 121, 117, 105, 111, 112, 94, 36, 13, 0, 113, 115, 100, 102, 103, 104, 106, 107, 108, 109,
	-7, -7, 0, 42, 119, 120, 99, 118, 98, 110, 44, 59, 58, 33, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 45, 0, 0, 0, 43, 0, 0, 0, 0, 127, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58,
	95, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
using translateShifted_French_t = char[256];
inline translateShifted_French_t translateShifted_French{0, 27, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, -80, 43, 8, 9,
	65, 90, 69, 82, 84, 89, 85, 73, 79, 80, -88, -93, 13, 0, 81, 83, 68, 70, 71, 72, 74, 75, 76, 77, 37, 126, 0, -75,
	87, 88, 67, 86, 66, 78, 63, 46, 47, -89, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 0, 0, 0,
	43, 0, 0, 0, 0, 127, 0, 0, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58, 95, 0, 0, 0, 0,
	0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
using translateNormal_Spanish_t = char[256];
inline translateNormal_Spanish_t translateNormal_Spanish{0, 27, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 39, -95, 8, 9,
	113, 119, 101, 114, 116, 121, 117, 105, 111, 112, 96, 43, 13, 0, 97, 115, 100, 102, 103, 104, 106, 107, 108, -15,
	-76, -70, 0, -25, 122, 120, 99, 118, 98, 110, 109, 44, 46, 45, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 45, 0, 0, 0, 43, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64,
	58, 95, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
using translateShifted_Spanish_t = char[256];
inline translateShifted_Spanish_t translateShifted_Spanish{0, 27, 33, 34, -73, 36, 37, 38, 47, 40, 41, 61, 63, -65, 8,
	9, 81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 94, 42, 13, 0, 65, 83, 68, 70, 71, 72, 74, 75, 76, -47, -88, -86, 0, -57,
	90, 88, 67, 86, 66, 78, 77, 59, 58, 95, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 0, 0, 0,
	43, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58, 95, 0, 0, 0, 0, 0,
	0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Guessed bound: the database gives none; the next data item starts 0x100 bytes on.
using translateNormal_Italian_t = char[256];
inline translateNormal_Italian_t translateNormal_Italian{0, 27, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 39, -20, 8, 9,
	113, 119, 101, 114, 116, 121, 117, 105, 111, 112, -24, 43, 13, 0, 97, 115, 100, 102, 103, 104, 106, 107, 108, -14,
	-32, 92, 0, -7, 122, 120, 99, 118, 98, 110, 109, 44, 46, 45, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 45, 0, 0, 0, 43, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58,
	95, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};

// Not named in the PDB or IDA; name chosen here. The shifted scan-code table after translateNormal_Italian, read by
// Controls::TranslateKeystroke (0xA238A0) at [scan code + 0x11A7D40].
using translateShifted_Italian_t = char[256];
inline translateShifted_Italian_t translateShifted_Italian{0, 27, 33, 34, -93, 36, 37, 38, 47, 40, 41, 61, 63, 94, 8, 9,
	81, 87, 69, 82, 84, 89, 85, 73, 79, 80, -23, 42, 13, 0, 65, 83, 68, 70, 71, 72, 74, 75, 76, -25, -80, 124, 0, -89,
	90, 88, 67, 86, 66, 78, 77, 59, 58, 95, 0, 42, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 0, 0, 0,
	43, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 64, 58, 95, 0, 0, 0, 0, 0,
	0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 47};
using povDirectionNames_t = const char* [8];
inline povDirectionNames_t povDirectionNames{"U", "UR", "R", "DR", "D", "DL", "L", "UL"};

// INISettingCollection: 0xfb43d8 calls 0x44f570.
extern SettingT<INISettingCollection> gs_bUseJoystick_Controls;

// INISettingCollection: 0xfb4478 calls 0x44f570.
extern SettingT<INISettingCollection> gs_bBackgroundKeyboard_Controls;

// INISettingCollection: 0xfb4518 calls 0x44f570.
extern SettingT<INISettingCollection> gs_bBackground_Mouse_Controls;

// IDA names; not in the PDB. SettingT<INISettingCollection>: their initialisers 0xFB4570, 0xFB4610, 0xFB46B0 and
// 0xFB4750 store SettingT<INISettingCollection>'s vtable and call its InitCollection 0x44F570.
extern SettingT<INISettingCollection> gs_fForegroundMouseMult_Controls;
extern SettingT<INISettingCollection> gs_fForegroundMouseBase_Controls;
extern SettingT<INISettingCollection> gs_fForegroundMouseAccelTop_Controls;
extern SettingT<INISettingCollection> gs_fForegroundMouseAccelBase_Controls;
