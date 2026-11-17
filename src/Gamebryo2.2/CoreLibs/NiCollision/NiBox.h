#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NiStream;

// an oriented box: centre, three unit axes and the half extent along each
class NiBox {
public:
	bool operator==(const NiBox& kBox) const;
	void LoadBinary(NiStream& stream);
	void SaveBinary(NiStream& stream);
	void CreateFromData(int32_t aiQuantity, const NiPoint3* pkVertex);

	NiPoint3 m_kCenter;
	NiPoint3 m_akAxis[3];
	float m_afExtent[3];
};
