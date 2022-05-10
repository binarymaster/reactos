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

#pragma once

#ifndef __MEMORY_H
#include "mm.h"
#endif

BOOLEAN SfiMachDetect(const char *CmdLine);
VOID SfiMachInit(const char *CmdLine);

BOOLEAN SfiConsKbHit(VOID);
int SfiConsGetCh(VOID);

VOID SfiPrepareForReactOS(VOID);

PFREELDR_MEMORY_DESCRIPTOR SfiMemGetMemoryMap(ULONG *MemoryMapSize);

BOOLEAN SfiDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN SfiDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
ULONG SfiDiskGetCacheableBlockCount(UCHAR DriveNumber);

TIMEINFO* SfiGetTime(VOID);

PCONFIGURATION_COMPONENT_DATA SfiHwDetect(VOID);
VOID SfiHwIdle(VOID);

/* EOF */
