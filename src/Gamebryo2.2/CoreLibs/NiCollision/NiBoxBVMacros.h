#pragma once

// The separating-axis tests of the box/triangle sweep (Gamebryo's NiBoxBVMacros.h). p is the projected distance, t
// the time, w the projected velocity, r the box's projected radius, d0/d1/d the projected triangle edges. The exe's
// compares follow these C++ conditions exactly, NaN included (checked in NiBoxBV::BoxTriTestIntersect, 0xEA2190).
// BT_TEST0 and BT_FIND0 are not here: the exe inlines their N.Dot(W), so their callers write them out.

#define BT_TEST1(p, t, w, d0, d1, r) \
	{ \
		if ((p) > (r)) { \
			float fMin; \
			if ((d0) > 0.0f) { \
				if ((d1) >= 0.0f) { \
					if ((p) + ((t) * (w)) > (r)) \
						return false; \
				} else { \
					fMin = (p) + (d1); \
					if ((fMin > (r)) && (fMin + ((t) * (w)) > (r))) \
						return false; \
				} \
			} else if ((d1) <= (d0)) { \
				fMin = (p) + (d1); \
				if ((fMin > (r)) && (fMin + ((t) * (w)) > (r))) \
					return false; \
			} else { \
				fMin = (p) + (d0); \
				if ((fMin > (r)) && (fMin + ((t) * (w)) > (r))) \
					return false; \
			} \
		} else if ((p) < -(r)) { \
			float fMax; \
			if ((d0) <= 0.0f) { \
				if ((d1) <= 0.0f) { \
					if ((p) + ((t) * (w)) < -(r)) \
						return false; \
				} else { \
					fMax = (p) + (d1); \
					if ((fMax < -(r)) && (fMax + ((t) * (w)) < -(r))) \
						return false; \
				} \
			} else if ((d1) >= (d0)) { \
				fMax = (p) + (d1); \
				if ((fMax < -(r)) && (fMax + ((t) * (w)) < -(r))) \
					return false; \
			} else { \
				fMax = (p) + (d0); \
				if ((fMax < -(r)) && (fMax + ((t) * (w)) < -(r))) \
					return false; \
			} \
		} \
	}

#define BT_TEST2(p, t, w, d, r) \
	{ \
		if ((p) > (r)) { \
			if ((d) >= 0.0f) { \
				if ((p) + ((t) * (w)) > (r)) \
					return false; \
			} else { \
				float fMin = (p) + (d); \
				if ((fMin > (r)) && (fMin + ((t) * (w)) > (r))) \
					return false; \
			} \
		} else if ((p) < -(r)) { \
			if ((d) <= 0.0f) { \
				if ((p) + ((t) * (w)) < -(r)) \
					return false; \
			} else { \
				float fMax = (p) + (d); \
				if ((fMax < -(r)) && (fMax + ((t) * (w)) < -(r))) \
					return false; \
			} \
		} \
	}

// The BT_TEST tests of NiBoxBV::BoxTriFindIntersect, which also keep the latest time of first contact: tmax, the
// separating axis type, the triangle vertex extr that meets the box first (0, 1 or 2) and the box side (1 or -1).
#define BT_FIND1(p, t, w, d0, d1, r, tmax, type, extr, side, axis) \
	{ \
		float fTmp; \
		if ((p) > (r)) { \
			float fMin; \
			if ((d0) > 0.0f) { \
				if ((d1) >= 0.0f) { \
					if ((p) + ((t) * (w)) > (r)) \
						return false; \
					fTmp = ((r) - (p)) / (w); \
					if (fTmp > (tmax)) { \
						(tmax) = fTmp; \
						(type) = (axis); \
						(extr) = 0; \
						(side) = 1; \
					} \
				} else { \
					fMin = (p) + (d1); \
					if (fMin > (r)) { \
						if (fMin + ((t) * (w)) > (r)) \
							return false; \
						fTmp = ((r) - fMin) / (w); \
						if (fTmp > (tmax)) { \
							(tmax) = fTmp; \
							(type) = (axis); \
							(extr) = 2; \
							(side) = 1; \
						} \
					} \
				} \
			} else if ((d1) <= (d0)) { \
				fMin = (p) + (d1); \
				if (fMin > (r)) { \
					if (fMin + ((t) * (w)) > (r)) \
						return false; \
					fTmp = ((r) - fMin) / (w); \
					if (fTmp > (tmax)) { \
						(tmax) = fTmp; \
						(type) = (axis); \
						(extr) = 2; \
						(side) = 1; \
					} \
				} \
			} else { \
				fMin = (p) + (d0); \
				if (fMin > (r)) { \
					if (fMin + ((t) * (w)) > (r)) \
						return false; \
					fTmp = ((r) - fMin) / (w); \
					if (fTmp > (tmax)) { \
						(tmax) = fTmp; \
						(type) = (axis); \
						(extr) = 1; \
						(side) = 1; \
					} \
				} \
			} \
		} else if ((p) < -(r)) { \
			float fMax; \
			if ((d0) <= 0.0f) { \
				if ((d1) <= 0.0f) { \
					if ((p) + ((t) * (w)) < -(r)) \
						return false; \
					fTmp = -((r) + (p)) / (w); \
					if (fTmp > (tmax)) { \
						(tmax) = fTmp; \
						(type) = (axis); \
						(extr) = 0; \
						(side) = -1; \
					} \
				} else { \
					fMax = (p) + (d1); \
					if (fMax < -(r)) { \
						if (fMax + ((t) * (w)) < -(r)) \
							return false; \
						fTmp = -((r) + fMax) / (w); \
						if (fTmp > (tmax)) { \
							(tmax) = fTmp; \
							(type) = (axis); \
							(extr) = 2; \
							(side) = -1; \
						} \
					} \
				} \
			} else if ((d1) >= (d0)) { \
				fMax = (p) + (d1); \
				if (fMax < -(r)) { \
					if (fMax + ((t) * (w)) < -(r)) \
						return false; \
					fTmp = -((r) + fMax) / (w); \
					if (fTmp > (tmax)) { \
						(tmax) = fTmp; \
						(type) = (axis); \
						(extr) = 2; \
						(side) = -1; \
					} \
				} \
			} else { \
				fMax = (p) + (d0); \
				if (fMax < -(r)) { \
					if (fMax + ((t) * (w)) < -(r)) \
						return false; \
					fTmp = -((r) + fMax) / (w); \
					if (fTmp > (tmax)) { \
						(tmax) = fTmp; \
						(type) = (axis); \
						(extr) = 1; \
						(side) = -1; \
					} \
				} \
			} \
		} \
	}

#define BT_FIND2(p, t, w, d, r, tmax, type, extr, side, axis) \
	{ \
		float fTmp; \
		if ((p) > (r)) { \
			if ((d) >= 0.0f) { \
				if ((p) + ((t) * (w)) > (r)) \
					return false; \
				fTmp = ((r) - (p)) / (w); \
				if (fTmp > (tmax)) { \
					(tmax) = fTmp; \
					(type) = (axis); \
					(extr) = 0; \
					(side) = 1; \
				} \
			} else { \
				float fMin = (p) + (d); \
				if (fMin > (r)) { \
					if (fMin + ((t) * (w)) > (r)) \
						return false; \
					fTmp = ((r) - fMin) / (w); \
					if (fTmp > (tmax)) { \
						(tmax) = fTmp; \
						(type) = (axis); \
						(extr) = 1; \
						(side) = 1; \
					} \
				} \
			} \
		} else if ((p) < -(r)) { \
			if ((d) <= 0.0f) { \
				if ((p) + ((t) * (w)) < -(r)) \
					return false; \
				fTmp = -((r) + (p)) / (w); \
				if (fTmp > (tmax)) { \
					(tmax) = fTmp; \
					(type) = (axis); \
					(extr) = 0; \
					(side) = -1; \
				} \
			} else { \
				float fMax = (p) + (d); \
				if (fMax < -(r)) { \
					if (fMax + ((t) * (w)) < -(r)) \
						return false; \
					fTmp = -((r) + fMax) / (w); \
					if (fTmp > (tmax)) { \
						(tmax) = fTmp; \
						(type) = (axis); \
						(extr) = 1; \
						(side) = -1; \
					} \
				} \
			} \
		} \
	}

// A box coordinate of the contact point: side times the extent, signed by the projection cmat (zero when it is 0).
#define BT_GET_COEFF_PLUS_MINUS(coeff, side, cmat, ext) \
	{ \
		if ((cmat) > 0.0f) \
			(coeff) = (side) * (ext); \
		else if ((cmat) < 0.0f) \
			(coeff) = -(side) * (ext); \
		else \
			(coeff) = 0.0f; \
	}

#define BT_GET_COEFF_MINUS_PLUS(coeff, side, cmat, ext) \
	{ \
		if ((cmat) > 0.0f) \
			(coeff) = -(side) * (ext); \
		else if ((cmat) < 0.0f) \
			(coeff) = (side) * (ext); \
		else \
			(coeff) = 0.0f; \
	}

// P = the box's center moved by T * V, plus the box coordinates x along its axes. Gamebryo loops over NiPoint3's
// operator[]; the exe inlines it, so the components are written out, in the source's order of sums.
#define BT_GET_POINT(P, box, T, V, x) \
	{ \
		const NiPoint3& kPtCenter = (box).m_kBox.m_kCenter; \
		const NiPoint3* pkPtAxis = (box).m_kBox.m_akAxis; \
		(P).x = kPtCenter.x + (T) * (V).x + (x)[0] * pkPtAxis[0].x + (x)[1] * pkPtAxis[1].x + (x)[2] * pkPtAxis[2].x; \
		(P).y = kPtCenter.y + (T) * (V).y + (x)[0] * pkPtAxis[0].y + (x)[1] * pkPtAxis[1].y + (x)[2] * pkPtAxis[2].y; \
		(P).z = kPtCenter.z + (T) * (V).z + (x)[0] * pkPtAxis[0].z + (x)[1] * pkPtAxis[1].z + (x)[2] * pkPtAxis[2].z; \
	}

// The box/box sweep's tests (NiBoxBV::BoxBoxFindIntersect). p is the projection of the centres' offset now, q (or
// q0 * q1 - q2 * q3) its projection at time t, r the boxes' summed projected radii. An axis that separates the boxes
// over the whole sweep returns false; otherwise the time they first touch along it becomes tmax, with the axis and
// the side, when it is later than tmax by more than FLT_EPSILON (a double constant in the exe). tmp and q are floats,
// as the exe stores them.
#define BB_FIND0(p, q, r, t, tmax, type, side, axis) \
	{ \
		float tmp; \
		if ((p) > (r)) { \
			if ((q) > (r)) \
				return false; \
			tmp = (t) * ((r) - (p)) / ((q) - (p)); \
			if (tmp > (tmax) + 1.1920928955078125e-07) { \
				(tmax) = tmp; \
				(type) = (axis); \
				(side) = 1; \
			} \
		} else if ((p) < -(r)) { \
			if ((q) < -(r)) \
				return false; \
			tmp = -(t) * ((r) + (p)) / ((q) - (p)); \
			if (tmp > (tmax) + 1.1920928955078125e-07) { \
				(tmax) = tmp; \
				(type) = (axis); \
				(side) = -1; \
			} \
		} \
	}

#define BB_FIND1(p, q0, q1, q2, q3, r, t, tmax, type, side, axis) \
	{ \
		float q, tmp; \
		if ((p) > (r)) { \
			q = ((q0) * (q1)) - ((q2) * (q3)); \
			if (q > (r)) \
				return false; \
			tmp = (t) * ((r) - (p)) / (q - (p)); \
			if (tmp > (tmax) + 1.1920928955078125e-07) { \
				(tmax) = tmp; \
				(type) = (axis); \
				(side) = 1; \
			} \
		} else if ((p) < -(r)) { \
			q = ((q0) * (q1)) - ((q2) * (q3)); \
			if (q < -(r)) \
				return false; \
			tmp = -(t) * ((r) + (p)) / (q - (p)); \
			if (tmp > (tmax) + 1.1920928955078125e-07) { \
				(tmax) = tmp; \
				(type) = (axis); \
				(side) = -1; \
			} \
		} \
	}

// The box/box sweep's box coordinates and contact point: the same as the box/triangle ones above.
#define BB_GET_COEFF_PLUS_MINUS(coeff, side, cmat, ext) BT_GET_COEFF_PLUS_MINUS(coeff, side, cmat, ext)
#define BB_GET_COEFF_MINUS_PLUS(coeff, side, cmat, ext) BT_GET_COEFF_MINUS_PLUS(coeff, side, cmat, ext)

// P = the box's center moved by T * V, plus the box coordinates coeff along its axes, written out per component in
// the source's order of sums.
#define BB_GET_POINT(P, box, T, V, coeff) \
	{ \
		const NiPoint3& kPtCenter = (box).m_kBox.m_kCenter; \
		const NiPoint3* pkPtAxis = (box).m_kBox.m_akAxis; \
		(P).x = kPtCenter.x + (T) * (V).x + (coeff)[0] * pkPtAxis[0].x + (coeff)[1] * pkPtAxis[1].x + \
			(coeff)[2] * pkPtAxis[2].x; \
		(P).y = kPtCenter.y + (T) * (V).y + (coeff)[0] * pkPtAxis[0].y + (coeff)[1] * pkPtAxis[1].y + \
			(coeff)[2] * pkPtAxis[2].y; \
		(P).z = kPtCenter.z + (T) * (V).z + (coeff)[0] * pkPtAxis[0].z + (coeff)[1] * pkPtAxis[1].z + \
			(coeff)[2] * pkPtAxis[2].z; \
	}
