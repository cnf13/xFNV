#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Reconstruction.hpp"

// Triangle-strip geometry data: one length per strip and one index list holding all the strips back to back.
class NiTriStripsData : public NiTriBasedGeomData {
public:
	NiTriStripsData();
	// Takes ownership of the given arrays.
	NiTriStripsData(int32_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor, NiPoint2* pkTexture,
		int32_t usNumTextureSets, int32_t eNBTMethod, int32_t usTriangles, int32_t usStrips, uint16_t* pusStripLengths,
		uint16_t* pusStripLists);
	~NiTriStripsData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	uint32_t GetBlockAllocationSize() const override;
	NiTriStripsData* IsTriStripsData() override { return this; }
	void CalculateNormals() override;
	void GetTriangleIndices(uint16_t i, uint16_t& i0, uint16_t& i1, uint16_t& i2) const override;
	void GetStripData(uint16_t& usStrips, const uint16_t*& pusStripLengths, const uint16_t*& pusTriList,
		uint32_t& uiStripLengthSum) const override;

	uint16_t GetStripCount() const { return m_usStrips; }
	uint16_t* GetStripLengths() const { return m_pusStripLengths; }
	uint16_t* GetStripLists() const { return m_pusStripLists; }
	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy of GetStripLists (thiscall, ret 0,
	// 0xA75F10), for the callers that call it instead of inlining it. Same function as
	// BSShaderProperty::GetSIBlockRenderPasses (0xA75F10), registered there.
	uint16_t* GetStripListsCall() const { return m_pusStripLists; }
	// Swaps in new strips. thiscall, ret 0xC, 0xA760C0.
	void Replace(uint16_t usStrips, uint16_t* pusStripLengths, uint16_t* pusStripLists);

	static NiObject* CreateObject();

	uint16_t m_usStrips;
	uint16_t* m_pusStripLengths;
	uint16_t* m_pusStripLists;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiTriStripsData);

inline constexpr fnvd::Vtable NiTriStripsData_vftable{};
