#include "Fallout/Magic/MagicSprayProjectile.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

MagicSprayProjectile::MagicSprayProjectile(MagicCaster* a1, MagicItem* a2, EffectItem* a3, EffectSetting* a4,
	NiPoint3 a5, NiPoint3 a6)
	: MagicProjectile(a1, a2, a3, a4, a5, a6) {
	MobileObject::InitHavok();
}

// The exe compares afDelta with 0.0 (fld, fcomp qword 0x1012060) and drops the result; there is no effect to write.
void MagicSprayProjectile::Update(float afDelta) {}
