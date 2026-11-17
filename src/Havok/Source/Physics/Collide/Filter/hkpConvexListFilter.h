#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpCollisionInput;

// Decides how the collision detection treats a convex list shape against another body: as a normal convex shape,
// as the list of its children, or as one convex hull. getConvexListCollisionType, slot 3, is pure; the exe has no
// vtable of its own for this class (abstract, no RTTI).
class hkpConvexListFilter : public hkReferencedObject {
public:
	// Values from the SDK.
	enum ConvexListCollisionType : int32_t {
		TREAT_CONVEX_LIST_AS_NORMAL = 0x0,
		TREAT_CONVEX_LIST_AS_LIST = 0x1,
		TREAT_CONVEX_LIST_AS_CONVEX = 0x2,
	};

	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpConvexListFilter() = default;
	hkpConvexListFilter(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}
	~hkpConvexListFilter() override;

	virtual ConvexListCollisionType getConvexListCollisionType(const hkpCdBody& convexListBody,
		const hkpCdBody& otherBody, const hkpCollisionInput& input) const = 0;
};

// The class's reflection record.
extern hkClass hkpConvexListFilterClass;
