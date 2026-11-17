#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"
#include "Reconstruction.hpp"

class NiDX9Renderer;
class NiShaderLibraryDesc;

// A shader library backed by a D3D shader DLL: the renderer it was last set to, the loaded DLL and the library's
// description. The PDB's class (gamebryo2.2\corelibs\nixenonrenderer\nid3dshaderlibrary.h); the exe's
// NiD3DShaderFactory::SetRenderer (0xE91E60) walks the libraries and calls their SetRenderer through the vtable.
// IDA types m_pkD3DRenderer as NiD3DRenderer*, a class src does not model; the PC exe's renderer is NiDX9Renderer.
// The PDB's constructors, destructor, operator= and five protected statics have no out-of-line copy in the exe and
// no address to overlay, so they are not declared.
class NiD3DShaderLibrary : public NiShaderLibrary {
public:
	// The description slots between ReleaseShader and SetRenderer (PDB: vftable offsets 20 and 24). The exe has no
	// out-of-line copy of either; the class is never instantiated in this build, only its SetRenderer slot is called.
	virtual NiShaderLibraryDesc* GetShaderLibraryDesc();
	virtual void SetShaderLibraryDesc(NiShaderLibraryDesc* apDesc);
	// Slot 7: the exe's factory call reads [vtable+0x1C] (0xE91E84 `mov eax,[edx+0x1c]; push edi; call eax`).
	virtual void SetRenderer(NiDX9Renderer* apRenderer);

	NiDX9Renderer* GetRenderer();

	NiDX9Renderer* m_pkD3DRenderer;
	// The SDK's HMODULE: a pointer to an opaque struct.
	HINSTANCE__* m_hLibrary;
	NiPointer<NiShaderLibraryDesc> m_spShaderLibraryDesc;
};
