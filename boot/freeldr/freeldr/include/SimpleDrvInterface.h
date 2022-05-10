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

/*
Simple driver interface for building a device tree

Usage:

LowerDriverInterface.RegisterInterface(UpperDriverInterface);

...

LowerDriverInterface.Start(){
	if (UpperDriverInterface.Probe()) UpperDriverInterface.Start()
}

...do work...

LowerDriverInterface.Stop(){
	UpperDriverInterface.Stop()
}

LowerDriverInterface.UnregisterInterfaces();
*/

#ifndef _SIMPLE_DRIVER_INTERFACE_H
#define _SIMPLE_DRIVER_INTERFACE_H

#include <freeldr.h>

typedef struct _ISD_DRIVER_INTERFACE ISD_DRIVER_INTERFACE, *PISD_DRIVER_INTERFACE;

typedef struct _ISD_DEVICE_PATH ISD_DEVICE_PATH, *PISD_DEVICE_PATH;

/**
  Register upper driver interface.

  Multiple registration of the same interface may be restricted if lower driver
  using 'Link' pointer of the upper driver interface to build a list of the registered interfaces.
  In this case caller need to make a copy of the interface object for each registration.

  @retval TRUE                     Success.
  @retval FALSE                    Error.

**/
typedef
ULONG
(* ISD_DRIVER_REGISTER_INTERFACE) (
	IN PISD_DRIVER_INTERFACE Driver
);

/**
  Unregister all upper driver interfaces.

  @retval 0                        Success.
  @retval Else                     Error.

**/
typedef
ULONG
(* ISD_DRIVER_UNREGISTER_INTERFACES) (
);

/**
  Probe a provided device if it is supported by this driver.

  @retval 0                        The device is supported by this driver.
  @retval Else                     The driver is not suppot this device.

**/
typedef
ULONG
(* ISD_DRIVER_PROBE) (
	IN VOID * Device
);

/**
  Start driver for provided device.

  @retval 0                 Driver is successfully started.
  @retval Else              Error.

**/
typedef
ULONG
(* ISD_DRIVER_START) (
	IN VOID * Device,
	OUT VOID ** Context
);

/**
  Stop driver for provided device.

  @retval 0                 Driver is successfully started.
  @retval Else              Error.

**/
typedef
ULONG
(* ISD_DRIVER_STOP) (
	IN VOID * Device,
	IN VOID * Context
);

struct _ISD_DRIVER_INTERFACE {
	//may be used by lower driver for the registration of the upper drivers, should be initialized with NULLs
	LIST_ENTRY Link;

	//register the upper driver interface
	//may be NULL if driver dont need to create upper devices
	ISD_DRIVER_REGISTER_INTERFACE RegisterInterface;

	ISD_DRIVER_UNREGISTER_INTERFACES UnregisterInterfaces;

	ISD_DRIVER_PROBE Probe;

	ISD_DRIVER_START Start;

	ISD_DRIVER_STOP Stop;
};

/*
Element of the device path
Can be passed to upper driver as part of 'Device' structure.
	Name - ASCII device name;
	Index - ID or address of the device on the bus. Name+Index must be unique;
	Parent - pointer to lower level device path element. If Parent == NULL - this is root device.
*/
struct _ISD_DEVICE_PATH {
	const CHAR * Name;
	ULONG Index;
	PISD_DEVICE_PATH Parent;
};

/* 
Always return 0, can be used as 'Probe' function.
*/
ULONG IsdAlwaysSuccessfulProbe(
	IN VOID * Device
);

/*
Probe the driver for the specified device and start it if probe was successful.
*/
ULONG
IsdProbeAndStartDevice (
    IN PISD_DRIVER_INTERFACE Driver,
	IN VOID * Device,
	OUT VOID ** Context
);

/*
Probe the driver for the specified device, start it if probe was successful and
register the provided 'Link' in 'DeviceList' list if start was successful.
*/
ULONG
IsdProbeAndStartAndRegisterDevice (
    IN PISD_DRIVER_INTERFACE Driver,
	IN VOID * Device,
	OUT VOID ** Context,
	OUT LIST_ENTRY * DeviceList,
	IN LIST_ENTRY * Link
);

/*
Stop the device and unregister the provided 'Link' if stop was successful.
*/
ULONG
IsdStopAndUnregisterDevice (
    IN PISD_DRIVER_INTERFACE Driver,
	IN VOID * Device,
	IN VOID * Context,
	IN LIST_ENTRY * Link
);

#endif
