#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpPhysicsData;
class hkpWorld;
class hkResource;
class hkStreamReader;
class hkStreamWriter;

// Havok's snapshot writer: it writes a whole world to a stream so that it can be read back later. It has no instances
// and no data; only the entry point this code calls is declared.
class hkpHavokSnapshot {
public:
    // The listener save hands the packfile writer: addObjectCallback swaps objects for the classes snapshots store them
    // as, keeping the objects it creates in m_objects; the destructor deletes them.
    class ConvertListener : public hkPackfileWriter::AddObjectListener {
    public:
        ~ConvertListener() override;

        void addObjectCallback(const void*& p, const hkClass*& k) override;

        hkArray<hkReferencedObject*> m_objects;
    };

    // Writes apWorld through apWriter; the hkBool result is written to apResult, which is returned. abSaveContactPoints
    // is an int32_t, not a bool, because the caller's whole value is passed on to hkpPhysicsData::populateFromWorld; a
    // bool would be zero-extended.
    static hkBool* save(
        hkBool* apResult,
        const hkpWorld* apWorld,
        hkStreamWriter* apWriter,
        hkBool abBinaryFormat,
        const hkStructureLayout::LayoutRules* apTargetLayout,
        int32_t abSaveContactPoints
    );
    // Writes apData of class arDataClass as a tagfile, through a local defaultConvertListener when apUserListener is
    // null. apTargetLayout is not read.
    static hkBool* save(
        hkBool* apResult,
        const void* apData,
        const hkClass& arDataClass,
        hkStreamWriter* apWriter,
        hkBool abBinaryFormat,
        const hkStructureLayout::LayoutRules* apTargetLayout,
        hkPackfileWriter::AddObjectListener* apUserListener
    );
    // Wraps apData in a one-entry hkRootLevelContainer and saves that.
    static hkBool* saveUnderRootLevel(
        hkBool* apResult,
        const void* apData,
        const hkClass& arDataClass,
        hkStreamWriter* apWriter,
        hkBool abBinaryFormat,
        const hkStructureLayout::LayoutRules* apTargetLayout
    );
    // Saves apData under the root level with hkpPhysicsDataClass.
    static hkBool* save(
        hkBool* apResult,
        const hkpPhysicsData* apData,
        hkStreamWriter* apWriter,
        hkBool abBinaryFormat,
        const hkStructureLayout::LayoutRules* apTargetLayout
    );
    // The physics data of a snapshot read from reader; the loaded resource goes to allocatedData.
    static hkpPhysicsData* load(hkStreamReader* reader, hkResource** allocatedData);
};

extern hkClass hkRootLevelContainerNamedVariantClass;
extern hkClass hkpSampledHeightFieldShapeClass;
inline constexpr fnvd::Vtable hkpHavokSnapshot_ConvertListener_vftable{};
