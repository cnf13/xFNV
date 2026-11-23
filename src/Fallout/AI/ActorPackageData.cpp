#include "Fallout/AI/ActorPackageData.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageDataUseWeapon.h"
#include "Fallout/AI/EscortPackage.h"
#include "Fallout/AI/GuardPackage.h"
#include "Fallout/AI/PatrolPackage.h"
#include "Fallout/AI/SandBoxPackage.h"

ActorPackageData* ActorPackageData::CreateActorPackageData(PACKAGE_TYPE aPackageType) {
    ActorPackageData* pData = nullptr;
    switch (aPackageType) {
        case 2: {
            pData = new EscortActorPackageData;
            break;
        }
        case 12: {
            pData = new SandBoxActorPackageData;
            break;
        }
        case 13: {
            pData = new PatrolActorPackageData;
            break;
        }
        case 14: {
            pData = new GuardActorPackageData;
            break;
        }
        case 16: {
            pData = new UseWeaponActorPackageData;
            break;
        }
        default: {
            break;
        }
    }
    return pData;
}
