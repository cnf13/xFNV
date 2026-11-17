// Prints the Direct3D 9 description of adapter 0, as the game sees it. WinMain compares it with Display:sD3DDevice
// (CreateDisplayDriver 0x4DA570) and, when they differ, starts FalloutNVLauncher.exe and exits; harness/run.sh
// writes this string into the ini so the game starts directly.

#include <cstdio>

#include <d3d9.h>

int main()
{
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        std::fprintf(stderr, "adapter: Direct3DCreate9 failed\n");
        return 1;
    }
    D3DADAPTER_IDENTIFIER9 identifier{};
    const HRESULT result = d3d->GetAdapterIdentifier(0, 0, &identifier);
    d3d->Release();
    if (FAILED(result)) {
        std::fprintf(stderr, "adapter: GetAdapterIdentifier failed (0x%08lX)\n", static_cast<unsigned long>(result));
        return 1;
    }
    std::printf("%s\n", identifier.Description);
    return 0;
}
