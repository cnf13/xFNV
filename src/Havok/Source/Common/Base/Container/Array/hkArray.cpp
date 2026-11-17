#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkHistogram.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "SDK/MSVC/ehvec.h"

using hkArray_lunsigned_schar_chkContainerHeapAllocator = hkArray<uint8_t, hkContainerHeapAllocator>;

using hkArray_lunsigned_schar_chkContainerHeapAllocator = hkArray<uint8_t, hkContainerHeapAllocator>;

using hkArray_lhkpRigidBody_p_chkContainerHeapAllocator = hkArray<hkpRigidBody*, hkContainerHeapAllocator>;

void clearAndDeallocate(hkArray<hkpRootCdBodyPair, hkContainerHeapAllocator>* apThis, [[maybe_unused]] void* apEdx) {
	hkContainerHeapAllocator v4 = hkContainerHeapAllocator{};
	hkMemoryAllocator* v1 = &(&v4)->get(apThis);
	apThis->_clearAndDeallocate(*v1);
}
