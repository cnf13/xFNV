#pragma once

#include "BSCore/BSStringT.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiTexture;

// The interface of a set of texture file names indexed by texture slot.
class BSTextureSet : public NiObject {
public:
	BSTextureSet();
	~BSTextureSet() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	virtual const char* GetTextureFilename(BSShaderProperty::TextureTypeEnum aiType) = 0;
	virtual void GetTexture(BSShaderProperty::TextureTypeEnum aiType, NiPointer<NiTexture>& aspTexture) = 0;
	virtual void SetTextureFilename(BSShaderProperty::TextureTypeEnum aiType, const char* apFilename) = 0;

	static NiRTTI ms_RTTI;
};

// The texture set of a shader property: one file name per texture slot.
class BSShaderTextureSet : public BSTextureSet {
public:
	BSShaderTextureSet();
	~BSShaderTextureSet() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	const char* GetTextureFilename(BSShaderProperty::TextureTypeEnum aiType) override;
	void GetTexture(BSShaderProperty::TextureTypeEnum aiType, NiPointer<NiTexture>& aspTexture) override;
	void SetTextureFilename(BSShaderProperty::TextureTypeEnum aiType, const char* apFilename) override;

	// The factory the stream registration holds: a texture set in a 0x38-byte engine block.
	static NiObject* CreateObject();

	BSStringT<char> pTextureNames[6];

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSShaderTextureSet);

inline constexpr fnvd::Vtable BSTextureSet_vftable{};
inline constexpr fnvd::Vtable BSShaderTextureSet_vftable{};
