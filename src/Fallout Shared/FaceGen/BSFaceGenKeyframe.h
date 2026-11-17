#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

enum BS_FACEGEN_MORPH_TYPE : int32_t;

class BSFaceGenKeyframe {
public:
	BSFaceGenKeyframe() = default;
	BSFaceGenKeyframe(BS_FACEGEN_MORPH_TYPE aeType);
	virtual ~BSFaceGenKeyframe();

	virtual BS_FACEGEN_MORPH_TYPE GetType() { return eType; }
	virtual bool SetType(BS_FACEGEN_MORPH_TYPE);
	virtual float GetTime();
	virtual bool SetTime(float);
	virtual bool AddToList(NiTPointerList<BSFaceGenKeyframe*>*) = 0;
	virtual bool MergeMorphDataFrom(BSFaceGenKeyframe*, float, bool, bool) = 0;
	virtual bool MergeMorphDataFrom(BSFaceGenKeyframe*, BSFaceGenKeyframe*, float) = 0;
	virtual bool ClearData(bool) = 0;
	virtual BSFaceGenKeyframe* CreateCopy() = 0;
	virtual void CopyFrom(BSFaceGenKeyframe*) = 0;
	virtual bool Compare(BSFaceGenKeyframe*) = 0;
	virtual float GetStrength() = 0;
	virtual bool IsEmpty() = 0;
	virtual void SetToZero() = 0;
	virtual bool UpdateToTarget(float, BSFaceGenKeyframe*) = 0;
	virtual bool IsTargetAvailable() = 0;
	virtual bool IsMultiple();
	virtual bool IsExclusive();

	// The deleting destructors free through ourFree (BSCore/MemoryManager.h).
	static void operator delete(void* apMemory);

	BS_FACEGEN_MORPH_TYPE eType;
	float fTime;
};

class BSFaceGenKeyframeMultiple : public BSFaceGenKeyframe {
public:
	BSFaceGenKeyframeMultiple() : pQuantities(nullptr), iNumQuantities(0) {}
	// The copy constructor.
	BSFaceGenKeyframeMultiple(BSFaceGenKeyframeMultiple* apSource);
	BSFaceGenKeyframeMultiple(BS_FACEGEN_MORPH_TYPE aeType, uint32_t aiNumQuantities);
	~BSFaceGenKeyframeMultiple() override;

	bool AddToList(NiTPointerList<BSFaceGenKeyframe*>*) override;
	bool MergeMorphDataFrom(BSFaceGenKeyframe*, float, bool, bool) override;
	bool MergeMorphDataFrom(BSFaceGenKeyframe*, BSFaceGenKeyframe*, float) override;
	bool ClearData(bool) override;
	BSFaceGenKeyframe* CreateCopy() override;
	void CopyFrom(BSFaceGenKeyframe*) override;
	bool Compare(BSFaceGenKeyframe*) override;
	float GetStrength() override;
	bool IsEmpty() override;
	void SetToZero() override;
	bool UpdateToTarget(float, BSFaceGenKeyframe*) override;
	bool IsTargetAvailable() override;
	bool IsMultiple() override;
	virtual bool IsValidQuantity(uint32_t);
	virtual void ResetQuantityArray(uint32_t, bool);

	float GetQuantity(uint32_t aiWhichMorph) const;
	void SetQuantityNoTest(uint32_t aiWhichMorph, float afQuantity);
	bool SetQuantity(uint32_t aiWhichMorph, float afQuantity);
	// Returns iNumQuantities.
	uint32_t GetNumQuantities();
	void operator=(BSFaceGenKeyframeMultiple* a2);

	float* pQuantities;
	uint32_t iNumQuantities;
};
