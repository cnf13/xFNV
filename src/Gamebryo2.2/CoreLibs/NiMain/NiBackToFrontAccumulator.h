#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Reconstruction.hpp"

class NiCloningProcess;
class NiGeometry;

// Accumulator that draws its geometry sorted from the farthest to the nearest.
class NiBackToFrontAccumulator : public NiAccumulator {
public:
	NiBackToFrontAccumulator();
	~NiBackToFrontAccumulator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	// LoadBinary, LinkObject, RegisterStreamables and SaveBinary fold into jumps to the NiAccumulator bodies
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void FinishAccumulating() override;
	void RegisterObjectArray(NiVisibleArray& kArray) override;
	// Fills m_ppkItems and m_pfDepths from the item list (depth along the camera's view direction) and sorts them.
	virtual void Sort();

	void CopyMembers(NiBackToFrontAccumulator* pkDest, NiCloningProcess& kCloning);
	// Quicksort of m_ppkItems and m_pfDepths between the inclusive indices, ascending by depth.
	void SortObjectsByDepth(int32_t l, int32_t r);
	// Median of the depths at the two ends and the middle.
	float ChoosePivot(int32_t l, int32_t r) const;

	NiTPointerList<NiGeometry*> m_kItems;
	NiTPointerList<NiGeometry*>* m_kItemsToSortAndRender;
	int32_t m_iNumItems;
	int32_t m_iMaxItems;
	NiGeometry** m_ppkItems;
	float* m_pfDepths;
	int32_t m_iCurrItem;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBackToFrontAccumulator_vftable{};
