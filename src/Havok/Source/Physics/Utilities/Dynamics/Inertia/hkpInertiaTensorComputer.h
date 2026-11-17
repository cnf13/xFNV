#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkGeometry;
class hkStridedVertices;
class hkpConstraintInstance;
class hkpRigidBody;
class hkpRigidBodyCinfo;
class hkpShape;

// A body's volume, mass, centre of mass and inertia tensor about that centre.
struct hkpMassProperties {
	// The copy assignment, out of line in the exe (thiscall, ret 4, returns this, 0xCF0820).
	hkpMassProperties& operator=(const hkpMassProperties& other);
	// Not named in the PDB or IDA; name chosen here (the PDB's scaleToDensity and scaleToMass
	// do other arithmetic). Multiplies m_mass and the inertia tensor by afScale. this comes in eax, afScale on the
	// stack (cdecl, 0xCEBFD0).
	void scaleMassAndInertia(float afScale);
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	void scaleToMass(float newMass);
	void scaleToDensity(float density);

	// Havok SDK signature (hkpUtilities.lib, tied by target).
	static const hkClass& staticClass();

	float m_volume;
	float m_mass;
	hkVector4 m_centerOfMass;
	hkMatrix3 m_inertiaTensor;
};

// Havok SDK: the file-local helper of hkpInertiaTensorComputer::computeShapeVolumeMassProperties. Adds the mass
// properties of shape, placed at transform with a mass equal to its volume, into massProperties, walking containers
// and wrapper shapes; surfaceThickness is the least thickness given to triangles.
void computeRecursiveShapeVolumeMassProperties(const hkpShape* shape, const hkTransform& transform,
	float minTriangleThickness, hkpMassProperties& massPropertiesOut);
// Havok SDK name; not in the PDB.
// Inferred type: a1: float: used as one at 0xced280
void clipInertia(float a1, float* a2);
// Havok SDK: the file-local helper of hkpInertiaTensorComputer::optimizeInertiasOfConstraintTree. Moves the
// constraints touching body out of constraints, recurses into the bodies on their other side, raises body's
// diagonal inertia towards their sum (bounded by inertiaFactorHint times its largest) and returns the inertia it
// stands for; 0 for a fixed or keyframed body (cdecl, the result in ST0, 0xCEDEC0).
float hkInertiaTensorComputer_optimizeInertiasOfConstraintTreeInt(hkArray<const hkpConstraintInstance*,
																	  hkContainerHeapAllocator>& constraints,
	hkpRigidBody* body, float inertiaFactorHint);

// One part of a compound body: its mass properties and where it sits in the body.
struct hkpMassElement {
	hkpMassProperties m_properties;
	hkTransform m_transform;
};

// The SDK's file-local helper of hkpInertiaTensorComputer.cpp: Mirtich's polyhedral mass-property integrals over a
// triangle mesh (projection, face and volume integrals). A, B and C are the axes of the current face's projection
// (C the one along its largest normal component); T0 is the volume, T1 the first, T2 the second and TP the product
// moments.
struct InternalInertiaTensorComputer {
	// The projection integrals P1..Pbbb of the triangle vertices[0..2].
	void compProjectionIntegrals(const hkVector4* v);
	// The face integrals Fa..Fcca of the triangle vertices[0..2] with the given normal.
	void compFaceIntegrals(const hkVector4* v, const hkVector4& n);
	// The volume integrals T0..TP over geom's triangles, each vertex moved by shift.
	void compVolumeIntegrals(const hkGeometry* geom, const hkVector4& shift);
	// The centre of mass and the inertia tensor about it from the volume integrals, for a body
	// of the given mass and density.
	void computeInertialTensorInternal(float mass, float density, hkVector4& r, hkMatrix3& J);

	// The centre and half extents of the AABB of numVertices vertices.
	static void compGeometryAabb(const hkVector4* vertices, int32_t numVerts, hkVector4& centerOut,
		hkVector4& aabbExtentsOut);

	int32_t A;
	int32_t B;
	int32_t C;
	float P1;
	float Pa;
	float Pb;
	float Paa;
	float Pab;
	float Pbb;
	float Paaa;
	float Paab;
	float Pabb;
	float Pbbb;
	float Fa;
	float Fb;
	float Fc;
	float Faa;
	float Fbb;
	float Fcc;
	float Faaa;
	float Fbbb;
	float Fccc;
	float Faab;
	float Fbbc;
	float Fcca;
	float T0;
	float T1[3];
	float T2[3];
	float TP[3];
};

// The SDK's mass-property helpers: they fill in a body's volume, mass and inertia tensor from a shape.
class hkpInertiaTensorComputer {
public:
	// Fills apMassProperties for apShape with the given volume.
	static void computeShapeVolumeMassProperties(const hkpShape* shape, float mass, hkpMassProperties& result);
	// Moves arInertia, taken about the centre of mass, to a point arShift away from it, for a
	// body of mass afMass.
	static void shiftInertiaFromCom(const hkVector4& shift, float mass, hkMatrix3& inertia);
	// Moves inertia, taken about a point shift away from the centre of mass, to the centre of mass (cdecl,
	// 0xCEBE80). The exe leaves &inertia in eax; every caller ignores it.
	static void shiftInertiaToCom(const hkVector4& shift, float mass, hkMatrix3& inertia);
	// The mass properties of a solid sphere (cdecl, ret 0, hkResult in eax, 0xCEBA60).
	static hkResult computeSphereVolumeMassProperties(float radius, float mass, hkpMassProperties& result);
	// The mass properties of a convex hull of the vertices grown by radius (cdecl, ret 0,
	// hkResult in eax, 0xCEBBB0).
	static hkResult computeConvexHullMassProperties(const hkStridedVertices& vertices, float radius,
		hkpMassProperties& result);
	// The mass properties of a triangle thickened to surfaceThickness (cdecl, ret 0, hkResult in eax, 0xCEE290).
	static hkResult computeTriangleSurfaceMassProperties(const hkVector4& v0, const hkVector4& v1, const hkVector4& v2,
		float mass, float surfaceThickness, hkpMassProperties& result);
	// The mass properties of a solid capsule (cdecl, ret 0, hkResult in eax, 0xCEECA0).
	static hkResult computeCapsuleVolumeMassProperties(const hkVector4& startAxis, const hkVector4& endAxis,
		float radius, float mass, hkpMassProperties& result);
	// The mass properties of a solid cylinder (cdecl, ret 0, hkResult in eax, 0xCEF6D0).
	static hkResult computeCylinderVolumeMassProperties(const hkVector4& startAxis, const hkVector4& endAxis,
		float radius, float mass, hkpMassProperties& result);
	// The mass properties of a solid box of the given half extents and mass. Returns hkResult:
	// 1 when the mass is not positive (nothing written), else 0.
	static hkResult computeBoxVolumeMassProperties(const hkVector4& halfExtents, float mass, hkpMassProperties& result);
	// The mass properties of the closed triangle mesh geom as a solid of the given mass (1 when not positive). Where
	// the mesh encloses no volume, those of its AABB as a box.
	static void computeGeometryVolumeMassProperties(const hkGeometry* geom, float mass, hkpMassProperties& result);
	// computeGeometryVolumeMassProperties, but returns hkResult 1 (nothing written) when the mass is not positive or
	// the mesh encloses no volume, else 0.
	static hkResult computeGeometryVolumeMassPropertiesChecked(const hkGeometry* geom, float mass,
		hkpMassProperties& result);
	// The mass properties of the elements combined into one body. Returns hkResult: 1 when there are no elements or
	// their total mass is not positive (nothing written), else 0.
	static hkResult combineMassProperties(const hkArray<hkpMassElement>& elements, hkpMassProperties& result);
	// The mass properties of a hollow sphere whose shell is surfaceThickness thick: the solid sphere's minus the inner
	// one's, scaled to mass. Returns hkResult: 1 when mass or the thickness is not positive or the thickness is not
	// below radius (nothing written), else 0.
	static hkResult computeSphereSurfaceMassProperties(float radius, float mass, float surfaceThickness,
		hkpMassProperties& result);
	// The same for a hollow box of the given half extents.
	static hkResult computeBoxSurfaceMassProperties(const hkVector4& halfExtents, float mass, float surfaceThickness,
		hkpMassProperties& result);
	// Replaces inertia by the diagonal matrix of its largest diagonal element.
	static void simplifyInertiaTensorToOrientedParticle(hkMatrix3& inertia);
	// Up to five Jacobi rotations of inertia, accumulated into principleAxisOut, whose columns are then made
	// orthonormal again.
	static void convertInertiaTensorToPrincipleAxis(hkMatrix3& inertia, hkRotation& principleAxisOut);
	// The mass properties of numVertices point masses of equal mass, striding bytes apart in vertexIn. Returns
	// hkResult 1 when there are none (nothing written), else 0.
	static hkResult computeVertexCloudMassProperties(const float* vertexIn, int32_t striding, int32_t numVertices,
		float mass, hkpMassProperties& result);
	// The mass properties of geom's triangles as plates surfaceThickness thick, the mass shared by area when
	// distributeUniformly is set, else equally. Returns hkResult 1 when mass or the thickness is not positive
	// (nothing written), else 0; result's volume is not written.
	static hkResult computeGeometrySurfaceMassProperties(const hkGeometry* geom, float surfaceThickness,
		hkBool distributeUniformly, float mass, hkpMassProperties& result);
	// The eigenvectors of inertiaTensor along which pShape's AABB, in the frame whose rows are those eigenvectors, is
	// longest (majorAxis) and second longest (minorAxis).
	static void calculatePrincipalAxis(const hkpShape* pShape, const hkMatrix3& inertiaTensor, hkVector4& majorAxis,
		hkVector4& minorAxis);
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	static void setAndScaleToMass(const hkpMassProperties& props, float mass, hkpRigidBodyCinfo& bodyInfo);
	// Havok SDK: props' mass, centre of mass and inertia tensor into bodyInfo.
	static void setMassProperties(const hkpMassProperties& massProperties, hkpRigidBodyCinfo& bodyInfo);
	// Havok SDK: setAndScaleToMass with props' volume times density.
	static void setAndScaleToDensity(const hkpMassProperties& props, float density, hkpRigidBodyCinfo& bodyInfo);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes).
	static hkResult computeVertexHullVolumeMassProperties(const float* vertexIn, int32_t striding, int32_t numVertices,
		float mass, hkpMassProperties& result);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes).
	static void optimizeInertiasOfConstraintTree(hkpConstraintInstance* const* constraints, int32_t numConstraints,
		hkpRigidBody* rootBody, float inertiaFactorHint);
};

inline float f0_5_ = 0.5f;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpMassPropertiesTypeInfo;
extern hkClass hkpMassPropertiesClass;
