#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cfloat>

class NiCloningProcess;

// Blends the values of several weighted, prioritised interpolators into one; the leaves blend one value type each.
class NiBlendInterpolator : public NiInterpolator {
public:
	// Bits of m_uFlags.
	enum {
		// a controller manager drives the interpolator: the stream holds no per-interpolator state
		MANAGER_CONTROLLED_MASK = 0x1,
		ONLY_USE_HIGHEST_WEIGHT_MASK = 0x2,
		COMPUTE_NORMALIZED_WEIGHTS_MASK = 0x4,
	};

	// One blended interpolator and its weight, priority and ease state.
	struct InterpArrayItem {
		// only the pointer is cleared: new[] leaves the other fields as the allocator returns them
		InterpArrayItem() {}

		// Reads the link, weight, normalized weight, priority (a 4-byte int before 10.1.0.110) and ease spinner.
		void LoadBinary(NiStream& kStream);
		void SaveBinary(NiStream& kStream);
		bool IsEqual(InterpArrayItem& kDest);
		InterpArrayItem& operator=(const InterpArrayItem& arItem);

		NiPointer<NiInterpolator> m_spInterpolator;
		float m_fWeight;
		float m_fNormalizedWeight;
		char m_cPriority;
		float m_fEaseSpinner;
		float m_fUpdateTime;
	};

	NiBlendInterpolator();
	// The exe passes the caller's whole dword on; bool would be zero-extended (the derived constructors push their
	// own argument slots as is; 0xA370B0 reads only the low bytes: `cmp byte [esp+0xC], 0`, `mov al, [esp+0x14]`).
	NiBlendInterpolator(int32_t abManagerControlled, float afWeightThreshold, int32_t aucArraySize);
	~NiBlendInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	// LinkObject folds into a jump to the NiKeyBasedInterpolator body
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void Collapse() override;
	void GuaranteeTimeRange(float fStartTime, float fEndTime) override;
	// Puts the interpolator in the first free item and returns its index; INVALID_INDEX when the array cannot grow.
	virtual uint8_t AddInterpInfo(NiInterpolator* pkInterpolator, float fWeight, char cPriority, float fEaseSpinner);
	virtual void RemoveInterpInfo(uint8_t ucIndex);
	virtual bool InterpolatorIsCorrectType(NiInterpolator* apInterpolator) const = 0;
	virtual bool ReallocateArrays();

	NiInterpolator* GetInterpolator(uint8_t ucIndex) const;
	bool GetManagerControlled() const { return (m_uFlags & MANAGER_CONTROLLED_MASK) != 0; }
	bool GetOnlyUseHighestWeight() const { return (m_uFlags & ONLY_USE_HIGHEST_WEIGHT_MASK) != 0; }
	// Fills in the normalized weights of the items when a weight, ease spinner or item changed since the last call.
	void ComputeNormalizedWeights();
	// The index of the item with the highest normalized weight; INVALID_INDEX when none is above 0.
	uint8_t GetHighestWeightedIndex();
	// The update time of the item at ucIndex, or the single time while the single interpolator stands for it.
	float GetTime(uint8_t ucIndex) const;

	void CopyMembers(NiBlendInterpolator* pkDest, NiCloningProcess& kCloning);
	void ComputeNormalizedWeightsFor2();

	// The sums are recomputed by the next ComputeNormalizedWeights.
	void ClearWeightSums() {
		m_fHighSumOfWeights = -FLT_MAX;
		m_fNextHighSumOfWeights = -FLT_MAX;
		m_fHighEaseSpinner = -FLT_MAX;
	}

	bool GetComputeNormalizedWeights() const { return (m_uFlags & COMPUTE_NORMALIZED_WEIGHTS_MASK) != 0; }

	void SetComputeNormalizedWeights(bool bComputeNormalizedWeights) {
		if (bComputeNormalizedWeights)
			m_uFlags |= COMPUTE_NORMALIZED_WEIGHTS_MASK;
		else
			m_uFlags &= ~COMPUTE_NORMALIZED_WEIGHTS_MASK;
	}

	// A manager-controlled interpolator updates the single interpolator at its own time; false when it has none.
	bool GetSingleUpdateTime(float& fTime) const {
		if (GetManagerControlled())
			fTime = m_fSingleTime;
		return fTime != INVALID_TIME;
	}

	// The same for one item.
	bool GetUpdateTimeForItem(float& fTime, const InterpArrayItem& kItem) const {
		if (GetManagerControlled())
			fTime = kItem.m_fUpdateTime;
		return fTime != INVALID_TIME;
	}

	int32_t GetHighestPriority();

	uint8_t m_uFlags;
	uint8_t m_ucArraySize;
	uint8_t m_ucInterpCount;
	uint8_t m_ucSingleIdx;
	char m_cHighPriority;
	char m_cNextHighPriority;
	InterpArrayItem* m_pkInterpArray;
	NiInterpolator* m_pkSingleInterpolator;
	float m_fWeightThreshold;
	float m_fSingleTime;
	float m_fHighSumOfWeights;
	float m_fNextHighSumOfWeights;
	float m_fHighEaseSpinner;

	// No interpolator: AddInterpInfo's failure, an empty single index.
	static constexpr uint8_t INVALID_INDEX = 0xFF;
	// An item or the single interpolator has no time to update at.
	static constexpr float INVALID_TIME = -FLT_MAX;
	// Items the interpolator array grows by when it is full; 0 stops it from growing.
	static inline uint8_t ms_ucArrayGrowBy = 2;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBlendInterpolator_vftable{};
