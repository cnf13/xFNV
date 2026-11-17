// The process and its threads on Windows: the Win32 calls the exe makes, unchanged.
#include "platform/System.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <shellapi.h>

namespace platform {

    namespace {

// The debugger's thread-naming record (MSDN "How to: Set a Thread Name in Native Code").
#pragma pack(push, 8)
        struct ThreadNameInfo {
            DWORD type;
            const char* name;
            DWORD threadId;
            DWORD flags;
        };
#pragma pack(pop)

        // Raises 0x406D1388 for the attached debugger; the __except continues as the exe's filter does.
        void RaiseThreadName(DWORD threadId, const char* name) {
            if (!IsDebuggerPresent()) {
                return;
            }
            ThreadNameInfo info{0x1000, name, threadId, 0};
            __try {
                RaiseException(
                    0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<const ULONG_PTR*>(&info)
                );
            }
            __except (EXCEPTION_CONTINUE_EXECUTION) {
            }
        }

        // MessageHandler's hook (0x1202D70 in the exe) and the caption it gives the Cancel button.
        HHOOK gMessageBoxHook = nullptr;
        const char* gCancelText = nullptr;

        // MessageHandler::IsCancelButton (0xC3C2B0): stops the enumeration at the child button captioned "Cancel"
        // (compared case-insensitively, as the exe's _tcsicmp), storing its handle where lParam (the Windows API's
        // untyped word) points.
        BOOL CALLBACK IsCancelButton(HWND window, LPARAM lParam) {
            if (window) {
                char text[260] = {};
                if (GetClassNameA(window, text, 260) && !_stricmp(text, "Button")) {
                    std::memset(text, 0, sizeof(text));
                    if (GetWindowTextA(window, text, 260) && text[0] && !_stricmp(text, "Cancel")) {
                        *reinterpret_cast<HWND*>(lParam) = window;
                        return FALSE;
                    }
                }
            }
            return TRUE;
        }

        // MessageHandler::MessageBoxCBTHookProc (0xC3C3A0): on a window's activation (HCBT_ACTIVATE; wParam is its
        // handle), renames its Cancel button and removes the hook. WPARAM and LPARAM are the Windows API's untyped
        // words.
        LRESULT CALLBACK MessageBoxCBTHookProc(int code, WPARAM wParam, LPARAM lParam) {
            if (code < 0) {
                return CallNextHookEx(gMessageBoxHook, code, wParam, lParam);
            }
            if (code == HCBT_ACTIVATE) {
                HWND cancelButton = nullptr;
                EnumChildWindows(
                    reinterpret_cast<HWND>(wParam), IsCancelButton, reinterpret_cast<LPARAM>(&cancelButton)
                );
                if (cancelButton) {
                    SetWindowTextA(cancelButton, gCancelText);
                    MessageBoxCancelTextEnd();
                }
            }
            return CallNextHookEx(gMessageBoxHook, code, wParam, lParam);
        }

    } // namespace

    bool SetThreadPriority(std::thread& thread, int32_t priority) {
        return ::SetThreadPriority(static_cast<HANDLE>(thread.native_handle()), priority) != 0;
    }

    void SetCurrentThreadIdealProcessor(uint32_t processor) {
        ::SetThreadIdealProcessor(GetCurrentThread(), processor);
    }

    void SetThreadName(std::thread& thread, const char* name) {
        RaiseThreadName(GetThreadId(static_cast<HANDLE>(thread.native_handle())), name);
    }

    void SetCurrentThreadName(const char* name) {
        RaiseThreadName(static_cast<DWORD>(-1), name);
    }

    int32_t ShowMessageBox(const char* text, const char* caption, uint32_t type) {
        return MessageBoxA(nullptr, text, caption, type);
    }

    bool SingleInstance(const char* name) {
        // The handle stays open for the process's life, as the exe's does.
        CreateMutexA(nullptr, FALSE, name);
        return GetLastError() != ERROR_ALREADY_EXISTS;
    }

    uint32_t CurrentThreadId() {
        return ::GetCurrentThreadId();
    }

    int64_t UtcOffset(int64_t utcSeconds) {
        using namespace std::chrono;
        return current_zone()->get_info(sys_seconds{seconds{utcSeconds}}).offset.count();
    }

    bool RemoteSession() {
        return GetSystemMetrics(SM_REMOTESESSION) != 0;
    }

    bool LauncherAvailable() {
        return true;
    }

    void MessageBoxCancelTextBegin(const char* text) {
        if (!gMessageBoxHook) {
            gCancelText = text;
            gMessageBoxHook = SetWindowsHookExA(WH_CBT, MessageBoxCBTHookProc, nullptr, GetCurrentThreadId());
        }
    }

    bool MessageBoxCancelTextActive() {
        return gMessageBoxHook != nullptr;
    }

    void MessageBoxCancelTextEnd() {
        if (gMessageBoxHook) {
            UnhookWindowsHookEx(gMessageBoxHook);
            gMessageBoxHook = nullptr;
        }
    }

    bool DebuggerPresent() {
        return IsDebuggerPresent() != 0;
    }

    void DebuggerBreak() {
        DebugBreak();
    }

    void DebuggerOutput(const char* text) {
        OutputDebugStringA(text);
    }

    void* LibraryLoad(const char* name) {
        return LoadLibraryA(name);
    }

    void* LibrarySymbol(void* library, const char* symbol) {
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), symbol));
    }

    bool LibraryFree(void* library) {
        return FreeLibrary(static_cast<HMODULE>(library)) != 0;
    }

    bool Open(const char* pathOrUrl) {
        // ShellExecuteA's result is an error code below 33.
        return reinterpret_cast<INT_PTR>(ShellExecuteA(nullptr, "open", pathOrUrl, nullptr, nullptr, SW_SHOWNORMAL)) >
            32;
    }

    MouseInfo MouseSettings() {
        MouseInfo info{false, 0, 6};
        HKEY key;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Mouse", 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS) {
            char data[12] = {};
            DWORD type;
            DWORD size = 10;
            if (RegQueryValueExA(key, "MouseSensitivity", nullptr, &type, reinterpret_cast<BYTE*>(data), &size) ==
                ERROR_SUCCESS) {
                info.sensitivity = std::atoi(data);
            }
            RegCloseKey(key);
        }
        // SwapMouseButton gives the previous state; the second call restores it.
        info.swapButtons = SwapMouseButton(FALSE) != 0;
        SwapMouseButton(info.swapButtons);
        info.doubleClickTime = GetDoubleClickTime();
        return info;
    }

    void* RegistryOpen(const char* key, bool create) {
        HKEY handle = nullptr;
        if (create) {
            DWORD disposition;
            if (RegCreateKeyExA(
                    HKEY_LOCAL_MACHINE, key, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &handle, &disposition
                ) != ERROR_SUCCESS) {
                return nullptr;
            }
        }
        else if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS, &handle) != ERROR_SUCCESS) {
            return nullptr;
        }
        return handle;
    }

    void RegistryClose(void* handle) {
        RegCloseKey(static_cast<HKEY>(handle));
    }

    bool RegistryWrite(void* handle, const char* name, uint32_t type, std::span<const uint8_t> data) {
        return RegSetValueExA(static_cast<HKEY>(handle), name, 0, type, data.data(), static_cast<DWORD>(data.size())) ==
            ERROR_SUCCESS;
    }

    bool RegistryRead(void* handle, const char* name, uint32_t& type, std::span<uint8_t> data, uint32_t& size) {
        DWORD valueType = 0;
        DWORD valueSize = static_cast<DWORD>(data.size());
        const LSTATUS status =
            RegQueryValueExA(static_cast<HKEY>(handle), name, nullptr, &valueType, data.data(), &valueSize);
        type = valueType;
        size = valueSize;
        return status == ERROR_SUCCESS;
    }

    std::span<const std::string> CommandLine() {
        static const std::vector<std::string> arguments = [] {
            std::vector<std::string> result;
            int count = 0;
            LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &count);
            if (!wide) {
                return result;
            }
            for (int i = 0; i < count; ++i) {
                const int length = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
                std::string argument(length > 0 ? length - 1 : 0, '\0');
                if (length > 1) {
                    WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, argument.data(), length, nullptr, nullptr);
                }
                result.push_back(std::move(argument));
            }
            LocalFree(wide);
            return result;
        }();
        return arguments;
    }

    void QuitOnDisplayLoss(SDL_Window*) {}

    bool DisplayLost() {
        return false;
    }

} // namespace platform
