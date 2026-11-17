#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

// Base of the libraries that supply shaders by name. The exe has no vtable of its own for it (no kVtbl_ entry): its
// description slots are pure in the SDK and filled by the derived libraries.
class NiRenderer;
class NiShader;
class NiShaderLibraryDesc;

class NiShaderLibrary : public NiRefObject {
public:
	// The slots the exe's callers reach: GetShader(renderer, name, implementation) at +0x8 and
	// ReleaseShader(name, implementation) at +0x10. MSVC (and clang's MS ABI) puts the ReleaseShader overloads in
	// reverse declaration order, so the name form is declared first to land in the later slot.
	virtual NiShader* GetShader(NiRenderer*, const char*, uint32_t) { return nullptr; }
	virtual bool ReleaseShader(const char*, uint32_t) { return false; }
	virtual bool ReleaseShader(NiShader*) { return false; }
	// Slot 5 (+0x14): NiSingleShaderMaterialLibrary::GetFirstMaterialDesc calls [vtable+0x14] on each
	// library and uses the result as an NiShaderLibraryDesc; slot 6 is the SDK's setter.
	virtual NiShaderLibraryDesc* GetShaderLibraryDesc() = 0;
	virtual void SetShaderLibraryDesc(NiShaderLibraryDesc* apDesc) = 0;

	char* m_pcName;
};
