#include "Havok/Source/Common/Serialize/Resource/hkResource.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"
#include "Havok/Source/Common/Serialize/Util/hkRootLevelContainer.h"

// getContentsPointer with the class name of hkRootLevelContainer and the builtin type info registry, which is fetched
// first.
template <> hkRootLevelContainer* hkResource::getContents<hkRootLevelContainer>() const {
	hkTypeInfoRegistry* pTypeRegistry = hkBuiltinTypeRegistry::s_instance->getTypeInfoRegistry();
	const char* pTypeName = hkRootLevelContainer::staticClass().getName();
	return static_cast<hkRootLevelContainer*>(getContentsPointer(pTypeName, pTypeRegistry));
}
