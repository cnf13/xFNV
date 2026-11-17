#include "Fallout/AI/ActorPackageData.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageDataUseWeapon.h"
#include "Fallout/AI/EscortPackage.h"
#include "Fallout/AI/GuardPackage.h"
#include "Fallout/AI/PatrolPackage.h"
#include "Fallout/AI/SandBoxPackage.h"

ActorPackageData* ActorPackageData::CreateActorPackageData(PACKAGE_TYPE aPackageType) {
	UseWeaponActorPackageData* v2;
	GuardActorPackageData* v3;
	PatrolActorPackageData* v4;
	SandBoxActorPackageData* v5;
	EscortActorPackageData* v6;
	void* v7;
	void* v8;
	void* apMemory;
	void* v10;
	void* v11;
	ActorPackageData* v12 = nullptr;
	switch ((int32_t)aPackageType) {
	case 2: {
		v11 = ourMalloc(sizeof(EscortActorPackageData));
		if (v11) {
			v6 = ::new (v11) EscortActorPackageData;
		} else {
			v6 = nullptr;
		}
		v12 = v6;
		break;
	}
	case 12: {
		v10 = ourMalloc(sizeof(SandBoxActorPackageData));
		if (v10) {
			v5 = ::new (v10) SandBoxActorPackageData;
		} else {
			v5 = nullptr;
		}
		v12 = v5;
		break;
	}
	case 13: {
		apMemory = ourMalloc(sizeof(PatrolActorPackageData));
		if (apMemory) {
			v4 = ::new (apMemory) PatrolActorPackageData;
		} else {
			v4 = nullptr;
		}
		v12 = v4;
		break;
	}
	case 14: {
		v8 = ourMalloc(sizeof(GuardActorPackageData));
		if (v8) {
			v3 = ::new (v8) GuardActorPackageData;
		} else {
			v3 = nullptr;
		}
		v12 = v3;
		break;
	}
	case 16: {
		v7 = ourMalloc(sizeof(UseWeaponActorPackageData));
		if (v7) {
			v2 = ::new (v7) UseWeaponActorPackageData;
		} else {
			v2 = nullptr;
		}
		v12 = v2;
		break;
	}
	default: {
		return v12;
	}
	}
	return v12;
}
