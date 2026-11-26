#include "Fallout/AI/Combat/VATSTarget.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "Fallout/AI/Combat/VATSBodyPart.h"

VATSTarget::VATSTarget() {}

VATSTarget::~VATSTarget() {}

void VATSTarget::operator delete(void* apMemory) {
    ourFree(apMemory);
}
