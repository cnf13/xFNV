#include "Havok/Source/Common/SceneData/Environment/hkxEnvironment.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

// interpretString's parser types (State, Token, Action and Transition). They are declared at file scope rather than
// inside the function, for the table below.
namespace hkxEnvironmentInterpretString {
enum State : int32_t {
	ST_JUMP_BLANKS_1 = 0,
	ST_NAME_SIMPLE = 1,
	ST_NAME_QUOTES = 2,
	ST_JUMP_BLANKS_2 = 3,
	ST_JUMP_BLANKS_3 = 4,
	ST_VALUE_SIMPLE = 5,
	ST_VALUE_QUOTES = 6,
	ST_JUMP_BLANKS_4 = 7,
	ST_END = 8,
	ST_ERROR = 9,
};
enum Token : int32_t {
	TK_BLANK = 0,
	TK_QUOTE = 1,
	TK_EQUAL = 2,
	TK_SEMICOLON = 3,
	TK_OTHER = 4,
	TK_EOS = 5,
};
enum Action : int32_t {
	AC_SKIP = 0,
	AC_ADD_TO_NAME = 1,
	AC_ADD_TO_VALUE = 2,
	AC_ADD_PAIR = 3,
	AC_REMOVE_VAR = 4,
	AC_NONE = 5,
	AC_ERROR = 6,
};
// One rule: in m_state on m_token, go to m_nextState doing m_action.
struct Transition {
	State m_state;
	Token m_token;
	State m_nextState;
	Action m_action;
};
} // namespace hkxEnvironmentInterpretString

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxEnvironmentVariableTypeInfo = {"hkxEnvironmentVariable", finishLoadedObjecthkxEnvironmentVariable,
	cleanupLoadedObjecthkxEnvironmentVariable, nullptr, &typeid(hkxEnvironment::Variable),
	sizeof(hkxEnvironment::Variable)};

// The reflected members of hkxEnvironment::Variable and hkxEnvironment.
using hkxEnvironment_VariableClass_MembersArray = const hkClassMember[2];
static hkxEnvironment_VariableClass_MembersArray hkxEnvironment_VariableClass_Members{
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
	{"value", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr}};
using hkxEnvironmentClass_MembersArray = const hkClassMember[1];
static hkxEnvironmentClass_MembersArray hkxEnvironmentClass_Members{{"variables", &hkxEnvironmentVariableClass, nullptr,
	{hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0, {0}, 0x8, nullptr}};

// interpretString's transition table: 32 rows, which are scanned in full, the last match winning.
using hkxEnvironment_interpretString_transitionsArray = const hkxEnvironmentInterpretString::Transition[32];
static hkxEnvironment_interpretString_transitionsArray hkxEnvironment_interpretString_transitions{
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::TK_BLANK,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::TK_SEMICOLON,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::TK_QUOTE,
		hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::TK_OTHER,
		hkxEnvironmentInterpretString::ST_NAME_SIMPLE, hkxEnvironmentInterpretString::AC_ADD_TO_NAME},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::TK_EOS,
		hkxEnvironmentInterpretString::ST_END, hkxEnvironmentInterpretString::AC_NONE},
	{hkxEnvironmentInterpretString::ST_NAME_SIMPLE, hkxEnvironmentInterpretString::TK_OTHER,
		hkxEnvironmentInterpretString::ST_NAME_SIMPLE, hkxEnvironmentInterpretString::AC_ADD_TO_NAME},
	{hkxEnvironmentInterpretString::ST_NAME_SIMPLE, hkxEnvironmentInterpretString::TK_BLANK,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_2, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_NAME_SIMPLE, hkxEnvironmentInterpretString::TK_EQUAL,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_3, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::TK_QUOTE,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_2, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::TK_OTHER,
		hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::AC_ADD_TO_NAME},
	{hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::TK_BLANK,
		hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::AC_ADD_TO_NAME},
	{hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::TK_SEMICOLON,
		hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::AC_ADD_TO_NAME},
	{hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::TK_EQUAL,
		hkxEnvironmentInterpretString::ST_NAME_QUOTES, hkxEnvironmentInterpretString::AC_ADD_TO_NAME},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_2, hkxEnvironmentInterpretString::TK_BLANK,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_2, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_2, hkxEnvironmentInterpretString::TK_EQUAL,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_3, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_3, hkxEnvironmentInterpretString::TK_BLANK,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_3, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_3, hkxEnvironmentInterpretString::TK_QUOTE,
		hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_3, hkxEnvironmentInterpretString::TK_OTHER,
		hkxEnvironmentInterpretString::ST_VALUE_SIMPLE, hkxEnvironmentInterpretString::AC_ADD_TO_VALUE},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_3, hkxEnvironmentInterpretString::TK_EOS,
		hkxEnvironmentInterpretString::ST_END, hkxEnvironmentInterpretString::AC_REMOVE_VAR},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_3, hkxEnvironmentInterpretString::TK_SEMICOLON,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::AC_REMOVE_VAR},
	{hkxEnvironmentInterpretString::ST_VALUE_SIMPLE, hkxEnvironmentInterpretString::TK_OTHER,
		hkxEnvironmentInterpretString::ST_VALUE_SIMPLE, hkxEnvironmentInterpretString::AC_ADD_TO_VALUE},
	{hkxEnvironmentInterpretString::ST_VALUE_SIMPLE, hkxEnvironmentInterpretString::TK_SEMICOLON,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::AC_ADD_PAIR},
	{hkxEnvironmentInterpretString::ST_VALUE_SIMPLE, hkxEnvironmentInterpretString::TK_BLANK,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_4, hkxEnvironmentInterpretString::AC_ADD_PAIR},
	{hkxEnvironmentInterpretString::ST_VALUE_SIMPLE, hkxEnvironmentInterpretString::TK_EOS,
		hkxEnvironmentInterpretString::ST_END, hkxEnvironmentInterpretString::AC_ADD_PAIR},
	{hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::TK_QUOTE,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_4, hkxEnvironmentInterpretString::AC_ADD_PAIR},
	{hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::TK_OTHER,
		hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::AC_ADD_TO_VALUE},
	{hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::TK_BLANK,
		hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::AC_ADD_TO_VALUE},
	{hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::TK_SEMICOLON,
		hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::AC_ADD_TO_VALUE},
	{hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::TK_EQUAL,
		hkxEnvironmentInterpretString::ST_VALUE_QUOTES, hkxEnvironmentInterpretString::AC_ADD_TO_VALUE},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_4, hkxEnvironmentInterpretString::TK_BLANK,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_4, hkxEnvironmentInterpretString::AC_SKIP},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_4, hkxEnvironmentInterpretString::TK_EOS,
		hkxEnvironmentInterpretString::ST_END, hkxEnvironmentInterpretString::AC_NONE},
	{hkxEnvironmentInterpretString::ST_JUMP_BLANKS_4, hkxEnvironmentInterpretString::TK_SEMICOLON,
		hkxEnvironmentInterpretString::ST_JUMP_BLANKS_1, hkxEnvironmentInterpretString::AC_SKIP}};
hkClass hkxEnvironmentVariableClass("hkxEnvironmentVariable", nullptr, 8, nullptr, 0, nullptr, 0,
	hkxEnvironment_VariableClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkxEnvironmentClass("hkxEnvironment", &hkReferencedObjectClass, 0x14, nullptr, 0, nullptr, 0,
	hkxEnvironmentClass_Members, 1, nullptr, nullptr, 0, 1);
hkTypeInfo hkxEnvironmentTypeInfo = {"hkxEnvironment", finishLoadedObjecthkxEnvironment,
	cleanupLoadedObjecthkxEnvironment, getVtablehkxEnvironment(), &typeid(hkxEnvironment), sizeof(hkxEnvironment)};

// Whether apcString holds a character that needs the value quoted: one at or below the space, '=' or ';'.
bool needsQuotes(const char* apcString) {
	char v1 = *apcString;
	if (!*apcString) {
		return 0;
	}
	while (v1 > 32 && (v1 != 61 && v1 != 59)) {
		v1 = *++apcString;
		if (!v1) {
			return 0;
		}
	}
	return 1;
}

const hkClass& hkxEnvironment::Variable::staticClass() {
	return hkxEnvironmentVariableClass;
}

// The member's destructor destroys the variables' strings and frees the array.
hkxEnvironment::~hkxEnvironment() {}

// The buffer is cleared first; each variable then writes its name and value, quoted when needsQuotes says so, and every
// variable but the last is followed by "; ".
void hkxEnvironment::convertToString(hkStringBuf& result) const {
	result.clear();
	for (int32_t i = 0; i < m_variables.m_size; ++i) {
		const Variable& variable = m_variables.m_data[i];
		const char* name = variable.m_name.cString();
		const char* value = variable.m_value.cString();
		const char* nameQuote = needsQuotes(name) ? "\"" : "";
		const char* valueQuote = needsQuotes(value) ? "\"" : "";
		result.appendPrintf("%s%s%s=%s%s%s", nameQuote, name, nameQuote, valueQuote, value, valueQuote);
		if (i < m_variables.m_size - 1) {
			result += "; ";
		}
	}
}

int32_t hkxEnvironment::findVariableByName(const char* name) {
	for (int32_t i = 0; i < m_variables.m_size; ++i) {
		if (!hkString::strCasecmp(m_variables.m_data[i].m_name.cString(), name)) {
			return i;
		}
	}
	return -1;
}

int32_t hkxEnvironment::getNumVariables() {
	return m_variables.m_size;
}

const char* hkxEnvironment::getVariableName(int32_t i) {
	return reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_variables.m_data[i].m_name.m_stringAndFlag) &
		~uintptr_t(1));
}

const char* hkxEnvironment::getVariableValue(int32_t i) {
	return reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_variables.m_data[i].m_value.m_stringAndFlag) &
		~uintptr_t(1));
}

const char* hkxEnvironment::getVariableValue(const char* name) {
	int32_t VariableByName = findVariableByName(name);
	if (VariableByName == -1) {
		return 0;
	} else {
		return reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_variables.m_data[VariableByName]
													 .m_value.m_stringAndFlag) &
			~uintptr_t(1));
	}
}

void hkxEnvironment::clear() {
	hkxEnvironment::Variable* m_data = static_cast<hkArrayBase<hkxEnvironment::Variable>&>(m_variables).begin();
	if (m_variables.m_size > 0) {
		uint32_t m_size = m_variables.m_size;
		do {
			m_data->m_value.~hkStringPtr();
			m_data->m_name.~hkStringPtr();
			++m_data;
			--m_size;
		} while (m_size);
	}
	m_variables.m_size = 0;
}

// The SDK's: a value sets the variable, appended when new; a null value removes it (the last variable moved into
// its place), and fails when there is no such variable.
hkResult hkxEnvironment::setVariable(const char* name, const char* value) {
	const int32_t index = findVariableByName(name);
	if (!value) {
		if (index == -1) {
			return HK_FAILURE;
		}
		// EXE-INLINED none: hkArrayBase<hkxEnvironment::Variable>::removeAt
		Variable* pVariable = &m_variables.m_data[index];
		pVariable->m_value.~hkStringPtr();
		pVariable->m_name.~hkStringPtr();
		--m_variables.m_size;
		if (m_variables.m_size != index) {
			hkString::memCpy4(&m_variables.m_data[index], &m_variables.m_data[m_variables.m_size],
				sizeof(Variable) / 4);
		}
		return HK_SUCCESS;
	}
	if (index != -1) {
		m_variables.m_data[index].m_value = value;
		return HK_SUCCESS;
	}
	{
		// The SDK's local Variable, its two strings built empty, then assigned.
		hkStringPtr newName;
		hkStringPtr newValue;
		newName = name;
		newValue = value;
		// EXE-INLINED none: hkArray<hkxEnvironment::Variable>::pushBack
		hkMemoryRouter& router = hkMemoryRouter::getInstance();
		if (m_variables.m_size == (m_variables.m_capacityAndFlags & hkArrayBase<Variable>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*router.m_heap, &m_variables, sizeof(Variable));
		}
		Variable* pSlot = &m_variables.m_data[m_variables.m_size];
		if (pSlot) {
			// EXE-INLINED none: hkxEnvironment::Variable::Variable(const Variable&)
			::new (&pSlot->m_name) hkStringPtr(newName);
			::new (&pSlot->m_value) hkStringPtr(newValue);
		}
		++m_variables.m_size;
	}
	return HK_SUCCESS;
}

// The two strings' destructors (hkInplaceArray<char, 128>'s: empty, free a heap buffer to the temp allocator, reset to
// the inline storage's flags) are written out where they run.
hkResult hkxEnvironment::interpretString(const char* str) {
	using namespace hkxEnvironmentInterpretString;
	State state = ST_JUMP_BLANKS_1;
	int32_t i = 0;
	hkStringBuf currentName;
	hkStringBuf currentValue;
	const int32_t strLength = hkString::strLen(str);
	const char* pcErrorPrefix;
	int32_t iErrorId;
	int32_t iErrorLine;
	do {
		Token token = TK_OTHER;
		if (i < strLength) {
			const char character = str[i];
			if (character <= ' ') {
				token = TK_BLANK;
			}
			if (character == '"') {
				token = TK_QUOTE;
			} else if (character == '=') {
				token = TK_EQUAL;
			} else if (character == ';') {
				token = TK_SEMICOLON;
			}
		} else {
			token = TK_EOS;
		}
		Action action = AC_ERROR;
		State nextState = ST_ERROR;
		for (const Transition& transition : hkxEnvironment_interpretString_transitions) {
			if (transition.m_state == state && transition.m_token == token) {
				nextState = transition.m_nextState;
				action = transition.m_action;
			}
		}
		switch (action) {
		case AC_SKIP:
			++i;
			break;
		case AC_ADD_TO_NAME: {
			const char acCharacter[2] = {str[i], 0};
			currentName += acCharacter;
			++i;
			break;
		}
		case AC_ADD_TO_VALUE: {
			const char acCharacter[2] = {str[i], 0};
			currentValue += acCharacter;
			++i;
			break;
		}
		case AC_ADD_PAIR:
			setVariable(currentName.m_string.begin(), currentValue.m_string.begin());
			currentName = (currentValue = "");
			++i;
			break;
		case AC_REMOVE_VAR:
			setVariable(currentName.m_string.begin(), nullptr);
			currentName = (currentValue = "");
			++i;
			break;
		case AC_NONE:
			break;
		case AC_ERROR:
			pcErrorPrefix = "Error parsing environment string: '";
			iErrorId = static_cast<int32_t>(0xABBA7881);
			iErrorLine = 308;
			goto failed;
		default:
			pcErrorPrefix = "Internal Error: Unknown action parsing environment string: '";
			iErrorId = static_cast<int32_t>(0xABBA0032);
			iErrorLine = 320;
			goto failed;
		}
		state = nextState;
	} while (state != ST_END);

	{
		hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
		currentValue.m_string.m_size = 0;
		if (currentValue.m_string.m_capacityAndFlags >= 0) {
			pTemp->bufFree(currentValue.m_string.begin(),
				currentValue.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
		}
		currentValue.m_string.m_data = nullptr;
		currentValue.m_string.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
		pTemp = hkMemoryRouter::getInstance().m_temp;
		currentName.m_string.m_size = 0;
		if (currentName.m_string.m_capacityAndFlags >= 0) {
			pTemp->bufFree(currentName.m_string.begin(),
				currentName.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
		}
		currentName.m_string.m_data = nullptr;
		currentName.m_string.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
		return HK_SUCCESS;
	}

failed:
	{
		char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << pcErrorPrefix << str << "'";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, iErrorId, buf, ".\\Environment\\hkxEnvironment.cpp",
			iErrorLine);
	}
	{
		hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
		currentValue.m_string.m_size = 0;
		if (currentValue.m_string.m_capacityAndFlags >= 0) {
			pTemp->bufFree(currentValue.m_string.begin(),
				currentValue.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
		}
		currentValue.m_string.m_data = nullptr;
		currentValue.m_string.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
		pTemp = hkMemoryRouter::getInstance().m_temp;
		currentName.m_string.m_size = 0;
		if (currentName.m_string.m_capacityAndFlags >= 0) {
			pTemp->bufFree(currentName.m_string.begin(),
				currentName.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
		}
		currentName.m_string.m_data = nullptr;
		currentName.m_string.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
	}
	return HK_FAILURE;
}

const hkClass& hkxEnvironment::staticClass() {
	return hkxEnvironmentClass;
}
