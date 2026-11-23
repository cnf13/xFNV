#include "Havok/Source/Common/Visualize/hkServerDebugDisplayHandler.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"

static int32_t computeGeometriesBytesRequired(const hkArrayBase<hkDisplayGeometry*>* geometries);

static int32_t computeGeometriesBytesRequired(const hkArrayBase<hkDisplayGeometry*>* geometries) {
    int32_t i;
    uint32_t v3;
    int32_t v1 = 0;
    for (i = 4; v1 < geometries->m_size; i += v3) {
        v3 = hkDisplaySerializeOStream::computeBytesRequired(geometries->m_data[v1++]);
    }
    return i;
}

hkServerDebugDisplayHandler::hkServerDebugDisplayHandler(hkDisplaySerializeOStream* stream) : m_outStream(stream) {
    m_outstreamLock = new hkCriticalSection(1000);
}

hkServerDebugDisplayHandler::~hkServerDebugDisplayHandler() {
    delete m_outstreamLock;
}

hkResult hkServerDebugDisplayHandler::addGeometry(
    const hkArrayBase<hkDisplayGeometry*>& geometries,
    const hkTransform& transform,
    hkUlong id,
    int32_t tag,
    hkUlong shapeIdHint
) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        int32_t v8 = computeGeometriesBytesRequired(&geometries);
        this->m_outStream->write32(v8 + 41);
        this->m_outStream->write8u(HK_ADD_GEOMETRY);
        sendGeometryData(geometries);
        this->m_outStream->writeTransform(transform);
        this->m_outStream->write64u(id);
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v10 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v10);
}

hkResult hkServerDebugDisplayHandler::addGeometryInstance(
    hkUlong originalInstanceID,
    const hkTransform& transform,
    hkUlong id,
    int32_t tag,
    hkUlong shapeIdHint
) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v8 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v8);
}

hkResult hkServerDebugDisplayHandler::setGeometryColor(int32_t color, hkUlong id, int32_t tag) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    if (m_outStream) {
        m_outStream->write32u(17u);
        this->m_outStream->write8u(HK_SET_COLOR_GEOMETRY);
        this->m_outStream->write32u(static_cast<uint32_t>(color));
        this->m_outStream->write64u(id);
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* m_outStream_1 = this->m_outStream;
    bool v7 = m_outStream_1 && m_outStream_1->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v7);
}

hkResult hkServerDebugDisplayHandler::updateGeometry(const hkTransform& transform, hkUlong id, int32_t tag) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    if (m_outStream) {
        m_outStream->write32u(41u);
        this->m_outStream->write8u(2);
        this->m_outStream->writeTransform(transform);
        this->m_outStream->write64u(id);
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* v6 = this->m_outStream;
    bool v7 = v6 && v6->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v7);
}

hkResult hkServerDebugDisplayHandler::removeGeometry(hkUlong id, int32_t tag, hkUlong shapeIdHint) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    if (m_outStream) {
        m_outStream->write32u(13u);
        this->m_outStream->write8u(4);
        this->m_outStream->write64u(id);
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* v6 = this->m_outStream;
    bool v7 = v6 && v6->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v7);
}

hkResult hkServerDebugDisplayHandler::updateCamera(
    const hkVector4& from,
    const hkVector4& to,
    const hkVector4& up,
    float nearPlane,
    float farPlane,
    float fov,
    const char* name
) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        int32_t length = hkString::strLen(name);
        this->m_outStream->write32u(length + 51);
        this->m_outStream->write8u(HK_UPDATE_CAMERA);
        this->m_outStream->writeQuadVector4(from);
        this->m_outStream->writeQuadVector4(to);
        this->m_outStream->writeQuadVector4(up);
        this->m_outStream->writeFloat32(nearPlane);
        this->m_outStream->writeFloat32(farPlane);
        this->m_outStream->writeFloat32(fov);
        this->m_outStream->write16u(length);
        this->m_outStream->writeRaw(name, length);
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v12 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v12);
}

hkResult hkServerDebugDisplayHandler::updateBehavior(
    const hkArrayBase<int32_t>& wordVarIdx,
    const hkArrayBase<int32_t>& wordStack,
    const hkArrayBase<int32_t>& quadVarIdx,
    const hkArrayBase<hkVector4>& quadStack,
    const hkArrayBase<char*>& activeNodes,
    const hkArrayBase<int32_t>& activeStateIds,
    const hkArrayBase<int32_t>& activeTransitions,
    const hkArrayBase<hkQsTransform>& transforms
) {
    // The node names' lengths, 0 for a null name: the SDK's local hkArray<int>, freed by hand at the end.
    hkArrayBase<int32_t> nameLengths;
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        uint32_t m_size = wordStack.m_size;
        uint32_t m_size_1 = quadStack.m_size;
        uint32_t packetSize = 4 *
                (activeTransitions.m_size +
                 (activeStateIds.m_size + (7 * transforms.m_size + 2 * (m_size + 2 * m_size_1)))) +
            25;
        nameLengths.m_data = nullptr;
        nameLengths.m_size = 0;
        nameLengths.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
        for (const char* nodeName : activeNodes) {
            if (nodeName) {
                int32_t length = hkString::strLen(nodeName);
                nameLengths._pushBack(hkMemoryRouter::getInstance().heap(), length);
                packetSize += 2 + length;
            }
            else {
                nameLengths._pushBack(hkMemoryRouter::getInstance().heap(), 0);
            }
        }
        this->m_outStream->write32u(packetSize);
        this->m_outStream->write8u(HK_UPDATE_BEHAVIOR);
        this->m_outStream->write32u(m_size);
        for (int32_t i = 0; i < wordStack.m_size; ++i) {
            this->m_outStream->write32u(static_cast<uint32_t>(wordVarIdx.m_data[i]));
            this->m_outStream->write32u(static_cast<uint32_t>(wordStack.m_data[i]));
        }
        this->m_outStream->write32u(m_size_1);
        for (int32_t i = 0; i < quadStack.m_size; ++i) {
            this->m_outStream->write32u(static_cast<uint32_t>(quadVarIdx.m_data[i]));
            this->m_outStream->writeQuadVector4(quadStack.m_data[i]);
        }
        this->m_outStream->write32u(static_cast<uint16_t>(activeNodes.m_size));
        for (int32_t i = 0; i < activeNodes.m_size; ++i) {
            if (activeNodes.m_data[i]) {
                int32_t length = nameLengths.m_data[i];
                this->m_outStream->write16u(length);
                this->m_outStream->writeRaw(activeNodes.m_data[i], length);
            }
        }
        this->m_outStream->write32u(static_cast<uint32_t>(transforms.m_size));
        for (const hkQsTransform& transform : transforms) {
            hkTransform arTransformOut_;
            transform.copyToTransform(arTransformOut_);
            this->m_outStream->writeTransform(arTransformOut_);
        }
        this->m_outStream->write32u(static_cast<uint32_t>(activeStateIds.m_size));
        for (int32_t stateId : activeStateIds) {
            this->m_outStream->write32u(static_cast<uint32_t>(stateId));
        }
        this->m_outStream->write32u(static_cast<uint32_t>(activeTransitions.m_size));
        for (int32_t transition : activeTransitions) {
            this->m_outStream->write32u(static_cast<uint32_t>(transition));
        }
        hkMemoryAllocator* m_heap = hkMemoryRouter::getInstance().m_heap;
        nameLengths.m_size = 0;
        if (nameLengths.m_capacityAndFlags >= 0) {
            m_heap->bufFree(nameLengths.m_data, sizeof(int32_t) * nameLengths.getCapacity());
        }
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v30 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v30);
}

hkResult hkServerDebugDisplayHandler::displayPoint(const hkVector4& position, int32_t color, int32_t id, int32_t tag) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    if (m_outStream) {
        m_outStream->write32u(29u);
        this->m_outStream->write8u(HK_DISPLAY_POINT);
        this->m_outStream->writeQuadVector4(position);
        this->m_outStream->write32u(static_cast<uint32_t>(color));
        // The id is sign-extended to the 64-bit field.
        this->m_outStream->write64u(static_cast<int64_t>(id));
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* m_outStream_1 = this->m_outStream;
    bool v8 = m_outStream_1 && m_outStream_1->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v8);
}

hkResult hkServerDebugDisplayHandler::displayLine(
    const hkVector4& start,
    const hkVector4& end,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    if (m_outStream) {
        m_outStream->write32u(41u);
        this->m_outStream->write8u(HK_DISPLAY_LINE);
        this->m_outStream->writeQuadVector4(start);
        this->m_outStream->writeQuadVector4(end);
        this->m_outStream->write32u(static_cast<uint32_t>(color));
        // The id is sign-extended to the 64-bit field.
        this->m_outStream->write64u(static_cast<int64_t>(id));
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* m_outStream_1 = this->m_outStream;
    bool v9 = m_outStream_1 && m_outStream_1->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v9);
}

hkResult hkServerDebugDisplayHandler::displayTriangle(
    const hkVector4& a,
    const hkVector4& b,
    const hkVector4& c,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        // The triangle goes out as its three edges, one line packet each: the corners copied into a closed loop, kept
        // 16-aligned.
        alignas(16) hkVector4 corners[4];
        corners[0] = a;
        corners[1] = b;
        corners[2] = c;
        corners[3] = a;
        // The id, sign-extended to the 64-bit field once, before the loop.
        uint64_t id64 = static_cast<int64_t>(id);
        for (int32_t i = 0; i < 3; ++i) {
            this->m_outStream->write32u(41u);
            this->m_outStream->write8u(HK_DISPLAY_LINE);
            this->m_outStream->writeQuadVector4(corners[i]);
            this->m_outStream->writeQuadVector4(corners[i + 1]);
            this->m_outStream->write32u(static_cast<uint32_t>(color));
            this->m_outStream->write64u(id64);
            this->m_outStream->write32(tag);
        }
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v13 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v13);
}

hkResult hkServerDebugDisplayHandler::displayText(const char* text, int32_t color, int32_t id, int32_t tag) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        // The packet carries a 16-bit length: longer text is cut at 0xFFFF bytes.
        int32_t length = hkString::strLen(text);
        if (length >= 0xFFFF) {
            length = 0xFFFF;
        }
        this->m_outStream->write32u(length + 19);
        this->m_outStream->write8u(HK_DISPLAY_TEXT);
        this->m_outStream->write16u(length);
        this->m_outStream->writeRaw(text, length);
        this->m_outStream->write32(color);
        // The id is sign-extended to the 64-bit field.
        this->m_outStream->write64u(static_cast<int64_t>(id));
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v9 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v9);
}

hkResult hkServerDebugDisplayHandler::display3dText(
    const char* text,
    const hkVector4& pos,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        // The packet carries a 16-bit length: longer text is cut at 0xFFFF bytes.
        int32_t length = hkString::strLen(text);
        if (length >= 0xFFFF) {
            length = 0xFFFF;
        }
        this->m_outStream->write32u(length + 31);
        this->m_outStream->write8u(HK_DISPLAY_TEXT_3D);
        this->m_outStream->write16u(length);
        this->m_outStream->writeRaw(text, length);
        this->m_outStream->write32(color);
        // The id is sign-extended to the 64-bit field.
        this->m_outStream->write64u(static_cast<int64_t>(id));
        this->m_outStream->write32(tag);
        this->m_outStream->writeFloat32(pos(0));
        this->m_outStream->writeFloat32(pos(1));
        this->m_outStream->writeFloat32(pos(2));
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v11 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v11);
}

hkResult hkServerDebugDisplayHandler::displayAnnotation(const char* text, int32_t id, int32_t tag) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        // The packet carries a 16-bit length: longer text is cut at 0xFFFF bytes.
        int32_t length = hkString::strLen(text);
        if (length >= 0xFFFF) {
            length = 0xFFFF;
        }
        this->m_outStream->write32u(length + 15);
        this->m_outStream->write8u(HK_DISPLAY_ANNOTATION);
        this->m_outStream->write16u(length);
        this->m_outStream->writeRaw(text, length);
        // The id is sign-extended to the 64-bit field.
        this->m_outStream->write64u(static_cast<int64_t>(id));
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v8 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v8);
}

hkResult hkServerDebugDisplayHandler::displayGeometry(
    const hkArrayBase<hkDisplayGeometry*>& geometries,
    const hkTransform& transform,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        int32_t v8 = computeGeometriesBytesRequired(&geometries);
        this->m_outStream->write32(v8 + 45);
        this->m_outStream->write8u(HK_DISPLAY_GEOMETRY_WITH_TRANSFORM);
        sendGeometryData(geometries);
        this->m_outStream->writeTransform(transform);
        this->m_outStream->write32u(static_cast<uint32_t>(color));
        // The id is sign-extended to the 64-bit field.
        this->m_outStream->write64u(static_cast<int64_t>(id));
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v10 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v10);
}

hkResult hkServerDebugDisplayHandler::displayGeometry(
    const hkArrayBase<hkDisplayGeometry*>& geometries,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    if (this->m_outStream) {
        int32_t v7 = computeGeometriesBytesRequired(&geometries);
        this->m_outStream->write32(v7 + 17);
        this->m_outStream->write8u(HK_DISPLAY_GEOMETRY);
        sendGeometryData(geometries);
        this->m_outStream->write32u(static_cast<uint32_t>(color));
        // The id is sign-extended to the 64-bit field.
        this->m_outStream->write64u(static_cast<int64_t>(id));
        this->m_outStream->write32(tag);
    }
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    bool v9 = m_outStream && m_outStream->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v9);
}

hkResult hkServerDebugDisplayHandler::sendMemStatsDump(const char* data, int32_t length) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    if (m_outStream) {
        // The packet's size: the command byte, the 32-bit length and the data.
        m_outStream->write32u(static_cast<uint32_t>(length + 5));
        this->m_outStream->write8u(HK_SEND_MEMSTATS_DUMP);
        this->m_outStream->write32(length);
        this->m_outStream->writeRaw(data, length);
    }
    hkDisplaySerializeOStream* m_outStream_1 = this->m_outStream;
    bool v7 = m_outStream_1 && m_outStream_1->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v7);
}

hkResult hkServerDebugDisplayHandler::holdImmediate() {
    EnterCriticalSection(&m_outstreamLock->m_section);
    hkDisplaySerializeOStream* m_outStream = this->m_outStream;
    if (m_outStream) {
        m_outStream->write32u(1u);
        this->m_outStream->write8u(11);
    }
    hkDisplaySerializeOStream* v3 = this->m_outStream;
    bool v4 = v3 && v3->isOk().m_bool;
    LeaveCriticalSection(&m_outstreamLock->m_section);
    return static_cast<hkResult>(!v4);
}

void hkServerDebugDisplayHandler::sendGeometryData(const hkArrayBase<hkDisplayGeometry*>& geometries) {
    EnterCriticalSection(&m_outstreamLock->m_section);
    this->m_outStream->write32(geometries.m_size);
    for (hkDisplayGeometry* geometry : geometries) {
        this->m_outStream->writeDisplayGeometry(geometry);
    }
    LeaveCriticalSection(&m_outstreamLock->m_section);
}
