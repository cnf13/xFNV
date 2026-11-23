#pragma once

#include "BSCore/BSCoreMessage.h"

#include <stdint.h>

class BSStream;

template <class T, uint32_t uiGrowStep>
class BSSimpleArray;
template <class Char>
class BSStringT;

char return_0_9();

class MessageHandler : public BSCoreMessage {
public:
    // The message box function Init installs (DoMessageBox calls it with the text, caption and type).
    using MessageBoxCallback = int32_t (*)(const char*, const char*, uint32_t);
    // The log file paths (260-byte buffers) and their getters and setters: a setter writes the personal folder path
    // followed by apFileName (strcpy_s then strcat_s).
    using FileNameBuffer = char[260];
    using MessageQueue = BSSimpleArray<const BSStringT<char>*, 1024>;

    MessageHandler(bool abStatic);
    ~MessageHandler() override;

    // Returns 0.
    int32_t Assert(const char* apMessage) override;
    // Returns 0.
    int32_t Assert(const char* apFileName, uint32_t aiLineNumber, const char* apMessage, ...) override;

    // Sets up the static message handler.
    static void InitSDM();
    // The handler singleton.
    static MessageHandler& Handler();
    // Counts up (abTF) or down the nesting that suppresses warnings.
    static void IncDisableWarningCount(bool abTF);
    // The number of warnings this thread has reported.
    static int32_t QWarningCount();
    // Sets the calling thread's warning count.
    static void SetWarningCount(int32_t aiCount);
    static const char* GetWarningFileName();
    static const char* GetProgrammerOutputFileName();
    static const char* GetCrashTraceFileName();
    static void SetProgramFlowFileName(const char* apFileName);
    static void SetProgrammerOutputFileName(const char* apFileName);
    static void SetCrashTraceFileName(const char* apFileName);
    static void SetWarningQueueing(bool abQueueToggle);
    // Re-enables every warning context and the warning message box.
    static void EnableWarningMessageBoxes();
    static int32_t DoMessageBox(const char* apText, const char* apCaption, uint32_t aiType);
    // Empty in this build.
    static void SetLoadingStream(BSStream* apLoadingStream);
    // Empty in this build.
    static void BSNiLog(int32_t aiLevel, const char* apMessage);
    // Sets up the static handler and the message box options, and enables every warning context.
    static void Init(
        bool abDisableWarningMessageBox,
        bool abAllowYesToAll,
        bool abSkipProgramFlows,
        bool abSkipInitializationFlows,
        int32_t (*const apMessageBoxCallback)(const char*, const char*, uint32_t)
    );
    // A Yes/No (and Yes to all) confirmation box; returns the button.
    static int32_t MenuMessageBoxYesNo(const char* apMessageText);
    static int32_t MenuMessageBoxYesNoAll(const char* apMessageText);
    static void Close();

protected:
    // Returns 0 in this build.
    static uint32_t NiWarning(const char* apWarning, const char* apCaption, void* apExtraData);

public:
    static void SetYesToAll(bool abTF);
    static void SetWarningFileName(const char* apFileName);
    static const char* GetProgramFlowFileName();
    static bool WriteFile(const char* apFileName, const char* apBuffer, bool abFlush);
    // Captions the Cancel button of the thread's next message box "Yes to all".
    static void InstallMessageBoxHook();
    static void RemoveMessageBoxHook();
    // On PC these queue functions are empty.
    static void AddToAssertQueue();
    static void AddToWarningQueue();
    static void DisplayAssertQueue();
    static void DisplayWarningQueue();
    static int32_t QWarningContext();

    // The handler is the static one set up by InitSDM (the constructor's abStatic); Close deletes the
    // handler only when this is false.
    bool bStatic;

    static inline bool bAllowYesToAll = true;
    static inline bool bSkipProgramFlows = true;
    static inline bool bSkipInitializationFlows = true;
    static inline MessageBoxCallback pMessageBoxCallback{};
    static inline int32_t iDisableWarningCount;
    static inline FileNameBuffer pWarningFileName{};
    static inline FileNameBuffer pProgramFlowFileName{};
    static inline FileNameBuffer pProgrammerOutput{};
    static inline FileNameBuffer pCrashTraceFileName{};
    static inline bool bDisableWarningMessageBox = true;
    static inline bool bWaitingForMessageBox;
    static inline MessageHandler* pHandler;

    static inline bool bYesToAll;
    static inline bool bAssertQueueOn;
    static inline bool bQueueAllAsserts;
    static inline bool bWarningQueueOn;
    // Set by TESDataHandler::RegisterError.
    static inline bool bShowNextWarning;
    static inline MessageQueue* qAssertQueue;
    static inline MessageQueue* qWarningQueue;
    // The calling thread's warning count, which QWarningCount/SetWarningCount read and write.
    static inline thread_local int32_t iWarningCount;
};
