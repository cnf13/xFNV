#pragma once

#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Reconstruction.hpp"

// Havok SDK signature (hkBase.lib, tied by bytes,data).
hkReferencedObject* hkTypeInfoRegistrycreate();

// The registry of the type infos that finish and clean up loaded objects, by type name. Only some of its members are
// declared.
class hkTypeInfoRegistry : public hkReferencedObject, public hkSingleton<hkTypeInfoRegistry> {
public:
	hkTypeInfoRegistry() { m_finishFlag.m_finishing = 1; }
	~hkTypeInfoRegistry() override;

	virtual void registerTypeInfo(const hkTypeInfo* info);
	virtual const hkTypeInfo* finishLoadedObject(void* obj, const char* className) const;
	virtual const hkTypeInfo* cleanupLoadedObject(void* obj, const char* className) const;
	virtual void merge(const hkTypeInfoRegistry& mergeFrom);
	virtual const hkTypeInfo* getTypeInfo(const char* typeName) const;

	hkStringMap<const hkTypeInfo*> m_map;
	// The flag the finish functions get.
	hkFinishLoadedObjectFlag m_finishFlag;

	// hkSingleton<hkTypeInfoRegistry>::s_instance.
	static inline hkTypeInfoRegistry* s_instance;
};
inline constexpr fnvd::Vtable hkTypeInfoRegistry_vftable{};
