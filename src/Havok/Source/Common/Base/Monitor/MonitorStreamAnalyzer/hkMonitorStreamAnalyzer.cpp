#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamFrameInfo.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "SDK/MSVC/string.h"

#include <bit>

// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
static hkMonitorStreamAnalyzer::Node* findChildNode(hkMonitorStreamAnalyzer::Node* a1, double a2, int32_t a3);
static void finishLoadedObjecthkMonitorStreamColorTableColorPair(void* p, int32_t finishing);
// Havok SDK signature (hkBase.lib, tied by bytes).
static void cleanupLoadedObjecthkMonitorStreamStringMapStringMap(void* a1);
static void cleanupLoadedObjecthkMonitorStreamColorTableColorPair(void* p);
static void finishLoadedObjecthkMonitorStreamColorTable(void* p, int32_t finishing);
// Havok SDK signature (hkBase.lib, tied by reloc,target).
static const void* getVtablehkMonitorStreamColorTable();
// Havok SDK signature (hkBase.lib, tied by target).
static void cleanupLoadedObjecthkMonitorStreamStringMap(void* a1);
// Havok SDK signature (hkBase.lib, tied by bytes,reloc,string).
static void printSingleNodeValues(hkOstream& a1, float a2, const hkMonitorStreamAnalyzer::Node* a3);
// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
static bool saveToTGA(int32_t* a1, hkOstream& a2, int32_t a3, int32_t a4);
// r_eax is the image.
static int32_t drawNumber(int32_t a1, int32_t a2, int32_t* r_eax);
// a1 is the image's width, a2 is the image, and r_eax is the digit.
static void drawDigit(int32_t a1, int32_t* a2, int32_t r_eax);
// writeStatisticsDetailsToTexture's colouring of frame aiFrame of a thread's frames (apFrames, the thread root's
// children): aiWidth columns of aiHeight pixels at apPixels, aiStride pixels per row, coloured from apColorTable by the
// names of the nodes sampled over afFrameTime from afStartTime on; names the table lacks go into apUnknownColors.
// apInfo is the thread's frame info.
static void drawStatistics(
    int32_t aiFrame,
    const hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>* apFrames,
    uint32_t* apPixels,
    int32_t aiHeight,
    const hkMonitorStreamColorTable* apColorTable,
    int32_t aiStride,
    int32_t aiWidth,
    float afFrameTime,
    float afStartTime,
    hkPointerMap<const char*, uint32_t>* apUnknownColors,
    const hkMonitorStreamFrameInfo* apInfo
);

// The 7-row bitmaps of the digits 0 to 9 that drawDigit draws, top
// row first: a nonzero nibble 7 - column of a row means the pixel in that column (0 to 7) is drawn.
using DigitBitmaps = uint32_t[10][7];

// Each digit's bitmap in digitBitmaps, by digit.
using DigitBitmapTable = uint32_t* [10];

static DigitBitmaps digitBitmaps{{0x800, 0x8080, 0x80008, 0x80008, 0x80008, 0x8080, 0x800},
                                 {0x8800, 0x80800, 0x800, 0x800, 0x800, 0x800, 0x88888},
                                 {0x8880, 0x80008, 0x8, 0x80, 0x800, 0x8000, 0x88888},
                                 {0x88880, 0x8, 0x8, 0x888, 0x8, 0x8, 0x88880},
                                 {0x80, 0x880, 0x8080, 0x8080, 0x88888, 0x80, 0x80},
                                 {0x8888, 0x8000, 0x8000, 0x8880, 0x8, 0x8, 0x8880},
                                 {0x880, 0x8000, 0x80000, 0x80880, 0x88008, 0x80008, 0x8880},
                                 {0x88888, 0x8, 0x80, 0x800, 0x8000, 0x8000, 0x8000},
                                 {0x8880, 0x80008, 0x80008, 0x8880, 0x80008, 0x80008, 0x8880},
                                 {0x8880, 0x80008, 0x80008, 0x8888, 0x8, 0x80, 0x88800}};
static DigitBitmapTable digitBitmapTable{digitBitmaps[0], digitBitmaps[1], digitBitmaps[2], digitBitmaps[3],
                                         digitBitmaps[4], digitBitmaps[5], digitBitmaps[6], digitBitmaps[7],
                                         digitBitmaps[8], digitBitmaps[9]};

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkMonitorStreamStringMapStringMapTypeInfo = {
    "hkMonitorStreamStringMapStringMap",
    finishLoadedObjecthkMonitorStreamStringMapStringMap,
    cleanupLoadedObjecthkMonitorStreamStringMapStringMap,
    nullptr,
    &typeid(hkMonitorStreamStringMap::StringMap),
    sizeof(hkMonitorStreamStringMap::StringMap)
};
hkTypeInfo hkMonitorStreamStringMapTypeInfo = {
    "hkMonitorStreamStringMap",
    finishLoadedObjecthkMonitorStreamStringMap,
    cleanupLoadedObjecthkMonitorStreamStringMap,
    nullptr,
    &typeid(hkMonitorStreamStringMap),
    sizeof(hkMonitorStreamStringMap)
};
hkTypeInfo hkMonitorStreamColorTableColorPairTypeInfo = {
    "hkMonitorStreamColorTableColorPair",
    finishLoadedObjecthkMonitorStreamColorTableColorPair,
    cleanupLoadedObjecthkMonitorStreamColorTableColorPair,
    nullptr,
    &typeid(hkMonitorStreamColorTable::ColorPair),
    sizeof(hkMonitorStreamColorTable::ColorPair)
};

// Reflection of the monitor stream's string map and colour table: the classes' member tables, the classes, and the
// colour table's type info.
using hkMonitorStreamStringMapStringMapClass_MembersTable = const hkClassMember[2];
static hkMonitorStreamStringMapStringMapClass_MembersTable hkMonitorStreamStringMapStringMapClass_Members{
    {"id",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT64},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::ALIGN_8},
     offsetof(hkMonitorStreamStringMap::StringMap, m_id),
     nullptr},
    {"string",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkMonitorStreamStringMap::StringMap, m_string),
     nullptr}
};
using hkMonitorStreamStringMapClass_MembersTable = const hkClassMember[1];
static hkMonitorStreamStringMapClass_MembersTable hkMonitorStreamStringMapClass_Members{
    {"map",
     &hkMonitorStreamStringMapStringMapClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     offsetof(hkMonitorStreamStringMap, m_map),
     nullptr}
};

// The color member's m_enum is null in the static table; an initialiser stores the hkColor ExtendedColors enum's
// pointer into it at start-up.
using hkMonitorStreamColorTableColorPairClass_MembersTable = hkClassMember[2];
static hkMonitorStreamColorTableColorPairClass_MembersTable hkMonitorStreamColorTableColorPairClass_Members{
    {"colorName",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkMonitorStreamColorTable::ColorPair, m_colorName),
     nullptr},
    {"color",
     nullptr,
     hkColorExtendedColorsEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT32},
     0,
     {0},
     offsetof(hkMonitorStreamColorTable::ColorPair, m_color),
     nullptr}
};
using hkMonitorStreamColorTableClass_MembersTable = const hkClassMember[2];
static hkMonitorStreamColorTableClass_MembersTable hkMonitorStreamColorTableClass_Members{
    {"colorPairs",
     &hkMonitorStreamColorTableColorPairClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     offsetof(hkMonitorStreamColorTable, m_colorPairs),
     nullptr},
    {"defaultColor",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkMonitorStreamColorTable, m_defaultColor),
     nullptr}
};
hkClass hkMonitorStreamStringMapStringMapClass(
    "hkMonitorStreamStringMapStringMap",
    nullptr,
    sizeof(hkMonitorStreamStringMap::StringMap),
    nullptr,
    0,
    nullptr,
    0,
    hkMonitorStreamStringMapStringMapClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkMonitorStreamStringMapClass(
    "hkMonitorStreamStringMap",
    nullptr,
    sizeof(hkMonitorStreamStringMap),
    nullptr,
    0,
    nullptr,
    0,
    hkMonitorStreamStringMapClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkMonitorStreamColorTableColorPairClass(
    "hkMonitorStreamColorTableColorPair",
    nullptr,
    sizeof(hkMonitorStreamColorTable::ColorPair),
    nullptr,
    0,
    nullptr,
    0,
    hkMonitorStreamColorTableColorPairClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkMonitorStreamColorTableClass(
    "hkMonitorStreamColorTable",
    &hkReferencedObjectClass,
    sizeof(hkMonitorStreamColorTable),
    nullptr,
    0,
    nullptr,
    0,
    hkMonitorStreamColorTableClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkMonitorStreamColorTableTypeInfo = {
    "hkMonitorStreamColorTable",
    finishLoadedObjecthkMonitorStreamColorTable,
    cleanupLoadedObjecthkMonitorStreamColorTable,
    getVtablehkMonitorStreamColorTable(),
    &typeid(hkMonitorStreamColorTable),
    sizeof(hkMonitorStreamColorTable)
};

static hkMonitorStreamAnalyzer::Node* findChildNode(hkMonitorStreamAnalyzer::Node* a1, double a2, int32_t a3) {
    hkMonitorStreamAnalyzer::Node** i;
    hkMonitorStreamAnalyzer::Node* v8;
    int32_t m_size = a1->m_children.m_size;
    int32_t v4 = 0;
    if (m_size > 2) {
        do {
            hkMonitorStreamAnalyzer::Node* v5 = a1->m_children.m_data[(m_size + v4) >> 1];
            if (v5->m_type.m_storage == 2) {
                break;
            }
            if (!(a2 < v5->m_absoluteStartTime)) {
                v4 = (m_size + v4) >> 1;
            }
            else {
                m_size = (m_size + v4) >> 1;
            }
        }
        while (m_size - v4 > 2);
    }
    int32_t v6 = v4;
    if (v4 >= m_size) {
        return nullptr;
    }
    for (i = &a1->m_children.m_data[v4];; ++i) {
        v8 = *i;
        if ((*i)->m_type.m_storage != 2 && v8->m_value[a3] + v8->m_absoluteStartTime >= a2) {
            break;
        }
        if (++v6 >= m_size) {
            return nullptr;
        }
    }
    if (!(a2 >= v8->m_absoluteStartTime)) {
        return nullptr;
    }

    return *i;
}

// The type info's finish-loaded-object callback for hkMonitorStreamColorTable::ColorPair: its finish constructor
// finishes the colour name.
static void finishLoadedObjecthkMonitorStreamColorTableColorPair(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (&static_cast<hkMonitorStreamColorTable::ColorPair*>(p)->m_colorName) hkStringPtr(flag);
    }
}

// The type info's cleanup callback for hkMonitorStreamStringMap::StringMap: destroys the entry's string.
static void cleanupLoadedObjecthkMonitorStreamStringMapStringMap(void* a1) {
    static_cast<hkMonitorStreamStringMap::StringMap*>(a1)->m_string.~hkStringPtr();
}

// The type info's cleanup callback for hkMonitorStreamColorTable::ColorPair: the pair's destructor destroys its colour
// name.
static void cleanupLoadedObjecthkMonitorStreamColorTableColorPair(void* p) {
    static_cast<hkMonitorStreamColorTable::ColorPair*>(p)->m_colorName.~hkStringPtr();
}

// The type info's finish-loaded-object callback for hkMonitorStreamColorTable: the null check, then its finish
// constructor.
static void finishLoadedObjecthkMonitorStreamColorTable(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkMonitorStreamColorTable(aFlag);
    }
}

static const void* getVtablehkMonitorStreamColorTable() {
    return hkGetVtableOf<hkMonitorStreamColorTable>();
}

static void cleanupLoadedObjecthkMonitorStreamStringMap(void* a1) {
    static_cast<hkArray<hkMonitorStreamStringMap::StringMap, hkContainerHeapAllocator>*>(a1)
        ->~hkArray<hkMonitorStreamStringMap::StringMap, hkContainerHeapAllocator>();
}

static void printSingleNodeValues(hkOstream& a1, float a2, const hkMonitorStreamAnalyzer::Node* a3) {
    alignas(64) char a1a[256];
    uint16_t v3 = a3->m_count[0];
    const uint16_t* m_count = a3->m_count;
    int32_t v5 = 0;
    if (v3) {
        v5 = v3;
    }
    uint16_t v6 = a3->m_count[1];
    if ((v6) && (v6 > v5)) {
        v5 = v6;
    }

    uint16_t v7 = a3->m_count[2];
    if ((v7) && (v7 > v5)) {
        v5 = v7;
    }

    uint16_t v8 = a3->m_count[3];
    if ((v8) && (v8 > v5)) {
        v5 = v8;
    }

    uint16_t v9 = a3->m_count[4];
    if ((v9) && (v9 > v5)) {
        v5 = v9;
    }

    uint16_t v10 = a3->m_count[5];
    if ((v10) && (v10 > v5)) {
        v5 = v10;
    }

    uint16_t v11 = a3->m_count[6];
    if ((v11) && (v11 > v5)) {
        v5 = v11;
    }

    uint16_t v12 = a3->m_count[7];
    if ((v12) && (v12 > v5)) {
        v5 = v12;
    }

    if (!(a2 > 0.0f)) {
        hkString::snprintf(a1a, 200, "%s (%i)", a3->m_name, v5);
    }
    else {
        hkString::snprintf(a1a, 200, "%s (%i) %4.1f%%", a3->m_name, v5, a3->m_value[1] * 100.0 / a2);
    }
    a1.printf("%-34s%s", a1a, "\t\t\t");
    for (float fValue : a3->m_value) {
        if (*m_count) {
            a1.printf("% 12.3f: ", fValue);
        }
        ++m_count;
    }
    a1.printf("\n");
}

// Writes the image as an uncompressed 32-bit TGA: the 18-byte header (image type 2, width, height, 32 bits per
// pixel), then width * height pixels.
static bool saveToTGA(int32_t* a1, hkOstream& a2, int32_t a3, int32_t a4) {
    char header[18];
    hkString::memSet(header, 0, 18);
    header[2] = 2;
    header[12] = static_cast<char>(a3 & 0xFF);
    header[13] = static_cast<char>(a3 >> 8);
    header[14] = static_cast<char>(a4 & 0xFF);
    header[15] = static_cast<char>(a4 >> 8);
    header[16] = 32;
    a2.write(header, 18);
    // The pixels go out as raw bytes.
    a2.write(reinterpret_cast<const char*>(a1), 4 * (a3 * a4));
    return true;
}

// Draws the four decimal digits of a1 (thousands first) into the image at r_eax, a2 pixels wide, each digit 7
// pixels further right.
static int32_t drawNumber(int32_t a1, int32_t a2, int32_t* r_eax) {
    int32_t result;
    for (int32_t i = 1000; i >= 1; i /= 10) {
        drawDigit(a2, r_eax, a1 / i % 10);
        result = i / 10;
        r_eax += 7;
    }
    return result;
}

// Colours one frame's columns of the statistics texture: samples the frame's tree at aiWidth times
// (outputStatsForFrame) and gives each column the colour of the sampled node's name, else of its nearest ancestor's,
// from the colour table. Names the table has go into a local cache; names it lacks go into apUnknownColors with the
// colour found so far. Each colour is stored aiHeight times down the column, aiStride pixels apart.
static void drawStatistics(
    int32_t aiFrame,
    const hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>* apFrames,
    uint32_t* apPixels,
    int32_t aiHeight,
    const hkMonitorStreamColorTable* apColorTable,
    int32_t aiStride,
    int32_t aiWidth,
    float afFrameTime,
    float afStartTime,
    hkPointerMap<const char*, uint32_t>* apUnknownColors,
    const hkMonitorStreamFrameInfo* apInfo
) {
    hkPointerMap<const char*, uint32_t> knownColors;
    const float sampleWidth = afFrameTime / static_cast<double>(aiWidth);
    hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator> samples;
    samples.reserve(aiWidth);
    if (apFrames->m_data[aiFrame]->m_children.m_size > 0) {
        samples.m_size = 0;
        const int32_t timer = apInfo->m_absoluteTimeCounter.m_storage !=
                static_cast<uint32_t>(hkMonitorStreamFrameInfo::ABSOLUTE_TIME_TIMER_0)
            ? apInfo->m_indexOfTimer1
            : apInfo->m_indexOfTimer0;
        outputStatsForFrame(afStartTime, sampleWidth, timer, aiWidth, apFrames->m_data[aiFrame], &samples);
        const int32_t numSamples = samples.m_size;
        for (int32_t i = 0; i < numSamples; ++i) {
            if (i >= aiWidth) {
                break;
            }
            uint32_t color = 0xFFFFFFFFu;
            for (hkMonitorStreamAnalyzer::Node* node = samples.m_data[i]; node; node = node->m_parent) {
                const char* name = node->m_name;
                uint32_t found;
                if (knownColors.get(name, &found) == HK_SUCCESS) {
                    color = found;
                    break;
                }
                if (apUnknownColors->get(name, &found) == HK_SUCCESS) {
                    color = found;
                    continue;
                }
                bool inTable = false;
                for (const hkMonitorStreamColorTable::ColorPair& rPair : apColorTable->m_colorPairs) {
                    if (!hkString::strCasecmp(rPair.m_colorName.cString(), name)) {
                        color = rPair.m_color;
                        knownColors.insert(name, color);
                        inTable = true;
                        break;
                    }
                }
                if (inTable) {
                    break;
                }
                apUnknownColors->insert(name, color);
            }
            uint32_t* pPixel = &apPixels[i];
            for (int32_t y = 0; y < aiHeight; ++y) {
                *pPixel = color;
                pPixel += aiStride;
            }
        }
    }
}

// Draws digit r_eax (an 8 by 7 pixel bitmap of digitBitmaps) into the image at a2, a1 pixels wide, bottom row
// first; a drawn pixel becomes opaque black.
static void drawDigit(int32_t a1, int32_t* a2, int32_t r_eax) {
    uint32_t* pBottomRow = &digitBitmapTable[r_eax][6];
    for (int32_t shift = 0; shift < 32; shift += 4) {
        int32_t* pPixel = a2 + shift / 4;
        uint32_t* pRow = pBottomRow;
        int32_t rows = 7;
        do {
            if (((*pRow << shift) & 0xF0000000u) != 0) {
                *pPixel = static_cast<int32_t>(0xFF000000u);
            }
            --pRow;
            pPixel += a1;
            --rows;
        }
        while (rows);
    }
}

// m_id is left as it is.
hkMonitorStreamStringMap::StringMap::StringMap() {}

const hkClass& hkMonitorStreamStringMap::StringMap::staticClass() {
    return hkMonitorStreamStringMapStringMapClass;
}

hkMonitorStreamStringMap::hkMonitorStreamStringMap() {}

const hkClass& hkMonitorStreamStringMap::staticClass() {
    return hkMonitorStreamStringMapClass;
}

const hkClass& hkMonitorStreamColorTable::ColorPair::staticClass() {
    return hkMonitorStreamColorTableColorPairClass;
}

// The colour pairs (and their names) go with their array's destructor.
hkMonitorStreamColorTable::~hkMonitorStreamColorTable() {}

void hkMonitorStreamColorTable::setupDefaultColorTable() {
    addColorPair("Physics", (-12566464));
    addColorPair("Narrowphase", (-10185235));
    addColorPair("Broadphase", (-16776961));
    addColorPair("3AxisSweep", (-15132304));
    addColorPair("WaitForExport", (-12042869));
    addColorPair("SplitIsle", (-5952982));
    addColorPair("PostCollide", (-657956));
    addColorPair("Maintenance", (-256));
    addColorPair("InterIsland", (-256));
    addColorPair("PendingOps", (-256));
    addColorPair("ConstraintCallbacks", (-256));
    addColorPair("SingleObj", (-2968436));
    addColorPair("TOIs", (-10496));
    addColorPair("PostSimCB", (-663885));
    addColorPair("Integrate", (-16744448));
    addColorPair("Actions", (-16711809));
    addColorPair("SetupJobs", (-13447886));
    addColorPair("BuildJacTask", (-13726889));
    addColorPair("BuildAccumulators", (-8355840));
    addColorPair("BuildJacobians", (-14513374));
    addColorPair("Solver", (-6751336));
    addColorPair("Solve", (-6751336));
    addColorPair("IntegrateMotions", (-7278960));
    addColorPair("SolverExport", (-7357297));
    addColorPair("GetNextJob", (-7667712));
    addColorPair("WaitForSignal", (-8388608));
    addColorPair("LockQueue", (-65536));
    addColorPair("CriticalLock", (-2354116));
    addColorPair("AnimationJobs", (-8388480));
    addColorPair("SampComb", (-2461482));
    addColorPair("Sample", (-12004916));
    addColorPair("Combine", (-3730043));
    addColorPair("Display Prepare", (-16777077));
    addColorPair("Display Release", (-16777077));
    addColorPair("Collide", (-16751616));
    addColorPair("hkSpuMoppRaycastQuery", (-7114533));
    addColorPair("KdBuildSetupJob", (-16777077));
    addColorPair("KdBuildFastDist", (-15132304));
    addColorPair("KdBuildFastDistSub", (-10185235));
}

uint32_t hkMonitorStreamColorTable::findColor(const char* color) {
    int32_t v3 = 0;
    if (m_colorPairs.m_size <= 0) {
        return m_defaultColor;
    }
    while (hkString::strCasecmp(
        reinterpret_cast<const char*>(
            reinterpret_cast<uintptr_t>(m_colorPairs.m_data[v3].m_colorName.m_stringAndFlag) & ~uintptr_t(1)
        ),
        color
    )) {
        if (++v3 >= m_colorPairs.m_size) {
            return m_defaultColor;
        }
    }
    return m_colorPairs.m_data[v3].m_color;
}

const hkClass& hkMonitorStreamColorTable::staticClass() {
    return hkMonitorStreamColorTableClass;
}

// A node with no children and cleared figures, appended to apParent's children when there is one.
hkMonitorStreamAnalyzer::Node::Node(Node* apParent, const char* apcName, NodeType aeType)
  : m_absoluteStartTime(0.0), m_parent(apParent), m_name(apcName), m_userFlags(0) {
    m_type = aeType;
    if (apParent) {
        apParent->m_children.pushBack(this);
    }
    for (int32_t i = 0; i < 8; ++i) {
        m_value[i] = 0.0f;
        m_count[i] = 0;
    }
}

// The node owns its children; their pointer array goes with its own destructor.
hkMonitorStreamAnalyzer::Node::~Node() {
    for (Node* pChild : m_children) {
        delete pChild;
    }
}

// Each timer index the frame uses (an unsigned compare with 7) gets the tick difference (unsigned) times its factor,
// and one more count; the start time is the absolute counter's ticks times its factor.
void hkMonitorStreamAnalyzer::Node::setTimers(
    const hkMonitorStreamFrameInfo& frameInfo,
    const hkMonitorStream::TimerCommand& start,
    const hkMonitorStream::TimerCommand& end
) {
    const uint32_t timer0 = static_cast<uint32_t>(frameInfo.m_indexOfTimer0);
    if (timer0 <= 7) {
        m_value[timer0] = static_cast<float>(
            static_cast<double>(end.m_time0 - start.m_time0) * frameInfo.m_timerFactor0 + m_value[timer0]
        );
        ++m_count[timer0];
    }
    const uint32_t timer1 = static_cast<uint32_t>(frameInfo.m_indexOfTimer1);
    if (timer1 <= 7) {
        m_value[timer1] = static_cast<float>(
            static_cast<double>(end.m_time1 - start.m_time1) * frameInfo.m_timerFactor1 + m_value[timer1]
        );
        ++m_count[timer1];
    }
    const uint32_t absoluteTimeCounter = frameInfo.m_absoluteTimeCounter.m_storage;
    if (absoluteTimeCounter == 0) {
        m_absoluteStartTime = frameInfo.m_timerFactor0 * static_cast<double>(start.m_time0);
    }
    else if (absoluteTimeCounter == 1) {
        m_absoluteStartTime = frameInfo.m_timerFactor1 * static_cast<double>(start.m_time1);
    }
}

void hkMonitorStreamAnalyzer::Node::operator delete(void* apMemory, size_t aSize) {
    hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
}

int32_t hkMonitorStreamAnalyzer::ThreadDrawInput::computePerThreadHeightToFit(
    int32_t textureHeight,
    int32_t numFrames,
    int32_t numThreads,
    int32_t frameGap,
    int32_t threadGap
) {
    return (textureHeight / numFrames - frameGap) / numThreads - threadGap;
}

// The debug buffer reserved for memorySize bytes and the per-thread frame lists sized by resetNumThreads.
hkMonitorStreamAnalyzer::hkMonitorStreamAnalyzer(int32_t memorySize, int32_t numThreads, int32_t numSpus)
  : m_nodeIdForFrameOverview("Physics") {
    g_lastFrameTime = 16666.6f;
    m_data.reserve(memorySize);
    resetNumThreads(numThreads, numSpus);
}

void hkMonitorStreamAnalyzer::checkAllThreadsCapturedSameNumFrames() const {
    ;
}

// Each thread's tree goes into an inplace array of six nodes, is drawn and freed.
void hkMonitorStreamAnalyzer::drawThreadsToTga(
    const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
    hkOstream& outStream
) {
    using NodeArray = hkInplaceArray<Node*, 6, hkContainerHeapAllocator>;
    const int32_t numThreads = m_frameInfos.m_size;
    NodeArray nodes;
    nodes.setSize(numThreads);
    for (int32_t i = 0; i < numThreads; ++i) {
        hkBool reuse;
        reuse.m_bool = false;
        nodes.m_data[i] = makeStatisticsTreeForMultipleFrames(i, reuse);
    }
    writeStatisticsDetailsToTga(nodes, input, m_frameInfos, outStream, nullptr);
    for (int32_t i = 0; i < numThreads; ++i) {
        Node* node = nodes.m_data[i];
        if (node) {
            node->~Node();
            hkMemoryRouter::getInstance().m_heap->blockFree(node, sizeof(Node));
        }
    }
}

char* hkMonitorStreamAnalyzer::getStreamBegin() {
    if (m_data.m_size) {
        return static_cast<hkArrayBase<char>&>(m_data).begin();
    }

    return nullptr;
}

hkBool hkMonitorStreamAnalyzer::captureFrameDetails(
    const char* monitorStreamBegin,
    const char* monitorStreamEnd,
    hkMonitorStreamFrameInfo& info
) {
    int32_t v6 = monitorStreamEnd - monitorStreamBegin;
    if ((m_data.m_capacityAndFlags & 0x3FFFFFFF) - m_data.m_size >= monitorStreamEnd - monitorStreamBegin) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>* v9 = &m_frameInfos.m_data[info.m_threadId];
        if (v9->m_size == (v9->m_capacityAndFlags & hkArrayBase<hkMonitorStreamFrameInfo>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, v9, sizeof(hkMonitorStreamFrameInfo));
        }
        hkMonitorStreamFrameInfo* v10 = &v9->m_data[v9->m_size];
        if (v10) {
            ::new (v10) hkMonitorStreamFrameInfo;
        }
        int32_t m_size = v9->m_size;
        hkMonitorStreamFrameInfo* v12 = &v9->m_data[m_size];
        v9->m_size = m_size + 1;
        *v12 = info;
        int32_t v13 = m_data.m_size;
        v12->m_frameStreamStart = v13;
        v12->m_frameStreamEnd = v6 + v13;
        int32_t v14 = m_data.m_size;
        m_data.m_size = v14 + v6;
        hkString::memCpy(&m_data.m_data[v14], monitorStreamBegin, v6);
        return 1;
    }

    return 0;
}

// The prefix is a NUL-terminated char array on the debug allocator, set up by hkInitIndentString and reset to "" for
// each frame; each open timer adds two spaces to it, each closing one takes them away. The command strings were applied
// (applyStringMap): their first character is the command, the rest the timer's name (after the monitor type's
// character). 'F', 'O', 'P' and 'p' do not advance (the loop spins on them, as the game does); any other unknown
// command ends the dump.
void hkMonitorStreamAnalyzer::writeRawStatistics(hkOstream& os) {
    using PrefixArray = hkArray<char, hkContainerDebugAllocator>;
    PrefixArray prefix;
    hkInitIndentString(&prefix);
    auto openIndent = [&prefix] {
        prefix.m_data[prefix.m_size - 1] = ' ';
        prefix.pushBack(' ');
        prefix.pushBack('\0');
    };
    auto closeIndent = [&prefix] {
        prefix.m_size -= 2;
        prefix.m_data[prefix.m_size - 1] = '\0';
    };

    os.printf(
        "StatisticsDumpInfo(num_threads=%i, num_spus=%i, num_frames=%i)\n", m_numThreads, m_numSpus,
        m_frameInfos.m_data[0].m_size
    );
    for (const auto& infos : m_frameInfos) {
        for (int32_t frame = 0; frame < infos.m_size; ++frame) {
            const hkMonitorStreamFrameInfo& info = infos.m_data[frame];
            os.printf(
                "FrameInfo(heading='%s', frame=%i, thread_id=%i, time_counter=%i)\n", info.m_heading, frame,
                info.m_threadId, info.m_absoluteTimeCounter.m_storage
            );
            // The frame is an untyped byte stream of commands of several sizes, and is read as such.
            const char* current = m_data.m_data + info.m_frameStreamStart;
            const char* end = m_data.m_data + info.m_frameStreamEnd;
            prefix.setSize(1);
            prefix.m_data[0] = '\0';
            while (current < end) {
                const hkMonitorStream::Command* command = reinterpret_cast<const hkMonitorStream::Command*>(current);
                const hkMonitorStream::TimerCommand* timer =
                    reinterpret_cast<const hkMonitorStream::TimerCommand*>(current);
                const char* name = command->m_commandAndMonitor;
                switch (name[0]) {
                    case 'E':
                        closeIndent();
                        os.printf("%sTimerEnd('%s',%u,%u)\n", prefix.m_data, name + 2, timer->m_time0, timer->m_time1);
                        current += sizeof(hkMonitorStream::TimerCommand);
                        break;
                    case 'F':
                    case 'O':
                    case 'P':
                    case 'p': break;
                    case 'L': {
                        const hkMonitorStream::TimerBeginListCommand* list =
                            reinterpret_cast<const hkMonitorStream::TimerBeginListCommand*>(current);
                        os.printf(
                            "%sTimerBegin('%s', %u, %u)\n", prefix.m_data, name + 2, list->m_time0, list->m_time1
                        );
                        os.printf(
                            "%sTimerSplit('%s', %u, %u)\n", prefix.m_data, list->m_nameOfFirstSplit + 2, list->m_time0,
                            list->m_time1
                        );
                        openIndent();
                        current += sizeof(hkMonitorStream::TimerBeginListCommand);
                        break;
                    }
                    case 'M': {
                        const hkMonitorStream::AddValueCommand* value =
                            reinterpret_cast<const hkMonitorStream::AddValueCommand*>(current);
                        os.printf("%sAddValue(%s,%f)\n", prefix.m_data, name + 1, static_cast<double>(value->m_value));
                        current += sizeof(hkMonitorStream::AddValueCommand);
                        break;
                    }
                    case 'N': current += sizeof(hkMonitorStream::Command); break;
                    case 'S':
                        closeIndent();
                        os.printf(
                            "%sTimerSplit('%s', %u, %u)\n", prefix.m_data, name + 2, timer->m_time0, timer->m_time1
                        );
                        openIndent();
                        current += sizeof(hkMonitorStream::TimerCommand);
                        break;
                    case 'T':
                        os.printf(
                            "%sTimerBegin('%s',%u,%u)\n", prefix.m_data, name + 2, timer->m_time0, timer->m_time1
                        );
                        openIndent();
                        current += sizeof(hkMonitorStream::TimerCommand);
                        break;
                    case 'l':
                        closeIndent();
                        os.printf(
                            "%sTimerEnd('%s', %u, %u)\n", prefix.m_data, name + 2, timer->m_time0, timer->m_time1
                        );
                        current += sizeof(hkMonitorStream::TimerCommand);
                        break;
                    case 'm': {
                        const hkMonitorStream::MemoryCommand* memory =
                            reinterpret_cast<const hkMonitorStream::MemoryCommand*>(current);
                        const int32_t sizeAndFlags = memory->m_sizeAndFlags;
                        os.printf(
                            "%s%s%s(ptr=0x%p, nbytes=%i)\n", prefix.m_data, sizeAndFlags < 0 ? "Free" : "Alloc",
                            name + 1, memory->m_ptr, sizeAndFlags & 0x7FFFFFFF
                        );
                        current += sizeof(hkMonitorStream::MemoryCommand);
                        break;
                    }
                    default: return;
                }
            }
        }
    }
}

// The root's children are sized to the frame count and each frame's tree is stored into its slot.
hkMonitorStreamAnalyzer::Node*
hkMonitorStreamAnalyzer::makeStatisticsTreeForMultipleFrames(int32_t threadId, hkBool reuseNodesIfPossible) {
    void* memory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(Node));
    Node* root = memory ? ::new (memory) Node(nullptr, "/", Node::NODE_TYPE_DIRECTORY) : nullptr;
    const int32_t numFrames = m_frameInfos.m_data[threadId].m_size;
    root->m_children.setSize(numFrames);
    for (int32_t i = 0; i < m_frameInfos.m_data[threadId].m_size; ++i) {
        const hkMonitorStreamFrameInfo& info = m_frameInfos.m_data[threadId].m_data[i];
        const char* frameStart = m_data.m_data + info.m_frameStreamStart;
        const char* frameEnd = m_data.m_data + info.m_frameStreamEnd;
        Node** slot = &root->m_children.m_data[i];
        *slot = makeStatisticsTreeForSingleFrame(frameStart, frameEnd, info, info.m_heading, reuseNodesIfPossible);
    }
    return root;
}

// One tree per thread (makeStatisticsTreeForMultipleFrames, no node reuse) into a heap array of nodes, written with the
// multithreaded summary, then freed.
void hkMonitorStreamAnalyzer::writeStatistics(hkOstream& outStream, int32_t reportLevel) {
    using NodeArray = hkArray<Node*, hkContainerHeapAllocator>;
    NodeArray nodes;
    for (int32_t i = 0; i < m_frameInfos.m_size; ++i) {
        hkBool reuse;
        reuse.m_bool = false;
        Node* tree = makeStatisticsTreeForMultipleFrames(i, reuse);
        nodes.pushBack(tree);
    }
    writeStatisticsDetails(outStream, nodes, m_numThreads, m_numSpus, reportLevel, m_nodeIdForFrameOverview, true);
    for (Node* node : nodes) {
        if (node) {
            node->~Node();
            hkMemoryRouter::getInstance().m_heap->blockFree(node, sizeof(Node));
        }
    }
}

void hkMonitorStreamAnalyzer::reset() {
    int32_t v1 = 0;
    bool v2 = m_frameInfos.m_size <= static_cast<int32_t>(0u);
    m_data.m_size = 0;
    if (!v2) {
        int32_t v3 = 0;
        do {
            ++v1;
            m_frameInfos.m_data[v3++].m_size = 0;
        }
        while (v1 < m_frameInfos.m_size);
    }
}

// Resizes m_frameInfos to numThreads + numSpus: the dropped arrays are freed and the new ones are empty; then sets the
// counts and calls reset().
void hkMonitorStreamAnalyzer::resetNumThreads(int32_t numThreads, int32_t numSpus) {
    m_frameInfos.setSize(numThreads + numSpus);
    m_numThreads = numThreads;
    m_numSpus = numSpus;
    reset();
}

// The line is built in textLine through lineOs and written to os with a newline. Each value column is padded with
// spaces to indentation + column * width + (depth - 1) * tab spacing, measured against the line so far (textLine's
// size, reloaded each time), or gets a tab. The per-group sums are floats; each average is the sum divided by the count
// as a double. The thread count sum is kept in a local.
void hkMonitorStreamAnalyzer::showCombinedThreadSummaryRec(
    hkOstream& os,
    Node* node,
    int32_t recursionDepth,
    int32_t numThreads,
    int32_t numSpus,
    const CombinedThreadSummaryOptions& options
) {
    hkArray<char> textLine;
    alignas(4) uint8_t lineOsStorage[sizeof(hkOstream)];
    hkOstream& lineOs = *::new (lineOsStorage) hkOstream(textLine);

    int32_t numColumns = numThreads + numSpus;
    if (recursionDepth != 0) {
        if (options.m_displayPartialTree.m_bool) {
            lineOs << (options.m_activeNode == node ? options.m_rightArrowChar : ' ');
        }
        for (int32_t i = (recursionDepth - 1) * options.m_tabSpacingForTimerNames; i > 0; --i) {
            lineOs << ' ';
        }
        if (options.m_displayPartialTree.m_bool) {
            if (node->m_children.m_size <= 0) {
                lineOs << ' ';
            }
            else if (node->m_userFlags & 1) {
                lineOs << options.m_downArrowChar;
            }
            else {
                lineOs << options.m_rightArrowChar;
            }
        }

        int32_t totalCount = 0;
        for (int32_t i = 0; i < numColumns; ++i) {
            totalCount += node->m_count[i];
        }
        lineOs.printf("%s (%i) ", node->m_name, totalCount);

        float threadSum = 0.0f;
        float spuSum = 0.0f;
        int32_t threadCount = 0;
        int32_t spuCount = 0;
        for (int32_t i = 0; i < numColumns; ++i) {
            if (options.m_useTabsNotSpacesForColumns.m_bool) {
                lineOs << '\t';
            }
            else {
                for (int32_t pad = options.m_indentationToFirstTimerValue + i * options.m_timerColumnWidth +
                         (recursionDepth - 1) * options.m_tabSpacingForTimerValues - textLine.m_size;
                     pad > 0; --pad) {
                    lineOs << ' ';
                }
            }
            lineOs.printf("%-6.3f (%i)", static_cast<double>(node->m_value[i]), static_cast<int32_t>(node->m_count[i]));
            if (i < numThreads) {
                threadSum = node->m_value[i] + threadSum;
                threadCount += node->m_count[i];
            }
            else {
                spuSum = node->m_value[i] + spuSum;
                spuCount += node->m_count[i];
            }
        }

        if (numThreads > 1) {
            if (options.m_useTabsNotSpacesForColumns.m_bool) {
                lineOs << '\t';
            }
            else {
                for (int32_t pad = options.m_indentationToFirstTimerValue + numColumns * options.m_timerColumnWidth +
                         (recursionDepth - 1) * options.m_tabSpacingForTimerValues - textLine.m_size;
                     pad > 0; --pad) {
                    lineOs << ' ';
                }
            }
            ++numColumns;
            lineOs.printf("%-6.3f (%i)", static_cast<double>(threadSum) / numThreads, threadCount);
        }
        if (numSpus > 1) {
            if (options.m_useTabsNotSpacesForColumns.m_bool) {
                lineOs << '\t';
            }
            else {
                for (int32_t pad = options.m_indentationToFirstTimerValue + numColumns * options.m_timerColumnWidth +
                         (recursionDepth - 1) * options.m_tabSpacingForTimerValues - textLine.m_size;
                     pad > 0; --pad) {
                    lineOs << ' ';
                }
            }
            lineOs.printf("%-6.3f (%i)", static_cast<double>(spuSum) / numSpus, spuCount);
        }
        os << textLine.m_data;
        os << '\n';
    }

    if (!options.m_displayPartialTree.m_bool || (node->m_userFlags & 1)) {
        for (Node* pChild : node->m_children) {
            showCombinedThreadSummaryRec(os, pChild, recursionDepth + 1, numThreads, numSpus, options);
        }
    }

    lineOs.~hkOstream();
}

void hkMonitorStreamAnalyzer::mergeTreesForCombinedThreadSummary(
    Node* mainTree,
    Node* subTree,
    int32_t destTimerId,
    int32_t sourceTimerId,
    float smoothingFactor
) {
    hkScaleTimersRecursive(mainTree, destTimerId, smoothingFactor);
    mergeSubTree(mainTree, subTree, destTimerId, sourceTimerId, smoothingFactor);
}

// The header line is built in textLine through lineOs (hkOstream over the array): "Timer Name", a column per thread and
// SPU, and the averages when there is more than one thread or SPU; each column is padded with spaces to the indentation
// (the widest timer name, stored into options) plus its column times the column width, measured against the line so
// far, or gets a tab. Then the tree under it (showCombinedThreadSummaryRec).
void hkMonitorStreamAnalyzer::showCombinedThreadSummaryForSingleFrame(
    hkMonitorStreamAnalyzer::Node* node,
    int32_t numThreads,
    int32_t numSpus,
    hkOstream& os,
    hkMonitorStreamAnalyzer::CombinedThreadSummaryOptions& options
) {
    using CharArray = hkArray<char, hkContainerHeapAllocator>;
    // The hkBool is zero-extended into the integer parameter findMaxTimerNameIndent takes.
    options.m_indentationToFirstTimerValue = findMaxTimerNameIndent(
        node, 0, options.m_tabSpacingForTimerNames, static_cast<uint8_t>(options.m_displayPartialTree.m_bool)
    );
    CharArray textLine;
    hkOstream lineOs(textLine);
    // Pads the line with spaces to the given column, or adds a tab.
    auto padToColumn = [&](int32_t column) {
        if (options.m_useTabsNotSpacesForColumns.m_bool) {
            lineOs << '\t';
        }
        else {
            for (int32_t pad =
                     options.m_timerColumnWidth * column - textLine.m_size + options.m_indentationToFirstTimerValue;
                 pad > 0; --pad) {
                lineOs << ' ';
            }
        }
    };
    lineOs.printf("Timer Name");
    if (numThreads > 1 || numSpus > 0) {
        for (int32_t i = 0; i < numThreads; ++i) {
            padToColumn(i);
            lineOs.printf("Thread %d", i);
        }
        for (int32_t i = 0; i < numSpus; ++i) {
            padToColumn(i + numThreads);
            lineOs.printf("Spu %d", i);
        }
        int32_t column = numThreads + numSpus;
        if (numThreads > 1) {
            padToColumn(column);
            ++column;
            lineOs.printf("Average Cpu");
        }
        if (numSpus > 1) {
            padToColumn(column);
            lineOs.printf("Average Spu");
        }
    }
    os << textLine.m_data;
    os.printf("\n\n");
    showCombinedThreadSummaryRec(os, node, 0, numThreads, numSpus, options);
}

// The command's first 32-bit value, swapped when endianSwap, is a padding value below 10 (skipped as 4 bytes; with
// endianSwap stored back swapped) or the address of the command's string, which is replaced through map (the lookup
// takes the value as read, unswapped) and decides the command's size. A missing string or an unknown command ends the
// walk. The 'O' case is handled like the 'L' case.
void hkMonitorStreamAnalyzer::applyStringMap(
    const char* frameStart,
    const char* frameEnd,
    const hkPointerMap<const void*, const char*, hkContainerHeapAllocator>& map,
    hkBool endianSwap
) {
    auto swap32 = [](uintptr_t v) -> uintptr_t {
        return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | ((v << 24) & 0xFF000000);
    };
    // The times of a timer command, swapped in place.
    auto swapTimes = [&swap32](hkMonitorStream::TimerCommand* apCommand) {
        apCommand->m_time0 = static_cast<uint32_t>(swap32(apCommand->m_time0));
        apCommand->m_time1 = static_cast<uint32_t>(swap32(apCommand->m_time1));
    };
    // The frame is an untyped byte stream of commands of several sizes, and is read as such.
    char* current = const_cast<char*>(frameStart);
    while (current < frameEnd) {
        hkMonitorStream::Command* command = reinterpret_cast<hkMonitorStream::Command*>(current);
        const char* commandAndMonitor = command->m_commandAndMonitor;
        if (endianSwap.m_bool) {
            const uintptr_t swapped = swap32(reinterpret_cast<uintptr_t>(commandAndMonitor));
            if (swapped < 10) {
                command->m_commandAndMonitor = reinterpret_cast<const char*>(swapped);
                current += sizeof(hkMonitorStream::Command);
                continue;
            }
        }
        else if (reinterpret_cast<uintptr_t>(commandAndMonitor) < 10) {
            current += sizeof(hkMonitorStream::Command);
            continue;
        }
        const char* str;
        if (map.get(commandAndMonitor, &str) != HK_SUCCESS) {
            return;
        }
        command->m_commandAndMonitor = str;
        switch (str[0]) {
            case 'E':
            case 'S':
            case 'T':
            case 'l': {
                hkMonitorStream::TimerCommand* timer = reinterpret_cast<hkMonitorStream::TimerCommand*>(current);
                current += sizeof(hkMonitorStream::TimerCommand);
                if (endianSwap.m_bool) {
                    swapTimes(timer);
                }
                break;
            }
            case 'F':
            case 'N':
            case 'P':
            case 'p': current += sizeof(hkMonitorStream::Command); break;
            case 'L': {
                hkMonitorStream::TimerBeginListCommand* list =
                    reinterpret_cast<hkMonitorStream::TimerBeginListCommand*>(current);
                current += sizeof(hkMonitorStream::TimerBeginListCommand);
                const char* split;
                if (map.get(list->m_nameOfFirstSplit, &split) == HK_SUCCESS) {
                    list->m_nameOfFirstSplit = split;
                }
                if (endianSwap.m_bool) {
                    swapTimes(list);
                }
                break;
            }
            case 'O': {
                hkMonitorStream::TimerBeginObjectNameCommand* object =
                    reinterpret_cast<hkMonitorStream::TimerBeginObjectNameCommand*>(current);
                current += sizeof(hkMonitorStream::TimerBeginObjectNameCommand);
                const char* objectName;
                if (map.get(object->m_objectName, &objectName) == HK_SUCCESS) {
                    object->m_objectName = objectName;
                }
                if (endianSwap.m_bool) {
                    swapTimes(object);
                }
                break;
            }
            case 'M': {
                hkMonitorStream::AddValueCommand* value = reinterpret_cast<hkMonitorStream::AddValueCommand*>(current);
                current += sizeof(hkMonitorStream::AddValueCommand);
                if (endianSwap.m_bool) {
                    float fValue = value->m_value;
                    const float fRead = fValue;
                    float fSwapped =
                        std::bit_cast<float>(static_cast<uint32_t>(swap32(std::bit_cast<uint32_t>(fRead))));
                    value->m_value = fSwapped;
                }
                break;
            }
            default: return;
        }
    }
}

// The arrow keys move the active node: up to the previous sibling's last expanded descendant (the parent from the first
// child, unless the parent is the root), down to the first child of an expanded node or else the next sibling of the
// node or of its nearest ancestor that has one, left collapses the node or moves to its parent and collapses that,
// right expands the node.
hkMonitorStreamAnalyzer::Node* hkMonitorStreamAnalyzer::navigateMonitors(
    const hkMonitorStreamAnalyzer::CursorKeys& keys,
    hkMonitorStreamAnalyzer::Node* activeNodeIn
) {
    Node* activeNode = activeNodeIn;
    if (!activeNodeIn) {
        return activeNode;
    }
    if (keys.m_upPressed.m_bool) {
        Node* parent = activeNodeIn->m_parent;
        Node** children = parent->m_children.m_data;
        Node* node = children[0];
        if (node == activeNodeIn) {
            if (parent->m_parent) {
                activeNode = parent;
            }
        }
        else {
            const int32_t numChildren = parent->m_children.m_size;
            for (int32_t i = 0; i < numChildren; ++i) {
                if (children[i] == activeNodeIn) {
                    node = children[i - 1];
                    break;
                }
            }
            for (int32_t n = node->m_children.m_size; n > 0; n = node->m_children.m_size) {
                if (!(node->m_userFlags & 1)) {
                    break;
                }
                node = node->m_children.m_data[n - 1];
            }
            activeNode = node;
        }
    }
    if (keys.m_downPressed.m_bool) {
        if (activeNode->m_children.m_size > 0 && (activeNode->m_userFlags & 1)) {
            activeNode = activeNode->m_children.m_data[0];
        }
        else {
            Node* node = activeNode;
            for (;;) {
                Node* parent = node->m_parent;
                const int32_t numChildren = parent->m_children.m_size;
                int32_t i = 0;
                for (; i < numChildren; ++i) {
                    if (parent->m_children.m_data[i] == node && i < numChildren - 1) {
                        break;
                    }
                }
                if (i < numChildren) {
                    activeNode = parent->m_children.m_data[i + 1];
                    break;
                }
                if (!parent->m_parent) {
                    break;
                }
                node = parent;
            }
        }
    }
    if (keys.m_leftPressed.m_bool) {
        const uint32_t userFlags = activeNode->m_userFlags;
        if (userFlags & 1) {
            activeNode->m_userFlags = userFlags & ~1u;
        }
        else {
            Node* parent = activeNode->m_parent;
            if (parent->m_parent) {
                activeNode = parent;
                parent->m_userFlags &= ~1u;
            }
        }
    }
    if (keys.m_rightPressed.m_bool) {
        const uint32_t userFlags = activeNode->m_userFlags;
        if (!(userFlags & 1)) {
            activeNode->m_userFlags = userFlags | 1;
        }
        activeNode->m_userFlags |= 1;
    }
    return activeNode;
}

// The sample's frame and thread come from y counted from the bottom, its column from x past the 32-pixel frame labels
// (none for one frame).
hkMonitorStreamAnalyzer::Node* hkMonitorStreamAnalyzer::reverseLookupNodeAtTgaSample(
    int32_t x,
    int32_t y,
    const hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodeList,
    const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
    hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator>& frameInfos
) {
    const int32_t numThreads = nodeList.m_size;
    const int32_t numFrames = input.m_numFrames;
    if (numThreads < 1) {
        return nullptr;
    }
    const int32_t startX = numFrames <= 1 ? 0 : 32;
    if (x < startX || x >= input.m_outputPixelWidth) {
        return nullptr;
    }
    const int32_t threadHeight = input.m_heightPerThread + input.m_gapBetweenThreads;
    const int32_t width = input.m_outputPixelWidth - startX;
    const int32_t frameHeight = numThreads * threadHeight + input.m_gapBetweenFrames;
    const int32_t height = numFrames * frameHeight;
    if (y < 0 || y >= height) {
        return nullptr;
    }
    float maxFrameTime = -1.0f;
    using FloatArray = hkArray<float, hkContainerHeapAllocator>;
    FloatArray startTimes;
    const int32_t frame = (height - y - 1) / frameHeight;
    const int32_t thread = (height - y - 1) % frameHeight / threadHeight;
    getTimerLimits(nodeList, input, frameInfos, maxFrameTime, startTimes);
    Node* node = getNodeSampledAtTick(
        frame, x - startX, &nodeList.m_data[thread]->m_children, width, maxFrameTime, startTimes.m_data[frame],
        frameInfos.m_data[thread].m_data
    );
    return node;
}

// Writes the version line, then by reportLevel's bits the summed timers of all frames (1), an ASCII bar per frame of
// the overview node's share of the average frame time (2), each frame's summary (4: one combined over the threads, or
// one per thread), with its timers by type (8), and each thread's details per frame (16).
void hkMonitorStreamAnalyzer::writeStatisticsDetails(
    hkOstream& outstream,
    hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodes,
    int32_t numThreads,
    int32_t numSpus,
    int32_t reportLevel,
    const char* nodeIdForFrameOverview,
    bool showMultithreadedSummary
) {
    outstream.printf("Havok version: %s\n", "Havok-7.1.0-r1");
    float fFrameTime = 0.0f;
    hkArray<Node*, hkContainerHeapAllocator>* pFrames = &nodes.m_data[0]->m_children;
    float fInvNumFrames;
    {
        Node sum(nullptr, "Sum", Node::NODE_TYPE_DIRECTORY);
        fInvNumFrames = static_cast<float>(1.0 / static_cast<double>(pFrames->m_size));
        for (Node* pFrame : *pFrames) {
            hkMakeSumRecursive(&sum, pFrame);
        }
        Node* pChild = hkFindChildByName(&sum, nodeIdForFrameOverview, static_cast<hkBool>(true));
        if (pChild) {
            do {
                fFrameTime = pChild->m_value[0] * fInvNumFrames + fFrameTime;
                pChild = hkFindNextChildByName(pChild, nodeIdForFrameOverview, &sum);
            }
            while (pChild);
            // Keeps a positive or NaN time.
            if (fFrameTime <= 0.0f) {
                fFrameTime = 1000.0f;
            }
        }
        else {
            fFrameTime = 1000.0f;
        }
        if ((reportLevel & 1) != 0) {
            outstream.printf("\n\n");
            outstream.printf("*********************************\n");
            outstream.printf("********** Total Times    *******\n");
            outstream.printf("*********************************\n");
            outstream.printf("Timers are added together\n");
            hkWriteRec(outstream, &sum, 0, fInvNumFrames);
        }
    }

    if ((reportLevel & 2) != 0) {
        outstream.printf("\n\n");
        outstream.printf("*********************************\n");
        outstream.printf("********** Per Frame Time *******\n");
        outstream.printf("*********************************\n");
        outstream.printf("Ascii Art all frames overview\n");
        char acBar[41];
        hkString::memSet(acBar, ' ', 40);
        acBar[40] = 0;
        for (int32_t f = 0; f < pFrames->m_size; ++f) {
            Node* pFrame = pFrames->m_data[f];
            if (pFrame->m_children.m_size == 0) {
                continue;
            }
            float fSum = 0.0f;
            const char* pName = "Unknown";
            for (Node* pChild = hkFindChildByName(pFrame, nodeIdForFrameOverview, static_cast<hkBool>(true)); pChild;
                 pChild = hkFindNextChildByName(pChild, nodeIdForFrameOverview, pFrame)) {
                fSum = pChild->m_value[0] + fSum;
                pName = pChild->m_name;
            }
            const float fBar = fSum * 20.0 / fFrameTime;
            int32_t iBar = static_cast<int32_t>(fBar);
            if (iBar < 0) {
                iBar = 0;
            }
            else if (iBar >= 40) {
                iBar = 39;
            }
            // A tab per four columns before the bar's end, spaces up to it, the '#', spaces to the next tab stop,
            // then tabs to column 40.
            char* pOut = acBar;
            int32_t iTabEnd = 0;
            if (iBar > 4) {
                for (int32_t k = 4; k < iBar; k += 4) {
                    *pOut++ = '\t';
                    iTabEnd += 4;
                }
            }
            int32_t iColumn = iTabEnd;
            if (iTabEnd < iBar) {
                const int32_t iSpaces = iBar - iTabEnd;
                std::memset(pOut, ' ', iSpaces);
                pOut += iSpaces;
                iColumn = iSpaces + iTabEnd;
            }
            *pOut++ = '#';
            ++iColumn;
            iTabEnd += 4;
            if (iColumn < iTabEnd) {
                const int32_t iSpaces = iTabEnd - iColumn;
                std::memset(pOut, ' ', iSpaces);
                pOut += iSpaces;
            }
            if (iTabEnd < 40) {
                const int32_t iTabs = static_cast<int32_t>((static_cast<uint32_t>(39 - iTabEnd) >> 2) + 1);
                std::memset(pOut, '\t', iTabs);
                pOut += iTabs;
            }
            *pOut = 0;
            outstream.printf(acBar);
            outstream.printf("%i %-12s %f\n", f, pName, static_cast<double>(fSum));
        }
    }

    if ((reportLevel & 4) != 0) {
        if (nodes.m_size != 1 && showMultithreadedSummary) {
            const int32_t iByType = reportLevel & 8;
            for (int32_t frame = 0; frame < nodes.m_data[0]->m_children.m_size; ++frame) {
                outstream.printf("\n");
                outstream.printf("****************************************\n");
                outstream.printf("****** Summary Frame:%i ******\n", frame);
                outstream.printf("****************************************\n");

                hkLocalArray<Node*> trees(nodes.m_size);
                trees.setSize(nodes.m_size);

                for (int32_t t = 0; t < nodes.m_size; ++t) {
                    // EXE-INLINED none: Node::operator new
                    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(Node));
                    Node* pTree = nullptr;
                    if (pMemory) {
                        pTree = ::new (pMemory) Node(nullptr, "", Node::NODE_TYPE_DIRECTORY);
                    }
                    trees.m_data[t] = pTree;
                    Node* pThread = nodes.m_data[t];
                    if (frame < pThread->m_children.m_size) {
                        hkMakeSumRecursive(trees.m_data[t], pThread->m_children.m_data[frame]);
                    }
                }
                // Only these options are written; the summary computes the indentation itself and reads the active node
                // and the arrows only for a partial tree.
                CombinedThreadSummaryOptions options;
                options.m_tabSpacingForTimerNames = 4;
                options.m_tabSpacingForTimerValues = 2;
                options.m_timerColumnWidth = 16;
                options.m_displayPartialTree.m_bool = false;
                options.m_useTabsNotSpacesForColumns.m_bool = false;
                for (int32_t t = 1; t < trees.m_size; ++t) {
                    mergeTreesForCombinedThreadSummary(trees.m_data[0], trees.m_data[t], t, 0, 0.0f);
                }
                showCombinedThreadSummaryForSingleFrame(trees.m_data[0], numThreads, numSpus, outstream, options);
                if (iByType) {
                    printStatisticsByType(outstream, trees.m_data[0], 0.01f);
                }
                for (int32_t t = 0; t < nodes.m_size; ++t) {
                    Node* pTree = trees.m_data[t];
                    if (pTree) {
                        // EXE-INLINED none: Node::operator delete
                        pTree->~Node();
                        hkMemoryRouter::getInstance().m_heap->blockFree(pTree, sizeof(Node));
                    }
                }
            }
        }
        else {
            for (int32_t frame = 0; frame < nodes.m_data[0]->m_children.m_size; ++frame) {
                for (int32_t t = 0; t < nodes.m_size; ++t) {
                    Node* pThread = nodes.m_data[t];
                    if (frame >= pThread->m_children.m_size) {
                        continue;
                    }
                    Node sum(nullptr, "Sum", Node::NODE_TYPE_DIRECTORY);
                    hkMakeSumRecursive(&sum, pThread->m_children.m_data[frame]);
                    outstream.printf("\n");
                    outstream.printf("****************************************\n");
                    outstream.printf("****** Summary Frame:%i Thread:%i ******\n", frame, t);
                    outstream.printf("****************************************\n");
                    outstream.printf("%s\n", pThread->m_children.m_data[frame]->m_name);
                    hkWriteRec(outstream, &sum, 0, 1.0f);
                    if ((reportLevel & 8) != 0) {
                        printStatisticsByType(outstream, &sum, 0.01f);
                    }
                }
            }
        }
    }

    if ((reportLevel & 16) != 0) {
        for (int32_t frame = 0; frame < nodes.m_data[0]->m_children.m_size; ++frame) {
            if (nodes.m_size <= 0) {
                continue;
            }
            // The SPU number counts from -numThreads with the thread index.
            int32_t iSpu = -numThreads;
            for (int32_t t = 0; t < nodes.m_size; ++t, ++iSpu) {
                Node* pThread = nodes.m_data[t];
                if (frame >= pThread->m_children.m_size) {
                    continue;
                }
                outstream.printf("\n\n");
                outstream.printf("***************************************\n");
                if (t < numThreads) {
                    outstream.printf("***** Details Frame-%i Thread:%i ******\n", frame, t);
                }
                else {
                    outstream.printf("***** Details Frame-%i Spu:%i ******\n", frame, iSpu);
                }
                outstream.printf("***************************************\n");
                outstream.printf("%s\n", pThread->m_children.m_data[frame]->m_name);
                hkWriteRec(outstream, pThread->m_children.m_data[frame], 0, 1.0f);
            }
        }
    }
}

// Collects every string the frame's commands point to (the command names, the first split names of timer lists and
// the object names) into map, each as its own key and value. A command word below 10 is a job marker, not a
// string, and is stepped over; an unknown command ends the walk.
void hkMonitorStreamAnalyzer::extractStringMap(
    const char* frameStart,
    const char* frameEnd,
    hkPointerMap<const void*, const char*, hkContainerHeapAllocator>& map
) {
    const char* current = frameStart;
    while (current < frameEnd) {
        // The frame is untyped bytes, read as the command structs at each step.
        const hkMonitorStream::Command* command = reinterpret_cast<const hkMonitorStream::Command*>(current);
        // The pointer's bits are compared unsigned against 10.
        if (reinterpret_cast<uintptr_t>(command->m_commandAndMonitor) < 10u) {
            current += sizeof(hkMonitorStream::Command);
            continue;
        }
        map.insert(command->m_commandAndMonitor, command->m_commandAndMonitor);
        switch (command->m_commandAndMonitor[0]) {
            case 'T':
            case 'E':
            case 'S':
            case 'l': current += sizeof(hkMonitorStream::TimerCommand); break;
            case 'L': {
                // The same untyped bytes, read as the longer command.
                const hkMonitorStream::TimerBeginListCommand* list =
                    reinterpret_cast<const hkMonitorStream::TimerBeginListCommand*>(current);
                map.insert(list->m_nameOfFirstSplit, list->m_nameOfFirstSplit);
                current += sizeof(hkMonitorStream::TimerBeginListCommand);
                break;
            }
            case 'O': {
                // The same untyped bytes, read as the longer command.
                const hkMonitorStream::TimerBeginObjectNameCommand* object =
                    reinterpret_cast<const hkMonitorStream::TimerBeginObjectNameCommand*>(current);
                map.insert(object->m_objectName, object->m_objectName);
                current += sizeof(hkMonitorStream::TimerBeginObjectNameCommand);
                break;
            }
            case 'M': current += sizeof(hkMonitorStream::AddValueCommand); break;
            case 'P':
            case 'p':
            case 'N': current += sizeof(hkMonitorStream::Command); break;
            default: return;
        }
    }
}

// The frame range is cut to the first thread's frames. startTimes gets one zeroed entry per requested frame; each
// frame's entry is the earliest start of the threads' top timers (from 1e30), and maxFrameTime the latest end of
// them relative to that start, on the frame's absolute timer. A start-time limit is added to each entry. A frame-time
// limit is the result as it is; without one, a single frame's limit moves 5% of the way from the smoothed value
// towards the next power of two (in milliseconds), by at most a quarter of it, and is at least 16666.
void hkMonitorStreamAnalyzer::getTimerLimits(
    const hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodeList,
    const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
    hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator>& frameInfos,
    float& maxFrameTime,
    hkArray<float, hkContainerHeapAllocator>& startTimes
) {
    const int32_t iNumThreads = nodeList.m_size;
    const int32_t iNumChildren = nodeList.m_data[0]->m_children.m_size;
    int32_t iFrameEnd = input.m_frameStart + input.m_numFrames;
    int32_t iNumFrames = input.m_numFrames;
    if (iFrameEnd > iNumChildren) {
        iNumFrames = iNumChildren - input.m_frameStart;
        iFrameEnd = iNumChildren;
    }
    if (iNumFrames <= 0) {
        return;
    }
    // EXE-INLINED none: hkArray<float>::setSize
    const int32_t iNewSize = input.m_numFrames;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t iHave = static_cast<int32_t>(startTimes.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK);
    if (iHave < iNewSize) {
        int32_t iRequest = 2 * iHave;
        if (iNewSize >= iRequest) {
            iRequest = iNewSize;
        }
        hkArrayUtil::_reserve(pHeap, &startTimes, iRequest, sizeof(float));
    }
    float* pFill = startTimes.m_data + startTimes.m_size;
    for (int32_t i = iNewSize - startTimes.m_size; i > 0; --i) {
        *pFill++ = 0.0f;
    }
    startTimes.m_size = iNewSize;
    maxFrameTime = 0.0f;
    const hkMonitorStreamFrameInfo* pInfos = frameInfos.m_data[0].m_data;
    for (int32_t iFrame = input.m_frameStart; iFrame < iFrameEnd; ++iFrame) {
        const int32_t iSlot = iFrame - input.m_frameStart;
        const hkMonitorStreamFrameInfo& info = pInfos[iFrame];
        const int32_t iTimer = info.m_absoluteTimeCounter.m_storage == hkMonitorStreamFrameInfo::ABSOLUTE_TIME_TIMER_0
            ? info.m_indexOfTimer0
            : info.m_indexOfTimer1;
        startTimes.m_data[iSlot] = 1e30f;
        for (int32_t iThread = 0; iThread < iNumThreads; ++iThread) {
            const hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>& children =
                nodeList.m_data[iThread]->m_children.m_data[iFrame]->m_children;
            if (children.m_size > 0) {
                for (const hkMonitorStreamAnalyzer::Node* pChild : children) {
                    const float fStart = static_cast<float>(pChild->m_absoluteStartTime);
                    const float fCurrent = startTimes.m_data[iSlot];
                    startTimes.m_data[iSlot] = fCurrent > fStart ? fStart : fCurrent;
                }
                for (const hkMonitorStreamAnalyzer::Node* pChild : children) {
                    const float fEnd = pChild->m_value[iTimer] + pChild->m_absoluteStartTime - startTimes.m_data[iSlot];
                    const float fMax = maxFrameTime;
                    maxFrameTime = fMax < fEnd ? fEnd : fMax;
                }
            }
        }
    }
    if (input.m_limitStartTime > 0.0f) {
        for (int32_t iFrame = input.m_frameStart; iFrame < iFrameEnd; ++iFrame) {
            const int32_t iSlot = iFrame - input.m_frameStart;
            startTimes.m_data[iSlot] = input.m_limitStartTime + startTimes.m_data[iSlot];
        }
    }
    if (input.m_limitFrameTime > 0.0f) {
        float fLimit = input.m_limitFrameTime;
        maxFrameTime = fLimit;
        return;
    }
    if (iNumFrames == 1) {
        // The next power of two above the truncated milliseconds (only the low 32 bits of the 64-bit result).
        uint32_t uiPower = static_cast<uint32_t>(static_cast<int64_t>(maxFrameTime / 1000.0));
        uiPower |= uiPower >> 1;
        uiPower |= uiPower >> 2;
        uiPower |= uiPower >> 4;
        uiPower |= uiPower >> 8;
        uiPower |= uiPower >> 16;
        ++uiPower;
        const double dSmoothed = g_lastFrameTime;
        const float fQuarter = 0.25 * dSmoothed;
        const float fTarget = 1000.0 * static_cast<double>(uiPower);
        const float fDiff = fTarget - dSmoothed;
        const float fStep = fDiff > fQuarter ? fQuarter : fDiff;
        const float fLimit = dSmoothed + fStep * 0.05000000074505806;
        maxFrameTime = fLimit;
        const float fClamped = fLimit < 16666.0 ? 16666.0f : fLimit;
        maxFrameTime = fClamped;
        g_lastFrameTime = fClamped;
    }
}

// The frame's commands build a tree under a new directory root. 'T', 'L' (the list's timer, then its first split's
// under it) and 'O' (named after the object) open timer nodes and push each opening command on both stacks; 'E' closes
// the open timer (its name, when given, must match the opening command's) and sets the node's times from the two
// commands, 'S' closes the open split and opens the next one under the same parent (its command replacing the stack's
// top), 'l' closes the last split and its list; each closing pops both stacks. 'P' and 'p' open and close a directory
// node, 'M' adds its value to a single-value node. A 32-bit value below 10 is padding (4 bytes), 'N' is skipped, 'm'
// skips a timer command, and 'F' does not advance, so the walk loops on it, as the game does. An empty stack, a missing
// parent, a name mismatch or an unknown command ends the walk. The second stack is only pushed and popped.
hkMonitorStreamAnalyzer::Node* hkMonitorStreamAnalyzer::makeStatisticsTreeForSingleFrame(
    const char* frameStart,
    const char* frameEnd,
    const hkMonitorStreamFrameInfo& frameInfo,
    const char* rootNodeName,
    hkBool reuseNodesIfPossible
) {
    using TimerCommand = hkMonitorStream::TimerCommand;
    using TimerStack = hkInplaceArray<TimerCommand, 16>;
    void* memory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(Node));
    Node* currentNode = memory ? ::new (memory) Node(nullptr, rootNodeName, Node::NODE_TYPE_DIRECTORY) : nullptr;
    Node* const rootNode = currentNode;
    TimerStack timerStack;
    TimerStack timerStackCopy;

    auto popBoth = [&timerStack, &timerStackCopy] {
        timerStack.popBack();
        timerStackCopy.setSize(timerStackCopy.m_size - 1);
    };

    // The frame is an untyped byte stream of commands of several sizes, and is read as such.
    const char* current = frameStart;
    while (current < frameEnd) {
        const hkMonitorStream::Command* command = reinterpret_cast<const hkMonitorStream::Command*>(current);
        const TimerCommand* timer = reinterpret_cast<const TimerCommand*>(current);
        const char* commandAndMonitor = command->m_commandAndMonitor;
        if (reinterpret_cast<uintptr_t>(commandAndMonitor) < 10) {
            current += sizeof(hkMonitorStream::Command);
            continue;
        }
        switch (commandAndMonitor[0]) {
            case 'T':
                currentNode = createNewNode(
                    commandAndMonitor + 2, Node::NODE_TYPE_TIMER, reuseNodesIfPossible.m_bool != 0, currentNode
                );
                timerStack.pushBack(*timer);
                timerStackCopy.pushBack(*timer);
                current += sizeof(TimerCommand);
                break;
            case 'E': {
                if (!timerStack.m_size) {
                    goto finish;
                }
                const char* name = commandAndMonitor + 2;
                const TimerCommand& start = timerStack.m_data[timerStack.m_size - 1];
                if (name[0] && hkString::strCmp(start.m_commandAndMonitor + 2, name)) {
                    goto finish;
                }
                currentNode->setTimers(frameInfo, start, *timer);
                currentNode = currentNode->m_parent;
                popBoth();
                current += sizeof(TimerCommand);
                break;
            }
            case 'O': {
                const hkMonitorStream::TimerBeginObjectNameCommand* object =
                    reinterpret_cast<const hkMonitorStream::TimerBeginObjectNameCommand*>(current);
                timerStack.pushBack(*timer);
                currentNode = createNewNode(
                    object->m_objectName, Node::NODE_TYPE_TIMER, reuseNodesIfPossible.m_bool != 0, currentNode
                );
                timerStackCopy.pushBack(*timer);
                current += sizeof(hkMonitorStream::TimerBeginObjectNameCommand);
                break;
            }
            case 'L': {
                const hkMonitorStream::TimerBeginListCommand* list =
                    reinterpret_cast<const hkMonitorStream::TimerBeginListCommand*>(current);
                timerStack.pushBack(*timer);
                timerStackCopy.pushBack(*timer);
                Node* listNode = createNewNode(
                    commandAndMonitor + 2, Node::NODE_TYPE_TIMER, reuseNodesIfPossible.m_bool != 0, currentNode
                );
                // The first split opens at the list's times.
                TimerCommand split;
                split.m_commandAndMonitor = list->m_nameOfFirstSplit;
                split.m_time0 = list->m_time0;
                split.m_time1 = list->m_time1;
                timerStack.pushBack(split);
                timerStackCopy.pushBack(split);
                currentNode = createNewNode(
                    split.m_commandAndMonitor + 2, Node::NODE_TYPE_TIMER, reuseNodesIfPossible.m_bool != 0, listNode
                );
                current += sizeof(hkMonitorStream::TimerBeginListCommand);
                break;
            }
            case 'S': {
                if (!timerStack.m_size) {
                    goto finish;
                }
                TimerCommand& start = timerStack.m_data[timerStack.m_size - 1];
                currentNode->setTimers(frameInfo, start, *timer);
                currentNode = createNewNode(
                    commandAndMonitor + 2, Node::NODE_TYPE_TIMER, reuseNodesIfPossible.m_bool != 0,
                    currentNode->m_parent
                );
                start.m_commandAndMonitor = timer->m_commandAndMonitor;
                start.m_time0 = timer->m_time0;
                start.m_time1 = timer->m_time1;
                current += sizeof(TimerCommand);
                break;
            }
            case 'l': {
                if (timerStack.m_size <= 1) {
                    goto finish;
                }
                const TimerCommand& split = timerStack.m_data[timerStack.m_size - 1];
                if (split.m_commandAndMonitor[0] != 'S') {
                    goto finish;
                }
                currentNode->setTimers(frameInfo, split, *timer);
                currentNode = currentNode->m_parent;
                popBoth();
                currentNode->setTimers(frameInfo, timerStack.m_data[timerStack.m_size - 1], *timer);
                currentNode = currentNode->m_parent;
                popBoth();
                current += sizeof(TimerCommand);
                break;
            }
            case 'M': {
                const hkMonitorStream::AddValueCommand* value =
                    reinterpret_cast<const hkMonitorStream::AddValueCommand*>(current);
                Node* node = createNewNode(
                    commandAndMonitor + 2, Node::NODE_TYPE_SINGLE, reuseNodesIfPossible.m_bool != 0, currentNode
                );
                const float sum = value->m_value + node->m_value[0];
                ++node->m_count[0];
                node->m_value[0] = sum;
                current += sizeof(hkMonitorStream::AddValueCommand);
                break;
            }
            case 'P':
                currentNode = createNewNode(
                    commandAndMonitor + 2, Node::NODE_TYPE_DIRECTORY, reuseNodesIfPossible.m_bool != 0, currentNode
                );
                current += sizeof(hkMonitorStream::Command);
                break;
            case 'p':
                currentNode = currentNode->m_parent;
                if (!currentNode) {
                    goto finish;
                }
                current += sizeof(hkMonitorStream::Command);
                break;
            case 'N': current += sizeof(hkMonitorStream::Command); break;
            case 'm': current += sizeof(TimerCommand); break;
            case 'F': break;
            default: goto finish;
        }
    }
finish:
    return rootNode;
}

int32_t hkMonitorStreamAnalyzer::findMaxTimerNameIndent(
    hkMonitorStreamAnalyzer::Node* node,
    int32_t recursionDepth,
    int32_t spacing,
    int32_t displayPartialTree
) {
    int32_t v4 = hkString::strLen(node->m_name);
    int32_t v5 = v4 + spacing * recursionDepth + 8;
    if (static_cast<uint8_t>(displayPartialTree) && (node->m_userFlags & 1) == 0) {
        return v4 + spacing * recursionDepth + 8;
    }
    for (Node* pChild : node->m_children) {
        int32_t MaxTimerNameIndent =
            hkMonitorStreamAnalyzer::findMaxTimerNameIndent(pChild, recursionDepth + 1, spacing, displayPartialTree);
        if (MaxTimerNameIndent > v5) {
            v5 = MaxTimerNameIndent;
        }
    }
    return v5;
}

// Draws the frames' timers into a new zeroed ARGB image (one pixel more per row than the output width), the newest
// frame at the bottom: per frame a 32-pixel frame index strip when there are several frames, then each thread's band.
// Blue lines mark each millisecond (when they are more than 5 pixels apart), shifted by the start-time limit's
// milliseconds, and double green lines each 16666 microseconds (a 60 Hz frame). No warning is issued per unknown timer
// when m_warnAboutMissingTimers is set.
void hkMonitorStreamAnalyzer::writeStatisticsDetailsToTexture(
    const hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodeList,
    const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
    hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator>& frameInfos,
    int32_t*& texture,
    int32_t& height,
    hkMonitorStreamAnalyzer::SampleInfo* sampleInfo
) {
    const int32_t iNumThreads = nodeList.m_size;
    if (iNumThreads < 1) {
        return;
    }
    float fMaxFrameTime = -1.0f;
    hkArray<float, hkContainerHeapAllocator> startTimes;
    getTimerLimits(nodeList, input, frameInfos, fMaxFrameTime, startTimes);
    const int32_t iAvailable = frameInfos.m_data[0].m_size - input.m_frameStart;
    const int32_t iNumFrames = input.m_numFrames > iAvailable ? iAvailable : input.m_numFrames;
    const int32_t iFrameEnd = input.m_frameStart + iNumFrames;
    const int32_t iThreadHeight = input.m_heightPerThread + input.m_gapBetweenThreads;
    const int32_t iHeight = (iThreadHeight * iNumThreads + input.m_gapBetweenFrames) * iNumFrames;
    const int32_t iXOffset = iNumFrames > 1 ? 32 : 0;
    const int32_t iWidth = input.m_outputPixelWidth - iXOffset;
    const int32_t iNumBytes = (input.m_outputPixelWidth + 1) * iHeight * static_cast<int32_t>(sizeof(int32_t));
    texture = static_cast<int32_t*>(hkMemoryRouter::easyAlloc(*hkMemoryRouter::getInstance().m_heap, iNumBytes));
    height = iHeight;
    hkString::memSet(texture, 0, iNumBytes);
    if (sampleInfo) {
        float fMax = fMaxFrameTime;
        sampleInfo->m_maxSampleTime = fMax;
    }
    hkPointerMap<const char*, uint32_t> unknownColors;
    int32_t iY = iHeight;
    for (int32_t iFrame = input.m_frameStart; iFrame < iFrameEnd; ++iFrame) {
        iY -= input.m_gapBetweenFrames;
        if (iNumFrames > 1) {
            drawNumber(iFrame, input.m_outputPixelWidth, texture + (iY - iThreadHeight) * input.m_outputPixelWidth);
        }
        for (int32_t iThread = 0; iThread < iNumThreads; ++iThread) {
            iY -= iThreadHeight;
            // The image is raw ARGB dwords, which the drawing helper writes unsigned.
            uint32_t* pPixels = reinterpret_cast<uint32_t*>(texture + (input.m_outputPixelWidth * iY + iXOffset));
            drawStatistics(
                iFrame, &nodeList.m_data[iThread]->m_children, pPixels, input.m_heightPerThread, input.m_colorTable,
                input.m_outputPixelWidth, iWidth, fMaxFrameTime, startTimes.m_data[iFrame - input.m_frameStart],
                &unknownColors, frameInfos.m_data[iThread].m_data
            );
        }
    }
    const float fPixelsPerMs = static_cast<double>(iWidth * 1000) / fMaxFrameTime;
    // The start-time limit's milliseconds (truncated, the remainder of 1000) times 0.001 in pixels.
    const int32_t iMilliseconds = static_cast<int32_t>(input.m_limitStartTime) % 1000;
    const int32_t iShift =
        static_cast<int32_t>(static_cast<double>(iMilliseconds) * 0.0010000000474974513 * fPixelsPerMs);
    if (fPixelsPerMs > 5.0f) {
        int32_t iLine = 0;
        for (int32_t x = iXOffset - iShift; x < input.m_outputPixelWidth;) {
            if (x >= 0) {
                for (int32_t iRow = 0; iRow < iHeight; ++iRow) {
                    texture[input.m_outputPixelWidth * iRow + x] = static_cast<int32_t>(hkColor::BLUE);
                }
            }
            ++iLine;
            x = static_cast<int32_t>(static_cast<double>(iLine) * fPixelsPerMs) - iShift + iXOffset;
        }
    }
    const int32_t iFrameStep = static_cast<int32_t>(static_cast<double>(iWidth * 16666) / fMaxFrameTime);
    if (iFrameStep > 0) {
        for (int32_t x = iXOffset - iShift; x < input.m_outputPixelWidth; x += iFrameStep) {
            if (x >= 0) {
                for (int32_t iRow = 0; iRow < iHeight; ++iRow) {
                    texture[input.m_outputPixelWidth * iRow + x] = static_cast<int32_t>(hkColor::LIME);
                    texture[input.m_outputPixelWidth * iRow + x + 1] = static_cast<int32_t>(hkColor::LIME);
                }
            }
        }
    }
}

void hkMonitorStreamAnalyzer::writeStatisticsDetailsToTga(
    const hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodeList,
    const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
    hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator>& frameInfos,
    hkOstream& outStream,
    hkMonitorStreamAnalyzer::SampleInfo* sampleInfo
) {
    int32_t* apPtr = nullptr;
    int32_t height = 0;
    hkMonitorStreamAnalyzer::writeStatisticsDetailsToTexture(nodeList, input, frameInfos, apPtr, height, sampleInfo);
    int32_t* v5 = apPtr;
    if (apPtr) {
        saveToTGA(apPtr, outStream, input.m_outputPixelWidth, height);
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkMemoryRouter::easyFree(*Instance->m_heap, v5);
    }
}

// m_this and the two node arrays go with their own destructors.
hkMonitorStreamAnalyzerPrintByTypeInfo::~hkMonitorStreamAnalyzerPrintByTypeInfo() {}

float __hkMin(float a, float b) {
    if (!(b > static_cast<double>(a))) {
        return b;
    }
    return a;
}

hkMonitorStreamAnalyzer::Node*
hkFindChildByName(hkMonitorStreamAnalyzer::Node* node, const char* childName, hkBool searchAnyChild) {
    int32_t v3 = 0;
    if (node->m_children.m_size <= 0) {
    LABEL_4:;
        if (searchAnyChild.m_bool && node->m_children.m_size > 0) {
            return *node->m_children.begin();
        }

        return nullptr;
    }

    while (hkString::strCmp(childName, node->m_children.m_data[v3]->m_name)) {
        if (++v3 >= node->m_children.m_size) {
            goto LABEL_4;
        }
    }
    return node->m_children.m_data[v3];
}

uint32_t hkRoundUpPow2(uint32_t n) {
    uint32_t v1 = ((n - 1) >> 1 | n - 1) >> 2 | ((n - 1) >> 1 | n - 1);
    return ((v1 >> 4 | v1) >> 8 | (v1 >> 4 | v1) | ((v1 >> 4 | v1) >> 8 | (v1 >> 4 | v1)) >> 16) + 1;
}

bool hkFindChildIndexByName(hkMonitorStreamAnalyzer::Node* apNode, const char* apcName, int32_t* apIndexInOut) {
    if (apNode->m_children.m_size > *apIndexInOut &&
        !hkString::strCmp(apNode->m_children.m_data[*apIndexInOut]->m_name, apcName)) {
        return true;
    }
    int32_t v4 = 0;
    if (apNode->m_children.m_size <= 0) {
        return false;
    }
    while (hkString::strCmp(apNode->m_children.m_data[v4]->m_name, apcName)) {
        if (++v4 >= apNode->m_children.m_size) {
            return false;
        }
    }
    *apIndexInOut = v4;
    return true;
}

void hkScaleTimersRecursive(hkMonitorStreamAnalyzer::Node* apNode, int32_t aiTimerId, float afFactor) {
    int32_t v3 = 0;
    apNode->m_value[aiTimerId] = apNode->m_value[aiTimerId] * afFactor;
    for (apNode->m_count[aiTimerId] = 0; v3 < apNode->m_children.m_size; ++v3) {
        hkScaleTimersRecursive(apNode->m_children.m_data[v3], aiTimerId, afFactor);
    }
}

// The destination id is clamped to 6 (signed). Once clamped, the child index hint is kept in aiDestTimerId. The weight
// 1 - afSmoothingFactor is a double; each sum is rounded to float. A missing child is allocated from the router's heap
// through the allocator's own blockAlloc; a failed allocation deliberately leaves a null node, as the game does. The
// child count is reloaded after each recursion and is the result (0 without children). The compares are signed.
int32_t mergeSubTree(
    hkMonitorStreamAnalyzer::Node* apMainTree,
    hkMonitorStreamAnalyzer::Node* apSubTree,
    int32_t aiDestTimerId,
    int32_t aiSourceTimerId,
    float afSmoothingFactor
) {
    const int32_t iDestTimerId = aiDestTimerId > 6 ? 6 : aiDestTimerId;
    int32_t& iChildIndex = aiDestTimerId;
    iChildIndex = 0;
    int32_t iNumChildren = 0;
    if (apSubTree->m_children.m_size > 0) {
        const double dWeight = 1.0 - static_cast<double>(afSmoothingFactor);
        int32_t i = 0;
        do {
            hkMonitorStreamAnalyzer::Node* pChild = apSubTree->m_children.m_data[i];
            hkMonitorStreamAnalyzer::Node* pMainChild;
            if (hkFindChildIndexByName(apMainTree, pChild->m_name, &iChildIndex)) {
                pMainChild = apMainTree->m_children.m_data[iChildIndex];
            }
            else {
                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                void* pMemory = pHeap->blockAlloc(sizeof(hkMonitorStreamAnalyzer::Node));
                if (pMemory) {
                    pMainChild = ::new (pMemory) hkMonitorStreamAnalyzer::Node(
                        apMainTree, pChild->m_name,
                        static_cast<hkMonitorStreamAnalyzer::Node::NodeType>(pChild->m_type.m_storage)
                    );
                }
                else {
                    pMainChild = nullptr;
                }
            }
            pMainChild->m_value[iDestTimerId] =
                static_cast<float>(pChild->m_value[aiSourceTimerId] * dWeight + pMainChild->m_value[iDestTimerId]);
            pMainChild->m_count[iDestTimerId] = pChild->m_count[aiSourceTimerId];
            mergeSubTree(pMainChild, pChild, iDestTimerId, aiSourceTimerId, afSmoothingFactor);
            iNumChildren = apSubTree->m_children.m_size;
            if (iChildIndex < iNumChildren - 1) {
                ++iChildIndex;
            }
            ++i;
        }
        while (i < iNumChildren);
    }
    return iNumChildren;
}

float __hkMax(float a1, float a2) {
    if (!(a2 < ((double)a1))) {
        return a2;
    }
    return a1;
}

void hkWriteRec(hkOstream& outstream, hkMonitorStreamAnalyzer::Node* node, int32_t RecDepth, float factor) {
    hkMonitorStreamAnalyzer::Node* v6;
    uint16_t v7;
    int32_t v8;
    int32_t v9;
    uint16_t v10;
    uint16_t v11;
    uint16_t v12;
    uint16_t v13;
    uint16_t v14;
    uint16_t v15;
    uint16_t v16;
    int32_t v22;
    int32_t v23;
    alignas(64) char a1[256];
    int32_t v4 = RecDepth;
    if (!RecDepth) {
        goto LABEL_46;
    }
    if (RecDepth > 1) {
        int32_t v5 = RecDepth - 1;
        do {
            outstream.printf("\t");
            --v5;
        }
        while (v5);
    }
    v6 = node;
    v7 = node->m_count[0];
    v8 = 0;
    v9 = 0;
    v22 = 0;
    if (v7) {
        v9 = 1;
        v8 = v7;
        v22 = v7;
    }
    v10 = node->m_count[1];
    if (v10) {
        ++v9;
        if (v10 > v8) {
            v8 = v10;
            v22 = v10;
        }
    }
    v11 = node->m_count[2];
    if (v11) {
        ++v9;
        if (v11 > v8) {
            v8 = v11;
            v22 = v11;
        }
    }
    v12 = node->m_count[3];
    if (v12) {
        ++v9;
        if (v12 > v8) {
            v8 = v12;
            v22 = v12;
        }
    }
    v13 = node->m_count[4];
    if (v13) {
        ++v9;
        if (v13 > v8) {
            v8 = v13;
            v22 = v13;
        }
    }
    v14 = node->m_count[5];
    if (v14) {
        ++v9;
        if (v14 > v8) {
            v8 = v14;
            v22 = v14;
        }
    }
    v15 = node->m_count[6];
    if (v15) {
        ++v9;
        if (v15 > v8) {
            v8 = v15;
            v22 = v15;
        }
    }
    v16 = node->m_count[7];
    if (v16) {
        ++v9;
        if (v16 > v8) {
            v8 = v16;
            v22 = v16;
        }
    }
    hkString::snprintf(a1, 200, "%s(%i)", node->m_name, v8);
    if (v8 > 1) {
        if (factor == 1.0f) {
            hkString::snprintf(a1, 200, "%s (%i)", node->m_name, v8);
        }
        else {
            hkString::snprintf(a1, 200, "%s (%4.1f)", node->m_name, factor * static_cast<double>(v22));
        }
    }
    else {
        hkString::snprintf(a1, 200, "%s", node->m_name);
    }
    outstream.printf("%-32s", a1);
    v23 = 3;
    do {
        outstream.printf("\t");
        --v23;
    }
    while (v23);
    if (v9 > 0) {
        uint16_t* m_count;
        int32_t v25;
        m_count = node->m_count;
        v25 = v9;
        for (int32_t i = 0; i < 8; ++i) {
            if (*m_count) {
                const char* v21;
                float v24 = node->m_value[i] * factor;
                if (*m_count < v8) {
                    v24 = static_cast<double>(v22) / static_cast<double>(*m_count) * v24;
                }
                if (--v25) {
                    v21 = "% 12.3f: ";
                }
                else {
                    v21 = "% 12.3f\n";
                }
                outstream.printf(v21, v24);
            }
            ++m_count;
        }
        v4 = RecDepth;
    LABEL_46:;
        v6 = node;
        goto LABEL_47;
    }
    outstream.printf("% 12.3f\n", 0.0);
    v4 = RecDepth;
LABEL_47:;
    int32_t v19 = 0;
    if (v6->m_children.m_size > 0) {
        int32_t v20 = v4 + 1;
        do {
            hkWriteRec(outstream, v6->m_children.m_data[v19++], v20, factor);
        }
        while (v19 < v6->m_children.m_size);
    }
}

hkMonitorStreamAnalyzer::Node* getNodeAtSample(
    hkMonitorStreamAnalyzer::Node* currentNode,
    hkMonitorStreamAnalyzer::Node* lastHit,
    double sampleTime,
    int32_t absoluteTimeIndex
) {
    hkMonitorStreamAnalyzer::Node* v4 = lastHit;
    if (!lastHit || !(lastHit->m_value[absoluteTimeIndex] + lastHit->m_absoluteStartTime >= sampleTime)) {
        v4 = currentNode;
    }
    if (v4) {
        while (true) {
            hkMonitorStreamAnalyzer::Node* ChildNode = findChildNode(v4, sampleTime, absoluteTimeIndex);
            if (!ChildNode) {
                break;
            }
            v4 = ChildNode;
        }
    }
    return v4;
}

void cleanupLoadedObjecthkMonitorStreamColorTable(void* p) {
    static_cast<hkMonitorStreamColorTable*>(p)->~hkMonitorStreamColorTable();
}

// The per-name statistics of the tree (hkBuildSizePerName) collected from a string map into a temp array, sorted by
// their second timer (hkCompareInfosBySize), then printed from the largest down to those above maxImportance times the
// largest (each with its parents and children, indented), every record destroyed and freed to the debug allocator on
// the way.
void printStatisticsByType(hkOstream& outstream, hkMonitorStreamAnalyzer::Node* rootNode, float maxImportance) {
    using Info_t = hkMonitorStreamAnalyzerPrintByTypeInfo;
    using Map_t = hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>;
    using StringMap_t = hkStringMap<Info_t*, hkContainerHeapAllocator>;
    using Sorted_t = hkArray<Info_t*, hkContainerTempAllocator>;
    Sorted_t sortedInfos;
    {
        StringMap_t nameToInfoMap;
        hkBuildSizePerName(rootNode, nameToInfoMap);
        sortedInfos.reserve(nameToInfoMap.m_map.m_numElems);
        Map_t::Iterator it = nameToInfoMap.m_map.getIterator();
        hkBool valid;
        valid = nameToInfoMap.m_map.isValid(it);
        while (valid) {
            sortedInfos.pushBackUnchecked(nameToInfoMap.getValue(it));
            it = nameToInfoMap.m_map.getNext(it);
            valid = nameToInfoMap.m_map.isValid(it);
        }
        if (sortedInfos.m_size > 1) {
            // EXE-INLINED none: hkAlgorithm::quickSort
            hkAlgorithm::quickSortRecursive<Info_t*, bool (*)(const Info_t*, const Info_t*)>(
                sortedInfos.m_data, 0, sortedInfos.m_size - 1, &hkCompareInfosBySize
            );
        }
    }

    float total = sortedInfos.m_data[sortedInfos.m_size - 1]->m_this.m_value[1];
    outstream.printf("\n\n");
    outstream.printf("************************************\n");
    outstream.printf("********** Per Type Times    *******\n");
    outstream.printf("************************************\n");
    int32_t i = sortedInfos.m_size - 1;
    if (i >= 0) {
        const double threshold = static_cast<double>(total) * maxImportance;
        do {
            Info_t* pInfo = sortedInfos.m_data[i];
            if (pInfo->m_this.m_value[1] > threshold) {
                outstream.printf("\n");
                outstream.printf("************************************\n\n");
                for (const hkMonitorStreamAnalyzer::Node& rParent : pInfo->m_parents) {
                    outstream.printf("\t\t");
                    printSingleNodeValues(outstream, 0.0f, &rParent);
                }
                printSingleNodeValues(outstream, total, &pInfo->m_this);
                for (const hkMonitorStreamAnalyzer::Node& rChild : pInfo->m_children) {
                    outstream.printf("\t\t");
                    printSingleNodeValues(outstream, 0.0f, &rChild);
                }
            }
            pInfo->~hkMonitorStreamAnalyzerPrintByTypeInfo();
            hkMemoryRouter::getInstance().debug().blockFree(pInfo, sizeof(Info_t));
        }
        while (--i >= 0);
    }
}

void hkMakeSum(const hkMonitorStreamAnalyzer::Node* node, hkMonitorStreamAnalyzer::Node* sum) {
    sum->m_value[0] = sum->m_value[0] + node->m_value[0];
    sum->m_count[0] += node->m_count[0];
    sum->m_value[1] = node->m_value[1] + sum->m_value[1];
    sum->m_count[1] += node->m_count[1];
    sum->m_value[2] = node->m_value[2] + sum->m_value[2];
    sum->m_count[2] += node->m_count[2];
    sum->m_value[3] = node->m_value[3] + sum->m_value[3];
    sum->m_count[3] += node->m_count[3];
    sum->m_value[4] = node->m_value[4] + sum->m_value[4];
    sum->m_count[4] += node->m_count[4];
    sum->m_value[5] = node->m_value[5] + sum->m_value[5];
    sum->m_count[5] += node->m_count[5];
    sum->m_value[6] = node->m_value[6] + sum->m_value[6];
    sum->m_count[6] += node->m_count[6];
    sum->m_value[7] = node->m_value[7] + sum->m_value[7];
    sum->m_count[7] += node->m_count[7];
}

// The entry for the node's name, the node's timers summed into it, then the same for each child and for the parent.
// The entry's m_children and m_parents hold one node per name, searched from the end; a missing one is appended, the
// array growing to the larger of one more and twice its capacity.
void hkBuildSizePerName(
    const hkMonitorStreamAnalyzer::Node* node,
    hkStringMap<hkMonitorStreamAnalyzerPrintByTypeInfo*, hkContainerHeapAllocator>& mapOut
) {
    using Node = hkMonitorStreamAnalyzer::Node;
    const char* pcName = node->m_name;
    hkMonitorStreamAnalyzerPrintByTypeInfo* pInfo = mapOut.getWithDefault(pcName, nullptr);
    if (!pInfo) {
        pInfo = static_cast<hkMonitorStreamAnalyzerPrintByTypeInfo*>(
            hkMemoryRouter::getInstance().debug().blockAlloc(sizeof(hkMonitorStreamAnalyzerPrintByTypeInfo))
        );
        if (pInfo) {
            ::new (&pInfo->m_this) Node(nullptr, nullptr, Node::NODE_TYPE_TIMER);
            ::new (&pInfo->m_parents) hkArray<Node, hkContainerDebugAllocator>();
            ::new (&pInfo->m_children) hkArray<Node, hkContainerDebugAllocator>();
        }
        else {
            pInfo = nullptr;
        }
        pInfo->m_this.m_name = pcName;
        mapOut.insert(pcName, pInfo);
    }
    // EXE-INLINED 0xE0BF50 (hkMakeSum)
    for (int32_t k = 0; k < 8; ++k) {
        pInfo->m_this.m_value[k] += node->m_value[k];
        pInfo->m_this.m_count[k] += node->m_count[k];
    }
    for (const Node* pChild : node->m_children) {
        hkBuildSizePerName(pChild, mapOut);
        const char* pcChildName = pChild->m_name;
        int32_t j = pInfo->m_children.m_size - 1;
        while (j >= 0 && hkString::strCmp(pcChildName, pInfo->m_children.m_data[j].m_name) != 0) {
            --j;
        }
        Node* pSum;
        if (j >= 0) {
            pSum = &pInfo->m_children.m_data[j];
        }
        else {
            hkMemoryAllocator& rDebug = hkMemoryRouter::getInstance().debug();
            int32_t iSize = pInfo->m_children.m_size;
            // EXE-INLINED none: hkArray<hkMonitorStreamAnalyzer::Node, hkContainerDebugAllocator>::expandOne
            int32_t iCapacity = pInfo->m_children.getCapacity();
            if (iCapacity < iSize + 1) {
                int32_t iNewCapacity = iSize + 1;
                if (iNewCapacity < 2 * iCapacity) {
                    iNewCapacity = 2 * iCapacity;
                }
                hkArrayUtil::_reserve(rDebug, &pInfo->m_children, iNewCapacity, static_cast<int32_t>(sizeof(Node)));
            }
            // The appended element's implicit constructor (its m_children only), then the node's own.
            Node* pNew = pInfo->m_children.m_data + pInfo->m_children.m_size;
            if (pNew) {
                pNew->m_children.m_data = nullptr;
                pNew->m_children.m_size = 0;
                pNew->m_children.m_capacityAndFlags = hkArrayBase<Node*>::DONT_DEALLOCATE_FLAG;
            }
            ++pInfo->m_children.m_size;
            pNew = pInfo->m_children.m_data + iSize;
            if (pNew) {
                pSum = ::new (pNew) Node(nullptr, pcChildName, Node::NODE_TYPE_TIMER);
            }
            else {
                pSum = nullptr;
            }
        }
        // EXE-INLINED 0xE0BF50 (hkMakeSum)
        for (int32_t k = 0; k < 8; ++k) {
            pSum->m_value[k] += pChild->m_value[k];
            pSum->m_count[k] += pChild->m_count[k];
        }
    }
    const Node* pParent = node->m_parent;
    if (pParent) {
        const char* pcParentName = pParent->m_name;
        int32_t j = pInfo->m_parents.m_size - 1;
        while (j >= 0 && hkString::strCmp(pcParentName, pInfo->m_parents.m_data[j].m_name) != 0) {
            --j;
        }
        Node* pSum;
        if (j >= 0) {
            pSum = &pInfo->m_parents.m_data[j];
        }
        else {
            hkMemoryAllocator& rDebug = hkMemoryRouter::getInstance().debug();
            int32_t iSize = pInfo->m_parents.m_size;
            // EXE-INLINED none: hkArray<hkMonitorStreamAnalyzer::Node, hkContainerDebugAllocator>::expandOne
            int32_t iCapacity = pInfo->m_parents.getCapacity();
            if (iCapacity < iSize + 1) {
                int32_t iNewCapacity = iSize + 1;
                if (iNewCapacity < 2 * iCapacity) {
                    iNewCapacity = 2 * iCapacity;
                }
                hkArrayUtil::_reserve(rDebug, &pInfo->m_parents, iNewCapacity, static_cast<int32_t>(sizeof(Node)));
            }
            // The appended element's implicit constructor (its m_children only), then the node's own.
            Node* pNew = pInfo->m_parents.m_data + pInfo->m_parents.m_size;
            if (pNew) {
                pNew->m_children.m_data = nullptr;
                pNew->m_children.m_size = 0;
                pNew->m_children.m_capacityAndFlags = hkArrayBase<Node*>::DONT_DEALLOCATE_FLAG;
            }
            ++pInfo->m_parents.m_size;
            pNew = pInfo->m_parents.m_data + iSize;
            if (pNew) {
                pSum = ::new (pNew) Node(nullptr, pcParentName, Node::NODE_TYPE_TIMER);
            }
            else {
                pSum = nullptr;
            }
        }
        const Node* pParentAgain = node->m_parent;
        // EXE-INLINED 0xE0BF50 (hkMakeSum)
        for (int32_t k = 0; k < 8; ++k) {
            pSum->m_value[k] += pParentAgain->m_value[k];
            pSum->m_count[k] += pParentAgain->m_count[k];
        }
    }
}

bool hkCompareInfosBySize(
    const hkMonitorStreamAnalyzerPrintByTypeInfo* a,
    const hkMonitorStreamAnalyzerPrintByTypeInfo* b
) {
    return b->m_this.m_value[1] > a->m_this.m_value[1];
}

// Every sample time before the first child's start gets a null entry (the times are doubles, stepped by the float
// sample width); then apRoot takes its children's span on timer aiTimer (start of the first, end of the last, plus 1),
// and each further sample before that end gets the node under it (getNodeAtSample, starting from the last hit), skipped
// where there is none.
void outputStatsForFrame(
    float afStartTime,
    float afSampleWidth,
    int32_t aiTimer,
    int32_t aiNumSamples,
    hkMonitorStreamAnalyzer::Node* apRoot,
    hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>* apSamples
) {
    hkMonitorStreamAnalyzer::Node** children = apRoot->m_children.m_data;
    hkMonitorStreamAnalyzer::Node* last = children[apRoot->m_children.m_size - 1];
    hkMonitorStreamAnalyzer::Node* first = children[0];
    int32_t numSamples = aiNumSamples;
    const double firstStart = first->m_absoluteStartTime - afStartTime;
    double sampleTime = 0.0;
    for (; numSamples; --numSamples) {
        if (!(sampleTime < firstStart)) {
            break;
        }
        sampleTime = sampleTime + afSampleWidth;
        apSamples->pushBack(nullptr);
    }
    const double lastEnd = last->m_value[aiTimer] + last->m_absoluteStartTime - afStartTime;
    const double rootStart = first->m_absoluteStartTime;
    apRoot->m_absoluteStartTime = rootStart;
    apRoot->m_value[aiTimer] = static_cast<float>(afStartTime + lastEnd - rootStart + 1.0);
    hkMonitorStreamAnalyzer::Node* node = nullptr;
    if (numSamples) {
        while (lastEnd > sampleTime) {
            node = getNodeAtSample(apRoot, node, sampleTime + afStartTime, aiTimer);
            if (node) {
                apSamples->pushBack(node);
            }
            --numSamples;
            sampleTime = afSampleWidth + sampleTime;
            if (!numSamples) {
                break;
            }
        }
    }
}

// The node under each of aiWidth columns of the frame is looked up into a Node* array on the heap (sized for the
// columns); the column's entry is the result when the frame node has children. The column width is the frame time over
// the column count, rounded to float.
hkMonitorStreamAnalyzer::Node* getNodeSampledAtTick(
    int32_t aiFrame,
    int32_t aiX,
    const hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>* apFrames,
    int32_t aiWidth,
    float afMaxFrameTime,
    float afStartTime,
    const hkMonitorStreamFrameInfo* apInfo
) {
    using NodeArray = hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>;
    const float sampleWidth = static_cast<float>(afMaxFrameTime / static_cast<double>(aiWidth));
    NodeArray samples;
    samples.reserve(aiWidth);
    hkMonitorStreamAnalyzer::Node* const* frameNode = &apFrames->m_data[aiFrame];
    hkMonitorStreamAnalyzer::Node* result = nullptr;
    if ((*frameNode)->m_children.m_size > 0) {
        samples.m_size = 0;
        const int32_t timer =
            apInfo->m_absoluteTimeCounter.m_storage ? apInfo->m_indexOfTimer1 : apInfo->m_indexOfTimer0;
        outputStatsForFrame(afStartTime, sampleWidth, timer, aiWidth, *frameNode, &samples);
        if (aiX < samples.m_size) {
            result = samples.m_data[aiX];
        }
    }
    return result;
}

// The SDK's inline reserve(64) and pushBack('\0') on the debug allocator, after the empty array's three stores.
hkArray<char, hkContainerDebugAllocator>* hkInitIndentString(hkArray<char, hkContainerDebugAllocator>* apPrefix) {
    apPrefix->m_data = nullptr;
    apPrefix->m_size = 0;
    apPrefix->m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
    apPrefix->reserve(64);
    apPrefix->pushBack('\0');

    return apPrefix;
}

hkMonitorStreamAnalyzer::Node* createNewNode(
    const char* name,
    hkMonitorStreamAnalyzer::Node::NodeType type,
    bool wantNodeReuse,
    hkMonitorStreamAnalyzer::Node* parent
) {
    if (wantNodeReuse) {
        int32_t v4 = 0;
        if (parent) {
            while (name && v4 < parent->m_children.m_size) {
                hkMonitorStreamAnalyzer::Node* v5 = parent->m_children.m_data[v4];
                if (v5->m_name && !hkString::strCmp(v5->m_name, name)) {
                    return parent->m_children.m_data[v4];
                }
                ++v4;
            }
        }
    }
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkMonitorStreamAnalyzer::Node* v8 = static_cast<hkMonitorStreamAnalyzer::Node*>(
        Instance->m_heap->blockAlloc(sizeof(hkMonitorStreamAnalyzer::Node))
    );
    if (v8) {
        return ::new (v8) hkMonitorStreamAnalyzer::Node(parent, name, type);
    }

    return nullptr;
}

hkMonitorStreamAnalyzer::Node* hkFindNextChildByName(
    const hkMonitorStreamAnalyzer::Node* oldChild,
    const char* childName,
    hkMonitorStreamAnalyzer::Node* node
) {
    int32_t m_size = node->m_children.m_size;
    int32_t v4 = 0;
    if (m_size > 0) {
        hkMonitorStreamAnalyzer::Node** m_data = node->m_children.begin();
        do {
            if (*m_data == oldChild) {
                break;
            }
            ++v4;
            ++m_data;
        }
        while (v4 < m_size);
    }
    int32_t v6 = v4 + 1;
    if (v6 >= m_size) {
        return nullptr;
    }
    while (hkString::strCmp(childName, node->m_children.m_data[v6]->m_name)) {
        if (++v6 >= node->m_children.m_size) {
            return nullptr;
        }
    }
    return node->m_children.m_data[v6];
}

hkMonitorStreamFrameInfo::hkMonitorStreamFrameInfo()
  : m_heading("Unknown Heading"), m_indexOfTimer0(0), m_indexOfTimer1(1), m_timerFactor0(1.0f), m_timerFactor1(1.0f),
    m_threadId(0), m_frameStreamStart(0), m_frameStreamEnd(0) {
    m_absoluteTimeCounter = ABSOLUTE_TIME_TIMER_0;
}
