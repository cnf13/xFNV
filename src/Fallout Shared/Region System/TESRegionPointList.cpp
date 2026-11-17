#include "Fallout Shared/Region System/TESRegionPointList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Region System/TESRegionPoint.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/ehvec.h"

#include <bit>

TESRegionPointList::ScaleResult::ScaleResult() {}

bool TESRegionPointList::ScaleResult::Matches(TESRegionPoint* apPoint, float afDistance) {
	if (!apPoint) {
		return false;
	}
	return FloatEQ(afDistance, fdist, 0.001f) && *apPoint == point;
}

// The bounds start inverted (FLT_MAX / -FLT_MAX) so that the first point sets them.
TESRegionPointList::TESRegionPointList(bool abOwnsPointMemory)
	: BSSimpleList(), pLastScaleResult(nullptr), bOwnsPointMemory(abOwnsPointMemory),
	  Minimums(0x1.fffffe0000000p+127f, 0x1.fffffe0000000p+127f),
	  Maximums(-0x1.fffffe0000000p+127f, -0x1.fffffe0000000p+127f), iDistanceInsideAtMax(1024), iCount(0) {}

// The cached scale result is raw ourMalloc memory.
TESRegionPointList::~TESRegionPointList() {
	Clear();
	if (pLastScaleResult) {
		ourFree(pLastScaleResult);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool TESRegionPointList::Load(TESFile* apFile) {
	if (!apFile || apFile->GetTESChunk() != RPLD_ID) {
		return 0;
	}
	uint32_t auiMaxSize = apFile->GetChunkSize();
	uint32_t uiPoints = auiMaxSize >> 3;
	if (auiMaxSize % 8) {
		Warning("Invalid Region Point List data in file \"%s\".", &apFile->m_Filename);
		return 0;
	} else {
		if (auiMaxSize && uiPoints) {
			NiPoint2* pPoints;
			void* ptr = ourMalloc(uiPoints <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * uiPoints : SIZE_MAX);
			if (ptr) {
				rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint2), uiPoints, &emptyPointConstructor);
				pPoints = static_cast<NiPoint2*>(ptr);
			} else {
				pPoints = 0;
			}
			apFile->GetChunkData(pPoints, auiMaxSize);
			iCount = 0;
			for (uint32_t i = 0; i < uiPoints; ++i) {
				TESRegionPoint* pPoint;
				if (apFile->QEndian()) {
					ENDIAN<float>(pPoints[i].x, 0);
					ENDIAN<float>(pPoints[i].y, 0);
				}
				void* pMemory = ourMalloc(sizeof(TESRegionPoint));
				if (pMemory) {
					pPoint = ::new (pMemory) TESRegionPoint(pPoints[i]);
				} else {
					pPoint = 0;
				}
				if (static_cast<double>(Minimums.x) > pPoint->Point.x) {
					Minimums.x = pPoint->Point.x;
				}
				if (static_cast<double>(Minimums.y) > pPoint->Point.y) {
					Minimums.y = pPoint->Point.y;
				}
				if (static_cast<double>(Maximums.x) < pPoint->Point.x) {
					Maximums.x = pPoint->Point.x;
				}
				if (static_cast<double>(Maximums.y) < pPoint->Point.y) {
					Maximums.y = pPoint->Point.y;
				}
				AddHead(pPoint);
				++iCount;
			}
			ourFree(pPoints);
			return 1;
		} else {
			return 0;
		}
	}
}

bool TESRegionPointList::Validate(bool abMinPointsTest) {
	return (!abMinPointsTest || iCount >= 3) && !SelfIntersects(1);
}

void TESRegionPointList::SetDistanceInsideAtMax(uint32_t aiDistance) {
	iDistanceInsideAtMax = aiDistance;
}

// The exe reads iCount (0x59BB30), each node's next (0x726070) and item (0x6815C0) through folded one-field
// accessors.
bool TESRegionPointList::SelfIntersects(bool abClosePolygon) {
	bool bQuit = false;
	if (iCount < 4) {
		return false;
	}
	BSSimpleList<TESRegionPoint*>* pP1 = this;
	BSSimpleList<TESRegionPoint*>* pP2 = pP1->m_pkNext;
	BSSimpleList<TESRegionPoint*>* pR1 = pP2->m_pkNext;
	BSSimpleList<TESRegionPoint*>* pR2 = pR1->m_pkNext;
	while (pP2->m_pkNext) {
		pR1 = pP2->m_pkNext;
		if (pR1) {
			pR2 = pR1->m_pkNext;
		} else {
			pR2 = nullptr;
		}
		bQuit = false;
		do {
			if (!pR2) {
				// The closing edge runs from the last point back to the list's head.
				if (abClosePolygon && pP1 != this) {
					pR2 = this;
				}
				bQuit = true;
			}
			if (pP1 && pP2 && pR1 && pR2 &&
				TESRegionPoint::LineSegmentsIntersect(pP1->m_item, pP2->m_item, pR1->m_item, pR2->m_item)) {
				return true;
			}
			pR1 = pR2;
			if (pR2) {
				pR2 = pR2->m_pkNext;
			}
		} while (!bQuit);
		pP1 = pP2;
		pP2 = pP2->m_pkNext;
	}
	return false;
}

bool TESRegionPointList::Add(TESRegionPoint* apPoint, float afMaxDelta) {
	TESRegionPointList* v4;
	double distanceSquared;
	TESRegionPoint* v6;
	if (!apPoint) {
		return (bool)0;
	}
	if (m_item) {
		v6 = apPoint;
		v4 = this;
		distanceSquared = (double)v4->m_item->GetDistanceSquared(v6);
		if ((afMaxDelta * afMaxDelta) > distanceSquared) {
			return (bool)0;
		}
	}
	BSSimpleList<TESRegionPoint*>::AddHead(apPoint);
	if (SelfIntersects(0)) {
		BSSimpleList<TESRegionPoint*>::RemoveHead();
		return (bool)0;
	} else {
		++iCount;
		if (((double)Minimums.x) > apPoint->GetX()) {
			Minimums.x = apPoint->GetX();
		}
		if (((double)Minimums.y) > apPoint->GetY()) {
			Minimums.y = apPoint->GetY();
		}
		if (((double)Maximums.x) < apPoint->GetX()) {
			Maximums.x = apPoint->GetX();
		}
		if (((double)Maximums.y) < apPoint->GetY()) {
			Maximums.y = apPoint->GetY();
		}
		return (bool)1;
	}
}

bool TESRegionPointList::IsInsideRegion(TESRegionPoint* apTest) {
	BSSimpleList<TESRegionPoint*>* pnext;
	int32_t icrossings = 0;
	if (!apTest || iCount < 3) {
		return 0;
	}
	float fxtest = apTest->Point.x;
	float fytest = apTest->Point.y;
	if (static_cast<double>(fxtest) < GetMinX() ||
		(static_cast<double>(fxtest) > GetMaxX() ||
			(static_cast<double>(fytest) < GetMinY() || static_cast<double>(fytest) > GetMaxY()))) {
		return 0;
	}
	BSSimpleList<TESRegionPoint*>* pcurrent = this;
	do {
		pnext = pcurrent->m_pkNext;
		if (!pnext) {
			pnext = this;
		}
		TESRegionPoint* pcurrentItem = pcurrent->m_item;
		TESRegionPoint* pnextItem = pnext->m_item;
		float fxcurrent = pcurrentItem->Point.x;
		float fxnext = pnextItem->Point.x;
		if ((fxtest < static_cast<double>(fxcurrent) && fxtest < static_cast<double>(fxnext)) ||
			((fxtest < static_cast<double>(fxcurrent) || fxtest < static_cast<double>(fxnext)) &&
				static_cast<double>(fxtest) < TESRegionPoint::XIntercept(pcurrentItem, pnextItem, fytest))) {
			float fycurrent = pcurrentItem->Point.y;
			float fynext = pnextItem->Point.y;
			if ((fytest < static_cast<double>(fycurrent) && fynext <= static_cast<double>(fytest)) ||
				(fytest >= static_cast<double>(fycurrent) && fynext > static_cast<double>(fytest))) {
				++icrossings;
			}
		}
		pcurrent = pnext;
	} while (pnext != this);
	return icrossings % 2 != 0;
}

// The exe's cleanup-only C++ EH frame is left out.
void TESRegionPointList::Save() {
	TESForm::AddChunk(RPLI_ID, iDistanceInsideAtMax);
	BSSimpleList<TESRegionPoint*>* pList = this;
	if (!IsEmpty()) {
		NiPoint2* pPoints;
		uint32_t iCount = 0;
		uint32_t iResizes = 0;
		uint32_t iCurrSize = 1024;
		NiPoint2* ptr = static_cast<NiPoint2*>(ourMalloc(8192 * sizeof(NiPoint2)));
		if (ptr) {
			rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint2), 0x2000, &emptyPointConstructor);
			pPoints = ptr;
		} else {
			pPoints = 0;
		}
		void* pPoints_3 = pPoints;
		while (pList && !pList->IsEmpty()) {
			NiPoint2* v1 = &pList->m_item->Point;
			float y = v1->y;
			*(static_cast<uint32_t*>(pPoints_3) + 2 * iCount) = std::bit_cast<uint32_t>(v1->x);
			*(static_cast<float*>(pPoints_3) + (2 * iCount++ + 1)) = y;
			if (iCount >= iCurrSize) {
				NiPoint2* pPoints_4;
				iCurrSize = (++iResizes + 1) << 10;
				NiPoint2* pTemp = static_cast<NiPoint2*>(pPoints_3);
				NiPoint2* pPoints_2 =
					static_cast<NiPoint2*>(ourMalloc((iResizes + 1) << 13 <= SIZE_MAX / sizeof(NiPoint2)
							? sizeof(NiPoint2) * ((iResizes + 1) << 13)
							: SIZE_MAX));
				if (pPoints_2) {
					rt::eh_vector_constructor_iterator(pPoints_2, sizeof(NiPoint2), (iResizes + 1) << 13,
						&emptyPointConstructor);
					pPoints_4 = pPoints_2;
				} else {
					pPoints_4 = 0;
				}
				pPoints_3 = pPoints_4;
				ourMemcpy(pPoints_4, pTemp, sizeof(NiPoint2) * iCount);
				ourFree(pTemp);
			}
			pList = pList->m_pkNext;
		}
		if (iCount) {
			TESForm::AddChunkArray(RPLD_ID, static_cast<const uint32_t*>(pPoints_3), 2 * iCount);
		}
		ourFree(pPoints_3);
	}
}

TESRegionPointList* TESRegionPointList::Clear() {
	while (iCount) {
		TESRegionPoint* m_item = this->m_item;
		RemoveHead();
		--iCount;
		if (m_item) {
			if (bOwnsPointMemory) {
				ourFree(m_item);
			}
		}
	}
	iCount = 0;
	Minimums.y = 3.4028235e+38f;
	Minimums.x = 3.4028235e+38f;
	Maximums.y = -3.4028235e+38f;
	Maximums.x = -3.4028235e+38f;
	return this;
}

float TESRegionPointList::GetMinX() {
	float fCurMinX = 3.4028235e+38f;
	BSSimpleList<TESRegionPoint*>* pcurrent = this;
	if (!(Minimums.x < 3.4028234663852886e+38)) {
		while (pcurrent && pcurrent->m_item) {
			if (static_cast<double>(fCurMinX) > pcurrent->m_item->Point.x) {
				fCurMinX = pcurrent->m_item->Point.x;
			}
			pcurrent = pcurrent->m_pkNext;
		}
		return fCurMinX;
	} else {
		return Minimums.x;
	}
}

float TESRegionPointList::GetMaxX() {
	float fCurMaxX = -3.4028235e+38f;
	BSSimpleList<TESRegionPoint*>* pcurrent = this;
	if (!(Maximums.x > -3.4028234663852886e+38)) {
		while (pcurrent && pcurrent->m_item) {
			if (static_cast<double>(fCurMaxX) < pcurrent->m_item->Point.x) {
				fCurMaxX = pcurrent->m_item->Point.x;
			}
			pcurrent = pcurrent->m_pkNext;
		}
		return fCurMaxX;
	} else {
		return Maximums.x;
	}
}

float TESRegionPointList::GetMinY() {
	float fCurMinY = 3.4028235e+38f;
	BSSimpleList<TESRegionPoint*>* pcurrent = this;
	if (!(Minimums.y < 3.4028234663852886e+38)) {
		while (pcurrent && pcurrent->m_item) {
			if (static_cast<double>(fCurMinY) > pcurrent->m_item->Point.y) {
				fCurMinY = pcurrent->m_item->Point.y;
			}
			pcurrent = pcurrent->m_pkNext;
		}
		return fCurMinY;
	} else {
		return Minimums.y;
	}
}

float TESRegionPointList::GetMaxY() {
	float fCurMaxY = -3.4028235e+38f;
	BSSimpleList<TESRegionPoint*>* pcurrent = this;
	if (!(Maximums.y > -3.4028234663852886e+38)) {
		while (pcurrent && pcurrent->m_item) {
			if (static_cast<double>(fCurMaxY) < pcurrent->m_item->Point.y) {
				fCurMaxY = pcurrent->m_item->Point.y;
			}
			pcurrent = pcurrent->m_pkNext;
		}
		return fCurMaxY;
	} else {
		return Maximums.y;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
TESRegionPointList* TESRegionPointList::Copy(TESRegionPointList* apCopyInto) {
	TESRegionPointList* pcopy;
	BSSimpleList<TESRegionPoint*>* pcurrent = this;
	if (apCopyInto) {
		pcopy = apCopyInto;
	} else {
		TESRegionPointList* pcopy_2;
		TESRegionPointList* apMemory = static_cast<TESRegionPointList*>(ourMalloc(sizeof(TESRegionPointList)));
		if (apMemory) {
			pcopy_2 = ::new (apMemory) TESRegionPointList(bOwnsPointMemory);
		} else {
			pcopy_2 = 0;
		}
		pcopy = pcopy_2;
	}
	while (pcurrent) {
		if (pcopy->bOwnsPointMemory) {
			TESRegionPoint** v2 = &pcurrent->m_item;
			TESRegionPoint* ppointCopy = (*v2)->Copy(0);
			if (!pcopy->Add(ppointCopy, 0.0f)) {
				ourFree(ppointCopy);
			}
		} else {
			TESRegionPoint** v3 = &pcurrent->m_item;
			pcopy->Add(*v3, 0.0f);
		}
		pcurrent = pcurrent->m_pkNext;
	}
	return pcopy;
}

// The exe's cleanup-only C++ EH frame is left out.
float TESRegionPointList::GetScaleBasedOnLocation(TESRegionPoint* apTest, float afDistInsideAtMax) {
	bool bQuit = 0;
	if (pLastScaleResult && pLastScaleResult->Matches(apTest, afDistInsideAtMax)) {
		return pLastScaleResult->fscale;
	} else {
		bool bisInsideRegion = IsInsideRegion(apTest);
		if (apTest && bisInsideRegion) {
			if (!(afDistInsideAtMax < 1.0)) {
				TESRegionPointList::ScaleResult* v5;
				float freturn;
				float fminDistPI = 3.4028235e+38f;
				BSSimpleList<TESRegionPoint*>* pcurrent = this;
				do {
					float fdistPI;
					BSSimpleList<TESRegionPoint*>* pnext = pcurrent->m_pkNext;
					if (!pnext) {
						pnext = this;
						bQuit = 1;
					}
					float fdistsqPV1 = apTest->GetDistanceSquared(pcurrent->m_item);
					float fdistsqPV2 = apTest->GetDistanceSquared(pnext->m_item);
					float fdistV1V2 = pcurrent->m_item->GetDistance(pnext->m_item);
					float fdistsqV1V2 = pcurrent->m_item->GetDistanceSquared(pnext->m_item);
					float v9 = fdistsqV1V2 - fdistsqPV2 + fdistsqPV1;
					float fdistV1I = v9 / (fdistV1V2 * 2.0);
					if (!(fdistV1V2 <= static_cast<double>(fdistV1I))) {
						if (!(fdistV1I <= 0.0)) {
							fdistPI = fdistsqPV1 - fdistV1I * fdistV1I;
						} else {
							fdistPI = fdistsqPV1;
						}
					} else {
						fdistPI = fdistsqPV2;
					}
					if (fminDistPI > static_cast<double>(fdistPI)) {
						fminDistPI = fdistPI;
					}
					pcurrent = pnext;
				} while (!bQuit);
				float fminDistPI_2 = std::sqrtf(fminDistPI);
				if (!(afDistInsideAtMax <= static_cast<double>(fminDistPI_2))) {
					freturn = fminDistPI_2 / afDistInsideAtMax;
				} else {
					freturn = 1.0f;
				}
				if (pLastScaleResult) {
					ourFree(pLastScaleResult);
				}
				void* pMemory = ourMalloc(sizeof(TESRegionPointList::ScaleResult));
				if (pMemory) {
					v5 = ::new (pMemory) TESRegionPointList::ScaleResult;
				} else {
					v5 = 0;
				}
				pLastScaleResult = v5;
				apTest->Copy(&pLastScaleResult->point);
				pLastScaleResult->fdist = afDistInsideAtMax;
				pLastScaleResult->fscale = freturn;
				return freturn;
			} else {
				return 1.0f;
			}
		} else {
			return 0.0f;
		}
	}
}
