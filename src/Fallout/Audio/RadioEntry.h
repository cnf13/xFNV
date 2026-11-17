#pragma once

#include "Fallout/Audio/FalloutRadio.h"

// IDA's name for the nested FalloutRadio::FORadioStation (the PDB nests it): the same 0x24-byte record, whose
// constructor (0x833150) and destructor (0x8321C0) IDA names RadioEntry's.
using RadioEntry = FalloutRadio::FORadioStation;
