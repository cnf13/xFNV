#include "Fallout Shared/Expression.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class ScriptLocals;

// Names the operator stack's instantiation for the registration macros (its template arguments hold a comma).
using OperatorStack = Stack<EXPRESSION_OPERATORS, 64>;

CExpression::CExpression() : m_ExpressionError(ERROR_EXPRESSION_NOERROR) {}

CExpression::~CExpression() {}

uint32_t CExpression::GetNextToken(char** appInput, char* apToken, EXPRESSION_OPERATORS& aeOperator, bool abEndian,
	const char** apScriptData) {
	char* tmp = *appInput;
	char* tok = apToken;
	bool bOperatorAllowed = true;
	aeOperator = EXP_COUNT;
	while (*tmp && *tmp < 33) {
		++tmp;
		if (abEndian) {
			if (apScriptData) {
				++*apScriptData;
			}
		}
	}
	while (*tmp) {
		char cCurrent = *tmp;
		// The expression text is a raw byte stream the exe reads 16-bit words from, unaligned.
		int16_t stmp1Endian = *reinterpret_cast<int16_t*>(tmp + 1);
		// A compiled reference: 'X', a 16-bit code in [0x1000, 0x1280), then a 16-bit length of the data after it.
		if (cCurrent == 'X' && stmp1Endian >= 0x1000 && stmp1Endian < 0x1280) {
			int16_t sSize = *reinterpret_cast<int16_t*>(tmp + 3);
			sSize += 5;
			ourMemcpy(tok, tmp, sSize);
			tok += sSize;
			tmp += sSize;
			break;
		}
		switch (cCurrent) {
		case 's':
		case 'l':
		case 'f':
		case 'G':
		case 'Z':
		case 'r': {
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			if (cCurrent != 'r') {
				goto LABEL_56;
			}
			bOperatorAllowed = false;
			break;
		}
		case 'n': {
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			goto LABEL_56;
		}
		case 'z': {
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			*tok++ = *tmp++;
			goto LABEL_56;
		}
		case '"': {
			*tok++ = cCurrent;
			++tmp;
			while (*tmp && *tmp != '"') {
				*tok++ = *tmp++;
			}
			if (*tmp == '"') {
				*tok++ = *tmp++;
			}
			goto LABEL_56;
		}
		case ' ':
		case '\t': {
			goto LABEL_56;
		}
		default: {
			uint32_t n;
			bool bMatch = false;
			bool bPartial = false;
			SCRIPT_OPERATOR* pOperator = scriptOperators;
			for (n = 0; n < EXP_COUNT; ++n) {
				const char* pText = pOperator->pOperator;
				bMatch = true;
				int32_t i = 0;
				while (*pText) {
					if (*pText != tmp[i]) {
						bMatch = false;
						break;
					}
					bPartial = true;
					++pText;
					++i;
				}
				if (bMatch) {
					break;
				}
				++pOperator;
			}
			aeOperator = static_cast<EXPRESSION_OPERATORS>(n);
			if (bMatch) {
				if (bOperatorAllowed) {
					const char* pText = scriptOperators[aeOperator].pOperator;
					while (*pText) {
						*tok++ = *pText++;
						++tmp;
					}
				} else {
					aeOperator = EXP_COUNT;
				}
				goto LABEL_56;
			}
			if (bPartial) {
				SetError(ERROR_EXPRESSION_SYNTAX);
			}
			*tok++ = cCurrent;
			bOperatorAllowed = false;
			++tmp;
			break;
		}
		}
	}
LABEL_56:;
	*tok = 0;
	*appInput = tmp;
	return tok - apToken;
}

// KEEP: unregistered. Each operand push copies all 16 bytes of the local operand (0x594A05: [ebp-0x10]..[ebp-1]),
// whose padding bytes [ebp-0xF]..[ebp-0x9] the exe never writes: it pushes uninitialised stack bytes.
double CExpression::Eval(const char* sptr, TESObjectREFR* apFunctionOwner, TESObjectREFR* apContainer, Script* script,
	ScriptLocals* scriptLocals, int32_t postfixSize, bool abEndian) {
	char v22[71];
	char apDest[520];
	char* tokenInPostfix;
	const char* pscriptData;
	ExpressionOperand eo;
	pscriptData = sptr;
	EXPRESSION_OPERATORS eOperator = EXP_COUNT;
	char v23 = 0;
	eo.Type = 0;
	eo.eoDouble = 0.0;
	if (!postfixSize || m_ExpressionError) {
		return 0.0;
	}
	m_operandStack.reset();
	if (postfixSize > 512) {
		SetError(ERROR_EXPRESSION_OUTOFMEMORY);
		return 0.0;
	}
	ourMemcpy(apDest, sptr, postfixSize);
	apDest[postfixSize] = 0;
	tokenInPostfix = apDest;
	char* token = v22;
	uint32_t uiSize = GetNextToken(&tokenInPostfix, v22, eOperator, abEndian, &pscriptData);
	if (!uiSize) {
		token = 0;
	}
	while (uiSize) {
		ExpressionOperand reo;
		ExpressionOperand leo;
		if (eOperator >= EXP_COUNT || eOperator <= EXP_RIGHTPAREN) {
			if (IsStringInteger(token)) {
				eo.Type = 0;
				eo.eoLong = std::atol(token);
			} else {
				if (IsStringFloat(token)) {
					eo.Type = 1;
					eo.eoDouble = std::atof(token);
				} else {
					uint32_t uiTokenOffset = 0;
					eo.Type = 1;
					if (abEndian) {
						Script::GetValue(eo.eoDouble, pscriptData, uiTokenOffset, apFunctionOwner, apContainer, script,
							scriptLocals, abEndian);
					} else {
						if (!Script::GetValue(eo.eoDouble, token, uiTokenOffset, apFunctionOwner, apContainer, script,
								scriptLocals, 0)) {
							goto LABEL_182;
						}
					}
				}
			}
			if (v23) {
				v23 = 0;
				goto LABEL_172;
			}
			goto LABEL_170;
		}
		if (m_operandStack.empty()) {
			goto LABEL_160;
		}
		reo = m_operandStack.pop();
		if (eOperator != EXP_UNARYMINUS) {
			if (m_operandStack.empty()) {
			LABEL_160:;
				SetError(ERROR_EXPRESSION_STACKUNDERFLOW);
				return 0.0;
			}
			leo = m_operandStack.pop();
			switch (eOperator) {
			case EXP_AND: {
				eo.Type = 0;
				if (leo.Type) {
					if (reo.Type) {
						int32_t v13 = 0.0 != leo.eoDouble && 0.0 != reo.eoDouble;
						eo.eoLong = v13;
					} else {
						int32_t v14 = 0.0 != leo.eoDouble && reo.eoLong;
						eo.eoLong = v14;
					}
				} else {
					if (reo.Type) {
						int32_t v15 = leo.eoLong && 0.0 != reo.eoDouble;
						eo.eoLong = v15;
					} else {
						int32_t v16 = leo.eoLong && reo.eoLong;
						eo.eoLong = v16;
					}
				}
				break;
			}
			case EXP_OR: {
				eo.Type = 0;
				if (leo.Type) {
					if (reo.Type) {
						int32_t v9 = 0.0 != leo.eoDouble || 0.0 != reo.eoDouble;
						eo.eoLong = v9;
					} else {
						int32_t v10 = 0.0 != leo.eoDouble || reo.eoLong;
						eo.eoLong = v10;
					}
				} else {
					if (reo.Type) {
						int32_t v11 = leo.eoLong || 0.0 != reo.eoDouble;
						eo.eoLong = v11;
					} else {
						int32_t v12 = leo.eoLong || reo.eoLong;
						eo.eoLong = v12;
					}
				}
				break;
			}
			case EXP_LESSEQUAL: {
				eo.Type = 0;
				if (leo.Type) {
					if (reo.Type) {
						eo.eoLong = reo.eoDouble >= leo.eoDouble;
					} else {
						eo.eoLong = static_cast<double>(reo.eoLong) >= leo.eoDouble;
					}
				} else {
					if (reo.Type) {
						eo.eoLong = static_cast<double>(leo.eoLong) <= reo.eoDouble;
					} else {
						eo.eoLong = leo.eoLong <= reo.eoLong;
					}
				}
				break;
			}
			case EXP_LESS: {
				eo.Type = 0;
				if (leo.Type) {
					if (reo.Type) {
						eo.eoLong = reo.eoDouble > leo.eoDouble;
					} else {
						eo.eoLong = static_cast<double>(reo.eoLong) > leo.eoDouble;
					}
				} else {
					if (reo.Type) {
						eo.eoLong = static_cast<double>(leo.eoLong) < reo.eoDouble;
					} else {
						eo.eoLong = leo.eoLong < reo.eoLong;
					}
				}
				break;
			}
			case EXP_GREATEREQUAL: {
				eo.Type = 0;
				if (leo.Type) {
					if (reo.Type) {
						eo.eoLong = reo.eoDouble <= leo.eoDouble;
					} else {
						eo.eoLong = static_cast<double>(reo.eoLong) <= leo.eoDouble;
					}
				} else {
					if (reo.Type) {
						eo.eoLong = static_cast<double>(leo.eoLong) >= reo.eoDouble;
					} else {
						eo.eoLong = leo.eoLong >= reo.eoLong;
					}
				}
				break;
			}
			case EXP_GREATER: {
				eo.Type = 0;
				if (leo.Type) {
					if (reo.Type) {
						eo.eoLong = reo.eoDouble < leo.eoDouble;
					} else {
						eo.eoLong = static_cast<double>(reo.eoLong) < leo.eoDouble;
					}
				} else {
					if (reo.Type) {
						eo.eoLong = static_cast<double>(leo.eoLong) > reo.eoDouble;
					} else {
						eo.eoLong = leo.eoLong > reo.eoLong;
					}
				}
				break;
			}
			case EXP_EQUAL: {
				eo.Type = 0;
				if (leo.Type) {
					if (reo.Type) {
						eo.eoLong = reo.eoDouble == leo.eoDouble;
					} else {
						eo.eoLong = static_cast<double>(reo.eoLong) == leo.eoDouble;
					}
				} else {
					if (reo.Type) {
						eo.eoLong = static_cast<double>(leo.eoLong) == reo.eoDouble;
					} else {
						eo.eoLong = leo.eoLong == reo.eoLong;
					}
				}
				break;
			}
			case EXP_NOTEQUAL: {
				eo.Type = 0;
				if (leo.Type) {
					if (reo.Type) {
						eo.eoLong = reo.eoDouble != leo.eoDouble;
					} else {
						eo.eoLong = static_cast<double>(reo.eoLong) != leo.eoDouble;
					}
				} else {
					if (reo.Type) {
						eo.eoLong = static_cast<double>(leo.eoLong) != reo.eoDouble;
					} else {
						eo.eoLong = leo.eoLong != reo.eoLong;
					}
				}
				break;
			}
			case EXP_SUBTRACT: {
				if (leo.Type) {
					eo.Type = 1;
					if (reo.Type) {
						eo.eoDouble = leo.eoDouble - reo.eoDouble;
					} else {
						eo.eoDouble = leo.eoDouble - static_cast<double>(reo.eoLong);
					}
				} else {
					if (reo.Type) {
						eo.Type = 1;
						eo.eoDouble = static_cast<double>(leo.eoLong) - reo.eoDouble;
					} else {
						eo.Type = 0;
						eo.eoLong = leo.eoLong - reo.eoLong;
					}
				}
				break;
			}
			case EXP_ADD: {
				if (leo.Type) {
					eo.Type = 1;
					if (reo.Type) {
						eo.eoDouble = leo.eoDouble + reo.eoDouble;
					} else {
						eo.eoDouble = static_cast<double>(reo.eoLong) + leo.eoDouble;
					}
				} else {
					if (reo.Type) {
						eo.Type = 1;
						eo.eoDouble = static_cast<double>(leo.eoLong) + reo.eoDouble;
					} else {
						eo.Type = 0;
						eo.eoLong = reo.eoLong + leo.eoLong;
					}
				}
				break;
			}
			case EXP_MULTIPLY: {
				if (leo.Type) {
					eo.Type = 1;
					if (reo.Type) {
						eo.eoDouble = leo.eoDouble * reo.eoDouble;
					} else {
						eo.eoDouble = static_cast<double>(reo.eoLong) * leo.eoDouble;
					}
				} else {
					if (reo.Type) {
						eo.Type = 1;
						eo.eoDouble = static_cast<double>(leo.eoLong) * reo.eoDouble;
					} else {
						eo.Type = 0;
						eo.eoLong = reo.eoLong * leo.eoLong;
					}
				}
				break;
			}
			case EXP_DIVIDE:
			case EXP_MODULUS: {
				if (leo.Type) {
					if (reo.Type) {
						if (0.0 == reo.eoDouble) {
							if (!abEndian) {
							LABEL_148:;
								SetError(ERROR_EXPRESSION_DIVIDEBYZERO);
								return 0.0;
							}
							reo.eoDouble = 1.0;
						}
						eo.Type = 1;
						if (eOperator == EXP_DIVIDE) {
							eo.eoDouble = leo.eoDouble / reo.eoDouble;
						} else {
							eo.eoDouble = static_cast<double>(static_cast<int32_t>(leo.eoDouble) %
								static_cast<int32_t>(reo.eoDouble));
						}
					} else {
						if (!reo.eoLong) {
							if (!abEndian) {
								goto LABEL_148;
							}
							reo.eoDouble = 1.0;
						}
						eo.Type = 1;
						if (eOperator == EXP_DIVIDE) {
							eo.eoDouble = leo.eoDouble / static_cast<double>(reo.eoLong);
						} else {
							eo.eoDouble = static_cast<double>(static_cast<int32_t>(leo.eoDouble) % reo.eoLong);
						}
					}
				} else {
					if (reo.Type) {
						if (0.0 == reo.eoDouble) {
							if (!abEndian) {
								goto LABEL_148;
							}
							reo.eoDouble = 1.0;
						}
						eo.Type = 1;
						if (eOperator == EXP_DIVIDE) {
							eo.eoDouble = static_cast<double>(leo.eoLong) / reo.eoDouble;
						} else {
							eo.eoDouble = static_cast<double>(leo.eoLong % static_cast<int32_t>(reo.eoDouble));
						}
					} else {
						if (!reo.eoLong) {
							if (!abEndian) {
								goto LABEL_148;
							}
							reo.eoDouble = 1.0;
						}
						eo.Type = 0;
						if (eOperator == EXP_DIVIDE) {
							eo.eoLong = leo.eoLong / reo.eoLong;
						} else {
							eo.eoLong = leo.eoLong % reo.eoLong;
						}
					}
				}
				break;
			}
			default: {
				Warning("SCRIPTS: Unhandled operator '%s' in Expression::Eval()", token);
				eo.Type = 0;
				eo.eoLong = 0;
				break;
			}
			}
		LABEL_170:;
			m_operandStack.push(eo);
			goto LABEL_172;
		}
		if (reo.Type) {
			eo.eoDouble = -reo.eoDouble;
		} else {
			eo.eoLong = -reo.eoLong;
		}
		eo.Type = reo.Type;
		m_operandStack.push(eo);
	LABEL_172:;
		if (uiSize) {
			pscriptData += uiSize;
			uiSize = GetNextToken(&tokenInPostfix, token, eOperator, abEndian, &pscriptData);
			if (!uiSize) {
				token = 0;
			}
		}
	}
	if (m_operandStack.empty()) {
	LABEL_182:;
		SetError(ERROR_EXPRESSION_SYNTAX);
		return 0.0;
	}
	eo = m_operandStack.pop();
	if (!m_operandStack.empty()) {
		SetError(ERROR_EXPRESSION_SYNTAX);
		return 0.0;
	}
	if (eo.Type) {
		return eo.eoDouble;
	} else {
		return static_cast<double>(eo.eoLong);
	}
}

// Stores the error, then logs its text through the start-up log (an empty function in this build).
void CExpression::SetError(EXPRESSION_ERROR errorCode) {
	m_ExpressionError = errorCode;
	InitializationFlow("%s", expressionErrors[errorCode]);
}

char* CExpression::InfixToPostfix(char* apInfix, uint32_t& auiSize) {
	EXPRESSION_OPERATORS eNewOperator;
	char* ifptr;
	uint32_t uiSize = 0;
	m_ExpressionError = ERROR_EXPRESSION_NOERROR;
	m_charStack.reset();
	m_operandStack.reset();
	// The exe stores each bracket through a one-byte stack local first.
	char cBracket;
	ifptr = m_Infix;
	cBracket = '(';
	*ifptr++ = cBracket;
	ourMemcpy(ifptr, apInfix, auiSize);
	ifptr += auiSize;
	cBracket = ')';
	*ifptr++ = cBracket;
	*ifptr++ = 0;
	ifptr = m_Infix;
	char* pfptr = m_Postfix;
	*pfptr = 0;
	bool bPrevWasValue = false;
	do {
		char a2[68];
		eNewOperator = EXP_COUNT;
		uiSize = GetNextToken(&ifptr, a2, eNewOperator, false, nullptr);
		if (uiSize) {
			if (eNewOperator >= EXP_COUNT) {
				*pfptr++ = ' ';
				ourMemcpy(pfptr, a2, uiSize);
				pfptr += uiSize;
			} else if (eNewOperator == EXP_LEFTPAREN) {
				if (m_charStack.full()) {
					SetError(ERROR_EXPRESSION_STACKOVERFLOW);
					return nullptr;
				}
				m_charStack.push(EXP_LEFTPAREN);
			} else if (eNewOperator == EXP_RIGHTPAREN) {
				if (m_charStack.empty()) {
					SetError(ERROR_EXPRESSION_SYNTAX);
					return nullptr;
				}
				while (1) {
					EXPRESSION_OPERATORS eOperator = m_charStack.pop();
					if (eOperator == EXP_LEFTPAREN) {
						break;
					}
					*pfptr++ = ' ';
					int32_t iLen = BSstrlen(scriptOperators[eOperator].pOperator);
					ourMemcpy(pfptr, scriptOperators[eOperator].pOperator, iLen);
					pfptr += iLen;
				}
			} else {
				// A minus with no value before it is the unary minus.
				if (eNewOperator == EXP_SUBTRACT && !bPrevWasValue) {
					eNewOperator = EXP_UNARYMINUS;
				}
				while (!m_charStack.empty()) {
					EXPRESSION_OPERATORS eOperator = m_charStack.pop();
					if (scriptOperators[eOperator].cPrecedence < scriptOperators[eNewOperator].cPrecedence) {
						m_charStack.push(eOperator);
						break;
					}
					*pfptr++ = ' ';
					int32_t iLen = BSstrlen(scriptOperators[eOperator].pOperator);
					ourMemcpy(pfptr, scriptOperators[eOperator].pOperator, iLen);
					pfptr += iLen;
				}
				if (m_charStack.full()) {
					SetError(ERROR_EXPRESSION_STACKOVERFLOW);
					return nullptr;
				}
				m_charStack.push(eNewOperator);
			}
		}
		if (eNewOperator == EXP_COUNT) {
			bPrevWasValue = true;
		} else if (eNewOperator < EXP_COUNT && eNewOperator > EXP_RIGHTPAREN) {
			bPrevWasValue = false;
		}
	} while (uiSize);
	*pfptr = 0;
	if (!m_charStack.empty()) {
		SetError(ERROR_EXPRESSION_SYNTAX);
		return nullptr;
	}
	auiSize = pfptr - m_Postfix;
	return m_Postfix;
}
