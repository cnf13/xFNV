#include "Fallout/AI/Combat/VATSBodyPart.h"

VATSBodyPart::VATSBodyPart()
  : eBodyPart(static_cast<uint32_t>(-1)), fPercentVisible(0.0f), fChance(0.0f), bOnScreen(false),
    bChanceCalculated(false), bFirstTimeShown(true), bNeedsRecalc(true), pMeter(nullptr), fAngleToStick(0.0f),
    bLeftOfCenter(false) {}
