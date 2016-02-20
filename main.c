/*
 * This file is part of mbent.
 * Copyright (c) 2016 Michael Banack <bob5972@banack.net>
 *
 * mbent is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mbent is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mbent.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * main.c --
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mbtypes.h"
#include "mbassert.h"
#include "MBString.h"
#include "IntMap.h"

typedef struct SimpleStats {
    uint32 bitSize;
    uint32 bitMask;
    uint64 rangeSize;
    uint64 numEntries;
    uint64 uniqueEntries;
    double average;
    double sum;

    bool hasEntropy;
    double entropy;
    IntMap counts;
} SimpleStats;

void SimpleStats_Create(SimpleStats *s, uint32 bitSize)
{
    Util_Zero(s, sizeof(*s));
    s->bitSize = bitSize;
    s->bitMask = (((uint64)1) << bitSize) - 1;
    s->rangeSize = (((uint64)1) << bitSize);
    s->numEntries = 0;
    s->average = 0;
    s->sum = 0;
    s->entropy = 0;

    IntMap_Create(&s->counts);
}

void SimpleStats_Destroy(SimpleStats *s)
{
    IntMap_Destroy(&s->counts);
}

static bool OverflowingAdd(double d, int64 c)
{
    ASSERT(c >= 0);
    return c > 0 && d + c <= d;
}

void SimpleStats_AddField(SimpleStats *s, uint32 field)
{
    ASSERT((field & s->bitMask) == field);
    s->numEntries++;
    ASSERT(!OverflowingAdd(s->sum, field));
    s->sum += field;

    VERIFY(s->numEntries < MAX_INT32);
    if (IntMap_Get(&s->counts, field) == 0) {
        s->uniqueEntries++;
    }
    IntMap_IncrementBy(&s->counts, field, 1);
}

void SimpleStats_Finish(SimpleStats *s)
{
    IntMapIterator it;
    s->average = s->sum / s->numEntries;

    if (s->bitSize <= 32) {
        IntMapIterator_Start(&it, &s->counts);
        while (IntMapIterator_HasNext(&it)) {
            uint32 key = IntMapIterator_GetNext(&it);

            int count = IntMap_Get(&s->counts, key);
            double freq, entropy;

            freq = count;
            freq /= s->numEntries;

            if (freq > 0) {
                entropy = -(freq * log2(freq));
                s->entropy += entropy;
            }

            s->hasEntropy = TRUE;
        }
    }
}

void SimpleStats_Print(SimpleStats *s)
{
    MBString label;
    MBString prefix;
    double percent, expected;

    MBString_Create(&label);
    MBString_Create(&prefix);

    if (s->bitSize == 8) {
        MBString_CopyCStr(&label, "Byte");
    } else if (s->bitSize == 16) {
        MBString_CopyCStr(&label, "Short");
    } else {
        ASSERT(s->bitSize == 32);
        MBString_CopyCStr(&label, "DWord");
    }

    MBString_Copy(&prefix, &label);
    MBString_AppendCStr(&prefix, " Total Count");
    printf("%20s: %lld\n",
           MBString_GetCStr(&prefix), s->numEntries);

    percent = (double)s->uniqueEntries / s->rangeSize * 100;
    expected = s->rangeSize;
    MBString_Copy(&prefix, &label);
    MBString_AppendCStr(&prefix, " Unique Count");
    printf("%20s: %15.3f, %2.1f%% (random: %15.1f, 100%%)\n",
           MBString_GetCStr(&prefix), (float)s->uniqueEntries,
           percent, expected);

    percent = (s->average / s->bitMask) * 100;
    expected = (double)s->bitMask / 2;
    MBString_Copy(&prefix, &label);
    MBString_AppendCStr(&prefix, " Average");
    printf("%20s: %15.3f, %2.1f%% (random: %15.1f, 50%% )\n",
           MBString_GetCStr(&prefix), s->average, percent, expected);

    if (s->hasEntropy) {
        MBString_Copy(&prefix, &label);
        MBString_AppendCStr(&prefix, " Entropy");

        percent = (s->entropy / s->bitSize) * 100;
        expected = s->bitSize;
        printf("%20s: %15.3f, %2.1f%% (random: %15.1f, 100%%)\n",
                MBString_GetCStr(&prefix), s->entropy, percent, expected);
    }

    printf("\n");

    MBString_Destroy(&label);
    MBString_Destroy(&prefix);
}

int main()
{
    int c;
    uint8  curByte  = 0;
    uint16 curShort = 0;
    uint32 curDword = 0;
    uint64 byteCount = 0;

    SimpleStats byteStats;
    SimpleStats shortStats;
    SimpleStats dwordStats;

    SimpleStats_Create(&byteStats,   8);
    SimpleStats_Create(&shortStats, 16);
    SimpleStats_Create(&dwordStats, 32);

    c = fgetc(stdin);
    while (c != EOF) {
        ASSERT(c >= 0 && c < 256);

        curByte = c;
        SimpleStats_AddField(&byteStats, curByte);

        curShort <<= 8;
        curShort |= c;
        if (byteCount % 2 == 1) {
            SimpleStats_AddField(&shortStats, curShort);
        }

        curDword <<= 8;
        curDword |= c;
        if (byteCount % 4 == 3) {
            SimpleStats_AddField(&dwordStats, curDword);
        }

        byteCount++;
        c = fgetc(stdin);
    }

    SimpleStats_Finish(&byteStats);
    SimpleStats_Finish(&shortStats);
    SimpleStats_Finish(&dwordStats);

    SimpleStats_Print(&byteStats);
    SimpleStats_Print(&shortStats);
    SimpleStats_Print(&dwordStats);

    SimpleStats_Destroy(&byteStats);
    SimpleStats_Destroy(&shortStats);
    SimpleStats_Destroy(&dwordStats);

    return 0;
}

