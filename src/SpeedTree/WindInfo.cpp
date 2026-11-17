#include "SpeedTree/WindInfo.h"

#include "SpeedTree/FileAccess.h"
#include "SpeedTree/LibGlobals_Source/IdvGlobals.h"

SIdvWindInfo::SIdvWindInfo()
	: m_cLeafFactors(0.5f, 0.5f, 0.0f), m_cLeafOscillation(0.0f, 0.0f, 0.0f), m_fStrength(0.0f) {}

// Reads the wind fields token by token up to File_EndWindInfo; the direction, branch oscillation, branch factors and
// enabled flag are read and dropped. It throws IdvFileError with a C++ throw on an unknown token, as
// SIdvBranchInfo::Parse (BranchInfo.cpp) does; the checker's Windows models do not include RaiseException, so a call
// that throws ends as "unverifiable". The exe's cleanup-only EH frame is left out; its _Tidy of the message string
// after the throw is unreachable.
void SIdvWindInfo::Parse(CTreeFileAccess& cFile) {
	int32_t eToken = cFile.ParseToken();
	do {
		switch (eToken) {
		case File_Wind_Direction: {
			float afDirection[3];
			cFile.ParseVector3(afDirection);
			break;
		}
		case File_Wind_BranchOscillation: {
			float afBranchOscillation[3];
			cFile.ParseVector3(afBranchOscillation);
			break;
		}
		case File_Wind_LeafOscillation: {
			float afLeafOscillation[3];
			float* pLeafOscillation = cFile.ParseVector3(afLeafOscillation);
			m_cLeafOscillation.m_afData[0] = pLeafOscillation[0];
			m_cLeafOscillation.m_afData[1] = pLeafOscillation[1];
			m_cLeafOscillation.m_afData[2] = pLeafOscillation[2];
			break;
		}
		case File_Wind_BranchFactors: {
			float afBranchFactors[3];
			cFile.ParseVector3(afBranchFactors);
			break;
		}
		case File_Wind_LeafFactors: {
			float afLeafFactors[3];
			float* pLeafFactors = cFile.ParseVector3(afLeafFactors);
			m_cLeafFactors.m_afData[0] = pLeafFactors[0];
			m_cLeafFactors.m_afData[1] = pLeafFactors[1];
			m_cLeafFactors.m_afData[2] = pLeafFactors[2];
			break;
		}
		case File_Wind_Strength:
			m_fStrength = cFile.ParseFloat();
			break;
		case File_Wind_Enabled:
			cFile.ParseBool();
			break;
		default: {
			std::string strDetails;
			FNVD_RECONSTRUCT(std::string)(&strDetails, "malformed general wind information");
			throw IdvFileError(strDetails, false);
		}
		}
		eToken = cFile.ParseToken();
	} while (eToken != File_EndWindInfo);
}
