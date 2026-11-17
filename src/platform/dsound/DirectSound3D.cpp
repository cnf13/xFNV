// DirectSound's 3D model for the audio layer (DirectSound.h): what a mono 3D buffer's settings and the listener's
// make of it, as DirectSound's software 3D (DS3DALG_NO_VIRTUALIZATION) does it for two speakers.
// - Distance: full volume inside the minimum distance; beyond it the gain is min / (min + rolloff * (d - min)) (min/d
//   at rolloff 1: 6 dB per doubling of the distance), held at its value at the maximum distance beyond that, or
//   silent there with DSBCAPS_MUTE3DATMAXDISTANCE.
// - Pan: from the source's direction against the listener's right vector. With s the sideways part of the direction
//   (-1 left, 0 ahead or behind, +1 right) the near ear stays at full level and the far ear has the power 1 - |s|
//   (5 dB down at 45 degrees, 12 dB at 70, silent only straight to the side), given as a DirectSound pan in mB.
//   A pan linear in the angle, Wine's, is 50 dB down at 45 degrees: a voice a little off centre came from one ear.
// - Doppler: the frequency ratio (c - vListener) / (c - vSource) of the velocities along the line from source to
//   listener, in metres per second (the distance factor) times the Doppler factor; c is 343.3 m/s.
// DS3DMODE_HEADRELATIVE takes the source's position and velocity in the listener's own axes; DS3DMODE_DISABLE plays
// the sound centred at full volume.
#include "platform/dsound/DirectSound.h"

#include <algorithm>
#include <cmath>

namespace audio::ds {

namespace {

struct Vector {
	float x;
	float y;
	float z;
};

Vector From(const D3DVECTOR& v) {
	return Vector{v.x, v.y, v.z};
}

Vector Minus(Vector a, Vector b) {
	return Vector{a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector Scaled(Vector a, float s) {
	return Vector{a.x * s, a.y * s, a.z * s};
}

float Dot(Vector a, Vector b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// DirectSound's axes are left-handed (x right, y up, z ahead): right = top x front.
Vector Cross(Vector a, Vector b) {
	return Vector{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float Length(Vector a) {
	return std::sqrt(Dot(a, a));
}

Vector Normalised(Vector a, Vector fallback) {
	const float length = Length(a);
	return length > 1e-12f ? Scaled(a, 1.0f / length) : fallback;
}

constexpr float kSpeedOfSound = 343.3f;

} // namespace

Spatial Compute(const ListenerParams& listener, const SourceParams& source, bool muteAtMax) {
	Spatial out;
	if (source.mode == DS3DMODE_DISABLE) {
		return out;
	}
	// The listener's axes: front, top made perpendicular to it, right.
	Vector front = Normalised(From(listener.front), Vector{0.0f, 0.0f, 1.0f});
	Vector top = From(listener.top);
	top = Normalised(Minus(top, Scaled(front, Dot(top, front))), Vector{0.0f, 1.0f, 0.0f});
	Vector right = Normalised(Cross(top, front), Vector{1.0f, 0.0f, 0.0f});
	Vector offset;
	Vector listenerVelocity;
	if (source.mode == DS3DMODE_HEADRELATIVE) {
		offset = From(source.position);
		front = Vector{0.0f, 0.0f, 1.0f};
		right = Vector{1.0f, 0.0f, 0.0f};
		listenerVelocity = Vector{0.0f, 0.0f, 0.0f};
	} else {
		offset = Minus(From(source.position), From(listener.position));
		listenerVelocity = From(listener.velocity);
	}
	const float distance = Length(offset);

	const float minDistance = source.minDistance;
	const float maxDistance = std::max(source.maxDistance, minDistance);
	if (muteAtMax && distance > maxDistance) {
		out.gain = 0.0f;
	} else {
		const float clamped = std::clamp(distance, minDistance, maxDistance);
		out.gain = minDistance / (minDistance + listener.rolloffFactor * (clamped - minDistance));
	}

	if (distance > 1e-6f) {
		const float side = std::clamp(Dot(offset, right) / distance, -1.0f, 1.0f);
		const float farPower = 1.0f - std::fabs(side);
		const float attenuation = farPower > 1e-10f ? std::min(-1000.0f * std::log10(farPower), 10000.0f) : 10000.0f;
		out.pan = static_cast<int32_t>(std::lround(side < 0.0f ? -attenuation : attenuation));
		// Doppler along the line from the source to the listener.
		const Vector toListener = Scaled(offset, -1.0f / distance);
		const float scale = listener.distanceFactor * listener.dopplerFactor;
		const float limit = kSpeedOfSound * 0.5f;
		const float sourceSpeed = std::clamp(Dot(From(source.velocity), toListener) * scale, -limit, limit);
		const float listenerSpeed = std::clamp(Dot(listenerVelocity, toListener) * scale, -limit, limit);
		out.ratio = (kSpeedOfSound - listenerSpeed) / (kSpeedOfSound - sourceSpeed);
	}
	return out;
}

} // namespace audio::ds
