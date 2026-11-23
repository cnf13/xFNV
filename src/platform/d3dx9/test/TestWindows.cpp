// The Windows side of the comparison program (D3DX9Compare.cpp): a hidden window and a HAL Direct3D 9 device on it,
// and the DLL loader. Kept apart because it includes the Windows SDK's headers, which clash with the game's own
// Direct3D declarations the rest of the program uses.

#include <windows.h>

#include <d3d9.h>

extern "C" void* TestCreateDevice() {
    HWND hWindow = CreateWindowA(
        "STATIC",
        "d3dx9 compare",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        64,
        64,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!hWindow || !pD3D) {
        return nullptr;
    }
    D3DPRESENT_PARAMETERS kParameters{};
    kParameters.Windowed = TRUE;
    kParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    kParameters.hDeviceWindow = hWindow;
    // Under headless gamescope the first creation sometimes fails: a few tries, a second apart.
    IDirect3DDevice9* pDevice = nullptr;
    for (int iTry = 0; iTry < 5 && !pDevice; ++iTry) {
        if (iTry) {
            Sleep(1000);
        }
        pD3D->CreateDevice(0, D3DDEVTYPE_HAL, hWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &kParameters, &pDevice);
    }
    return pDevice;
}

extern "C" void* TestLoadLibrary(const char* apPath) {
    return LoadLibraryA(apPath);
}

extern "C" void* TestGetProc(void* apModule, const char* apName) {
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(apModule), apName));
}
