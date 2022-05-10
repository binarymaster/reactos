/*
 *  FreeLoader SFI support
 *  Copyright (C) 2021  Xen (https://gitlab.com/XenRE)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include <hwdefs/sfitable.h>
#include <hwdefs/SfiTimer.h>

#define DefaultDelayCount 100000

//expect to wait 10 msec
#define TimerCalibrationDelay 10000

static unsigned int delay_count = DefaultDelayCount;

extern struct sfi_timer_table_entry * SfiTimer;

static
VOID
__StallExecutionProcessor(ULONG Loops)
{
    register volatile unsigned int i;
    for (i = 0; i < Loops; i++);
}

VOID StallExecutionProcessor(ULONG Microseconds)
{
    ULONGLONG LoopCount = ((ULONGLONG)delay_count * (ULONGLONG)Microseconds) / 1000ULL;
    __StallExecutionProcessor((ULONG)LoopCount);
}

BOOLEAN
SfiCalibrateStallExecution(VOID)
{
	UINT8 * TimerRegBase;
	ULONG AttempLeft, StartVal, EndVal;
	UINT64 DelayCorr;

	if (SfiTimer == NULL) return FALSE;

	if (SfiTimer->phy_addr.HighPart != 0) return FALSE;
	TimerRegBase = (UINT8 *)SfiTimer->phy_addr.LowPart;
	
	WRITE_REGISTER_ULONG((ULONG *)(TimerRegBase + SFI_TIMER_CONTROL_ADDR), SFI_TIMER_CTRL_ENABLE | SFI_TIMER_CTRL_INT_DISABLE);
	
	AttempLeft = 3;
	while (AttempLeft)
	{
		StartVal = READ_REGISTER_ULONG((ULONG *)(TimerRegBase + SFI_TIMER_CURRENT_VALUE_ADDR));
		StallExecutionProcessor(TimerCalibrationDelay);
		EndVal = READ_REGISTER_ULONG((ULONG *)(TimerRegBase + SFI_TIMER_CURRENT_VALUE_ADDR));
		//SFI timer will count down
		if (StartVal > EndVal)
		{
			StartVal -= EndVal;
			DelayCorr = delay_count;
			DelayCorr *= (UINT64)(SfiTimer->freq) * (UINT64)TimerCalibrationDelay;
			DelayCorr /= ((UINT64)StartVal * 1000000ULL);
			if (DelayCorr < 0x80000000ULL)
			{
				delay_count = (unsigned int)DelayCorr;
				return TRUE;
			}
		}
		//calibration failed - try again
		AttempLeft --;
	}
	return FALSE;
}
