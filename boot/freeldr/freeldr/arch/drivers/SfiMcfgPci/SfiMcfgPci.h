/*++

Copyright (c) 2005 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciBus.h
  
Abstract:

  PCI Bus Driver

Revision History

  FreeLoader ISD mod (C) 2021  Xen (https://gitlab.com/XenRE)

--*/

#ifndef _EFI_PCI_BUS_H
#define _EFI_PCI_BUS_H

#include <freeldr.h>
 
#include <ntddk.h>

#include <SimpleDrvInterface.h>

#include <efidef.h>

#include <arch/drivers/interface/PciIo.h>

#include <hwdefs/Pci.h>
#include <hwdefs/Acpi.h>

//
// Driver Produced Protocol Prototypes
//

#define VGABASE1  0x3B0
#define VGALIMIT1 0x3BB

#define VGABASE2  0x3C0
#define VGALIMIT2 0x3DF

#define ISABASE   0x100
#define ISALIMIT  0x3FF

typedef enum {
  PciBarTypeUnknown = 0,
  PciBarTypeIo16,
  PciBarTypeIo32,
  PciBarTypeMem32,
  PciBarTypePMem32,
  PciBarTypeMem64,
  PciBarTypePMem64,
  PciBarTypeIo,
  PciBarTypeMem,
  PciBarTypeMaxType
} PCI_BAR_TYPE;

typedef struct {
  UINT64        BaseAddress;
  UINT64        Length;
  UINT64        Alignment;
  PCI_BAR_TYPE  BarType;
  BOOLEAN       Prefetchable;
  UINT8         MemType;
  UINT8         Offset;
} PCI_BAR;

#define EFI_BRIDGE_IO32_DECODE_SUPPORTED        0x0001 
#define EFI_BRIDGE_PMEM32_DECODE_SUPPORTED      0x0002 
#define EFI_BRIDGE_PMEM64_DECODE_SUPPORTED      0x0004 
#define EFI_BRIDGE_IO16_DECODE_SUPPORTED        0x0008  
#define EFI_BRIDGE_PMEM_MEM_COMBINE_SUPPORTED   0x0010  
#define EFI_BRIDGE_MEM64_DECODE_SUPPORTED       0x0020
#define EFI_BRIDGE_MEM32_DECODE_SUPPORTED       0x0040

typedef struct _PCI_IO_DEVICE {
  LIST_ENTRY Link;

  PISD_DRIVER_INTERFACE Driver;
  VOID * DriverContext;

  EFI_PCI_IO_PROTOCOL                       PciIo;

  //
  // PCI configuration space header type
  //
  PCI_TYPE00                                Pci;

  //
  // Bus number, Device number, Function number
  //
  UINT8                                     BusNumber;
  UINT8                                     DeviceNumber;
  UINT8                                     FunctionNumber;

  //
  // BAR for this PCI Device
  //
  PCI_BAR                                   PciBar[PCI_MAX_BAR];

  //
  // The attribute this PCI device currently set
  //
  UINT64                                    Attributes;

  //
  // The attributes this PCI device actually supports
  //
  UINT64                                    Supports;

  BOOLEAN                                   IsPciExp;

  UINT64                                    McfgConfigAddress;
} PCI_IO_DEVICE;


#define PCI_IO_DEVICE_FROM_PCI_IO_THIS(a) \
  BASE_CR (a, PCI_IO_DEVICE, PciIo)

#define PCI_IO_DEVICE_FROM_PCI_DRIVER_OVERRIDE_THIS(a) \
  BASE_CR (a, PCI_IO_DEVICE, PciDriverOverride)

#define PCI_IO_DEVICE_FROM_LINK(a) \
  BASE_CR (a, PCI_IO_DEVICE, Link)

#define IS_ISA_BRIDGE(_p)       IS_CLASS2 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA)  
#define IS_INTEL_ISA_BRIDGE(_p) (IS_CLASS2 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA_PDECODE) && ((_p)->Hdr.VendorId == 0x8086) && ((_p)->Hdr.DeviceId == 0x7110))
#define IS_PCI_GFX(_p)     IS_CLASS2 (_p, PCI_CLASS_DISPLAY, PCI_CLASS_DISPLAY_OTHER)

#endif
