#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"

class NiDX9Renderer;
class NiShaderLibraryDesc;

// A shader library backed by a D3D shader DLL: the renderer it was last set to, the loaded DLL and the library's
// description. NiD3DShaderFactory::SetRenderer walks the libraries and calls their SetRenderer through the vtable. The
// constructors, destructor, operator= and five protected statics are not declared.
class NiD3DShaderLibrary : public NiShaderLibrary {
public:
	// The description virtuals sit between ReleaseShader and SetRenderer. The class is never instantiated; only
	// SetRenderer is called.
	virtual NiShaderLibraryDesc* GetShaderLibraryDesc();
	virtual void SetShaderLibraryDesc(NiShaderLibraryDesc* apDesc);
	virtual void SetRenderer(NiDX9Renderer* apRenderer);

	NiDX9Renderer* GetRenderer();

	NiDX9Renderer* m_pkD3DRenderer;
	// The SDK's HMODULE: a pointer to an opaque struct.
	HINSTANCE__* m_hLibrary;
	NiPointer<NiShaderLibraryDesc> m_spShaderLibraryDesc;
};
