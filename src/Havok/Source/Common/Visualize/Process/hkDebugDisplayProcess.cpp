#include "Havok/Source/Common/Visualize/Process/hkDebugDisplayProcess.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplay.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"

// The two displayGeometry overloads share a name, so the member-pointer type is needed to tell them apart.
using hkDebugDisplayProcess_DisplayGeometry =
    hkResult (hkDebugDisplayProcess::*)(const hkArrayBase<hkDisplayGeometry*>&, int32_t, int32_t, int32_t);
using hkDebugDisplayProcess_DisplayGeometryWithTransform = hkResult (hkDebugDisplayProcess::*)(
    const hkArrayBase<hkDisplayGeometry*>&,
    const hkTransform&,
    int32_t,
    int32_t,
    int32_t
);

hkDebugDisplayProcess::hkDebugDisplayProcess() : hkProcess(true) {
    if (!m_tag) {
        registerProcess();
    }
    hkDebugDisplay::s_instance->addDebugDisplayHandler(this);
}

hkDebugDisplayProcess::~hkDebugDisplayProcess() {
    hkDebugDisplay::s_instance->removeDebugDisplayHandler(this);
}

// The display handler functions replace the caller's tag with the process's own and forward the call to the display
// handler hkProcess keeps, so every argument but the tag passes through unchanged.
hkResult hkDebugDisplayProcess::addGeometry(
    const hkArrayBase<hkDisplayGeometry*>& geometries,
    const hkTransform& transform,
    hkUlong id,
    int32_t tag,
    hkUlong shapeId
) {
    return m_displayHandler->addGeometry(geometries, transform, id, m_tag, shapeId);
}

hkResult hkDebugDisplayProcess::addGeometryInstance(
    hkUlong origId,
    const hkTransform& transform,
    hkUlong id,
    int32_t tag,
    hkUlong shapeId
) {
    return m_displayHandler->addGeometryInstance(origId, transform, id, m_tag, shapeId);
}

hkResult hkDebugDisplayProcess::setGeometryColor(int32_t color, hkUlong id, int32_t tag) {
    return m_displayHandler->setGeometryColor(color, id, m_tag);
}

hkResult hkDebugDisplayProcess::updateGeometry(const hkTransform& transform, hkUlong id, int32_t tag) {
    return m_displayHandler->updateGeometry(transform, id, m_tag);
}

hkResult hkDebugDisplayProcess::removeGeometry(hkUlong id, int32_t tag, hkUlong shapeId) {
    return m_displayHandler->removeGeometry(id, m_tag, shapeId);
}

hkResult hkDebugDisplayProcess::updateCamera(
    const hkVector4& a1,
    const hkVector4& a2,
    const hkVector4& a3,
    float a4,
    float a5,
    float a6,
    const char* a7
) {
    return m_displayHandler->updateCamera(a1, a2, a3, a4, a5, a6, a7);
}

hkResult hkDebugDisplayProcess::updateBehavior(
    const hkArrayBase<int32_t>& a1,
    const hkArrayBase<int32_t>& a2,
    const hkArrayBase<int32_t>& a3,
    const hkArrayBase<hkVector4>& a4,
    const hkArrayBase<char*>& a5,
    const hkArrayBase<int32_t>& a6,
    const hkArrayBase<int32_t>& a7,
    const hkArrayBase<hkQsTransform>& a8
) {
    return m_displayHandler->updateBehavior(a1, a2, a3, a4, a5, a6, a7, a8);
}

hkResult hkDebugDisplayProcess::displayPoint(const hkVector4& position, int32_t color, int32_t id, int32_t tag) {
    return m_displayHandler->displayPoint(position, color, id, m_tag);
}

hkResult hkDebugDisplayProcess::displayLine(
    const hkVector4& start,
    const hkVector4& end,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    return m_displayHandler->displayLine(start, end, color, id, m_tag);
}

hkResult hkDebugDisplayProcess::displayTriangle(
    const hkVector4& v0,
    const hkVector4& v1,
    const hkVector4& v2,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    return m_displayHandler->displayTriangle(v0, v1, v2, color, id, m_tag);
}

hkResult hkDebugDisplayProcess::displayText(const char* text, int32_t color, int32_t id, int32_t tag) {
    return m_displayHandler->displayText(text, color, id, m_tag);
}

hkResult hkDebugDisplayProcess::display3dText(
    const char* text,
    const hkVector4& position,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    return m_displayHandler->display3dText(text, position, color, id, m_tag);
}

hkResult hkDebugDisplayProcess::displayAnnotation(const char* text, int32_t color, int32_t tag) {
    return m_displayHandler->displayAnnotation(text, color, m_tag);
}

hkResult hkDebugDisplayProcess::displayGeometry(
    const hkArrayBase<hkDisplayGeometry*>& geometries,
    const hkTransform& transform,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    return m_displayHandler->displayGeometry(geometries, transform, color, id, m_tag);
}

hkResult hkDebugDisplayProcess::displayGeometry(
    const hkArrayBase<hkDisplayGeometry*>& geometries,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    return m_displayHandler->displayGeometry(geometries, color, id, m_tag);
}

hkResult hkDebugDisplayProcess::sendMemStatsDump(const char* a1, int32_t a2) {
    return m_displayHandler->sendMemStatsDump(a1, a2);
}

hkResult hkDebugDisplayProcess::holdImmediate() {
    return m_displayHandler->holdImmediate();
}

void hkDebugDisplayProcess::registerProcess() {
    m_tag = hkProcessFactory::s_instance->registerProcess(hkDebugDisplayProcess_Name, &hkDebugDisplayProcess::create);
}

hkProcess* hkDebugDisplayProcess::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkDebugDisplayProcess* v2 =
        static_cast<hkDebugDisplayProcess*>(Instance->m_heap->blockAlloc(sizeof(hkDebugDisplayProcess)));
    v2->m_memSizeAndFlags = sizeof(hkDebugDisplayProcess);
    hkDebugDisplayProcess* v3 = ::new (v2) hkDebugDisplayProcess;
    if (v3) {
        return static_cast<hkProcess*>(v3);
    }
    else {
        return 0;
    }
}
