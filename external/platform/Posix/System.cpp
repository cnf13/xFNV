// The process and its threads on POSIX: pthreads, dlopen, a lock file, SDL's message box and URL opener, and the
// registry-backed settings as an ini file in the user folder.
#include "platform/System.h"
#include "platform/Ini.h"
#include "platform/Paths.h"
#include "Posix/CommandLine.h"

#include <atomic>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

namespace platform {

namespace {

std::vector<std::string> gArguments;

bool EqualNoCase(const char* a, const char* b) {
	for (; *a && *b; ++a, ++b) {
		if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
			return false;
		}
	}
	return *a == *b;
}

// pthread_setname_np takes at most 15 characters.
void NameThread(pthread_t thread, const char* name) {
	char shortName[16] = {};
	std::strncpy(shortName, name ? name : "", sizeof(shortName) - 1);
	pthread_setname_np(thread, shortName);
}

// The folder of the game's files in the user's local data (where the game itself puts plugins.txt).
std::filesystem::path GameDataFolder() {
	std::filesystem::path folder = std::filesystem::path(UserFolder(UserFolderKind::LocalAppData)) / "FalloutNV";
	std::error_code error;
	std::filesystem::create_directories(folder, error);
	return folder;
}

std::string RegistryFile() {
	return (GameDataFolder() / "Registry.ini").string();
}

// MessageBoxCancelTextBegin's pending caption and the thread whose next message box takes it.
std::mutex gCancelTextMutex;
const char* gCancelText = nullptr;
std::thread::id gCancelTextThread;

}  // namespace

void SetCommandLine(int argc, char** argv) {
	gArguments.assign(argv, argv + argc);
}

bool SetThreadPriority(std::thread&, int32_t) {
	return true;
}

void SetCurrentThreadIdealProcessor(uint32_t) {
}

void SetThreadName(std::thread& thread, const char* name) {
	NameThread(thread.native_handle(), name);
}

void SetCurrentThreadName(const char* name) {
	NameThread(pthread_self(), name);
}

int32_t ShowMessageBox(const char* text, const char* caption, uint32_t type) {
	// The Cancel caption MessageBoxCancelTextBegin left for this thread's next box, taken once.
	const char* cancelText = "Cancel";
	{
		std::lock_guard lock(gCancelTextMutex);
		if (gCancelText && gCancelTextThread == std::this_thread::get_id()) {
			cancelText = gCancelText;
			gCancelText = nullptr;
		}
	}
	// MB_* button sets and the ID each button returns.
	enum : int { IDOK = 1, IDCANCEL, IDABORT, IDRETRY, IDIGNORE, IDYES, IDNO, IDTRYAGAIN = 10, IDCONTINUE };
	struct Button {
		int id;
		const char* text;
	};
	const std::vector<Button> kSets[] = {
		{{IDOK, "OK"}},
		{{IDOK, "OK"}, {IDCANCEL, cancelText}},
		{{IDABORT, "Abort"}, {IDRETRY, "Retry"}, {IDIGNORE, "Ignore"}},
		{{IDYES, "Yes"}, {IDNO, "No"}, {IDCANCEL, cancelText}},
		{{IDYES, "Yes"}, {IDNO, "No"}},
		{{IDRETRY, "Retry"}, {IDCANCEL, cancelText}},
		{{IDCANCEL, cancelText}, {IDTRYAGAIN, "Try Again"}, {IDCONTINUE, "Continue"}},
	};
	const uint32_t setIndex = type & 0xF;
	const std::vector<Button>& set = kSets[setIndex < std::size(kSets) ? setIndex : 0];
	// MB_DEFBUTTON1..4 in bits 8-11; Escape gives Cancel where there is one, OK for a lone OK (as Windows does).
	const size_t defaultIndex = (type >> 8) & 0xF;
	int escapeId = 0;
	std::vector<SDL_MessageBoxButtonData> buttons;
	for (size_t i = 0; i < set.size(); ++i) {
		SDL_MessageBoxButtonFlags flags = 0;
		if (i == (defaultIndex < set.size() ? defaultIndex : 0)) {
			flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		}
		if (set[i].id == IDCANCEL || (set.size() == 1 && set[i].id == IDOK)) {
			flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
			escapeId = set[i].id;
		}
		buttons.push_back({flags, set[i].id, set[i].text});
	}
	SDL_MessageBoxFlags flags = SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT;
	switch (type & 0xF0) {
		case 0x10:
			flags |= SDL_MESSAGEBOX_ERROR;
			break;
		case 0x30:
			flags |= SDL_MESSAGEBOX_WARNING;
			break;
		default:
			flags |= SDL_MESSAGEBOX_INFORMATION;
			break;
	}
	const SDL_MessageBoxData data{flags, nullptr, caption ? caption : "", text ? text : "",
		static_cast<int>(buttons.size()), buttons.data(), nullptr};
	int pressed = -1;
	if (!SDL_ShowMessageBox(&data, &pressed)) {
		// No display: the text goes to stderr and the result is MessageBoxA's failure.
		std::fprintf(stderr, "%s: %s\n", caption ? caption : "", text ? text : "");
		return 0;
	}
	if (pressed < 0) {
		return escapeId ? escapeId : IDCANCEL;
	}
	return pressed;
}

bool SingleInstance(const char* name) {
	const std::string path = (GameDataFolder() / (std::string(name) + ".lock")).string();
	// The descriptor stays open, so the lock holds for the process's life (as the exe's mutex handle does).
	const int file = open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
	if (file < 0) {
		return true;
	}
	struct flock lock{};
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	if (fcntl(file, F_SETLK, &lock) != 0 && (errno == EACCES || errno == EAGAIN)) {
		close(file);
		return false;
	}
	return true;
}

uint32_t CurrentThreadId() {
	return static_cast<uint32_t>(gettid());
}

int64_t UtcOffset(int64_t utcSeconds) {
	time_t time = static_cast<time_t>(utcSeconds);
	tm local;
	if (!localtime_r(&time, &local)) {
		return 0;
	}
	return local.tm_gmtoff;
}

bool RemoteSession() {
	return false;
}

bool LauncherAvailable() {
	return false;
}

void MessageBoxCancelTextBegin(const char* text) {
	std::lock_guard lock(gCancelTextMutex);
	if (!gCancelText) {
		gCancelText = text;
		gCancelTextThread = std::this_thread::get_id();
	}
}

bool MessageBoxCancelTextActive() {
	std::lock_guard lock(gCancelTextMutex);
	return gCancelText != nullptr;
}

void MessageBoxCancelTextEnd() {
	std::lock_guard lock(gCancelTextMutex);
	gCancelText = nullptr;
}

bool DebuggerPresent() {
	std::FILE* file = std::fopen("/proc/self/status", "r");
	if (!file) {
		return false;
	}
	char line[256];
	long tracer = 0;
	while (std::fgets(line, sizeof(line), file)) {
		if (std::strncmp(line, "TracerPid:", 10) == 0) {
			tracer = std::strtol(line + 10, nullptr, 10);
			break;
		}
	}
	std::fclose(file);
	return tracer != 0;
}

void DebuggerBreak() {
	std::raise(SIGTRAP);
}

void DebuggerOutput(const char* text) {
	std::fputs(text, stderr);
}

void* LibraryLoad(const char* name) {
	// The renderer loads Direct3D 9 at run time: dxvk-native's library stands in for it.
	if (EqualNoCase(name, "d3d9.dll")) {
		name = "libdxvk_d3d9.so.0";
	}
	return dlopen(name, RTLD_NOW | RTLD_LOCAL);
}

void* LibrarySymbol(void* library, const char* symbol) {
	return library ? dlsym(library, symbol) : nullptr;
}

bool LibraryFree(void* library) {
	return library && dlclose(library) == 0;
}

bool Open(const char* pathOrUrl) {
	std::string url = pathOrUrl;
	if (url.find("://") == std::string::npos) {
		// A Windows program (the game starts FalloutNVLauncher.exe when its display settings do not fit) is never
		// handed to the desktop: POSIX has none to run, so it fails as ShellExecuteA does for a missing file.
		if (url.size() >= 4 && EqualNoCase(url.c_str() + url.size() - 4, ".exe")) {
			return false;
		}
		// A file: its absolute native path, percent-encoded outside the unreserved characters and `/`.
		std::error_code error;
		const std::string path = std::filesystem::absolute(NativePath(pathOrUrl), error).string();
		url = "file://";
		for (const unsigned char c : path) {
			if (std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == '~') {
				url += static_cast<char>(c);
			} else {
				char escaped[4];
				std::snprintf(escaped, sizeof(escaped), "%%%02X", c);
				url += escaped;
			}
		}
	}
	return SDL_OpenURL(url.c_str());
}

MouseInfo MouseSettings() {
	return {false, 500, 10};
}

// A handle is the key's name, the section of Registry.ini; values are `name=<type>:<data>` with REG_SZ data as
// quoted text and every other type as hex bytes.
void* RegistryOpen(const char* key, bool create) {
	if (!create) {
		char probe[4];
		if (IniGetKeys(RegistryFile().c_str(), key, "", probe) == 0) {
			return nullptr;
		}
	}
	return new std::string(key);
}

void RegistryClose(void* handle) {
	delete static_cast<std::string*>(handle);
}

bool RegistryWrite(void* handle, const char* name, uint32_t type, std::span<const uint8_t> data) {
	if (!handle) {
		return false;
	}
	std::string text = std::to_string(type) + ":";
	if (type == 1) {
		text += '"';
		for (const uint8_t byte : data) {
			if (!byte) {
				break;
			}
			text += static_cast<char>(byte);
		}
		text += '"';
	} else {
		for (const uint8_t byte : data) {
			char hex[3];
			std::snprintf(hex, sizeof(hex), "%02x", byte);
			text += hex;
		}
	}
	return IniWriteString(RegistryFile().c_str(), static_cast<std::string*>(handle)->c_str(), name, text.c_str());
}

bool RegistryRead(void* handle, const char* name, uint32_t& type, std::span<uint8_t> data, uint32_t& size) {
	if (!handle) {
		return false;
	}
	std::vector<char> text(2 * data.size() + 64 > 4096 ? 2 * data.size() + 64 : 4096);
	const uint32_t length = IniGetString(RegistryFile().c_str(), static_cast<std::string*>(handle)->c_str(), name,
		"", text);
	const char* colon = static_cast<const char*>(std::memchr(text.data(), ':', length));
	if (!colon) {
		return false;
	}
	type = static_cast<uint32_t>(std::strtoul(text.data(), nullptr, 10));
	std::vector<uint8_t> bytes;
	const char* payload = colon + 1;
	const char* end = text.data() + length;
	if (type == 1) {
		if (end - payload >= 2 && *payload == '"' && end[-1] == '"') {
			++payload;
			--end;
		}
		bytes.assign(payload, end);
		bytes.push_back(0);
	} else {
		for (const char* p = payload; p + 1 < end; p += 2) {
			const char pair[3] = {p[0], p[1], 0};
			bytes.push_back(static_cast<uint8_t>(std::strtoul(pair, nullptr, 16)));
		}
	}
	size = static_cast<uint32_t>(bytes.size());
	if (bytes.size() > data.size()) {
		return false;
	}
	std::memcpy(data.data(), bytes.data(), bytes.size());
	return true;
}

std::span<const std::string> CommandLine() {
	return gArguments;
}

namespace {

// Xlib's I/O error handlers, with the Display as an opaque pointer. On a lost connection Xlib calls the process's
// XIOErrorHandler, then the display's XIOErrorExitHandler (libX11 1.7 and later), in the thread whose call found the
// connection gone; the default of each calls exit(). When both return, the display stays marked broken
// (XlibDisplayIOError) and Xlib's later calls on it fail without reaching the server.
using XIOErrorHandler = int32_t (*)(void* display);
using XSetIOErrorHandlerFunc = XIOErrorHandler (*)(XIOErrorHandler handler);
using XIOErrorExitHandler = void (*)(void* display, void* data);
using XSetIOErrorExitHandlerFunc = void (*)(void* display, XIOErrorExitHandler handler, void* data);

// Xlib's default handler's report, once, without its exit(); the display's exit handler then decides.
int32_t OnIOError(void*) {
	static std::atomic<bool> reported{false};
	if (!reported.exchange(true)) {
		std::fputs("platform: the X display connection is lost\n", stderr);
	}
	return 0;
}

// Set once the window's display connection is gone.
std::atomic<bool> gDisplayLost{false};

// The quit request goes out once, whichever call meets the broken connection first.
void OnDisplayLost(void*, void*) {
	if (gDisplayLost.exchange(true)) {
		return;
	}
	SDL_Event event = {};
	event.type = SDL_EVENT_QUIT;
	SDL_PushEvent(&event);
}

}  // namespace

void QuitOnDisplayLoss(SDL_Window* window) {
	void* display = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
		nullptr);
	if (!display) {
		return;
	}
	// SDL loads libX11 itself (RTLD_LOCAL): its handle, without loading a second copy.
	void* library = dlopen("libX11.so.6", RTLD_NOW | RTLD_NOLOAD);
	if (!library) {
		return;
	}
	const auto setHandler = reinterpret_cast<XSetIOErrorHandlerFunc>(dlsym(library, "XSetIOErrorHandler"));
	const auto setExitHandler = reinterpret_cast<XSetIOErrorExitHandlerFunc>(dlsym(library, "XSetIOErrorExitHandler"));
	if (setHandler && setExitHandler) {
		// Process-wide: a display without this exit handler (SDL's request display) still exits in its default.
		setHandler(&OnIOError);
		setExitHandler(display, &OnDisplayLost, nullptr);
	}
	dlclose(library);
}

bool DisplayLost() {
	return gDisplayLost.load();
}

}  // namespace platform
