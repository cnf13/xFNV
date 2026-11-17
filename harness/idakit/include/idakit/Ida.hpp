// The IDA SDK headers idakit uses, in one place and one order. The build defines __LINUX__, __EA64__ and
// USE_STANDARD_FILE_FUNCTIONS / USE_DANGEROUS_FUNCTIONS (the SDK otherwise poisons the C library names the standard
// headers use).
#pragma once

#include <pro.h>

#include <auto.hpp>
#include <bytes.hpp>
#include <frame.hpp>
#include <funcs.hpp>
#include <gdl.hpp>
#include <hexrays.hpp>
#include <ida.hpp>
#include <idalib.hpp>
#include <idp.hpp>
#include <lines.hpp>
#include <nalt.hpp>
#include <name.hpp>
#include <segment.hpp>
#include <typeinf.hpp>
#include <ua.hpp>
#include <xref.hpp>
// x86's operand helpers (x86_base_reg, ...); last: it includes allins.hpp (no include guard) and breaks hexrays.hpp
// when it comes first
#include <intel.hpp>
