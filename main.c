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

bool OverflowingAdd(double d, int64 c)
{
    ASSERT(c >= 0);
    return c > 0 && d + c <= d;
}

void PrintAverage(const char *label, double average, uint32 max)
{
    double percent = (average / max) * 100;
    double expected = (double)max / 2;
    printf("%15s: %15.3f, %2.1f%% (random: %15.1f, 50%%)\n",
           label, average, percent, expected);
}



int main()
{
    int c;
    double byteAverage = 0;
    uint64 byteCount = 0;
    double shortAverage = 0;
    uint16 curShort = 0;
    double dwordAverage = 0;
    uint32 curDword = 0;

    c = fgetc(stdin);
    while (c != EOF) {
        ASSERT(c >= 0 && c < 256);
        ASSERT(!OverflowingAdd(byteAverage, c));
        byteAverage += c;
        byteCount++;

        curShort <<= 8;
        curShort |= c;
        if (byteCount % 2 == 1) {
            ASSERT(!OverflowingAdd(shortAverage, curShort));
            shortAverage += curShort;
        }

        curDword <<= 8;
        curDword |= c;
        if (byteCount % 4 == 3) {
            ASSERT(!OverflowingAdd(dwordAverage, curDword));
            dwordAverage += curDword;
        }

        c = fgetc(stdin);
    }

    byteAverage /= byteCount;
    PrintAverage("Byte Average", byteAverage, MAX_UINT8);
    shortAverage /= (byteCount / 2);
    PrintAverage("Short Average", shortAverage, MAX_UINT16);
    dwordAverage /= (byteCount / 4);
    PrintAverage("DWord Average", dwordAverage, MAX_UINT32);

    return 0;
}

