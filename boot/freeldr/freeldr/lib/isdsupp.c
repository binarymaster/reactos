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
#include <SimpleDrvInterface.h>

ULONG IsdAlwaysSuccessfulProbe(
	IN VOID * Device
)
{
	return 0;
}

ULONG
IsdProbeAndStartDevice (
	IN PISD_DRIVER_INTERFACE Driver,
	IN VOID * Device,
	OUT VOID ** Context
)
{
	ULONG Status;

	Status = Driver->Probe(Device);
	if (Status) return Status;

	Status = Driver->Start(Device, Context);
	return Status;
}

ULONG
IsdProbeAndStartAndRegisterDevice (
	IN PISD_DRIVER_INTERFACE Driver,
	IN VOID * Device,
	OUT VOID ** Context,
	OUT LIST_ENTRY * DeviceList,
	IN LIST_ENTRY * Link
)
{
	ULONG Status;

	Status = IsdProbeAndStartDevice(Driver, Device, Context);
	if (Status) return Status;
	
	InsertTailList(DeviceList, Link); 
	return Status;
}

ULONG
IsdStopAndUnregisterDevice (
	IN PISD_DRIVER_INTERFACE Driver,
	IN VOID * Device,
	IN VOID * Context,
	IN LIST_ENTRY * Link
)
{
	ULONG Status;
	
	Status = Driver->Stop(Device, Context);
	if (Status) return Status;

	if (Link->Flink) RemoveEntryList(Link);
	Link->Flink = NULL;
	Link->Blink = NULL;

	return Status;
}
