#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldSnapshotViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Utilities/hkpMousePickingViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

int32_t s_movableObjectColor = hkColor::rgbFromChars(240, 240, 240, 255);
int32_t s_fixedObjectColor = hkColor::rgbFromChars(120, 120, 120, 255);
int32_t s_convexRadiusMovableObjectColor = hkColor::rgbFromChars(140, 240, 140, 140);
int32_t s_convexRadiusFixedObjectColor = hkColor::rgbFromChars(70, 200, 70, 140);

hkpWorldViewerBase::hkpWorldViewerBase(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkProcess(true), m_context(nullptr) {
    for (hkProcessContext* context : contexts) {
        if (!hkString::strCmp("Physics", context->getType())) {
            m_context = static_cast<hkpPhysicsContext*>(context);
            break;
        }
    }
    if (m_context) {
        m_context->addWorldAddedListener(this);
        m_context->addReference();
    }
}

hkpWorldViewerBase::~hkpWorldViewerBase() {
    if (m_context) {
        m_context->removeWorldAddedListener(this);
        m_context->removeReference();
        m_context = nullptr;
    }
}
