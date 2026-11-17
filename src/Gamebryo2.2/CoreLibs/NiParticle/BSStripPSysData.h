#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"

class NiCloningProcess;
class NiStream;

// particle data of a strip system: per strip, a ring of points that become the ribbon's vertices.
class BSStripPSysData : public NiPSysData {
public:
	// one point of a strip.
	struct PointData {
		NiPoint3 kPoint;
		NiPoint3 kVertex[4];
		NiPoint3 kDir;
		NiPoint3 kNormal[2];
		NiPoint3 kVelocity;
		NiColorA kColor;
		float fSize;
		float fRadii;
	};

	// one strip: a ring buffer of points.
	struct StripData {
		// aiIndex wrapped into [0, usMaxPointCount).
		int32_t NormalizeIndex(int32_t aiIndex) const {
			int32_t iIndex = aiIndex % usMaxPointCount;
			if (iIndex < 0) {
				iIndex += usMaxPointCount;
			}
			return iIndex;
		}

		int32_t PrevIndex(int32_t aiIndex) const { return NormalizeIndex(aiIndex - 1); }
		void IncFirstIndex() { iFirstIndex = NormalizeIndex(iFirstIndex + 1); }
		void IncLastIndex() { iLastIndex = NormalizeIndex(iLastIndex + 1); }

		PointData* pPointList;
		uint16_t usPointCount;
		uint16_t usHighestPointCount;
		int32_t iStartIndex;
		int32_t iStopIndex;
		// Copied from the owner's 16-bit count, but stored and divided by as a signed 32-bit value.
		int32_t usMaxPointCount;
		int32_t iFirstIndex;
		int32_t iLastIndex;
		float fLastTime;
	};

	BSStripPSysData();
	~BSStripPSysData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void RemoveParticle(int32_t ausParticle) override;

	// Resets strip ausParticle to no points.
	void InitializePoint(uint16_t ausParticle);
	float GetPointLastUpdateTime(uint16_t ausParticle) const;
	uint16_t GetPointCount(uint16_t ausParticle) const;
	// usMaxPointCount.
	uint16_t GetMaxPointCount() const { return usMaxPointCount; }
	const NiPoint3* GetPointPoint(uint16_t ausParticle, uint16_t ausPoint) const;
	float GetPointSize(uint16_t ausParticle, uint16_t ausPoint) const;
	float GetPointRadii(uint16_t ausParticle, uint16_t ausPoint) const;
	// &kVertex[0] of point ausPoint of strip ausParticle.
	const NiPoint3* GetPointVerts(uint16_t ausParticle, uint16_t ausPoint) const;
	// &kNormal[0] of point ausPoint of strip ausParticle.
	const NiPoint3* GetPointNorms(uint16_t ausParticle, uint16_t ausPoint) const;
	// The u step between two points of strip ausParticle: 1 / (usHighestPointCount - 1), rounded to float.
	float GetTexCoordDelta(uint16_t ausParticle) const;
	// The color of the point, or NiColorA::WHITE when the data has no colors.
	const NiColorA* GetPointColor(uint16_t ausParticle, uint16_t ausPoint) const;
	// Appends the particle's current position to its strip and rebuilds the ribbon vertices of the last two points;
	// a particle past its life span is skipped. The time is not used.
	void UpdatePoints(uint16_t ausParticle, float afTime);
	// NiPSysData's copy, then the cap sizes, the point limit and a copy of every strip's
	// points into pkDest when cloning.
	void CopyMembers(BSStripPSysData* pkDest, NiCloningProcess& kCloning);

	// The class' stream loader entry.
	static NiObject* CreateObject();

	StripData* pStripList;
	uint16_t usMaxPointCount;
	float fStartCapSize;
	float fEndCapSize;
	bool bDoZPrePass;

	static NiRTTI ms_RTTI;
};
