#pragma once

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCollisionInput;

// Inferred type: bodyA: hkpCdBody* from 0xd33c35 (parameter 0 of hkpConvexListAgent::staticGetPenetrations); bodyB:
// hkpCdBody* from 0xd33c35 (parameter 1 of hkpConvexListAgent::staticGetPenetrations); input: hkpCollisionInput* from
// 0xd33c35 (parameter 2 of hkpConvexListAgent::staticGetPenetrations)
void staticGetPenetrationsFunc(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCdBodyPairCollector& collector);
