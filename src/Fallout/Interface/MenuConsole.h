#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Reconstruction.hpp"

#include <cstdarg>

class NiSourceTexture;

// The console: printed lines, input history and the batch recorder.
class MenuConsole {
public:
	// The constructor.
	MenuConsole();
	~MenuConsole();

	// Initializes the console.
	void Init();
	// Scrolls the printed lines by aiLines.
	void ScrollBy(int32_t aiNumLines);
	// Redraws the console's lines.
	void RefreshLines();
	// True while the console is open.
	bool IsVisible();
	// Prepares the console for menu mode.
	void OnEnterMenuMode();
	// Opens or closes the console; true when it opened (al).
	bool ToggleVisible();
	// Feeds the menu key aiKey to the open console; true when it used it (thiscall, ecx
	// unused, ret 4, al, 0x71B210).
	bool Idle(int32_t aiKey);
	// Formats apString with xlist and prints it as a console line.
	void PrintLine(const char* apString, va_list xlist);
	// Formats apString with the variadic arguments and prints it as a console line (cdecl variadic, 0x71D030).
	void PrintLine(const char* apString, ...);
	void SetVisible(char acState);
	void DumpTextures();
	// Whether apTexture's file name has a "\menus" directory.
	char IsMenuTexture(NiSourceTexture* apTexture);
	// The exe's null path leaves eax as it came; its one caller (0x5D9170) never reads it.
	void SetOutputFilename(const char* apOutputFileName);
	void WriteToOutputFile(const char* apOutText);

	// The console singleton; created on demand when abCreate is set.
	static MenuConsole* Instance(bool abCreate);
	static void operator delete(void* apMemory);
	static void Free();

	// The script compiler, the printed lines, the input history and the console metrics.
	void* pConsoleCompiler;
	NiTList<BSStringT<char>> xStrings;
	NiTList<BSStringT<char>> xInputs;
	int32_t iInputsPos;
	int32_t iNumStrings;
	uint32_t iLastString;
	bool bLastMenuMode;
	uint8_t padding29[3];
	uint32_t iConsoleLineSpacing;
	int32_t iConsoleXPos;
	int32_t iConsoleYPos;
	// The console open state. PDB: unsigned char (uc prefix); the exe reads it with movsx and compares it signed
	// (SetVisible 0x71D620), and stores the caller's byte unchanged.
	int8_t cVisible;
	bool bIsActive;
	bool bRecordingBatch;
	uint8_t unk3B;
	int32_t iRecordingLineCount;
	// The recorded batch lines, 100 characters each.
	char cRecording[20][100];
	// The file WriteToOutputFile appends console output to, empty for none.
	char pOutputFileName[260];

	static inline MenuConsole* pInstance;
};

// IDA names; not in the PDB.
inline bool g_bHasPlayerUsedConsole;

// The game settings MenuConsole.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> iConsoleHistorySize;
extern SettingT<INISettingCollection> iConsoleVisibleLines;
extern SettingT<INISettingCollection> iConsoleTextYPos;
extern SettingT<INISettingCollection> iConsoleLineSpacing;
extern SettingT<INISettingCollection> iConsoleFont;
extern SettingT<INISettingCollection> iConsoleTextXPos;
