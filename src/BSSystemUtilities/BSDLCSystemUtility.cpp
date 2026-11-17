#include "BSSystemUtilities/BSDLCSystemUtility.h"

#include "BSCore/BSSimpleList.h"
#include "BSSystem/BSContentPackage.h"
#include "BSSystemUtilities/Xenon/BSDLCSystemUtility_Xenon.h"
#include "Reconstruction.hpp"

void BSDLCSystemUtility::Idle() {
	;
}

uint32_t BSDLCSystemUtility::MountDLC(uint32_t anPlayerIndex, BSDLCSystemUtility::MountCallback cb) {
	nullsub(this, nullptr);
	BSSimpleList<BSContentPackage*>* p_m_validDLC = &m_validDLC;
	uint32_t v11 = 0;
	int32_t v8 = 0;
	for (BSContentPackage*& rpItem : m_validDLC) {
		if (rpItem) {
			++v8;
		}
	}
	if (cb) {
		while (p_m_validDLC) {
			if (!p_m_validDLC->GetNext() && !p_m_validDLC->m_item) {
				break;
			}
			cb(v11++, v8 - 1, 1, p_m_validDLC->m_item);
			p_m_validDLC = p_m_validDLC->GetNext();
		}
		cb(-1, -1, 0, 0);
		return 0;
	} else {
		uint32_t count = 0;
		for (BSContentPackage*& rpItem : m_validDLC) {
			if (rpItem) {
				++count;
			}
		}
		return count;
	}
}
