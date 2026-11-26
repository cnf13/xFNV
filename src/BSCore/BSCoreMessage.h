#pragma once

#include "BSCore/MemoryManager.h"

#include <cstdint>

// The warning categories a context-tagged Warning call names.
enum WARNING_TYPES : int32_t {
    WARN_DEFAULT = 0,
    WARN_COMBAT = 1,
    WARN_ANIMATION = 2,
    WARN_AI = 3,
    WARN_SCRIPTS = 4,
    WARN_SAVELOAD = 5,
    WARN_DIALOGUE = 6,
    WARN_QUESTS = 7,
    WARN_PACKAGES = 8,
    WARN_EDITOR = 9,
    WARN_MODELS = 10,
    WARN_TEXTURES = 11,
    WARN_PLUGINS = 12,
    WARN_MASTERFILE = 13,
    WARN_FORMS = 14,
    WARN_MAGIC = 15,
    WARN_SHADERS = 16,
    WARN_PATHFINDING = 17,
    WARN_MENUS = 18,
    WARN_AUDIO = 19,
    WARN_CELLS = 20,
    WARN_HAVOK = 21,
    WARN_WATER = 22,
    WARN_INGAME_MESSAGE = 23,
    MAX_WARNING_TYPES = 24,
};

// The engine's message handler. The release build's virtuals are empty stubs returning 0; MessageHandler overrides
// the destructor and both Asserts.
class BSCoreMessage {
public:
    enum MsgReturn : int32_t {
        MR_NONE = 0,
        MR_ACCEPT = 1,
        MR_REJECT = 2,
        MR_CANCEL = 3,
    };

    enum MsgStyle : int32_t {
        MS_ASSERT = 0,
        MS_WARNING = 1,
        MS_PROGFLOW = 2,
        MS_INITFLOW = 3,
        MS_DEBUGSTRING = 4,
        MS_OK = 5,
        MS_OKCANCEL = 6,
        MS_YESNO = 7,
        MS_YESNOCANCEL = 8,
        MS_RETRYCANCEL = 9,
        MS_NOYES = 10,
    };

    // One flag per warning context: its warnings are not shown.
    using WarningsDisabledTable = bool[24];

    BSCoreMessage();
    virtual ~BSCoreMessage();

    virtual void Error(const char* apMessage);
    virtual int32_t Warning(const char* apMessage);
    virtual int32_t Assert(const char* apMessage);
    virtual int32_t Assert(const char* apFileName, uint32_t aiLineNumber, const char* apMessage, ...);
    // Shows or logs a message.
    virtual MsgReturn
    Message(MsgStyle aStyle, const char* apcText, const char* apcCaption, uint32_t auiTextID, uint32_t auiCaptionID);

    void DisableContextMessages(WARNING_TYPES aWarningContext);
    // Calls itself on pHandler when one is set.
    void PassToHandler5(uint32_t a1, uint32_t a2, float a3, uint32_t a4, uint32_t a5);
    // Calls itself on pHandler when one is set.
    void StatusOff();
    bool CheckWarningContexts(WARNING_TYPES aWarningContext);

    static bool HasHandler();
    static BSCoreMessage* GetInstance();
    static void SetWarningDisabled(int32_t aWarningContext, bool abValue);
    static void operator delete(void* apMemory) {
        MemoryManager::Instance().Deallocate(apMemory);
    }

    static inline BSCoreMessage* pHandler;
    // The context warnings are reported in.
    static inline WARNING_TYPES sCurrentWarningContext;
    static inline WarningsDisabledTable bWarningsDisabled{};
};
