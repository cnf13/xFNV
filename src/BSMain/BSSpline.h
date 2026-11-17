#pragma once

// A Hermite spline's parameter and its four blend factors (PathingSpline's first member, Spline). The value at the
// parameter is the blend of two end values and their tangents.
class BSSpline {
public:
	// The blend of the end values and tangents at the current parameter.
	template <typename T>
	T Interpolate(const T& aStartPoint, const T& aEndPoint, const T& aStartTangent, const T& aEndTangent) const;

	float fParameter;
	float fH00;
	float fH10;
	float fH01;
	float fH11;
};
// The Hermite blend: each end value and tangent weighted by its factor, summed in this order.
template <typename T>
T BSSpline::Interpolate(const T& aStartPoint, const T& aEndPoint, const T& aStartTangent, const T& aEndTangent) const {
	return fH00 * aStartPoint + fH10 * aStartTangent + fH01 * aEndPoint + fH11 * aEndTangent;
}
