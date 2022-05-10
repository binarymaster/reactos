/*++

Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciIo.c
  
Abstract:

  PCI I/O Abstraction Driver

Revision History

  FreeLoader ISD mod (C) 2021  Xen (https://gitlab.com/XenRE)

--*/

#include "SfiMcfgPci.h"

#define EFI_PCI_ADDRESS(bus, dev, func, reg) \
  (UINT64) ( \
  (((UINTN) bus) << 24) | \
  (((UINTN) dev) << 16) | \
  (((UINTN) func) << 8) | \
  (((UINTN) (reg)) < 256 ? ((UINTN) (reg)) : (UINT64) (LShiftU64 ((UINT64) (reg), 32))))

#define EFI_PCI_EXP_ADDRESS(bus, dev, func, reg) \
  (UINT64) ( \
  (((UINTN) bus) << 20) | \
  (((UINTN) dev) << 15) | \
  (((UINTN) func) << 12) | \
  (((UINTN) (reg))))

typedef union {
  UINT8   volatile  *buf;
  UINT8   volatile  *ui8;
  UINT16  volatile  *ui16;
  UINT32  volatile  *ui32;
  UINT64  volatile  *ui64;
  UINTN   volatile  ui;
} PTR;

ULONG
SfiMcfgPciRegisterDriver (
	PISD_DRIVER_INTERFACE Driver
);

ULONG
SfiMcfgPciUnregisterDrivers (VOID);

ULONG
SfiMcfgPciStart (
	MCFG_STRUCTURE * SfiMcfgData,
	VOID ** Context
);

ULONG
SfiMcfgPciStop(
	IN VOID * Device,
	IN VOID * Context
);

ISD_DRIVER_INTERFACE SfiMcfgPciDriverInterface = {
	{
		NULL,
		NULL,
	},
	SfiMcfgPciRegisterDriver,
	SfiMcfgPciUnregisterDrivers,
	IsdAlwaysSuccessfulProbe,
	(ISD_DRIVER_START)SfiMcfgPciStart,
	SfiMcfgPciStop
};

//
// PCI I/O Support Function Prototypes
//
//

BOOLEAN 
CheckBarType ( 
  IN PCI_IO_DEVICE  *PciIoDevice,
  UINT8             BarIndex,
  PCI_BAR_TYPE      BarType
);

EFI_STATUS
PciIoVerifyBarAccess (
  PCI_IO_DEVICE                 *PciIoDevice,
  UINT8                         BarIndex,
  PCI_BAR_TYPE                  Type,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  UINT64                        *Offset
);

EFI_STATUS
PciIoVerifyConfigAccess (
  PCI_IO_DEVICE                 *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT64                     *Offset
);

EFI_STATUS
EFIAPI
PciIoPollMem (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
);
  
EFI_STATUS
EFIAPI
PciIoPollIo (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
);    

EFI_STATUS
EFIAPI
PciIoMemRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoIoRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoIoWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoConfigRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoConfigWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoCopyMem (
  IN     EFI_PCI_IO_PROTOCOL  *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
);

EFI_STATUS
EFIAPI
PciIoMap (
  IN     EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
);

EFI_STATUS
EFIAPI
PciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
);

EFI_STATUS
EFIAPI
PciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL    *This,
  IN  EFI_MEMORY_TYPE        MemoryType,
  IN  UINTN                  Pages,
  OUT VOID                   **HostAddress,
  IN  UINT64                 Attributes
);

EFI_STATUS
EFIAPI
PciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  );

EFI_STATUS
EFIAPI
PciIoFlush (
  IN  EFI_PCI_IO_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  OUT UINTN                *Segment,
  OUT UINTN                *Bus,
  OUT UINTN                *Device,
  OUT UINTN                *Function
  );

EFI_STATUS
EFIAPI
PciIoAttributes (
  IN  EFI_PCI_IO_PROTOCOL              *This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result   OPTIONAL
  );

EFI_STATUS
EFIAPI
PciIoGetBarAttributes(
  IN  EFI_PCI_IO_PROTOCOL    *This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports,   OPTIONAL
  OUT VOID                           **Resources  OPTIONAL
  );

EFI_STATUS
EFIAPI
PciIoSetBarAttributes(
  IN     EFI_PCI_IO_PROTOCOL  *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  );

LIST_ENTRY PciDriverList = {
	&PciDriverList,
	&PciDriverList
};

LIST_ENTRY McfgPciDevicesList = {
	&McfgPciDevicesList,
	&McfgPciDevicesList
};

//
// Pci Io Protocol Interface
//
EFI_PCI_IO_PROTOCOL  PciIoInterface = {
  PciIoPollMem,
  PciIoPollIo,
  {
    PciIoMemRead,
    PciIoMemWrite
  },
  {
    PciIoIoRead,
    PciIoIoWrite
  },
  {
    PciIoConfigRead,
    PciIoConfigWrite
  },
  PciIoCopyMem,
  PciIoMap,
  PciIoUnmap,
  PciIoAllocateBuffer,
  PciIoFreeBuffer,
  PciIoFlush,
  PciIoGetLocation,
  PciIoAttributes,
  PciIoGetBarAttributes,
  PciIoSetBarAttributes,
  0,
  NULL
};

#define EFI_PCIE_CAPABILITY_ID_VENDOR        0x0B

/* Fixed BAR fields */
#define EFI_PCIE_VNDR_CAP_ID_FIXED_BAR 0x00	/* Fixed BAR (TBD) */

EFI_STATUS PciGetFixedBarLength(IN PCI_IO_DEVICE *Dev)
{
	EFI_STATUS Status;
	UINT32 pos, cap_data, i;
	PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER pcie_cap;
	UINT32 FixedBarLength[PCI_MAX_BAR];

	pos = EFI_PCIE_CAPABILITY_BASE_OFFSET;
	while (pos) {
		Status = Dev->PciIo.Pci.Read(&Dev->PciIo, EfiPciIoWidthUint8, pos, sizeof(pcie_cap), &pcie_cap);
		if (Status != EFI_SUCCESS) return Status;
			
		if (pcie_cap.CapabilityId == 0xffff) return EFI_NOT_FOUND;

		if (pcie_cap.CapabilityId == EFI_PCIE_CAPABILITY_ID_VENDOR) {
			pos += sizeof(pcie_cap);
			Status = Dev->PciIo.Pci.Read(&Dev->PciIo, EfiPciIoWidthUint8, pos, sizeof(cap_data), &cap_data);
			if (Status != EFI_SUCCESS) return Status;
			if ((cap_data & 0xffff) == EFI_PCIE_VNDR_CAP_ID_FIXED_BAR) {
				pos += sizeof(cap_data);
				Status = Dev->PciIo.Pci.Read(&Dev->PciIo, EfiPciIoWidthUint32, pos, PCI_MAX_BAR, FixedBarLength);
				if (Status != EFI_SUCCESS) return Status;
				for (i = 0; i < PCI_MAX_BAR; i++) Dev->PciBar[i].Length = FixedBarLength[i];
				return EFI_SUCCESS;
			}
		}

		pos = pcie_cap.NextCapabilityOffset;
	}

	return 0;
}

EFI_STATUS
InitializePciIoInstance (
  PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

  Initializes a PCI I/O Instance

Arguments:
  
Returns:

  None

--*/

{
  CopyMem (&PciIoDevice->PciIo, &PciIoInterface, sizeof (EFI_PCI_IO_PROTOCOL));
  return EFI_SUCCESS;
}

EFI_STATUS
CreateMcfgPciIoDevice (
  EFI_PHYSICAL_ADDRESS McfgConfigAddress,
  UINT8 BusNumber,
  UINT8 DeviceNumber,
  UINT8 FunctionNumber,
  PCI_IO_DEVICE  **PciIoDevice
  )
{
	EFI_STATUS Status;
	PCI_IO_DEVICE *Dev;
	UINT32 i;

	Dev = AllocateZeroPool(sizeof(PCI_IO_DEVICE));
	if (Dev == NULL) return EFI_OUT_OF_RESOURCES;
		
	Dev->McfgConfigAddress = McfgConfigAddress;
	Dev->IsPciExp = TRUE;
	Dev->BusNumber = BusNumber;
	Dev->DeviceNumber = DeviceNumber;
	Dev->FunctionNumber = FunctionNumber;
	Dev->Driver = NULL;
	Dev->DriverContext = NULL;

	Status = InitializePciIoInstance(Dev);
	if (!EFI_ERROR (Status)) {
		Status = Dev->PciIo.Pci.Read (&Dev->PciIo, EfiPciIoWidthUint8, 0, sizeof(PCI_TYPE00), &Dev->Pci);
	}

    if (!EFI_ERROR (Status)) {
		if (Dev->Pci.Hdr.VendorId == 0xFFFF) Status = EFI_NOT_FOUND;
	}

    if (!EFI_ERROR (Status)) {
		//supported only devices with fixed bar feature
		Status = PciGetFixedBarLength(Dev);
	}

	//init BARs
    if (!EFI_ERROR (Status)) {
		for (i = 0; i < PCI_MAX_BAR; i++) {
			if ((Dev->Pci.Device.Bar[i] != 0) && (Dev->PciBar[i].Length != 0)) {
				if ((Dev->Pci.Device.Bar[i] & 1)) {
					//
					// Device I/Os
					//
					if (Dev->Pci.Device.Bar[i] & 0xFFFF0000) { // It is a IO32 bar
						Dev->PciBar[i].BarType = PciBarTypeIo32;
					} else { // It is a IO16 bar
						Dev->PciBar[i].BarType  = PciBarTypeIo16;
					}
					Dev->PciBar[i].BaseAddress = Dev->Pci.Device.Bar[i] & 0xfffffffc;
					Dev->PciBar[i].Alignment = 3;
				} else {
					if ((Dev->Pci.Device.Bar[i] & 7) == 0) { //memory space; anywhere in 32 bit address space
						if (Dev->Pci.Device.Bar[i] & 0x08) {
							Dev->PciBar[i].BarType = PciBarTypePMem32;
						} else {
							Dev->PciBar[i].BarType = PciBarTypeMem32;
						}
						Dev->PciBar[i].BaseAddress = Dev->Pci.Device.Bar[i] & 0xfffffff0;
						Dev->PciBar[i].Alignment = 0xf;
					} else { //64 bit bars is not supported here (and should never meet)
						Dev->PciBar[i].BarType = PciBarTypeUnknown;
					}
				}
				Dev->PciBar[i].BarType = PciBarTypeMem32;
			} else {
				Dev->PciBar[i].BarType = PciBarTypeUnknown;
			}
		}
	}

	if (EFI_ERROR (Status)) {
		ExFreePool(Dev);
		return Status;
	}

	*PciIoDevice = Dev;
	return EFI_SUCCESS;
}

EFI_STATUS
DeletePciIoDevice (
  PCI_IO_DEVICE  *PciIoDevice
  )
{
	EFI_STATUS Status;

	if (PciIoDevice->Driver != NULL) {
		Status = IsdStopAndUnregisterDevice(PciIoDevice->Driver, &PciIoDevice->PciIo, PciIoDevice->DriverContext, &PciIoDevice->Link);
		if (EFI_ERROR (Status)) return Status;
	}
	ExFreePool(PciIoDevice);
	return EFI_SUCCESS;
}

ULONG
SfiMcfgPciRegisterDriver (
	PISD_DRIVER_INTERFACE Driver
)
{
	InsertTailList(&PciDriverList, &Driver->Link);
	return EFI_SUCCESS;
}

ULONG
SfiMcfgPciUnregisterDrivers (VOID)
{
	InitializeListHead(&PciDriverList);
	return EFI_SUCCESS;
}

ULONG
SfiMcfgPciStart (
	MCFG_STRUCTURE * SfiMcfgData,
	VOID ** Context
)
{
	PCI_IO_DEVICE *PciDev;
	EFI_STATUS Status;
	UINT8 DeviceNumber, FunctionNumber;
	LIST_ENTRY *Link;
	PISD_DRIVER_INTERFACE Driver;

	//enumerate and init devices
	for (DeviceNumber = 0; DeviceNumber <= PCI_MAX_DEVICE; DeviceNumber ++) {
		for (FunctionNumber = 0; FunctionNumber <= PCI_MAX_FUNC; FunctionNumber ++) {
			Status = CreateMcfgPciIoDevice(SfiMcfgData->BaseAddress, SfiMcfgData->StartBusNumber, DeviceNumber, FunctionNumber, &PciDev);
			if (!EFI_ERROR (Status)) {
				Link = GetFirstNode (&PciDriverList);
				while (!IsNull (&PciDriverList, Link)) {
					Driver = BASE_CR(Link, ISD_DRIVER_INTERFACE, Link);

					Status = IsdProbeAndStartAndRegisterDevice(Driver, &PciDev->PciIo, &PciDev->DriverContext, &McfgPciDevicesList, &PciDev->Link);
					if (!EFI_ERROR (Status)) {
						PciDev->Driver = Driver;
						break;
					}
					
					Link = GetNextNode (&PciDriverList, Link);
				}
				if (PciDev->Driver == NULL) {
					//noone cares about this device - delete it
					DeletePciIoDevice(PciDev);
				}
			}
		}
	}
	return EFI_SUCCESS;
}

ULONG
SfiMcfgPciStop(
	IN VOID * Device,
	IN VOID * Context
)
{
	LIST_ENTRY *Link;
	LIST_ENTRY *NextLink;
	PCI_IO_DEVICE *PciDev;
	EFI_STATUS Status = EFI_SUCCESS;

	Link = GetFirstNode (&McfgPciDevicesList);
	while (!IsNull (&McfgPciDevicesList, Link)) {
		PciDev = BASE_CR(Link, PCI_IO_DEVICE, Link);
		NextLink = GetNextNode (&McfgPciDevicesList, Link);
		Status = DeletePciIoDevice(PciDev);
		if (EFI_ERROR (Status)) return Status;
		Link = NextLink;
	}
	return Status;
}

EFI_STATUS
PciIoVerifyBarAccess (
  PCI_IO_DEVICE                   *PciIoDevice,
  UINT8                           BarIndex,
  PCI_BAR_TYPE                    Type,
  IN EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN UINTN                        Count,
  UINT64                          *Offset
  )
/*++

Routine Description:

  Verifies access to a PCI Base Address Register (BAR)

Arguments:

Returns:

  None

--*/
{
  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (BarIndex == EFI_PCI_IO_PASS_THROUGH_BAR) {
    return EFI_SUCCESS;
  }

  //
  // BarIndex 0-5 is legal
  //
  if (BarIndex >= PCI_MAX_BAR) {
    return EFI_INVALID_PARAMETER;
  }

  if (!CheckBarType (PciIoDevice, BarIndex, Type)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Width is EfiPciIoWidthFifoUintX then convert to EfiPciIoWidthUintX
  // If Width is EfiPciIoWidthFillUintX then convert to EfiPciIoWidthUintX
  //
  if (Width >= EfiPciIoWidthFifoUint8 && Width <= EfiPciIoWidthFifoUint64) {
    Count = 1;
  }

  Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);

  if ((*Offset + Count * ((UINTN)1 << Width)) - 1 >= PciIoDevice->PciBar[BarIndex].Length) {
    return EFI_INVALID_PARAMETER;
  }

  *Offset = *Offset + PciIoDevice->PciBar[BarIndex].BaseAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
PciIoVerifyConfigAccess (
  PCI_IO_DEVICE                 *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT64                     *Offset
  )
/*++

Routine Description:

  Verifies access to a PCI Config Header

Arguments:

Returns:

  None

--*/
{
  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Width is EfiPciIoWidthFifoUintX then convert to EfiPciIoWidthUintX
  // If Width is EfiPciIoWidthFillUintX then convert to EfiPciIoWidthUintX
  //
  Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);

  if (PciIoDevice->IsPciExp) {
    if ((*Offset + Count * ((UINTN)1 << Width)) - 1 >= PCI_EXP_MAX_CONFIG_OFFSET) {
      return EFI_UNSUPPORTED;
    }

    *Offset = EFI_PCI_EXP_ADDRESS (PciIoDevice->BusNumber, PciIoDevice->DeviceNumber, PciIoDevice->FunctionNumber, *Offset);

  } else {
    if ((*Offset + Count * ((UINTN)1 << Width)) - 1 >= PCI_MAX_CONFIG_OFFSET) {
      return EFI_UNSUPPORTED;
    }

    *Offset = EFI_PCI_ADDRESS (PciIoDevice->BusNumber, PciIoDevice->DeviceNumber, PciIoDevice->FunctionNumber, *Offset);
  }

  return EFI_SUCCESS;
}

//
// Internal function
//

EFI_STATUS
EFIAPI
IoMemR (
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                      Count,
  IN  BOOLEAN                    InStrideFlag,
  IN  PTR                        In,
  IN  BOOLEAN                    OutStrideFlag,
  OUT PTR                        Out
  )
/*++

Routine Description:

  Private service to provide the memory read/write

Arguments:

  Width of the Memory Access
  Count of the number of accesses to perform

Returns:

  Status

  EFI_SUCCESS           - Successful transaction
  EFI_INVALID_PARAMETER - Unsupported width and address combination

--*/
{
  UINTN  Stride;
  UINTN  InStride;
  UINTN  OutStride;


  Width     = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);
  Stride    = (UINTN)1 << Width;
  InStride  = InStrideFlag  ? Stride : 0;
  OutStride = OutStrideFlag ? Stride : 0;

  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiPciIoWidthUint8:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      *In.ui8 = READ_REGISTER_UCHAR(Out.buf);
    }
    break;
  case EfiPciIoWidthUint16:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      *In.ui16 = READ_REGISTER_USHORT(Out.buf);
    }
    break;
  case EfiPciIoWidthUint32:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      *In.ui32 = READ_REGISTER_ULONG(Out.buf);
    }
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IoMemW (
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                      Count,
  IN  BOOLEAN                    InStrideFlag,
  IN  PTR                        In,
  IN  BOOLEAN                    OutStrideFlag,
  OUT PTR                        Out
  )
/*++

Routine Description:

  Private service to provide the memory read/write

Arguments:

  Width of the Memory Access
  Count of the number of accesses to perform

Returns:

  Status

  EFI_SUCCESS           - Successful transaction
  EFI_INVALID_PARAMETER - Unsupported width and address combination

--*/
{
  UINTN  Stride;
  UINTN  InStride;
  UINTN  OutStride;


  Width     = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);
  Stride    = (UINTN)1 << Width;
  InStride  = InStrideFlag  ? Stride : 0;
  OutStride = OutStrideFlag ? Stride : 0;

  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiPciIoWidthUint8:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      WRITE_REGISTER_UCHAR(In.buf, *Out.ui8);
    }
    break;
  case EfiPciIoWidthUint16:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      WRITE_REGISTER_USHORT(In.buf, *Out.ui16);
    }
    break;
  case EfiPciIoWidthUint32:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      WRITE_REGISTER_ULONG(In.buf, *Out.ui32);
    }
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IoMemRead (
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     Address,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  UINTN                          AlignMask;
  PTR                            In;
  PTR                            Out;

  if ( Buffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if (Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  In.buf  = Buffer;
  Out.buf = (VOID *)(UINTN) Address;
  if ((UINT32)Width <= EfiPciIoWidthUint64) {
    return IoMemR (Width, Count, TRUE, In, TRUE, Out);
  }
  if (Width >= EfiPciIoWidthFifoUint8 && Width <= EfiPciIoWidthFifoUint64) {
    return IoMemR (Width, Count, TRUE, In, FALSE, Out);
  }
  if (Width >= EfiPciIoWidthFillUint8 && Width <= EfiPciIoWidthFillUint64) {
    return IoMemR (Width, Count, FALSE, In, TRUE, Out);
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
IoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     Address,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  UINTN  AlignMask;
  PTR    In;
  PTR    Out;

  if ( Buffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if (Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  In.buf  = (VOID *)(UINTN) Address;
  Out.buf = Buffer;
  if ((UINT32)Width <= EfiPciIoWidthUint64) {
    return IoMemW (Width, Count, TRUE, In, TRUE, Out);
  }
  if (Width >= EfiPciIoWidthFifoUint8 && Width <= EfiPciIoWidthFifoUint64) {
    return IoMemW (Width, Count, FALSE, In, TRUE, Out);
  }
  if (Width >= EfiPciIoWidthFillUint8 && Width <= EfiPciIoWidthFillUint64) {
    return IoMemW (Width, Count, TRUE, In, FALSE, Out);
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
IoIoRead (
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     UserAddress,
  IN     UINTN                      Count,
  IN OUT VOID                       *UserBuffer
  )
{
  UINTN                         InStride;
  UINTN                         OutStride;
  UINTN                         AlignMask;
  UINTN                         Address;
  PTR                           Buffer;
  UINT16                        Data16;
  UINT32                        Data32;
  
 
  if ( UserBuffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  Address    = (UINTN)  UserAddress;
  Buffer.buf = (UINT8 *)UserBuffer;
 
  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width & 0x03) == EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if ( Address & AlignMask ) {
    return EFI_INVALID_PARAMETER;
  }

  InStride  = 1 << (Width & 0x03);
  OutStride = InStride;
  if (Width >=EfiPciIoWidthFifoUint8 && Width <= EfiPciIoWidthFifoUint64) {
    InStride = 0;
  }
  if (Width >=EfiPciIoWidthFillUint8 && Width <= EfiPciIoWidthFillUint64) {
    OutStride = 0;
  }
  Width = Width & 0x03;

  //
  // Loop for each iteration and move the data
  //

  switch (Width) {
  case EfiPciIoWidthUint8:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      *Buffer.ui8 = READ_PORT_UCHAR(Address);
    }
    break;

  case EfiPciIoWidthUint16:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      if (Buffer.ui & 0x1) {
        Data16 = READ_PORT_USHORT(Address);
        *Buffer.ui8     = (UINT8)(Data16 & 0xff);
        *(Buffer.ui8+1) = (UINT8)((Data16 >> 8) & 0xff);
      } else {
        *Buffer.ui16 = READ_PORT_USHORT(Address);
      }
    }
    break;

  case EfiPciIoWidthUint32:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      if (Buffer.ui & 0x3) {
        Data32 = READ_PORT_ULONG(Address);
        *Buffer.ui8     = (UINT8)(Data32 & 0xff);
        *(Buffer.ui8+1) = (UINT8)((Data32 >> 8) & 0xff);
        *(Buffer.ui8+2) = (UINT8)((Data32 >> 16) & 0xff);
        *(Buffer.ui8+3) = (UINT8)((Data32 >> 24) & 0xff);
      } else {
        *Buffer.ui32 = READ_PORT_ULONG(Address);
      }
    }
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
IoIoWrite (
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     UserAddress,
  IN UINTN                      Count,
  IN OUT VOID                   *UserBuffer
  )
{
  UINTN                          InStride;
  UINTN                          OutStride;
  UINTN                          AlignMask;
  UINTN                          Address;
  PTR                            Buffer;
  UINT16                         Data16;
  UINT32                         Data32;

  if ( UserBuffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  Address    = (UINTN)  UserAddress;
  Buffer.buf = (UINT8 *)UserBuffer;
  
  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width & 0x03) == EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if ( Address & AlignMask ) {
    return EFI_INVALID_PARAMETER;
  }

  InStride  = 1 << (Width & 0x03);
  OutStride = InStride;
  if (Width >=EfiPciIoWidthFifoUint8 && Width <= EfiPciIoWidthFifoUint64) {
    InStride = 0;
  }
  if (Width >=EfiPciIoWidthFillUint8 && Width <= EfiPciIoWidthFillUint64) {
    OutStride = 0;
  }
  Width = Width & 0x03;

  //
  // Loop for each iteration and move the data
  //

  switch (Width) {
  case EfiPciIoWidthUint8:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      WRITE_PORT_UCHAR(Address, *Buffer.ui8);
    }
    break;

  case EfiPciIoWidthUint16:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      if (Buffer.ui & 0x1) {
        Data16 = *Buffer.ui8;
        Data16 = Data16 | (*(Buffer.ui8+1) << 8);
        WRITE_PORT_USHORT(Address, Data16);
      } else {
        WRITE_PORT_USHORT(Address, *Buffer.ui16);
      }
    }
    break;
  case EfiPciIoWidthUint32:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      if (Buffer.ui & 0x3) {
        Data32 = *Buffer.ui8;
        Data32 = Data32 | (*(Buffer.ui8+1) << 8);
        Data32 = Data32 | (*(Buffer.ui8+2) << 16);
        Data32 = Data32 | (*(Buffer.ui8+3) << 24);
        WRITE_PORT_ULONG(Address, Data32);
      } else {
        WRITE_PORT_ULONG(Address, *Buffer.ui32);
      }
    }
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IoPollMem ( 
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     Address,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
{
  EFI_STATUS  Status;
  UINT64      NumberOfTicks;

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  if ((UINT32)Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // No matter what, always do a single poll.
  //
  Status = IoMemRead (Width, Address, 1, Result);
  if ( EFI_ERROR(Status) ) {
    return Status;
  }    
 
  if ( (*Result & Mask) == Value ) {
    return EFI_SUCCESS;
  }

  if (Delay == 0) {
    return EFI_SUCCESS;
  } else {

    NumberOfTicks = Delay / 100;
    if ( (Delay % 100) !=0 ) {
      NumberOfTicks += 1;
    }
    NumberOfTicks += 1;
  
    while ( NumberOfTicks ) {

      Stall (10);

      Status = IoMemRead (Width, Address, 1, Result);
      if ( EFI_ERROR(Status) ) {
        return Status;
      }
    
      if ( (*Result & Mask) == Value ) {
        return EFI_SUCCESS;
      }

      NumberOfTicks -= 1;
    }
  }
  return EFI_TIMEOUT;
}

EFI_STATUS
EFIAPI
IoPollIo ( 
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     Address,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
{
  EFI_STATUS  Status;
  UINT64      NumberOfTicks;

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // No matter what, always do a single poll.
  //
  Status = IoIoRead (Width, Address, 1, Result);
  if ( EFI_ERROR(Status) ) {
    return Status;
  }    
  if ( (*Result & Mask) == Value ) {
    return EFI_SUCCESS;
  }

  if (Delay == 0) {
    return EFI_SUCCESS;
  } else {

    NumberOfTicks = Delay / 100;
    if ( (Delay % 100) !=0 ) {
      NumberOfTicks += 1;
    }
    NumberOfTicks += 1;
  
    while ( NumberOfTicks ) {

      Stall(10);
    
      Status = IoIoRead (Width, Address, 1, Result);
      if ( EFI_ERROR(Status) ) {
        return Status;
      }
    
      if ( (*Result & Mask) == Value ) {
        return EFI_SUCCESS;
      }

      NumberOfTicks -= 1;
    }
  }
  return EFI_TIMEOUT;
}

EFI_STATUS
EFIAPI
PciIoPollMem (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
/*++

Routine Description:

  Poll PCI Memmory

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, 1, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = IoPollMem (
    Width,
    Offset,
    Mask,
    Value,
    Delay,
    Result
    );
  return Status;
}

EFI_STATUS
EFIAPI
PciIoPollIo (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
/*++

Routine Description:

  Poll PCI IO

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, 1, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_UNSUPPORTED;
  /*Status = IoPollIo (
    Width,
    Offset,
    Mask,
    Value,
    Delay,
    Result
    );

  return Status;*/
}

EFI_STATUS
EFIAPI
PciIoMemRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Memory Read Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Buffer == NULL){
    return EFI_INVALID_PARAMETER;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = IoMemRead (
    Width,
    Offset,
    Count,
    Buffer
    );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Memory Write Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Buffer == NULL){
    return EFI_INVALID_PARAMETER;
  }
  
  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = IoMemWrite (
    Width,
    Offset,
    Count,
    Buffer
    );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoIoRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI I/O Read Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Buffer == NULL){
    return EFI_INVALID_PARAMETER;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = IoIoRead (
    Width,
    Offset,
    Count,
    Buffer
    );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoIoWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI I/O Write Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Buffer == NULL){
    return EFI_INVALID_PARAMETER;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = IoIoWrite (
    Width,
    Offset,
    Count,
    Buffer
    );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoConfigRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Configuration Read Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;
  UINT64        Address;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Address     = Offset;
  Status      = PciIoVerifyConfigAccess (PciIoDevice, Width, Count, &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Address += PciIoDevice->McfgConfigAddress;

  Status = IoMemRead (
    Width,
    Address,
    Count,
    Buffer
    );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoConfigWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Configuration Write Cycle

Arguments:

Returns:

  None

--*/
{
  //never write to MCFG space, but return success
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoCopyMem (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
  )
/*++

Routine Description:

  Copy PCI Memory

Arguments:

Returns:

  None

--*/
{

  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
PciIoMap (
  IN     EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
/*++

Routine Description:

  Maps a memory region for DMA

Arguments:

Returns:

  None

--*/
{
  if ((UINT32)Operation >= EfiPciIoOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (HostAddress == NULL || NumberOfBytes == NULL || DeviceAddress == NULL || Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //All addresses equals in flat mode
  *DeviceAddress = ((UINTN)HostAddress);
  *Mapping = HostAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  )
/*++

Routine Description:

  Unmaps a memory region for DMA

Arguments:

Returns:

  None

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 Pages,
  OUT VOID                  **HostAddress,
  IN  UINT64                Attributes
  )
/*++

Routine Description:

  Allocates a common buffer for DMA

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;

  *HostAddress = ExAllocatePoolWithTag(NonPagedPool, (Pages * PAGE_SIZE), 'PCID');

  if (*HostAddress == NULL) Status = EFI_OUT_OF_RESOURCES; else Status = EFI_SUCCESS;

  return Status;
}

EFI_STATUS
EFIAPI
PciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  )
/*++

Routine Description:

  Frees a common buffer 

Arguments:

Returns:

  None

--*/
{
  if( HostAddress == NULL ){
     return EFI_INVALID_PARAMETER;
  } 

  ExFreePool(HostAddress);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoFlush (
  IN  EFI_PCI_IO_PROTOCOL  *This
  )
/*++

Routine Description:

  Flushes a DMA buffer

Arguments:

Returns:

  None

--*/

{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  OUT UINTN                *Segment,
  OUT UINTN                *Bus,
  OUT UINTN                *Device,
  OUT UINTN                *Function
  )
/*++

Routine Description:

  Gets a PCI device's current bus number, device number, and function number.

Arguments:

Returns:

  None

--*/
{
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Segment == NULL || Bus == NULL || Device == NULL || Function == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Segment  = 0;
  *Bus      = PciIoDevice->BusNumber;
  *Device   = PciIoDevice->DeviceNumber;
  *Function = PciIoDevice->FunctionNumber;

  return EFI_SUCCESS;
}

BOOLEAN
CheckBarType (
  IN PCI_IO_DEVICE       *PciIoDevice,
  UINT8                  BarIndex,
  PCI_BAR_TYPE           BarType
  )
/*++

Routine Description:

  Sets a PCI controllers attributes on a resource range

Arguments:

Returns:

  None

--*/
{
  switch (BarType) {

  case PciBarTypeMem:

    if (PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeMem32  &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypePMem32 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypePMem64 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeMem64    ) {
      return FALSE;
    }

    return TRUE;

  case PciBarTypeIo:
    if (PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeIo32 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeIo16){
      return FALSE;
    }

    return TRUE;

  default:
    break;
  }

  return FALSE;
}

EFI_STATUS
EFIAPI
PciIoAttributes (
  IN EFI_PCI_IO_PROTOCOL                       * This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;

  PCI_IO_DEVICE *PciIoDevice;
  UINT64         NewAttributes;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  switch (Operation) {
  case EfiPciIoAttributeOperationGet:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    *Result = PciIoDevice->Attributes;
    return EFI_SUCCESS;

  case EfiPciIoAttributeOperationSupported:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    *Result = PciIoDevice->Supports;
    return EFI_SUCCESS;

  case EfiPciIoAttributeOperationEnable:
    if(Attributes & ~(PciIoDevice->Supports)) {
      return EFI_UNSUPPORTED;
    }
    NewAttributes = PciIoDevice->Attributes | Attributes;
    break;
  case EfiPciIoAttributeOperationDisable:
    if(Attributes & ~(PciIoDevice->Supports)) {
      return EFI_UNSUPPORTED;
    }
    NewAttributes = PciIoDevice->Attributes & (~Attributes);
    break;
  case EfiPciIoAttributeOperationSet:
    if(Attributes & ~(PciIoDevice->Supports)) {
      return EFI_UNSUPPORTED;
    }
    NewAttributes = Attributes;
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  //
  // If VGA_IO is set, then set VGA_MEMORY too.  This driver can not enable them seperately.
  //
  if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO) {
    NewAttributes |= EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY;
  }

  //
  // If VGA_MEMORY is set, then set VGA_IO too.  This driver can not enable them seperately.
  //
  if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY) {
    NewAttributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO;
  }
  
  //
  // On SFI dont change anything in MCFG table
  //

  PciIoDevice->Attributes = NewAttributes;

  return Status;
}

EFI_STATUS
EFIAPI
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL             * This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
{
  UINT8                             *Configuration;
  PCI_IO_DEVICE                     *PciIoDevice;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *AddressSpace;
  EFI_ACPI_END_TAG_DESCRIPTOR       *End;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Supports == NULL && Resources == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BarIndex >= PCI_MAX_BAR) || (PciIoDevice->PciBar[BarIndex].BarType == PciBarTypeUnknown)) {
    return EFI_UNSUPPORTED;
  }

  //
  // This driver does not support modifications to the WRITE_COMBINE or
  // CACHED attributes for BAR ranges.
  //
  if (Supports != NULL) {
    *Supports = PciIoDevice->Supports & EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED & EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE;
  }

  if (Resources != NULL) {
    Configuration = AllocateZeroPool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Configuration == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AddressSpace = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;

    AddressSpace->Desc         = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    AddressSpace->Len          = (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);

    AddressSpace->AddrRangeMin = PciIoDevice->PciBar[BarIndex].BaseAddress;
    AddressSpace->AddrLen      = PciIoDevice->PciBar[BarIndex].Length;
    AddressSpace->AddrRangeMax = PciIoDevice->PciBar[BarIndex].Alignment;

    switch (PciIoDevice->PciBar[BarIndex].BarType) {
    case PciBarTypeIo16:
    case PciBarTypeIo32:
      //
      // Io
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_IO;
      break;

    case PciBarTypeMem32:
      //
      // Mem
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // 32 bit
      //
      AddressSpace->AddrSpaceGranularity = 32;
      break;

    case PciBarTypePMem32:
      //
      // Mem
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // prefechable
      //
      AddressSpace->SpecificFlag = 0x6;
      //
      // 32 bit
      //
      AddressSpace->AddrSpaceGranularity = 32;
      break;

    case PciBarTypeMem64:
      //
      // Mem
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // 64 bit
      //
      AddressSpace->AddrSpaceGranularity = 64;
      break;

    case PciBarTypePMem64:
      //
      // Mem
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // prefechable
      //
      AddressSpace->SpecificFlag = 0x6;
      //
      // 64 bit
      //
      AddressSpace->AddrSpaceGranularity = 64;
      break;

    default:
      break;
    }

    //
    // put the checksum
    //
    End           = (EFI_ACPI_END_TAG_DESCRIPTOR *) (AddressSpace + 1);
    End->Desc     = ACPI_END_TAG_DESCRIPTOR;
    End->Checksum = 0;

    *Resources    = Configuration;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoSetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;
  UINT64        NonRelativeOffset;
  UINT64        Supports;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  //
  // Make sure Offset and Length are not NULL
  //
  if (Offset == NULL || Length == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PciIoDevice->PciBar[BarIndex].BarType == PciBarTypeUnknown) {
    return EFI_UNSUPPORTED;
  }
  //
  // This driver does not support setting the WRITE_COMBINE or the CACHED attributes.
  // If Attributes is not 0, then return EFI_UNSUPPORTED.
  //
  Supports = PciIoDevice->Supports & EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED & EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE;

  if (Attributes != (Attributes & Supports)) {
    return EFI_UNSUPPORTED;
  }
  //
  // Attributes must be supported.  Make sure the BAR range describd by BarIndex, Offset, and
  // Length are valid for this PCI device.
  //
  NonRelativeOffset = *Offset;
  Status = PciIoVerifyBarAccess (
            PciIoDevice,
            BarIndex,
            PciBarTypeMem,
            EfiPciIoWidthUint8,
            (UINT32) *Length,
            &NonRelativeOffset
            );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
