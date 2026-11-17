#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The two tags hkArrayUtil's element operations dispatch on: plain data (no construction or destruction to run) and
// classes.
typedef hkTraitBool<true> hkTypeIsPod;

// The fallback comparison of hkIsPodType's test: a type that does not declare itself POD compares through this one
// and gets the false type. Only named inside sizeof, never defined.
template <typename T> hkBool::CompileTimeFalseType operator==(const T&, const hkTypeIsPod&);

typedef hkTraitBool<false> hkTypeIsClass;

// hkTypeIsPod for the scalars, pointers and classes that declare HK_DECLARE_POD_TYPE, else hkTypeIsClass.
template <typename T>
struct hkIsPodType : public hkTraitBool<(sizeof(*static_cast<T*>(0) == *static_cast<const hkTypeIsPod*>(0)) ==
						 sizeof(hkBool::CompileTimeTrueType))> {};

// Declared inside a class: its arrays treat it as plain data.
#define HK_DECLARE_POD_TYPE() hkBool::CompileTimeTrueType operator==(const hkTypeIsPod&) const;

// Declared inside a class template: plain data when TYPE is.
#define HK_DECLARE_POD_TYPE_IF_POD(TYPE) \
	hkBool::CompileTimeTrueType operator==(const hkTraitBool<hkIsPodType<TYPE>::result>&) const;

#define HK_DEFINE_AS_POD_TYPE(T) \
	template <> struct hkIsPodType<T> : public hkTypeIsPod {};

// hkLong and hkUlong are not listed: no scalar here is spelled as long, and hkUlong is uintptr_t, one of the types
// below.
HK_DEFINE_AS_POD_TYPE(bool);
HK_DEFINE_AS_POD_TYPE(hkBool);
HK_DEFINE_AS_POD_TYPE(char);
HK_DEFINE_AS_POD_TYPE(int8_t);
HK_DEFINE_AS_POD_TYPE(uint8_t);
HK_DEFINE_AS_POD_TYPE(int16_t);
HK_DEFINE_AS_POD_TYPE(uint16_t);
HK_DEFINE_AS_POD_TYPE(int32_t);
HK_DEFINE_AS_POD_TYPE(uint32_t);
HK_DEFINE_AS_POD_TYPE(int64_t);
HK_DEFINE_AS_POD_TYPE(uint64_t);
HK_DEFINE_AS_POD_TYPE(hkUFloat8);
HK_DEFINE_AS_POD_TYPE(hkHalf);
HK_DEFINE_AS_POD_TYPE(float);
HK_DEFINE_AS_POD_TYPE(double);

HK_DEFINE_AS_POD_TYPE(hkVariant);

template <typename T> struct hkIsPodType<T*> : public hkTypeIsPod {};
template <typename T, uint32_t N> struct hkIsPodType<T[N]> : public hkIsPodType<T> {};
