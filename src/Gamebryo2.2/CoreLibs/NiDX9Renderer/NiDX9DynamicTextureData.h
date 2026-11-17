#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Reconstruction.hpp"

// The DX9 renderer's data of a dynamic (CPU-written) texture. Not in the Xbox pdb.
// Only the members called so far; the rest is not reconstructed yet.
class NiDX9DynamicTextureData : public NiDX9TextureData {
public:
	NiDX9DynamicTextureData(NiTexture* pkTexture, NiDX9Renderer* pkRenderer);

	NiDX9DynamicTextureData* GetAsDynamicTexture() const override { return const_cast<NiDX9DynamicTextureData*>(this); }

	// slots 45-47; the lockable system-memory copy the CPU writes when the device
	// cannot lock the texture itself
	virtual IDirect3DBaseTexture9* GetTexture() const { return m_pkD3DLockableSysMemTexture; }
	virtual bool IsTextureLocked() const;
	virtual void SetTextureLocked(bool abLocked) { m_bTextureLocked = abLocked; }
	~NiDX9DynamicTextureData() override;

	// Creates the D3D surface(s) of the texture and returns its format; null on failure.
	NiPixelFormat* CreateSurf(NiTexture* pkTexture, bool abDynamicTexturesCapable);

	// Creates the renderer data of apTexture (an NiDynamicTexture); null for a tiled texture or when the surface
	// cannot be created. IDA name; not in the PDB.
	static NiDX9DynamicTextureData* Create(NiTexture* apTexture);

	IDirect3DBaseTexture9* m_pkD3DLockableSysMemTexture;
	bool m_bTextureLocked;
};

inline constexpr fnvd::Vtable NiDX9DynamicTextureData_vftable{};
