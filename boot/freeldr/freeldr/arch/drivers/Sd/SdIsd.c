/** @file
  The SdDxe driver is used to manage the SD memory card device.

  It produces BlockIo and BlockIo2 protocols to allow upper layer
  access the SD memory card device.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  FreeLoader ISD mod (C) 2021  Xen (https://gitlab.com/XenRE)

**/

#include "SdIsd.h"

ULONG
SdDriverRegisterDriver (
	IN PISD_DRIVER_INTERFACE Driver
);

ULONG
SdDriverUnregisterDrivers ();

const CHAR * SdDeviceName = "sd";

//
// Driver Interface
//
ISD_DRIVER_INTERFACE SdDriverInterface = {
  {
    NULL,
    NULL,
  },
  SdDriverRegisterDriver,
  SdDriverUnregisterDrivers,
  (ISD_DRIVER_PROBE)SdDriverBindingSupported,
  (ISD_DRIVER_START)SdDriverBindingStart,
  (ISD_DRIVER_STOP)SdDriverBindingStop
};

//
// Template for SD_DEVICE data structure.
//
SD_DEVICE mSdDeviceTemplate = {
  {                            // DevicePath
    NULL,
	0,
    NULL
  },
  0xFF,                        // Slot
  FALSE,                       // SectorAddressing
  {                            // BlockIo
    EFI_BLOCK_IO_PROTOCOL_REVISION,
    NULL,
    SdReset,
    SdReadBlocks,
    SdWriteBlocks,
    SdFlushBlocks
  },
  {                            // BlockMedia
    0,                         // MediaId
    FALSE,                     // RemovableMedia
    TRUE,                      // MediaPresent
    FALSE,                     // LogicPartition
    FALSE,                     // ReadOnly
    FALSE,                     // WritingCache
    0x200,                     // BlockSize
    0,                         // IoAlign
    0                          // LastBlock
  },
  {                            // Queue
    NULL,
    NULL
  },
  {                            // Csd
    0,
  },
  {                            // Cid
    0,
  },
  {                            // ModelName
    0,
  },
  NULL                         // Private
};

static PISD_DRIVER_INTERFACE BlockIoDriver = NULL;

/**
  Decode and print SD CSD Register content.

  @param[in] Csd           Pointer to SD_CSD data structure.

  @retval EFI_SUCCESS      The function completed successfully
**/
static
EFI_STATUS
DumpCsd (
  IN SD_CSD  *Csd
  )
{
  //SD_CSD2 *Csd2;

  DEBUG((DEBUG_INFO, "== Dump Sd Csd Register==\n"));
  DEBUG((DEBUG_INFO, "  CSD structure                    0x%x\n", Csd->CsdStructure));
  DEBUG((DEBUG_INFO, "  Data read access-time 1          0x%x\n", Csd->Taac));
  DEBUG((DEBUG_INFO, "  Data read access-time 2          0x%x\n", Csd->Nsac));
  DEBUG((DEBUG_INFO, "  Max. bus clock frequency         0x%x\n", Csd->TranSpeed));
  DEBUG((DEBUG_INFO, "  Device command classes           0x%x\n", Csd->Ccc));
  DEBUG((DEBUG_INFO, "  Max. read data block length      0x%x\n", Csd->ReadBlLen));
  DEBUG((DEBUG_INFO, "  Partial blocks for read allowed  0x%x\n", Csd->ReadBlPartial));
  DEBUG((DEBUG_INFO, "  Write block misalignment         0x%x\n", Csd->WriteBlkMisalign));
  DEBUG((DEBUG_INFO, "  Read block misalignment          0x%x\n", Csd->ReadBlkMisalign));
  DEBUG((DEBUG_INFO, "  DSR implemented                  0x%x\n", Csd->DsrImp));
  if (Csd->CsdStructure == 0) {
    DEBUG((DEBUG_INFO, "  Device size                      0x%x\n", Csd->CSizeLow | (Csd->CSizeHigh << 2)));
    DEBUG((DEBUG_INFO, "  Max. read current @ VDD min      0x%x\n", Csd->VddRCurrMin));
    DEBUG((DEBUG_INFO, "  Max. read current @ VDD max      0x%x\n", Csd->VddRCurrMax));
    DEBUG((DEBUG_INFO, "  Max. write current @ VDD min     0x%x\n", Csd->VddWCurrMin));
    DEBUG((DEBUG_INFO, "  Max. write current @ VDD max     0x%x\n", Csd->VddWCurrMax));
  } else {
    //Csd2 = (SD_CSD2*)(VOID*)Csd;
    //DEBUG((DEBUG_INFO, "  Device size                      0x%x\n", Csd2->CSizeLow | (Csd->CSizeHigh << 16)));
  }
  DEBUG((DEBUG_INFO, "  Erase sector size                0x%x\n", Csd->SectorSize));
  DEBUG((DEBUG_INFO, "  Erase single block enable        0x%x\n", Csd->EraseBlkEn));
  DEBUG((DEBUG_INFO, "  Write protect group size         0x%x\n", Csd->WpGrpSize));
  DEBUG((DEBUG_INFO, "  Write protect group enable       0x%x\n", Csd->WpGrpEnable));
  DEBUG((DEBUG_INFO, "  Write speed factor               0x%x\n", Csd->R2WFactor));
  DEBUG((DEBUG_INFO, "  Max. write data block length     0x%x\n", Csd->WriteBlLen));
  DEBUG((DEBUG_INFO, "  Partial blocks for write allowed 0x%x\n", Csd->WriteBlPartial));
  DEBUG((DEBUG_INFO, "  File format group                0x%x\n", Csd->FileFormatGrp));
  DEBUG((DEBUG_INFO, "  Copy flag (OTP)                  0x%x\n", Csd->Copy));
  DEBUG((DEBUG_INFO, "  Permanent write protection       0x%x\n", Csd->PermWriteProtect));
  DEBUG((DEBUG_INFO, "  Temporary write protection       0x%x\n", Csd->TmpWriteProtect));
  DEBUG((DEBUG_INFO, "  File format                      0x%x\n", Csd->FileFormat));

  return EFI_SUCCESS;
}

/**
  Get SD device model name.

  @param[in, out] Device   The pointer to the SD_DEVICE data structure.
  @param[in]      Cid      Pointer to SD_CID data structure.

  @retval EFI_SUCCESS      The function completed successfully

**/
EFI_STATUS
GetSdModelName (
  IN OUT SD_DEVICE         *Device,
  IN     SD_CID            *Cid
  )
{
  ZeroMem (Device->ModelName, sizeof (Device->ModelName));
  CopyMem (Device->ModelName, Cid->OemId, sizeof (Cid->OemId));
  Device->ModelName[sizeof (Cid->OemId)] = ' ';
  CopyMem (Device->ModelName + sizeof (Cid->OemId) + 1, Cid->ProductName, sizeof (Cid->ProductName));
  Device->ModelName[sizeof (Cid->OemId) + sizeof (Cid->ProductName)] = ' ';
  CopyMem (Device->ModelName + sizeof (Cid->OemId) + sizeof (Cid->ProductName) + 1, Cid->ProductSerialNumber, sizeof (Cid->ProductSerialNumber));

  return EFI_SUCCESS;
}

/**
  Discover user area partition in the SD device.

  @param[in] Device          The pointer to the SD_DEVICE data structure.

  @retval EFI_SUCCESS        The user area partition in the SD device is successfully identified.
  @return Others             Some error occurs when identifying the user area.

**/
EFI_STATUS
DiscoverUserArea (
  IN SD_DEVICE             *Device
  )
{
  EFI_STATUS                        Status;
  SD_CSD                            *Csd;
  SD_CSD2                           *Csd2;
  SD_CID                            *Cid;
  UINT64                            Capacity;
  UINT32                            DevStatus;
  UINT16                            Rca;
  UINT32                            CSize;
  UINT32                            CSizeMul;
  UINT32                            ReadBlLen;

  //
  // Deselect the device to force it enter stby mode.
  // Note here we don't judge return status as some SD devices return
  // error but the state has been stby.
  //
  SdSelect (Device, 0);

  Status = SdSetRca (Device, &Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "DiscoverUserArea(): Assign new Rca = 0x%x fails with %r\n", Rca, Status));
    return Status;
  }

  Csd    = &Device->Csd;
  Status = SdGetCsd (Device, Rca, Csd);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  DumpCsd (Csd);

  Cid    = &Device->Cid;
  Status = SdGetCid (Device, Rca, Cid);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  GetSdModelName (Device, Cid);

  Status = SdSelect (Device, Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "DiscoverUserArea(): Reselect the device 0x%x fails with %r\n", Rca, Status));
    return Status;
  }

  Status = SdSendStatus (Device, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Csd->CsdStructure == 0) {
    Device->SectorAddressing = FALSE;
    CSize     = (Csd->CSizeHigh << 2 | Csd->CSizeLow) + 1;
    CSizeMul  = (1 << (Csd->CSizeMul + 2));
    ReadBlLen = (1 << (Csd->ReadBlLen));
    Capacity  = MultU64x32 (MultU64x32 ((UINT64)CSize, CSizeMul), ReadBlLen);
  } else {
    Device->SectorAddressing = TRUE;
    Csd2      = (SD_CSD2*)(VOID*)Csd;
    CSize     = (Csd2->CSizeHigh << 16 | Csd2->CSizeLow) + 1;
    Capacity  = MultU64x32 ((UINT64)CSize, SIZE_512KB);
  }

  Device->BlockIo.Media               = &Device->BlockMedia;
  Device->BlockMedia.IoAlign          = Device->Private->PassThru->IoAlign;
  Device->BlockMedia.BlockSize        = 0x200;
  Device->BlockMedia.LastBlock        = 0x00;
  Device->BlockMedia.RemovableMedia   = TRUE;
  Device->BlockMedia.MediaPresent     = TRUE;
  Device->BlockMedia.LogicalPartition = FALSE;
  Device->BlockMedia.LastBlock        = DivU64x32 (Capacity, Device->BlockMedia.BlockSize) - 1;
  Device->BlockMedia.DevicePath       = &Device->DevicePath;

  return Status;
}

/**
  Scan SD Bus to discover the device.

  @param[in]  Private             The SD driver private data structure.
  @param[in]  Slot                The slot number to check device present.

  @retval EFI_SUCCESS             Successfully to discover the device and attach
                                  SdMmcIoProtocol to it.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack
                                  of resources.
  @retval EFI_ALREADY_STARTED     The device was discovered before.
  @retval Others                  Fail to discover the device.

**/
EFI_STATUS
EFIAPI
DiscoverSdDevice (
  IN  SD_DRIVER_PRIVATE_DATA      *Private,
  IN  UINT8                       Slot
  )
{
  EFI_STATUS                      Status;
  SD_DEVICE                       *Device;

  Device = &Private->Device;

  Device->Slot       = Slot;
  Device->Private    = Private;
  InitializeListHead (&Device->Queue);

  //
  // Expose user area in the Sd memory card to upper layer.
  //
  Status = DiscoverUserArea (Device);

  if (!EFI_ERROR (Status)) {
    if (BlockIoDriver != NULL) {
      Status = IsdProbeAndStartDevice(BlockIoDriver, &Device->BlockIo, &Private->DriverContext);
	  if (!EFI_ERROR (Status)) Private->Driver = BlockIoDriver;
	}
  }

  return Status;
}


ULONG
SdDriverRegisterDriver (
	IN PISD_DRIVER_INTERFACE Driver
)
{
	BlockIoDriver = Driver;
	return EFI_SUCCESS;
}

ULONG
SdDriverUnregisterDrivers ()
{
	BlockIoDriver = NULL;
	return EFI_SUCCESS;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
SdDriverBindingSupported (
  IN EFI_SD_MMC_DEVICE *Device
  )
{
  if (Device->Slot->CardType != SdCardType) return EFI_UNSUPPORTED;

  return EFI_SUCCESS;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
SdDriverBindingStart (
  IN EFI_SD_MMC_DEVICE *Device,
  OUT VOID ** Context
  )
{
  EFI_STATUS                       Status;
  SD_DRIVER_PRIVATE_DATA           *Private;

  Private = AllocateZeroPool (sizeof (SD_DRIVER_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Private->PassThru = Device->PassThru;
  Private->Driver   = NULL;
  Private->Device   = mSdDeviceTemplate;

  Private->Device.DevicePath.Name = SdDeviceName;
  Private->Device.DevicePath.Parent = Device->DevicePath;
  Private->Device.DevicePath.Index = Device->Slot->SlotNumber;

  Status = DiscoverSdDevice (Private, Device->Slot->SlotNumber);

  if (!EFI_ERROR (Status)) *Context = Private;

Error:
  if (EFI_ERROR (Status)) {

    FreePool (Private);
  }
  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
SdDriverBindingStop (
  IN EFI_SD_MMC_DEVICE *Device,
  IN SD_DRIVER_PRIVATE_DATA *Private
  )
{
  EFI_STATUS                          Status = EFI_SUCCESS;

  if (Private->Driver) Status = Private->Driver->Stop(&Private->Device.BlockIo, Private->DriverContext);

  if (!EFI_ERROR (Status)) FreePool (Private);

  return Status;
}
