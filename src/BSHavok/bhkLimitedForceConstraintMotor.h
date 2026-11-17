#pragma once

class hkpLimitedForceConstraintMotor;
class NiStream;

// The load and save every Havok motor shares for its force limits: a tag class with static helpers and no data of its
// own.
class bhkLimitedForceConstraintMotor {
public:
	// Reads the motor's minimum, then its maximum force, each one 4-byte component of the stream.
	static void LoadBinary(NiStream& arStream, hkpLimitedForceConstraintMotor& arMotor);
	// Writes the motor's minimum, then its maximum force, each one 4-byte component of the stream.
	static void SaveBinary(NiStream& arStream, hkpLimitedForceConstraintMotor& arMotor);
};
