#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"

#include <cstdarg>

class NiSourceTexture;
class ScriptCompiler;

// The console: printed lines, input history and the batch recorder.
class MenuConsole {
public:
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
    // Opens or closes the console; returns true when it opened.
    bool ToggleVisible();
    // Feeds the menu key aiKey to the open console; true when it used it.
    bool Idle(int32_t aiKey);
    // Formats apString with xlist and prints it as a console line.
    void PrintLine(const char* apString, va_list xlist);
    // Formats apString with the variadic arguments and prints it as a console line.
    void PrintLine(const char* apString, ...);
    void SetVisible(char acState);
    void DumpTextures();
    // Whether apTexture's file name has a "\menus" directory.
    char IsMenuTexture(NiSourceTexture* apTexture);
    void SetOutputFilename(const char* apOutputFileName);
    void WriteToOutputFile(const char* apOutText);

    // The console singleton; created on demand when abCreate is set.
    static MenuConsole* Instance(bool abCreate);
    static void operator delete(void* apMemory);
    static void Free();

    // The script compiler, the printed lines, the input history and the console metrics.
    ScriptCompiler* pConsoleCompiler;
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
    // The console open state. It is read and compared signed, and stores the caller's byte unchanged.
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

inline bool g_bHasPlayerUsedConsole;

// The game settings MenuConsole.cpp defines.
extern SettingT<INISettingCollection> iConsoleHistorySize;
extern SettingT<INISettingCollection> iConsoleVisibleLines;
extern SettingT<INISettingCollection> iConsoleTextYPos;
extern SettingT<INISettingCollection> iConsoleLineSpacing;
extern SettingT<INISettingCollection> iConsoleFont;
extern SettingT<INISettingCollection> iConsoleTextXPos;
