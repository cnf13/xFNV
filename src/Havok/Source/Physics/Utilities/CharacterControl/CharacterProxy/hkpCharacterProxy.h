#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"

class hkpAllCdPointCollector;
class hkpCharacterProxyCinfo;
struct hkpCharacterProxyInteractionResults;
class hkpCharacterProxyListener;
class hkpLinearCastInput;
class hkpRigidBody;
struct hkpRootCdPoint;
struct hkpSimplexSolverInput;
struct hkpSimplexSolverOutput;
class hkpSurfaceConstraintInfo;
class hkpSurfaceInfo;
class hkStatisticsCollector;
struct hkStepInfo;

// When the plane at index r_eax in r_edi faces up by more than 0.01 but less than maxSlopeCos, appends a copy whose
// normal is that plane's part across up, normalized; returns whether it appended one.
bool addMaxSlopePlane(float maxSlopeCos, const hkVector4& up, int32_t r_eax,
	hkArray<hkpSurfaceConstraintInfo, hkContainerHeapAllocator>& r_edi);

// What the proxy's multithreaded integrate writes the object interactions it applied into: the start of an array of
// hkpCharacterProxyInteractionResults, of which at most the integrate's maxInteractions entries are filled.
struct hkpCharacterProxyIntegrateCommand {
	hkpCharacterProxyInteractionResults* m_objectInteraction;
};

// The deprecated form of hkpSurfaceInfo that checkSupportDeprecated fills: whether the surface supports the character
// and whether it slides, and the surface's normal and velocity.
struct hkpSurfaceInfoDeprecated {
	hkBool m_isSupported;
	hkBool m_isSliding;
	hkVector4 m_surfaceNormal;
	hkVector4 m_surfaceVelocity;
};

// A character's collision body: a phantom that is swept through the world, the velocity it is moved with and the
// contact points it currently rests on, the listeners, bodies and phantoms it knows and its slope and recovery
// settings.
class hkpCharacterProxy : public hkReferencedObject, public hkpEntityListener, public hkpPhantomListener {
public:
	// The property key the proxy binds itself to on its shape phantom.
	enum : uint32_t {
		PROPERTY_KEY_CHARACTER_PROXY = 0x1300
	};

	hkpCharacterProxy(const hkpCharacterProxyCinfo& arInfo);
	~hkpCharacterProxy() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void entityRemovedCallback(hkpEntity* entity) override;
	void phantomRemovedCallback(hkpPhantom* phantom) override;

	virtual void updateManifold(const hkpAllCdPointCollector& startPointCollector,
		const hkpAllCdPointCollector& castCollector, hkBool isMultithreaded);
	virtual void extractSurfaceConstraintInfo(const hkpRootCdPoint& hit, hkpSurfaceConstraintInfo& surface,
		float timeTravelled);

	// The distance between the two contact points along the surface.
	float surfaceDistance(const hkpRootCdPoint& p1, const hkpRootCdPoint& p2) const;
	// The velocity the proxy is moved with.
	void setVelocity(const hkVector4* avVelocity);
	const hkVector4& getLinearVelocity() const;
	// The position of the shape phantom.
	const hkVector4& getPosition() const;
	// Moves the shape phantom to aPosition, with the keep distance and contact tolerance as extra AABB tolerance.
	void setPosition(const hkVector4& position);
	// Moves the proxy one step with the given collectors.
	void integrateWithCollectors(const hkStepInfo& stepInfo, const hkVector4& gravity,
		hkpAllCdPointCollector& castCollector, hkpAllCdPointCollector& startPointCollector);
	// The proxy's step. A non-null command makes it the multithreaded form, which
	// records at most maxInteractions object impulses there.
	void integrateImplementation(const hkStepInfo& stepInfo, const hkVector4& gravity,
		hkpAllCdPointCollector& castCollector, hkpAllCdPointCollector& startPointCollector,
		hkpCharacterProxyIntegrateCommand* command, int32_t maxInteractions);
	int32_t findSurface(const hkpRootCdPoint& a1) const;

	// Copies the creation info into the proxy: the settings, the normalized up vector and the shape phantom, which
	// also loses its old listeners and the proxy's own property.
	void updateFromCinfo(const hkpCharacterProxyCinfo& info);

	// The remaining members are declared only. The const getShapePhantom overload is left out.
	void getCinfo(hkpCharacterProxyCinfo& info) const;
	void integrate(const hkStepInfo& stepInfo, const hkVector4& worldGravity);
	void integrateMultithreaded(const hkStepInfo& stepInfo, const hkVector4& gravity,
		hkpCharacterProxyIntegrateCommand* command, int32_t maxInteractions);
	void checkSupport(const hkVector4& direction, hkpSurfaceInfo& ground);
	void checkSupportWithCollector(const hkVector4& direction, hkpSurfaceInfo& ground,
		hkpAllCdPointCollector& startPointCollector);
	void refreshManifold(hkpAllCdPointCollector& startPointCollector);
	const hkArray<hkpRootCdPoint>& getManifold() const;
	void setLinearVelocity(const hkVector4& vel);
	hkpShapePhantom* getShapePhantom();
	void checkSupportDeprecated(const hkVector4& direction, hkpSurfaceInfoDeprecated& ground);
	void addCharacterProxyListener(hkpCharacterProxyListener* cpl);
	void removeCharacterProxyListener(hkpCharacterProxyListener* cpl);
	void linearCast(hkBool doWorldCast, const hkpLinearCastInput& arInput, hkpAllCdPointCollector& castCollector,
		hkpAllCdPointCollector* startCollector);
	// Applies the manifold's contacts to the bodies and characters they touch.
	void applySurfaceInteractions(const hkStepInfo& stepInfo, const hkVector4& worldGravity,
		hkpCharacterProxyIntegrateCommand* command, int32_t maxInteractions);
	void extractSurfaceVelocity(const hkpRootCdPoint& hit, hkVector4& velocityOut);
	void convertFractionToDistance(const hkpRootCdPoint* cHit, int32_t numHits, const hkVector4& displacement);
	void fireContactAdded(const hkpRootCdPoint& point);
	void fireContactRemoved(const hkpRootCdPoint& point);
	void fireConstraintsProcessed(const hkArray<hkpRootCdPoint>& manifold, hkpSimplexSolverInput& input);
	float moveToLinearCastHitPosition(const hkpSimplexSolverOutput& output, const hkpAllCdPointCollector& castCollector,
		const hkpLinearCastInput& castInput, hkVector4& position);

	// DeletingDestruct's free (the thread's Havok heap): names one of the three bases' operator deletes.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkVector4 m_velocity;
	hkVector4 m_oldDisplacement;
	hkpShapePhantom* m_shapePhantom;
	float m_dynamicFriction;
	float m_staticFriction;
	// The gap before m_up is hkVector4 alignment padding; there is no member there.
	hkVector4 m_up;
	float m_extraUpStaticFriction;
	float m_extraDownStaticFriction;
	float m_keepDistance;
	float m_keepContactTolerance;
	float m_contactAngleSensitivity;
	int32_t m_userPlanes;
	float m_maxCharacterSpeedForSolver;
	float m_characterStrength;
	float m_characterMass;
	hkArray<hkpRootCdPoint> m_manifold;
	hkArray<hkpCharacterProxyListener*> m_listeners;
	hkArray<hkpRigidBody*> m_bodies;
	hkArray<hkpPhantom*> m_phantoms;
	float m_maxSlopeCosine;
	float m_penetrationRecoverySpeed;
	int32_t m_maxCastIterations;
	bool m_refreshManifoldInCheckSupport;
};
