#include "BSMain/MessageHandler.h"

#include "BSCore/BSCoreMessage.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSUtilities.h"
#include "BSSystem/BSSystemFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/System.h"

// The lock WriteFile takes around its file writes.
static BSCriticalSection CriticalSection;

char return_0_9() {
    return 0;
}

void sub_C3C240() {
    ;
}

MessageHandler::MessageHandler(bool abStatic) : bStatic(abStatic) {
    MessageHandler::pHandler = this;
}

// Deletes the warning file when its size is 70 bytes, and clears the handler pointer if it is this one.
MessageHandler::~MessageHandler() {
    uint64_t auiSize;
    if (BSSystemFile::GetSizeOfFile(MessageHandler::GetWarningFileName(), auiSize) == BSSystemFile::EC_NONE &&
        auiSize == 70) {
        BSSystemFile::DeleteFileA(MessageHandler::GetWarningFileName());
    }
    if (MessageHandler::pHandler == this) {
        MessageHandler::pHandler = nullptr;
    }
}

int32_t MessageHandler::Assert(const char* apMessage) {
    return 0;
}

int32_t MessageHandler::Assert(const char* apFileName, uint32_t aiLineNumber, const char* apMessage, ...) {
    return 0;
}

void MessageHandler::InitSDM() {
    MessageHandler* v0;
    void* v1 = (&MemoryManager_Singleton)->Allocate(sizeof(MessageHandler));
    if (v1) {
        v0 = ::new (v1) MessageHandler(false);
    }
    else {
        v0 = nullptr;
    }
    MessageHandler::pHandler = v0;
}

MessageHandler& MessageHandler::Handler() {
    return *MessageHandler::pHandler;
}

void MessageHandler::IncDisableWarningCount(bool abTF) {
    if (abTF) {
        NiAtomicIncrement(MessageHandler::iDisableWarningCount);
    }
    else {
        NiAtomicDecrement(MessageHandler::iDisableWarningCount);
    }
    if (MessageHandler::iDisableWarningCount < 0) {
        MessageHandler::iDisableWarningCount = 0;
    }
}

int32_t MessageHandler::QWarningCount() {
    return GetThreadTLSData()->iWarningCount;
}

void MessageHandler::SetWarningCount(int32_t aiCount) {
    GetThreadTLSData()->iWarningCount = aiCount;
}

const char* MessageHandler::GetWarningFileName() {
    return pWarningFileName;
}

const char* MessageHandler::GetProgrammerOutputFileName() {
    return pProgrammerOutput;
}

const char* MessageHandler::GetCrashTraceFileName() {
    return pCrashTraceFileName;
}

void MessageHandler::SetProgramFlowFileName(const char* apFileName) {
    strcpy_s(pProgramFlowFileName, 260, BSUtilities::pPersonalPath);
    strcat_s(pProgramFlowFileName, 260, apFileName);
}

void MessageHandler::SetProgrammerOutputFileName(const char* apFileName) {
    strcpy_s(pProgrammerOutput, 260, BSUtilities::pPersonalPath);
    strcat_s(pProgrammerOutput, 260, apFileName);
}

void MessageHandler::SetCrashTraceFileName(const char* apFileName) {
    strcpy_s(pCrashTraceFileName, 260, BSUtilities::pPersonalPath);
    strcat_s(pCrashTraceFileName, 260, apFileName);
}

void MessageHandler::SetWarningQueueing(bool abQueueToggle) {
    bWarningQueueOn = (bool)((uint8_t)abQueueToggle);
}

void MessageHandler::EnableWarningMessageBoxes() {
    for (bool& rbDisabled : BSCoreMessage::bWarningsDisabled) {
        rbDisabled = false;
    }
    bDisableWarningMessageBox = false;
}

int32_t MessageHandler::DoMessageBox(const char* apText, const char* apCaption, uint32_t aiType) {
    int32_t iretVal = 0;
    if (MessageHandler::pMessageBoxCallback) {
        char v4 = 0;
        if ((aiType & 3) != 0) {
            MessageHandler::InstallMessageBoxHook();
            if (platform::MessageBoxCancelTextActive()) {
                aiType = (aiType & 0xFFFFCFFFu) | 0x2000;
                v4 = 1;
            }
        }
        iretVal = MessageHandler::pMessageBoxCallback(apText, apCaption, aiType);
        if (v4) {
            MessageHandler::RemoveMessageBoxHook();
        }
    }
    return iretVal;
}

void MessageHandler::SetLoadingStream(BSStream*) {
    ;
}

void MessageHandler::BSNiLog(int32_t, const char*) {
    ;
}

void MessageHandler::Init(
    bool abDisableWarningMessageBox,
    bool abAllowYesToAll,
    bool abSkipProgramFlows,
    bool abSkipInitializationFlows,
    int32_t (*const apMessageBoxCallback)(const char*, const char*, uint32_t)
) {
    InitSDM();
    bDisableWarningMessageBox = abDisableWarningMessageBox;
    bAllowYesToAll = abAllowYesToAll;
    bSkipProgramFlows = abSkipProgramFlows;
    bSkipInitializationFlows = abSkipInitializationFlows;
    pMessageBoxCallback = apMessageBoxCallback;
    for (bool& rbDisabled : BSCoreMessage::bWarningsDisabled) {
        rbDisabled = false;
    }
    BSCoreMessage::pHandler = pHandler;
}

// MB_TASKMODAL | MB_ICONEXCLAMATION | MB_YESNO (0x2034).
int32_t MessageHandler::MenuMessageBoxYesNo(const char* apMessageText) {
    bWaitingForMessageBox = true;
    int32_t iResult = DoMessageBox(apMessageText, "Confirm", 0x2034);
    bWaitingForMessageBox = false;
    return iResult;
}

// MB_TASKMODAL | MB_ICONEXCLAMATION | MB_YESNOCANCEL (0x2033).
int32_t MessageHandler::MenuMessageBoxYesNoAll(const char* apMessageText) {
    bWaitingForMessageBox = true;
    int32_t iResult = DoMessageBox(apMessageText, "Confirm", 0x2033);
    bWaitingForMessageBox = false;
    return iResult;
}

uint32_t MessageHandler::NiWarning(const char*, const char*, void*) {
    return (uint32_t)0;
}

void MessageHandler::SetWarningFileName(const char* apFileName) {
    strcpy_s(MessageHandler::pWarningFileName, 260, BSUtilities::pPersonalPath);
    strcat_s(MessageHandler::pWarningFileName, 260, apFileName);
}

const char* MessageHandler::GetProgramFlowFileName() {
    return MessageHandler::pProgramFlowFileName;
}

// Returns true at once without writing anything, as the game does: the file writing (BSSystemFile, DoWrite, DoFlush) is
// unreachable in the game.
bool MessageHandler::WriteFile(
    [[maybe_unused]] const char* apFileName,
    [[maybe_unused]] const char* apBuffer,
    [[maybe_unused]] bool abFlush
) {
    return true;
}

// Captions the Cancel button of the thread's next message box "Yes to all".
void MessageHandler::InstallMessageBoxHook() {
    platform::MessageBoxCancelTextBegin("Yes to all");
}

void MessageHandler::RemoveMessageBoxHook() {
    platform::MessageBoxCancelTextEnd();
}

void MessageHandler::AddToAssertQueue() {
    ;
}

void MessageHandler::AddToWarningQueue() {
    ;
}

void MessageHandler::DisplayAssertQueue() {
    ;
}

void MessageHandler::DisplayWarningQueue() {
    ;
}
