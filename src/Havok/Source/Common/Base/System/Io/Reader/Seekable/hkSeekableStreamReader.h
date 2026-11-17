#pragma once

#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"

// A stream reader that can seek: marks are supported and are taken and returned to with tell and seek. As in
// hkStreamReader, hkResult is an int (0 is success).
class hkSeekableStreamReader : public hkStreamReader {
public:
	// Says yes.
	hkBool* markSupported(hkBool* arResult) const override;
	// Takes the current position as the mark; fails (returns 1) when tell gives -1. The limit is not used.
	hkResult setMark(int32_t markLimit) override;
	// Seeks to the mark, from the start.
	hkResult rewindToMark() override;
	// Says yes.
	hkBool* seekTellSupported(hkBool* arResult) const override;

	int32_t m_markPos;
};
