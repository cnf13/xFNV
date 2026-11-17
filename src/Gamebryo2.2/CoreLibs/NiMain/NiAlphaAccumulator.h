#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiBackToFrontAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// Back-to-front accumulator that draws opaque geometry first and honours the no-sort hint.
class NiAlphaAccumulator : public NiBackToFrontAccumulator {
public:
	NiAlphaAccumulator();
	~NiAlphaAccumulator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void RegisterObjectArray(NiVisibleArray& kArray) override;
	bool RegisterObject(NiGeometry* apGeometry) override;
	void Sort() override;

	// Sorts as interface geometry when abInterface (defined in InterfaceManager.cpp).
	void SetInterfaceSort(bool abInterface);

	static NiObject* CreateObject();

	bool m_bObserveNoSortHint;
	bool m_bSortByClosestPoint;
	bool m_bInterfaceSort;
	uint8_t padding33;

	static NiRTTI ms_RTTI;
};
