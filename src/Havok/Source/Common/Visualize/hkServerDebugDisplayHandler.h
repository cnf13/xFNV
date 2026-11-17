#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"

#include <stdint.h>

class hkCriticalSection;
class hkDisplayGeometry;
class hkDisplaySerializeOStream;
class hkTransform;

// Serializes debugger display commands to the visual debugger's output stream.
class hkServerDebugDisplayHandler : public hkReferencedObject, public hkDebugDisplayHandler {
public:
    hkServerDebugDisplayHandler(hkDisplaySerializeOStream* stream);
    ~hkServerDebugDisplayHandler() override;

    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult addGeometry(
        const hkArrayBase<hkDisplayGeometry*>& geometries,
        const hkTransform& transform,
        hkUlong id,
        int32_t tag,
        hkUlong shapeIdHint
    ) override;
    hkResult addGeometryInstance(hkUlong, const hkTransform&, hkUlong, int32_t, hkUlong) override;
    hkResult setGeometryColor(int32_t, hkUlong, int32_t) override;
    hkResult updateGeometry(const hkTransform&, hkUlong, int32_t) override;
    hkResult removeGeometry(hkUlong, int32_t, hkUlong) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult updateCamera(
        const hkVector4& from,
        const hkVector4& to,
        const hkVector4& up,
        float nearPlane,
        float farPlane,
        float fov,
        const char* name
    ) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult updateBehavior(
        const hkArrayBase<int32_t>& wordVarIdx,
        const hkArrayBase<int32_t>& wordStack,
        const hkArrayBase<int32_t>& quadVarIdx,
        const hkArrayBase<hkVector4>& quadStack,
        const hkArrayBase<char*>& activeNodes,
        const hkArrayBase<int32_t>& activeStateIds,
        const hkArrayBase<int32_t>& activeTransitions,
        const hkArrayBase<hkQsTransform>& transforms
    ) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult displayPoint(const hkVector4& position, int32_t color, int32_t id, int32_t tag) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult displayLine(const hkVector4& start, const hkVector4& end, int32_t color, int32_t id, int32_t tag) override;
    // Havok SDK signature (hkVisualize.lib, tied by vtable).
    hkResult displayTriangle(
        const hkVector4& a,
        const hkVector4& b,
        const hkVector4& c,
        int32_t color,
        int32_t id,
        int32_t tag
    ) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult displayText(const char* text, int32_t color, int32_t id, int32_t tag) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult display3dText(const char* text, const hkVector4& pos, int32_t color, int32_t id, int32_t tag) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult displayAnnotation(const char* text, int32_t id, int32_t tag) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult displayGeometry(
        const hkArrayBase<hkDisplayGeometry*>& geometries,
        const hkTransform& transform,
        int32_t color,
        int32_t id,
        int32_t tag
    ) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult
    displayGeometry(const hkArrayBase<hkDisplayGeometry*>& geometries, int32_t color, int32_t id, int32_t tag) override;
    // Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
    hkResult sendMemStatsDump(const char* data, int32_t length) override;
    hkResult holdImmediate() override;

protected:
    // Writes the geometries' count, then each geometry, to the out stream under the lock.
    void sendGeometryData(const hkArrayBase<hkDisplayGeometry*>& geometries);

public:
    // DeletingDestruct's free (hkReferencedObject's heap block); names one of the two bases' operators.
    static void operator delete(void* apMemory) {
        hkReferencedObject::operator delete(apMemory);
    }

    hkDisplaySerializeOStream* m_outStream;
    hkCriticalSection* m_outstreamLock;
};
