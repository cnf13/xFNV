#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Reconstruction.hpp"

class NiStream;

// A named marker in an animation sequence (start, end, sound cues, ...).
class NiTextKey : public NiMemObject {
public:
	// The text starts at the empty handle; the time is left as it is.
	NiTextKey();
	// Runs Destruct. It stays user-declared: the exe's key arrays are new[]'d with a count cookie, which
	// NiTNewInterface<NiTextKey>::Allocate writes only for a type that is not trivially destructible.
	~NiTextKey();

	float GetTime() const { return m_fTime; }
	const NiFixedString& GetText() const { return m_kText; }

	void LoadBinary(NiStream& stream);
	void SaveBinary(NiStream& stream);
	void RegisterStreamables(NiStream& stream);
	void SetText(const NiFixedString& kText);

	float m_fTime;
	NiFixedString m_kText;
};
