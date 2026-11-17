#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkResource;
class hkStreamReader;
class hkStreamWriter;
class hkpPhysicsData;
class hkpWorld;

// Havok's snapshot writer: it writes a whole world to a stream so that it can be read back later. It has no
// instances and no data; only the entry point the reconstructed code calls is declared.
class hkpHavokSnapshot {
public:
	// The listener save hands the packfile writer: addObjectCallback swaps objects for the classes snapshots store
	// them as, keeping the objects it creates in m_objects; the destructor deletes them. The exe's vtable has 4
	// slots; it overrides the destructor and addObjectCallback.
	class ConvertListener : public hkPackfileWriter::AddObjectListener {
	public:
		~ConvertListener() override;

		void addObjectCallback(const void*& p, const hkClass*& k) override;

		hkArray<hkReferencedObject*> m_objects;
	};

	// Writes apWorld through apWriter; the hkBool result goes through the hidden pointer apResult, which is returned
	// (cdecl, ret 0, 0xCC0D60). PDB: bool saveContactPoints (src's abSaveContactPoints); 0xCC0D60 pushes it whole to
	// hkpPhysicsData::populateFromWorld. The exe passes the caller's whole dword on; bool would be zero-extended.
	static hkBool* save(hkBool* apResult, const hkpWorld* apWorld, hkStreamWriter* apWriter, hkBool abBinaryFormat,
		const hkStructureLayout::LayoutRules* apTargetLayout, int32_t abSaveContactPoints);
	// Writes apData of class arDataClass as a tagfile, through the PDB's local defaultConvertListener when
	// apUserListener is null (cdecl, ret 0, 0xCC0B50). apTargetLayout is not read.
	static hkBool* save(hkBool* apResult, const void* apData, const hkClass& arDataClass, hkStreamWriter* apWriter,
		hkBool abBinaryFormat, const hkStructureLayout::LayoutRules* apTargetLayout,
		hkPackfileWriter::AddObjectListener* apUserListener);
	// Wraps apData in a one-entry hkRootLevelContainer (the PDB's local container) and saves that (cdecl, ret 0,
	// 0xCC0BC0).
	static hkBool* saveUnderRootLevel(hkBool* apResult, const void* apData, const hkClass& arDataClass,
		hkStreamWriter* apWriter, hkBool abBinaryFormat, const hkStructureLayout::LayoutRules* apTargetLayout);
	// Saves apData under the root level with hkpPhysicsDataClass.
	static hkBool* save(hkBool* apResult, const hkpPhysicsData* apData, hkStreamWriter* apWriter, hkBool abBinaryFormat,
		const hkStructureLayout::LayoutRules* apTargetLayout);
	// The physics data of a snapshot read from reader; the loaded resource goes to allocatedData (HK_CALL, 0xCC0850).
	static hkpPhysicsData* load(hkStreamReader* reader, hkResource** allocatedData);
};

extern hkClass hkRootLevelContainerNamedVariantClass;
extern hkClass hkpSampledHeightFieldShapeClass;
inline constexpr fnvd::Vtable hkpHavokSnapshot_ConvertListener_vftable{};
