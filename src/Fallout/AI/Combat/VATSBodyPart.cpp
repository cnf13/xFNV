#include "Fallout/AI/Combat/VATSBodyPart.h"

VATSBodyPart::VATSBodyPart()
	: eBodyPart(uint32_t(-1)), fPercentVisible(0.0f), fChance(0.0f), bOnScreen(false), bChanceCalculated(false),
	  bFirstTimeShown(true), bNeedsRecalc(true), pMeter(nullptr), unk34(0.0f), byte38(false) {}
