#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"

bool EffectArchetypes::CheckFlag(int32_t aiArchetype, uint32_t aiFlags) {
	return ((((int32_t)aiFlags) & ((int32_t)EffectArchetypes::Archetypes[aiArchetype].iFlags)) != 0);
}

const char* EffectArchetypes::GetName(int32_t aiArchetype) {
	return EffectArchetypes::Archetypes[aiArchetype].pcName;
}

char EffectArchetypes::RegisterActiveEffect(ArchetypeID aeArchtype, InstantiateFunction apInstantiateFunction) {
	int8_t v3 = 0;
	if (EffectArchetypes::Archetypes[aeArchtype].pInstantiateFunction) {
		return v3;
	}
	EffectArchetypes::Archetypes[aeArchtype].pInstantiateFunction = apInstantiateFunction;
	return 1;
}

EffectSetting* EffectItem::GetEffectSetting() {
	return pEffectSetting;
}
