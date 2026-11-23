#include "BSSystemUtilities/BSDLCSystemUtility.h"

#include "BSCore/BSSimpleList.h"
#include "BSSystem/BSContentPackage.h"
#include "BSSystemUtilities/Xenon/BSDLCSystemUtility_Xenon.h"

void BSDLCSystemUtility::Idle() {
    ;
}

uint32_t BSDLCSystemUtility::MountDLC(uint32_t anPlayerIndex, BSDLCSystemUtility::MountCallback cb) {
    nullsub(this, nullptr);
    BSSimpleList<BSContentPackage*>* plist = &m_validDLC;
    uint32_t uiIndex = 0;
    int32_t iPackageCount = 0;
    for (BSContentPackage*& rpItem : m_validDLC) {
        if (rpItem) {
            ++iPackageCount;
        }
    }
    if (cb) {
        while (plist) {
            if (!plist->GetNext() && !plist->m_item) {
                break;
            }
            cb(uiIndex++, iPackageCount - 1, true, plist->m_item);
            plist = plist->GetNext();
        }
        cb(-1, -1, false, nullptr);
        return 0;
    }

    uint32_t count = 0;
    for (BSContentPackage*& rpItem : m_validDLC) {
        if (rpItem) {
            ++count;
        }
    }
    return count;
}
