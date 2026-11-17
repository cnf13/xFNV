#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"

// Additional geometry data packed into one interleaved vertex stream for a shader declaration.
class BSPackedAdditionalGeometryData : public NiAdditionalGeometryData {
public:
	enum SD_FLAGS : uint32_t {
		SD_SKINNED = 0x1,
		SD_COLOR = 0x2,
		SD_COMP_VERTICES = 0x4,
		SD_COMP_NORMALS = 0x8,
		SD_COMP_TEXCOORDS = 0x10,
		SD_COMP_COLORS = 0x20,
		SD_COMP_TANGENTS = 0x40,
		SD_COMP_BLENDW = 0x80,
		SD_MAX_PERMUTATIONS = 0x100,
	};

	// A data block whose buffer comes from Bethesda's heap.
	class NiBSPackedAGDDataBlock : public NiAGDDataBlock {
	public:
		NiBSPackedAGDDataBlock();
		~NiBSPackedAGDDataBlock() override;

		uint8_t* AllocateBuffer(uint32_t uiBufferSize) override;
		void ProtectBuffer(void* pvBuffer, uint32_t auiBufferSize) override;
		void UnprotectBuffer(void* pvBuffer, uint32_t auiBufferSize) override;
		void FreeBuffer(void* pvBuffer) override;
	};

	BSPackedAdditionalGeometryData();
	~BSPackedAdditionalGeometryData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool GetVBReady() override { return true; }

protected:
	NiAdditionalGeometryData::NiAGDDataBlock* CreateAGDDataBlock() override;

public:
	// Allocates and constructs an object with both packed fields zero.
	static NiObject* CreateObject();

	uint32_t m_uiShaderDeclarationIndex;
	uint32_t m_uiVertexStride;

	static NiRTTI ms_RTTI;
};
