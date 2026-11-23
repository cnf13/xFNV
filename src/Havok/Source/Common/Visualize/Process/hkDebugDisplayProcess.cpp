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
    const hkVector4& from,
    const hkVector4& to,
    const hkVector4& up,
    float nearPlane,
    float farPlane,
    float fov,
    const char* name
) {
    return m_displayHandler->updateCamera(from, to, up, nearPlane, farPlane, fov, name);
}

hkResult hkDebugDisplayProcess::updateBehavior(
    const hkArrayBase<int32_t>& wordVarIdx,
    const hkArrayBase<int32_t>& wordStack,
    const hkArrayBase<int32_t>& quadVarIdx,
    const hkArrayBase<hkVector4>& quadStack,
    const hkArrayBase<char*>& activeNodes,
    const hkArrayBase<int32_t>& activeStateIds,
    const hkArrayBase<int32_t>& activeTransitions,
    const hkArrayBase<hkQsTransform>& transforms
) {
    return m_displayHandler->updateBehavior(
        wordVarIdx,
        wordStack,
        quadVarIdx,
        quadStack,
        activeNodes,
        activeStateIds,
        activeTransitions,
        transforms
    );
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

hkResult hkDebugDisplayProcess::sendMemStatsDump(const char* data, int32_t length) {
    return m_displayHandler->sendMemStatsDump(data, length);
}

hkResult hkDebugDisplayProcess::holdImmediate() {
    return m_displayHandler->holdImmediate();
}

void hkDebugDisplayProcess::registerProcess() {
    m_tag = hkProcessFactory::s_instance->registerProcess(getName(), &hkDebugDisplayProcess::create);
}

hkProcess* hkDebugDisplayProcess::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    return new hkDebugDisplayProcess;
}
