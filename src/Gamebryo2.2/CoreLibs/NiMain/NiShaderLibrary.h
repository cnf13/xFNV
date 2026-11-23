#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

// Base of the libraries that supply shaders by name. Its description functions are pure and filled by the derived
// libraries.
class NiRenderer;
class NiShader;
class NiShaderLibraryDesc;

class NiShaderLibrary : public NiRefObject {
public:
    // The ReleaseShader overloads are placed in the vtable in reverse declaration order, so the name form is declared
    // first to land in the later slot.
    virtual NiShader* GetShader(NiRenderer*, const char*, uint32_t) {
        return nullptr;
    }
    virtual bool ReleaseShader(const char*, uint32_t) {
        return false;
    }
    virtual bool ReleaseShader(NiShader*) {
        return false;
    }
    virtual NiShaderLibraryDesc* GetShaderLibraryDesc() = 0;
    virtual void SetShaderLibraryDesc(NiShaderLibraryDesc* apDesc) = 0;

    char* m_pcName;
};
