#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <stdint.h>

class hkStringBuf;

// Whether s holds a character that needs the value quoted: one at or below the space, '=' or ';'.
bool needsQuotes(const char* apcString);

// Stores named string variables exported with a scene.
class hkxEnvironment : public hkReferencedObject {
public:
    struct Variable {
        static const hkClass& staticClass();

        hkStringPtr m_name;
        hkStringPtr m_value;
    };

    // Implicit default constructor: the getVtable function declares the local, then constructs it again in place. Every
    // member's own default leaves it destructible.
    hkxEnvironment() = default;
    hkxEnvironment(hkFinishLoadedObjectFlag flag);
    ~hkxEnvironment() override;

    // Empties stringOut and writes the variables into it as name=value pairs separated by "; ", each name and value
    // quoted when needsQuotes says so.
    void convertToString(hkStringBuf& result) const;
    int32_t findVariableByName(const char* name);
    int32_t getNumVariables();
    const char* getVariableName(int32_t i);
    const char* getVariableValue(int32_t i);
    const char* getVariableValue(const char* name);
    void clear();
    hkResult setVariable(const char* name, const char* value);
    // Parses "name=value; name2="quoted value"; name3=" into variables (a name with no value removes it), with a
    // table-driven state machine.
    hkResult interpretString(const char* str);

    static const hkClass& staticClass();

    hkArray<Variable, hkContainerHeapAllocator> m_variables;
};

extern hkClass hkxEnvironmentVariableClass;
extern hkClass hkxEnvironmentClass;
extern hkTypeInfo hkxEnvironmentTypeInfo;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkxEnvironmentVariableTypeInfo;
