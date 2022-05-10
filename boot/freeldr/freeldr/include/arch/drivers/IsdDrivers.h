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

#ifndef __ISD_DRIVERS_H__
#define __ISD_DRIVERS_H__

#include <SimpleDrvInterface.h>

extern ISD_DRIVER_INTERFACE SfiMcfgPciDriverInterface;
extern ISD_DRIVER_INTERFACE SdMmcHciDriverInterface;
extern ISD_DRIVER_INTERFACE SdDriverInterface;
extern ISD_DRIVER_INTERFACE EmmcDriverInterface;
extern ISD_DRIVER_INTERFACE BlockIoDriverInterface;
extern ISD_DRIVER_INTERFACE MrstKeypadDriverInterface;

#endif
