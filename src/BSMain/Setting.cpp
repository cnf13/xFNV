#include "BSMain/Setting.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSShader/BSFadeNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "SDK/MSVC/string.h"

// The engine calls these accessors on null settings, so the null test on this is intended.
#pragma clang diagnostic ignored "-Wundefined-bool-conversion"

const char* Setting::String() {
    if (this) {
        return uValue.str;
    }
    return nullptr;
}

int32_t& Setting::Int() {
    if (this) {
        return uValue.i;
    }
    DefaultValue.i = 0;
    return DefaultValue.i;
}

uint32_t& Setting::UInt() {
    if (this) {
        return uValue.uint;
    }
    DefaultValue.uint = 0;
    return DefaultValue.uint;
}

float& Setting::Float() {
    if (this) {
        return uValue.f;
    }
    DefaultValue.f = 0.0f;
    return DefaultValue.f;
}

bool& Setting::Bool() {
    if (this) {
        return uValue.b;
    }
    DefaultValue.b = false;
    return DefaultValue.b;
}

Setting::operator uint32_t() {
    return Int();
}

Setting::operator float() {
    return Float();
}

Setting::operator bool() {
    return Bool();
}

Setting::operator const char*() {
    return String();
}

// The old key is freed through the engine's free, which takes void*: the const_cast drops the field's const.
Setting& Setting::operator=(const char* apValue) {
    if (this) {
        int32_t length;
        if (this->Key()) {
            length = std::strlen(this->Key()) + 1;
        }
        else {
            length = 0;
        }
        if (apValue) {
            size_t v6 = std::strlen(apValue) + 1;
            char* pstr = static_cast<char*>(MemoryManager::Instance().Allocate(length + v6));
            if (pstr) {
                std::strcpy(&pstr[length], apValue);
                uValue.str = &pstr[length];
                if (length) {
                    strcpy_s(pstr, length, this->Key());
                    *pstr = 83;
                    if (*this->Key() == 83) {
                        MemoryManager::Instance().Deallocate(const_cast<char*>(this->Key()));
                    }
                    pKey = pstr;
                }
            }
        }
        else {
            uValue.uint = 0;
        }
    }
    return *this;
}

Setting& Setting::operator=(float afValue) {
    if (this) {
        uValue.f = afValue;
    }
    return *this;
}

bool Setting::IsEqual(bool abValue) {
    if (this) {
        return uValue.b == abValue;
    }
    return false;
}

Setting::SETTING_TYPE Setting::DataType(const char* apKey) {
    Setting::SETTING_TYPE v2 = Setting::ST_NONE;
    if (apKey) {
        switch (*apKey) {
            case 83:
            case 115: {
                v2 = Setting::ST_STRING;
                break;
            }
            case 97: {
                v2 = Setting::ST_RGBA;
                break;
            }
            case 98: {
                v2 = Setting::ST_BINARY;
                break;
            }
            case 99: {
                v2 = Setting::ST_CHAR;
                break;
            }
            case 102: {
                v2 = Setting::ST_FLOAT;
                break;
            }
            case 104: {
                v2 = Setting::ST_UCHAR;
                break;
            }
            case 105: {
                v2 = Setting::ST_INT;
                break;
            }
            case 114: {
                v2 = Setting::ST_RGB;
                break;
            }
            case 117: {
                v2 = Setting::ST_UINT;
                break;
            }
            default: {
                return v2;
            }
        }
    }
    return v2;
}

int32_t Setting::GetType() {
    return DataType(pKey);
}

void LOD::iFadeNodeMinNearDistance::Set(int32_t aiValue) {
    BSFadeNode::iMinNearDistance = aiValue;
}

uint32_t LOD::iFadeNodeMinNearDistance::Get() {
    return BSFadeNode::iMinNearDistance.Int();
}

// Stores apID and the value in a setting.
Setting::Setting(const char* apID, Setting::Info aValue) : uValue(aValue), pKey(apID) {}

// The char case calls the char overload with the stored byte zero-extended, the int case the int32_t one ("%s = %d").
void Setting::GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) {
    char name[260];
    strcpy_s(name, 256, "  ");
    strcat_s(name, 256, this->Key());
    switch (Setting::DataType(this->Key())) {
        case Setting::ST_BINARY: {
            Setting::Info* value;
            if (this) {
                value = &uValue;
            }
            else {
                Setting::DefaultValue.b = false;
                value = &Setting::DefaultValue;
            }
            char* element = NiGetViewerString(name, value->b);
            apStringsA->Add(element);
            break;
        }
        case Setting::ST_CHAR:
        case Setting::ST_UCHAR: {
            Setting::Info* p_uValue;
            if (this) {
                p_uValue = &uValue;
            }
            else {
                Setting::DefaultValue.b = false;
                p_uValue = &Setting::DefaultValue;
            }
            char* ViewerString_1 = NiGetViewerString(name, static_cast<char>(p_uValue->b));
            apStringsA->Add(ViewerString_1);
            [[fallthrough]];
        }
        case Setting::ST_INT: {
            Setting::Info* v5;
            if (this) {
                v5 = &uValue;
            }
            else {
                Setting::DefaultValue.uint = 0;
                v5 = &Setting::DefaultValue;
            }
            char* ViewerString_5 = NiGetViewerString(name, static_cast<int32_t>(v5->uint));
            apStringsA->Add(ViewerString_5);
            break;
        }
        case Setting::ST_FLOAT: {
            Setting::Info* v4;
            if (this) {
                v4 = &uValue;
            }
            else {
                Setting::DefaultValue.f = 0.0f;
                v4 = &Setting::DefaultValue;
            }
            char* ViewerString_7 = NiGetViewerString(name, v4->f);
            apStringsA->Add(ViewerString_7);
            break;
        }
        case Setting::ST_STRING: {
            char* ViewerString_9;
            if (this) {
                ViewerString_9 = NiGetViewerString(
                    static_cast<const char*>(name),
                    static_cast<const char*>(uValue.str)
                );
            }
            else {
                ViewerString_9 = NiGetViewerString(name, static_cast<const char*>(nullptr));
            }
            char* v10 = ViewerString_9;
            apStringsA->Add(v10);
            break;
        }
        default: {
            Setting::Info* v3;
            if (this) {
                v3 = &uValue;
            }
            else {
                Setting::DefaultValue.uint = 0;
                v3 = &Setting::DefaultValue;
            }
            char* ViewerString_6 = NiGetViewerString(name, v3->uint);
            apStringsA->Add(ViewerString_6);
            break;
        }
    }
}

void Setting::SetInt(int32_t aiValue) {
    if (this) {
        uValue.uint = static_cast<uint32_t>(aiValue);
    }
}

Setting::Setting(const char* apID, float afValue) : uValue{.f = afValue}, pKey(apID) {}

bool Setting::operator==(int32_t aiValue) {
    return this && uValue.uint == aiValue;
}

uint32_t Setting::DataSize() {
    uint32_t v4 = 0;
    if (pKey) {
        switch (*pKey) {
            case 83:
            case 115: {
                Setting::Info v3;
                if (this) {
                    v3.uint = uValue.uint;
                }
                else {
                    v3.uint = 0;
                }
                if (v3.uint) {
                    char* Str;
                    if (this) {
                        Str = uValue.str;
                    }
                    else {
                        Str = nullptr;
                    }
                    v4 = std::strlen(Str) + 1;
                }
                break;
            }
            case 97:
            case 102:
            case 105:
            case 114:
            case 117: {
                v4 = 4;
                break;
            }
            case 98:
            case 99:
            case 104: {
                v4 = 1;
                break;
            }
            default: {
                return v4;
            }
        }
    }
    return v4;
}

// Frees a key the setting owns (an 'S' string key); the key is freed through the engine's free, which takes void*.
Setting::~Setting() {
    if (pKey && (*pKey == 'S')) {
        ourFree(const_cast<char*>(pKey));
    }
}

Setting::Setting(const char* apID, bool abValue) : uValue{.b = abValue}, pKey(apID) {}

void Setting::operator delete(void* apMemory) {
    ourFree(apMemory);
}

Setting& Setting::operator=(bool abValue) {
    if (this) {
        uValue.b = abValue;
    }
    return *this;
}
