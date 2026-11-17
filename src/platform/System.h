#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <thread>

struct SDL_Window;

// The process and its threads: what neither the standard library nor SDL's window covers for the game. Names avoid
// the Win32 SDK's A/W macros (MessageBox, LoadLibrary), so they are safe beside windows.h.
namespace platform {

    // SetThreadPriority with a THREAD_PRIORITY_* value (-15, -2..2, 15) on a running std::thread. POSIX: no effect,
    // true (a normal-policy thread's priority cannot be raised without privileges).
    bool SetThreadPriority(std::thread& thread, int32_t priority);

    // SetThreadIdealProcessor(GetCurrentThread(), processor): a scheduling hint for the calling thread. POSIX: no
    // effect (no soft hint; a hard affinity would be a different behaviour).
    void SetCurrentThreadIdealProcessor(uint32_t processor);

    // The debugger's thread name. Win32: the 0x406D1388 exception, raised only while a debugger is attached (as the
    // exe's code does). POSIX: pthread_setname_np, the first 15 characters.
    void SetThreadName(std::thread& thread, const char* name);
    void SetCurrentThreadName(const char* name);

    // MessageBoxA without an owner window: type takes MB_* (buttons in the low 4 bits, MB_ICON* in bits 4-7), the
    // result is the IDOK..IDNO value of the pressed button (IDCANCEL 2 when closed). POSIX: SDL_ShowMessageBox.
    int32_t ShowMessageBox(const char* text, const char* caption, uint32_t type);

    // Claims the named single instance for the process's life: false when another process holds it. Win32: a named
    // mutex (ERROR_ALREADY_EXISTS). POSIX: an exclusive lock on `<LocalAppData>/FalloutNV/<name>.lock`.
    bool SingleInstance(const char* name);

    // The calling thread's system id: GetCurrentThreadId (Win32), gettid (POSIX: Linux), the ids the crash log reads.
    uint32_t CurrentThreadId();

    // The system time zone's offset from UTC, in seconds, at a UTC time given in seconds since 1970-01-01 (local time
    // is UTC plus the offset). Win32: std::chrono::current_zone (Windows' own zone through ICU). POSIX: localtime_r's
    // tm_gmtoff (TZ or /etc/localtime); not libstdc++'s current_zone, whose zone database is a static shared_ptr
    // destroyed at exit after the game's heap that allocated it is gone.
    int64_t UtcOffset(int64_t utcSeconds);

    // GetSystemMetrics(SM_REMOTESESSION): whether the process runs in a remote desktop session. POSIX: false.
    bool RemoteSession();

    // MessageHandler's "Yes to all" button: until MessageBoxCancelTextEnd, the next message box the calling thread
    // shows has its Cancel button captioned text, and the renaming then ends by itself. Win32: the exe's WH_CBT hook on
    // the thread (MessageHandler::InstallMessageBoxHook 0xC3C250, MessageBoxCBTHookProc 0xC3C3A0, IsCancelButton
    // 0xC3C2B0, RemoveMessageBoxHook 0xC3C280), which renames the box's "Cancel" button on its activation. POSIX:
    // ShowMessageBox's Cancel text. Begin does nothing while a renaming is pending; Active says whether one is.
    void MessageBoxCancelTextBegin(const char* text);
    bool MessageBoxCancelTextActive();
    void MessageBoxCancelTextEnd();

    // IsDebuggerPresent, DebugBreak, OutputDebugStringA. POSIX: a tracer (TracerPid), SIGTRAP, the text on stderr.
    bool DebuggerPresent();
    void DebuggerBreak();
    void DebuggerOutput(const char* text);

    // LoadLibraryA, GetProcAddress, FreeLibrary; nullptr / false on failure. POSIX: dlopen, with `D3D9.DLL` (any case)
    // mapped to dxvk-native's `libdxvk_d3d9.so.0`; other `.dll` names fail as missing libraries.
    void* LibraryLoad(const char* name);
    void* LibrarySymbol(void* library, const char* symbol);
    bool LibraryFree(void* library);

    // Opens a file or URL with the user's default handler (ShellExecuteA's "open"); false on failure. POSIX: a game
    // path through NativePath, as a file:// URL to SDL_OpenURL.
    bool Open(const char* pathOrUrl);

    // Whether the game's launcher (FalloutNVLauncher.exe, which writes FalloutPrefs.ini for the adapter) can be
    // started. Win32: true. POSIX: false, there is no launcher; the game does the launcher's part itself
    // (CreateDisplayDriver).
    bool LauncherAvailable();

    // The mouse settings the game reads at start-up: the swapped-buttons flag (SwapMouseButton), GetDoubleClickTime's
    // milliseconds, and HKCU\Control Panel\Mouse's MouseSensitivity (6 when missing). POSIX: false, 500, 10.
    struct MouseInfo {
        bool swapButtons;
        uint32_t doubleClickTime;
        int32_t sensitivity;
    };
    MouseInfo MouseSettings();

    // The settings store RegSettingCollection (XRegSetting) reads and writes: HKLM\<key> on Win32 (KEY_ALL_ACCESS),
    // the section [key] of `<LocalAppData>/FalloutNV/Registry.ini` on POSIX. RegistryOpen gives nullptr when the key
    // is missing and create is false. Types are REG_* values (1 REG_SZ, 3 REG_BINARY, 4 REG_DWORD).
    void* RegistryOpen(const char* key, bool create);
    void RegistryClose(void* handle);
    // RegSetValueExA: stores data as the named value of the given type.
    bool RegistryWrite(void* handle, const char* name, uint32_t type, std::span<const uint8_t> data);
    // RegQueryValueExA: on success sets type and size (bytes, a REG_SZ's NUL included) and fills data; false when the
    // value is missing or longer than data (size then holds its length).
    bool RegistryRead(void* handle, const char* name, uint32_t& type, std::span<uint8_t> data, uint32_t& size);

    // The command line as arguments, the program first (CommandLineToArgvW(GetCommandLineW()) in UTF-8; POSIX: argv).
    std::span<const std::string> CommandLine();

    // Makes the loss of the window's display connection a quit request (SDL_EVENT_QUIT, which the game's event handler
    // takes as the window's close), so the game leaves through its own quit path. Win32: no effect (the display does
    // not go away under a window). POSIX: on X11, Xlib's I/O error handler and the display's exit handler, whose
    // defaults call exit() and run the static destructors under the game's live threads; SDL's Wayland part sends the
    // quit itself.
    void QuitOnDisplayLoss(SDL_Window* window);

    // Whether QuitOnDisplayLoss's display connection is gone. Xlib extension requests on it then crash (libXi's grab
    // calls), so a window's grab is not released on it. Win32: false.
    bool DisplayLost();

} // namespace platform
