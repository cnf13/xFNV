#include "Havok/Source/Animation/Animation/Motion/hkaAnimatedReferenceFrame.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// The reflection data of hkaAnimatedReferenceFrame (the PDB's hkaAnimatedReferenceFrameClass.obj; the SDK has no .cpp
// here to name the file, so it takes the header's stem): the class record, with no members of its own.
hkClass hkaAnimatedReferenceFrameClass("hkaAnimatedReferenceFrame", &hkReferencedObjectClass,
	sizeof(hkaAnimatedReferenceFrame), nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);
