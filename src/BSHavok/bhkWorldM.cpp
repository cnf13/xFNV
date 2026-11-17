#include "BSHavok/bhkWorldM.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"

#include <cmath>

NiRTTI bhkWorldM::ms_RTTI("bhkWorldM", &bhkWorld::ms_RTTI);

// The base's constructor, then the three extents; the broad phase's extent is the maximum corner of arInfo's
// broad-phase box.
bhkWorldM::bhkWorldM(const bhkWorldCinfo& arInfo, hkVector4* arWorldSize, hkVector4* arBorderSize)
	: bhkWorld(arInfo), WorldTotalSize(*arWorldSize), BorderSize(*arBorderSize),
	  BroadPhaseSize(arInfo.m_broadPhaseWorldAabb.m_max) {}

const NiRTTI* bhkWorldM::GetRTTI() const {
	return &bhkWorldM::ms_RTTI;
}

// The exe's cleanup-only C++ EH frame is left out. It inlines AutoMemContext and Add for every string; SetSize and
// SetAt stay out of line.
void bhkWorldM::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkWorld::GetViewerStrings(apStrings);
	char* pcString = NiGetViewerString(ms_RTTI.GetName());
	apStrings->Add(pcString);
	NiPoint3 pnt;
	HK2NI(pnt, WorldTotalSize);
	pcString = pnt.GetViewerString("WorldTotalSize");
	apStrings->Add(pcString);
	HK2NI(pnt, BorderSize);
	pcString = pnt.GetViewerString("BorderSize");
	apStrings->Add(pcString);
}

// KEEP, unregistered: the exe's only caller, bhkWorldM::CheckForMove 0xC9A1F0, keeps `this` in ecx across this call
// and passes it on to bhkWorld::SethkOrigin (call at 0xC9A29B, no reload of ecx). The exe body never writes ecx; a src
// thiscall body may clobber it, and did: smoke smk16 crashed in SethkOrigin with this = 1. C++ cannot promise ecx
// survives a call.
// Per axis: the sign of the coordinate (1 when it is not negative, -1 for a negative coordinate or NaN), the broad
// phase's overhang past the world on that side, and a pull back when it points the same way. The exe adds the overhang
// at x87 precision and stores it as a float.
void bhkWorldM::CalcNewCenter(hkVector4& arCenter) {
	float fValue = arCenter.m_quad[0];
	float fBroadPhase = BroadPhaseSize.m_quad[0];
	float fTotal = WorldTotalSize.m_quad[0];
	int32_t iSign = -1;
	if (fValue >= 0.0f)
		iSign = 1;
	float fSign = static_cast<float>(iSign);
	float fShift = static_cast<float>(fValue + (static_cast<double>(fBroadPhase) - fTotal) * fSign);
	if (fShift * fSign > 0.0f)
		arCenter.m_quad[0] -= fShift;

	fValue = arCenter.m_quad[1];
	fBroadPhase = BroadPhaseSize.m_quad[1];
	fTotal = WorldTotalSize.m_quad[1];
	iSign = -1;
	if (fValue >= 0.0f)
		iSign = 1;
	fSign = static_cast<float>(iSign);
	fShift = static_cast<float>(fValue + (static_cast<double>(fBroadPhase) - fTotal) * fSign);
	if (fShift * fSign > 0.0f)
		arCenter.m_quad[1] -= fShift;

	fValue = arCenter.m_quad[2];
	fBroadPhase = BroadPhaseSize.m_quad[2];
	fTotal = WorldTotalSize.m_quad[2];
	iSign = -1;
	if (fValue >= 0.0f)
		iSign = 1;
	fSign = static_cast<float>(iSign);
	fShift = static_cast<float>(fValue + (static_cast<double>(fBroadPhase) - fTotal) * fSign);
	if (fShift * fSign > 0.0f)
		arCenter.m_quad[2] -= fShift;
}

// Converts arPos to Havok units in a 16-aligned local and returns whether CheckForMove moved the world.
bool bhkWorldM::CheckForMove(const NiPoint3& arPos) {
	hkVector4 pos;
	return CheckForMove(NI2HK(pos, arPos));
}

// Each axis of arPos's offset from the origin is tested against the border size until one reaches it (a NaN never
// does). Then arPos is pulled back by CalcNewCenter and the origin moves there, unless the new centre is within 0.5
// of the old origin on x, y and z. The exe keeps the old origin in xmm0 across CalcNewCenter's call.
bool bhkWorldM::CheckForMove(const hkVector4& arPos) {
	hkVector4 oldPos;
	oldPos = Origin;
	hkVector4 delta;
	delta.setSub4(arPos, oldPos);
	bool bnew = false;
	for (uint32_t ui = 0; ui < 3 && !bnew; ++ui) {
		float fdist = static_cast<float>(std::fabs(delta(static_cast<int32_t>(ui))));
		bnew = BorderSize(static_cast<int32_t>(ui)) <= fdist;
	}
	if (!bnew)
		return bnew;
	hkVector4 newPos;
	newPos = arPos;
	CalcNewCenter(newPos);
	bnew = !newPos.equals3(oldPos, 0.5f);
	if (bnew)
		SethkOrigin(newPos);
	return bnew;
}
