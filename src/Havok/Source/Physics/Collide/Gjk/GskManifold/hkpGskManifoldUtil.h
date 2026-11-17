#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpContactMgr;
class hkpGskCache;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;

// One GSK manifold being built: the contact vertices, the manifold's master normal and the four values the
// assembler keeps with them. Offsets from the Xbox PDB.
struct hkpGskManifoldWork {
	hkpCdVertex m_vertices[16];
	hkVector4 m_masterNormal;
	hkPadSpu<float> m_radiusA;
	hkPadSpu<float> m_radiusB;
	hkPadSpu<float> m_keepContact;
	hkPadSpu<float> m_radiusSumSqrd;
};

// What adding a point to a GSK manifold did: replaced one of the four points, added it, or rejected it.
enum hkpGskManifoldAddStatus : int32_t {
	HK_GSK_MANIFOLD_POINT_REPLACED0 = 0,
	HK_GSK_MANIFOLD_POINT_REPLACED1 = 1,
	HK_GSK_MANIFOLD_POINT_REPLACED2 = 2,
	HK_GSK_MANIFOLD_POINT_REPLACED3 = 3,
	HK_GSK_MANIFOLD_POINT_ADDED = 4,
	HK_GSK_MANIFOLD_POINT_REJECTED = 5,
	HK_GSK_MANIFOLD_TWO_POINT2_REJECTED = 6,
};

// Which contact points hkGskManifold_addPoint gives a contact point id: every point, or only the points that are not
// potentials (the PDB's two enumerators; the exe passes it as a dword, the last argument of hkGskManifold_addPoint).
enum hkpGskManifoldUtilMgrHandling : int32_t {
	HK_GSK_MANIFOLD_CREATE_ID_ALWAYS = 0,
	HK_GSK_MANIFOLD_NO_ID_FOR_POTENTIALS = 1,
};

// What hkGskManifold_convertFeature made of a contact point (the PDB's enumerators).
enum hkGskManifold_convertFeatureResult : int32_t {
	HK_FEATURE_OK = 0,
	HK_FEATURE_INVALID = 1,
};

// The contact hkGskManifold_convertFeature makes of a manifold point (the PDB's record; the exe writes the three
// vectors at +0, +0x10 and +0x20 of its edx argument, 0xDAD990).
struct hkGskManifoldConvertOut {
	hkVector4 m_pointOnA;
	hkVector4 m_AminusB;
	hkVector4 m_normal;
};

// Of five contact points, picks the one to drop so the four kept span the largest area; returns its index.
// The exe passes the points in ecx (LTCG register convention, ret 0): fastcall with one argument.
int32_t hkGskManifold_findRedundant5thPoint(hkpProcessCdPoint* resultPointArray);
bool hkGskManifold_doesPointExistAndResort(hkpGskManifold& manifold, const hkpGskCache& newPoint);
// Drops the vertex ids no contact point uses and renumbers the points' vertex offsets. Usercall (0xDAE220, ret 0):
// manifold in edx, which hkGskManifold_removePoint (0xDAE340) leaves set when it jumps there; ecx is not read.
void hkGskManifold_garbageCollect(hkpGskManifold& manifold);
// Removes the manifold's contact point at the index: the last point takes its place and the vertex ids that follow
// the points move down over the freed one, then the vertex ids no point uses are dropped.
void hkGskManifold_removePoint(hkpGskManifold& manifold, int32_t index);
// Turns the manifold's points from firstPointIndex on into contacts in processOut; drops the points no longer valid
// or beyond the keep distance.
void hkGskManifold_verifyAndGetPoints(hkpGskManifold& manifold, const hkpGskManifoldWork& work, int32_t firstPointIndex,
	hkpProcessCollisionOutput& processOut, hkpContactMgr* contactMgr);
// Turns feature's vertex pair (its dimA * 4 + dimB picks the case; its vertex ids are byte
// offsets into work.m_vertices) into out: the point on A, the separating vector A - B and work.m_masterNormal.
// HK_FEATURE_INVALID when the feature is beyond work.m_radiusSumSqrd (+0x11C); two cases return a lane mask
// (not eax; and eax, 7 at 0xDADD1E and 0xDAE1CC), so the result is the PDB's int. Usercall: work in ecx, out in
// edx (stores at +0, +0x10, +0x20), feature on the stack ([ebp+8]), ret 0 (0xDAD990).
int32_t hkGskManifold_convertFeature(const hkpGskManifoldWork& work, hkGskManifoldConvertOut& out,
	const hkpGskManifold::ContactPoint& feature);
// The SDK's convertPointEdge, convertEdgePoint and convertPointPoint: a feature of that kind converted as
// hkGskManifold_convertFeature does. No exe code calls them (convertFeature has its own copies). Usercall, ret 0:
// convertPointEdge (0xDAD720) and convertEdgePoint (0xDAD860) take work in eax, feature in ecx, out in edi;
// convertPointPoint (0xDAD6A0) work in edx, feature in eax, out in esi.
int32_t hkGskManifold_convertPointEdge(const hkpGskManifoldWork& work, const hkpGskManifold::ContactPoint& feature,
	hkGskManifoldConvertOut& out);
int32_t hkGskManifold_convertEdgePoint(const hkpGskManifoldWork& work, const hkpGskManifold::ContactPoint& feature,
	hkGskManifoldConvertOut& out);
int32_t hkGskManifold_convertPointPoint(const hkpGskManifoldWork& work, const hkpGskManifold::ContactPoint& feature,
	hkGskManifoldConvertOut& out);
// Adds newPoint to the manifold in slot 0, getting it a contact point id from mgr; a full manifold first drops its
// redundant point.
hkpGskManifoldAddStatus hkGskManifold_addPoint(hkpGskManifold& manifold, const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& output, const hkpGskCache& newPoint,
	hkpProcessCdPoint* newCdPointInResultArray, hkpProcessCdPoint* resultPointArray, hkpContactMgr* mgr,
	hkpGskManifoldUtilMgrHandling mgrHandling);
