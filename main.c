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
#include "mbtypes.h"
#include "mbassert.h"

typedef struct SimpleStats {
    uint32 bitSize;
    uint32 bitMask;
    uint64 numData;
    double average;
    double sum;
} SimpleStats;

void SimpleStats_Init(SimpleStats *s, uint32 bitSize)
{
    Util_Zero(s, sizeof(*s));
    s->bitSize = bitSize;
    s->bitMask = (((uint64)1) << bitSize) - 1;
    s->numData = 0;
    s->average = 0;
    s->sum = 0;
}

static bool OverflowingAdd(double d, int64 c)
{
    ASSERT(c >= 0);
    return c > 0 && d + c <= d;
}

void SimpleStats_AddField(SimpleStats *s, uint32 field)
{
    ASSERT((field & s->bitMask) == field);
    s->numData++;
    ASSERT(!OverflowingAdd(s->sum, field));
    s->sum += field;
}
void SimpleStats_Finish(SimpleStats *s)
{
    s->average = s->sum / s->numData;
}

void SimpleStats_Print(SimpleStats *s)
{
    uint32 max;

    const char *label;
    if (s->bitSize == 8) {
        label = "Byte";
        max = MAX_UINT8;
    } else if (s->bitSize == 16) {
        label = "Short";
        max = MAX_UINT16;
    } else {
        ASSERT(s->bitSize == 32);
        label = "DWord";
        max = MAX_UINT32;
    }

    double percent = (s->average / max) * 100;
    double expected = (double)max / 2;
    printf("%15s: %15.3f, %2.1f%% (random: %15.1f, 50%%)\n",
           label, s->average, percent, expected);
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

    SimpleStats_Init(&byteStats,   8);
    SimpleStats_Init(&shortStats, 16);
    SimpleStats_Init(&dwordStats, 32);

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

    return 0;
}

