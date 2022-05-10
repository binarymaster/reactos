/** @file
  This driver is used to manage SD/MMC PCI host controllers which are compliance
  with SD Host Controller Simplified Specification version 3.00 plus the 64-bit
  System Addressing support in SD Host Controller Simplified Specification version
  4.20.

  It would expose EFI_SD_MMC_PASS_THRU_PROTOCOL for upper layer use.

  Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  FreeLoader ISD mod (C) 2021  Xen (https://gitlab.com/XenRE)

**/

#include <freeldr.h>

#include <efidef.h>

#include <hwdefs/Pci.h>
#include <arch/drivers/interface/PciIo.h>

#include "SdMmcPciHcIsd.h"

#define SLOT_INIT_TEMPLATE {0, UnknownSlot, 0, 0, 0, 0, 0, \
                               {EDKII_SD_MMC_BUS_WIDTH_IGNORE,\
                               EDKII_SD_MMC_CLOCK_FREQ_IGNORE,\
                               {EDKII_SD_MMC_DRIVER_STRENGTH_IGNORE}}}

const CHAR * SdMmcHciDeviceName = "sdhci";

//
// Template for SD/MMC host controller private data.
//
SD_MMC_HC_PRIVATE_DATA gSdMmcPciHcTemplate = {
  NULL,                             // PciIo
  {                                 // PassThru
    sizeof (UINT32),
    SdMmcPassThruPassThru,
    SdMmcPassThruGetNextSlot,
    SdMmcPassThruResetDevice
  },
  0,                                // PciAttributes
  0,                                // PreviousSlot
  NULL,                             // TimerEvent
  NULL,                             // ConnectEvent
                                    // Queue
  INITIALIZE_LIST_HEAD_VARIABLE (gSdMmcPciHcTemplate.Queue),
  {
    NULL,
    0,
    NULL
  },
  {                                 // Slot
    SLOT_INIT_TEMPLATE,
    SLOT_INIT_TEMPLATE,
    SLOT_INIT_TEMPLATE,
    SLOT_INIT_TEMPLATE,
    SLOT_INIT_TEMPLATE,
    SLOT_INIT_TEMPLATE
  },
  {                                 // Capability
    {0},
  },
  {                                 // MaxCurrent
    0,
  },
  {
    0                               // ControllerVersion
  },
  {                                 // ConnectedDevice
    {0},
  },
  {                                 // Driver
    NULL,
  },
  {                                 // DriverContext
    NULL,
  }
};

LIST_ENTRY SdMmcDriverList = {
	&SdMmcDriverList,
	&SdMmcDriverList
};

//
// Prioritized function list to detect card type.
// User could add other card detection logic here.
//
CARD_TYPE_DETECT_ROUTINE mCardTypeDetectRoutineTable[] = {
  EmmcIdentification,
  SdCardIdentification,
  NULL
};

ULONG
SdMmcHciRegisterDriver (
	IN PISD_DRIVER_INTERFACE Driver
);

ULONG
SdMmcHciUnregisterDrivers ();

ISD_DRIVER_INTERFACE SdMmcHciDriverInterface = {
	{
		NULL,
		NULL,
	},
	SdMmcHciRegisterDriver,
	SdMmcHciUnregisterDrivers,
	(ISD_DRIVER_PROBE)SdMmcPciHcDriverBindingSupported,
	(ISD_DRIVER_START)SdMmcPciHcDriverBindingStart,
	(ISD_DRIVER_STOP)SdMmcPciHcDriverBindingStop
};

ULONG
SdMmcHciRegisterDriver (
	IN PISD_DRIVER_INTERFACE Driver
)
{
	InsertTailList(&SdMmcDriverList, &Driver->Link);
	return EFI_SUCCESS;
}

ULONG
SdMmcHciUnregisterDrivers ()
{
	InitializeListHead(&SdMmcDriverList);
	return EFI_SUCCESS;
}

/**
  Call back function when the timer event is signaled.

  @param[in]  Context   Pointer to the context data registered to the
                        Event.

**/
VOID
EFIAPI
ProcessAsyncTaskList (
  IN VOID*              Context
  )
{
  SD_MMC_HC_PRIVATE_DATA              *Private;
  LIST_ENTRY                          *Link;
  SD_MMC_HC_TRB                       *Trb;
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET *Packet;
  BOOLEAN                             InfiniteWait;

  Private = (SD_MMC_HC_PRIVATE_DATA*)Context;

  //
  // Check if the first entry in the async I/O queue is done or not.
  //
  Status = EFI_SUCCESS;
  Trb    = NULL;
  Link   = GetFirstNode (&Private->Queue);
  if (!IsNull (&Private->Queue, Link)) {
    Trb = SD_MMC_HC_TRB_FROM_THIS (Link);
    if (!Private->Slot[Trb->Slot].MediaPresent) {
      Status = EFI_NO_MEDIA;
      goto Done;
    }
    if (!Trb->Started) {
      //
      // Check whether the cmd/data line is ready for transfer.
      //
      Status = SdMmcCheckTrbEnv (Private, Trb);
      if (!EFI_ERROR (Status)) {
        Trb->Started = TRUE;
        Status = SdMmcExecTrb (Private, Trb);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
      } else {
        goto Done;
      }
    }
    Status = SdMmcCheckTrbResult (Private, Trb);
  }

Done:
  if ((Trb != NULL) && (Status == EFI_NOT_READY)) {
    Packet = Trb->Packet;
    if (Packet->Timeout == 0) {
      InfiniteWait = TRUE;
    } else {
      InfiniteWait = FALSE;
    }
    if ((!InfiniteWait) && (Trb->Timeout-- == 0)) {
      RemoveEntryList (Link);
      Trb->Packet->TransactionStatus = EFI_TIMEOUT;
      if (Trb->EventCallback) Trb->EventCallback(Trb->EventCallbackContext);
      SdMmcFreeTrb (Trb);
      DEBUG ((DEBUG_VERBOSE, "ProcessAsyncTaskList(): Signal Event EFI_TIMEOUT\n"));
      return;
    }
  } else if ((Trb != NULL) && (Status == EFI_CRC_ERROR) && (Trb->Retries > 0)) {
    Trb->Retries--;
    Trb->Started = FALSE;
  } else if ((Trb != NULL)) {
    RemoveEntryList (Link);
    Trb->Packet->TransactionStatus = Status;
    if (Trb->EventCallback) Trb->EventCallback(Trb->EventCallbackContext);
    SdMmcFreeTrb (Trb);
    DEBUG ((DEBUG_VERBOSE, "ProcessAsyncTaskList(): Signal Event with %r\n", Status));
  }
  return;
}

/**
  Sd removable device enumeration callback function when the timer event is signaled.

  @param[in]  Context   Pointer to the context data registered to the
                        Event.

**/
VOID
EFIAPI
SdMmcPciHcEnumerateDevice (
  IN VOID*              Context
  )
{
  SD_MMC_HC_PRIVATE_DATA              *Private;
  EFI_STATUS                          Status;
  UINT8                               Slot;
  BOOLEAN                             MediaPresent;
  UINT32                              RoutineNum;
  CARD_TYPE_DETECT_ROUTINE            *Routine;
  UINTN                               Index;
  LIST_ENTRY                          *Link;
  LIST_ENTRY                          *NextLink;
  SD_MMC_HC_TRB                       *Trb;
  PISD_DRIVER_INTERFACE Driver;

  Private = (SD_MMC_HC_PRIVATE_DATA*)Context;

  for (Slot = 0; Slot < SD_MMC_HC_MAX_SLOT; Slot++) {
    if ((Private->Slot[Slot].Enable) && (Private->Slot[Slot].SlotType == RemovableSlot)) {
      Status = SdMmcHcCardDetect (Private->PciIo, Slot, &MediaPresent);
      if ((Status == EFI_MEDIA_CHANGED) && !MediaPresent) {
        DEBUG ((DEBUG_INFO, "SdMmcPciHcEnumerateDevice: device disconnected at slot %d of pci %p\n", Slot, Private->PciIo));
        Private->Slot[Slot].MediaPresent = FALSE;
        if (Private->Driver[Slot] != NULL) {
          Private->Driver[Slot]->Stop(&Private->ConnectedDevice[Slot], Private->DriverContext[Slot]);
          Private->Driver[Slot] = NULL;
        }
        Private->Slot[Slot].Initialized  = FALSE;
        //
        // Signal all async task events at the slot with EFI_NO_MEDIA status.
        //
        for (Link = GetFirstNode (&Private->Queue);
             !IsNull (&Private->Queue, Link);
             Link = NextLink) {
          NextLink = GetNextNode (&Private->Queue, Link);
          Trb = SD_MMC_HC_TRB_FROM_THIS (Link);
          if (Trb->Slot == Slot) {
            RemoveEntryList (Link);
            Trb->Packet->TransactionStatus = EFI_NO_MEDIA;
		  
            if (Trb->EventCallback) Trb->EventCallback(Trb->EventCallbackContext);
            SdMmcFreeTrb (Trb);
          }
        }
        //
        // Notify the upper layer the connect state change through ReinstallProtocolInterface.
        //

      }
      if ((Status == EFI_MEDIA_CHANGED) && MediaPresent) {
        DEBUG ((DEBUG_INFO, "SdMmcPciHcEnumerateDevice: device connected at slot %d of pci %p\n", Slot, Private->PciIo));
        //
        // Reset the specified slot of the SD/MMC Pci Host Controller
        //
        Status = SdMmcHcReset (Private, Slot);
        if (EFI_ERROR (Status)) {
          continue;
        }
        //
        // Reinitialize slot and restart identification process for the new attached device
        //
        Status = SdMmcHcInitHost (Private, Slot);
        if (EFI_ERROR (Status)) {
          continue;
        }

        Private->Slot[Slot].MediaPresent = TRUE;
        Private->Slot[Slot].Initialized  = TRUE;
        Private->Driver[Slot] = NULL;
        Private->DriverContext[Slot] = NULL;
        RoutineNum = sizeof (mCardTypeDetectRoutineTable) / sizeof (CARD_TYPE_DETECT_ROUTINE);
        for (Index = 0; Index < RoutineNum; Index++) {
          Routine = &mCardTypeDetectRoutineTable[Index];
          if (*Routine != NULL) {
            Status = (*Routine) (Private, Slot);
            if (!EFI_ERROR (Status)) {
              Link = GetFirstNode (&SdMmcDriverList);
              while (!IsNull (&SdMmcDriverList, Link)) {
                  Driver = BASE_CR(Link, ISD_DRIVER_INTERFACE, Link);
                  Status = IsdProbeAndStartDevice(Driver, &Private->ConnectedDevice[Slot], &Private->DriverContext[Slot]);
                  if (!EFI_ERROR (Status)) {
                    Private->Driver[Slot] = Driver;
                    break;
                  }
                  Link = GetNextNode (&SdMmcDriverList, Link);
              }
              break;
            }
          }
        }
        //
        // This card doesn't get initialized correctly.
        //
        if (Index == RoutineNum) {
          Private->Slot[Slot].Initialized = FALSE;
        }

        //
        // Notify the upper layer the connect state change through ReinstallProtocolInterface.
        //

      }
    }
  }

  return;
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
SdMmcPciHcDriverBindingSupported (
	IN EFI_PCI_IO_PROTOCOL *PciIo
)
{
	EFI_STATUS Status;
	PCI_TYPE00                PciData;

	Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0, sizeof (PciData), &PciData);

	if (EFI_ERROR (Status)) return Status;
	//
	// Examine SD PCI Host Controller PCI Configuration table fields.
	//
	if ((PciData.Hdr.ClassCode[2] == PCI_CLASS_SYSTEM_PERIPHERAL) &&
		(PciData.Hdr.ClassCode[1] == PCI_SUBCLASS_SD_HOST_CONTROLLER) &&
		((PciData.Hdr.ClassCode[0] == 0x00) || (PciData.Hdr.ClassCode[0] == 0x01))) {
		DEBUG ((DEBUG_INFO,"probe success %x %x %x %x %x\n", PciData.Hdr.VendorId, PciData.Hdr.DeviceId, PciData.Hdr.ClassCode[0], PciData.Hdr.ClassCode[1], PciData.Hdr.ClassCode[2]));

		return EFI_SUCCESS;
	}

	return EFI_UNSUPPORTED;
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
  @retval Others                   The driver failded to start the device.

**/

EFI_STATUS
SdMmcPciHcDriverBindingStart (
	IN EFI_PCI_IO_PROTOCOL  *PciIo,
	OUT VOID ** Context
)
{
  EFI_STATUS                      Status;
  SD_MMC_HC_PRIVATE_DATA          *Private;
  UINT64                          Supports;
  UINT64                          PciAttributes;
  UINT8                           SlotNum;
  UINT8                           FirstBar;
  UINT8                           Slot;
  UINT8                           Index;
  CARD_TYPE_DETECT_ROUTINE        *Routine;
  UINT32                          RoutineNum;
  BOOLEAN                         MediaPresent;
  LIST_ENTRY                      *Link;
  PISD_DRIVER_INTERFACE           Driver;
  UINTN                           Segment, Bus, Device, Function;

  DEBUG ((DEBUG_INFO, "SdMmcPciHcDriverBindingStart: Start\n"));

  //
  // Enable the SD Host Controller MMIO space
  //
  Private = NULL;
  Status  = PciIo->Attributes (
                     PciIo,
                     EfiPciIoAttributeOperationGet,
                     0,
                     &PciAttributes
                     );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status  = PciIo->GetLocation (
                     PciIo,
                     &Segment,
                     &Bus,
                     &Device,
                     &Function
                     );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );

  if (!EFI_ERROR (Status)) {
    Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
    Status    = PciIo->Attributes (
                         PciIo,
                         EfiPciIoAttributeOperationEnable,
                         Supports,
                         NULL
                         );
  } else {
    goto Done;
  }

  Private = AllocateCopyPool (sizeof (SD_MMC_HC_PRIVATE_DATA), &gSdMmcPciHcTemplate);
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Private->PciIo            = PciIo;
  Private->PciAttributes    = PciAttributes;
  InitializeListHead (&Private->Queue);

  Private->DevicePath.Name = SdMmcHciDeviceName;
  Private->DevicePath.Index = ((Bus * (PCI_MAX_DEVICE + 1)) + Device * (PCI_MAX_FUNC + 1)) + Function;

  //
  // Get SD/MMC Pci Host Controller Slot info
  //
  Status = SdMmcHcGetSlotInfo (PciIo, &FirstBar, &SlotNum);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  for (Slot = FirstBar; Slot < (FirstBar + SlotNum); Slot++) {
    Private->Slot[Slot].Enable = TRUE;
    Private->Slot[Slot].SlotNumber = Slot;
    //
    // Get SD/MMC Pci Host Controller Version
    //
    Status = SdMmcHcGetControllerVersion (PciIo, Slot, &Private->ControllerVersion[Slot]);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = SdMmcHcGetCapability (PciIo, Slot, &Private->Capability[Slot]);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Private->BaseClkFreq[Slot] = Private->Capability[Slot].BaseClkFreq;

    DumpCapabilityReg (Slot, &Private->Capability[Slot]);
    DEBUG ((
      DEBUG_INFO,
      "Slot[%d] Base Clock Frequency: %dMHz\n",
      Slot,
      Private->BaseClkFreq[Slot]
      ));

    //
    // If any of the slots does not support 64b system bus
    // do not enable 64b DMA in the PCI layer.
    //
    if ((Private->ControllerVersion[Slot] == SD_MMC_HC_CTRL_VER_300 &&
         Private->Capability[Slot].SysBus64V3 == 0) ||
        (Private->ControllerVersion[Slot] == SD_MMC_HC_CTRL_VER_400 &&
         Private->Capability[Slot].SysBus64V3 == 0) ||
        (Private->ControllerVersion[Slot] >= SD_MMC_HC_CTRL_VER_410 &&
         Private->Capability[Slot].SysBus64V4 == 0)) {
    }

    Status = SdMmcHcGetMaxCurrent (PciIo, Slot, &Private->MaxCurrent[Slot]);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Private->Slot[Slot].SlotType = Private->Capability[Slot].SlotType;
    if ((Private->Slot[Slot].SlotType != RemovableSlot) && (Private->Slot[Slot].SlotType != EmbeddedSlot)) {
      DEBUG ((DEBUG_INFO, "SdMmcPciHcDxe doesn't support the slot type [%d]!!!\n", Private->Slot[Slot].SlotType));
      continue;
    }

    //
    // Reset the specified slot of the SD/MMC Pci Host Controller
    //
    Status = SdMmcHcReset (Private, Slot);
    if (EFI_ERROR (Status)) {
      continue;
    }
    //
    // Check whether there is a SD/MMC card attached
    //
    if (Private->Slot[Slot].SlotType == RemovableSlot) {
      Status = SdMmcHcCardDetect (PciIo, Slot, &MediaPresent);
      if (EFI_ERROR (Status) && (Status != EFI_MEDIA_CHANGED)) {
        continue;
      } else if (!MediaPresent) {
        DEBUG ((
          DEBUG_INFO,
          "SdMmcHcCardDetect: No device attached in Slot[%d]!!!\n",
          Slot
          ));
        continue;
      }
    }

    Status = SdMmcHcInitHost (Private, Slot);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Private->ConnectedDevice[Slot].Slot = &Private->Slot[Slot];
    Private->ConnectedDevice[Slot].PassThru = &Private->PassThru;
    Private->ConnectedDevice[Slot].DevicePath = &Private->DevicePath;
    Private->Driver[Slot] = NULL;
    Private->DriverContext[Slot] = NULL;
    Private->Slot[Slot].MediaPresent = TRUE;
    Private->Slot[Slot].Initialized  = TRUE;
    RoutineNum = sizeof (mCardTypeDetectRoutineTable) / sizeof (CARD_TYPE_DETECT_ROUTINE);
    for (Index = 0; Index < RoutineNum; Index++) {
      Routine = &mCardTypeDetectRoutineTable[Index];
      if (*Routine != NULL) {
        Status = (*Routine) (Private, Slot);
        if (!EFI_ERROR (Status)) {
          Link = GetFirstNode (&SdMmcDriverList);
          while (!IsNull (&SdMmcDriverList, Link)) {
            Driver = BASE_CR(Link, ISD_DRIVER_INTERFACE, Link);

            Status = IsdProbeAndStartDevice(Driver, &Private->ConnectedDevice[Slot], &Private->DriverContext[Slot]);
            if (!EFI_ERROR (Status)) {
              Private->Driver[Slot] = Driver;
              break;
			}

            Link = GetNextNode (&SdMmcDriverList, Link);
          }
          break;
        }
      }
    }
    //
    // This card doesn't get initialized correctly.
    //
    if (Index == RoutineNum) {
      Private->Slot[Slot].Initialized = FALSE;
    }
  }

  //
  // Start the asynchronous I/O monitor
  //
  ProcessAsyncTaskList(Private);

  //
  // Start the Sd removable device connection enumeration
  //
  SdMmcPciHcEnumerateDevice(Private);

  DEBUG ((DEBUG_INFO, "SdMmcPciHcDriverBindingStart: %r End\n", Status));

Done:
  if (EFI_ERROR (Status)) {
    if ((Private != NULL) && (Private->PciAttributes != 0)) {
      //
      // Restore original PCI attributes
      //
      PciIo->Attributes (
               PciIo,
               EfiPciIoAttributeOperationSet,
               Private->PciAttributes,
               NULL
               );
    }

    if (Private != NULL) {
      FreePool (Private);
    }
  } else {
    *Context = Private;
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
SdMmcPciHcDriverBindingStop (
	IN EFI_PCI_IO_PROTOCOL  *PciIo,
	IN VOID *Context
)
{
  EFI_STATUS                          Status;
  SD_MMC_HC_PRIVATE_DATA              *Private;
  LIST_ENTRY                          *Link;
  LIST_ENTRY                          *NextLink;
  SD_MMC_HC_TRB                       *Trb;
  UINT8                           Slot;

  DEBUG ((DEBUG_INFO, "SdMmcPciHcDriverBindingStop: Start\n"));

  Private = (SD_MMC_HC_PRIVATE_DATA *)Context;

  for (Slot = 0; Slot < SD_MMC_HC_MAX_SLOT; Slot++) {
    if (Private->Slot[Slot].Enable && Private->Slot[Slot].Initialized) {
      if (Private->Driver[Slot] != NULL) {
        Status = Private->Driver[Slot]->Stop(&Private->ConnectedDevice[Slot], Private->DriverContext[Slot]);
        if (EFI_ERROR (Status)) return Status;
        Private->Driver[Slot] = NULL;
      }
    }
  }

  //
  // As the timer is closed, there is no needs to use TPL lock to
  // protect the critical region "queue".
  //
  for (Link = GetFirstNode (&Private->Queue);
       !IsNull (&Private->Queue, Link);
       Link = NextLink) {
    NextLink = GetNextNode (&Private->Queue, Link);
    RemoveEntryList (Link);
    Trb = SD_MMC_HC_TRB_FROM_THIS (Link);
    Trb->Packet->TransactionStatus = EFI_ABORTED;
    if (Trb->EventCallback) Trb->EventCallback(Trb->EventCallbackContext);
    SdMmcFreeTrb (Trb);
  }

  //
  // Restore original PCI attributes
  //
  PciIo  = Private->PciIo;
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSet,
                    Private->PciAttributes,
                    NULL
                    );

  FreePool (Private);

  DEBUG ((DEBUG_INFO, "SdMmcPciHcDriverBindingStop: End with %r\n", Status));

  return Status;
}

/**
  Execute TRB synchronously.

  @param[in] Private  Pointer to driver private data.
  @param[in] Trb      Pointer to TRB to execute.

  @retval EFI_SUCCESS  TRB executed successfully.
  @retval Other        TRB failed.
**/
EFI_STATUS
SdMmcPassThruExecSyncTrb (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN SD_MMC_HC_TRB           *Trb
  )
{
  EFI_STATUS  Status;

  //
  // Wait async I/O list is empty before execute sync I/O operation.
  //
  while (!IsListEmpty (&Private->Queue)) {
    ProcessAsyncTaskList(Private);
  }

  while (Trb->Retries) {
    Status = SdMmcWaitTrbEnv (Private, Trb);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = SdMmcExecTrb (Private, Trb);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = SdMmcWaitTrbResult (Private, Trb);
    if (Status == EFI_CRC_ERROR) {
      Trb->Retries--;
    } else {
      return Status;
    }
  }

  return Status;
}

/**
  Sends SD command to an SD card that is attached to the SD controller.

  The PassThru() function sends the SD command specified by Packet to the SD card
  specified by Slot.

  If Packet is successfully sent to the SD card, then EFI_SUCCESS is returned.

  If a device error occurs while sending the Packet, then EFI_DEVICE_ERROR is returned.

  If Slot is not in a valid range for the SD controller, then EFI_INVALID_PARAMETER
  is returned.

  If Packet defines a data command but both InDataBuffer and OutDataBuffer are NULL,
  EFI_INVALID_PARAMETER is returned.

  @param[in]     This           A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]     Slot           The slot number of the SD card to send the command to.
  @param[in,out] Packet         A pointer to the SD command data structure.
  @param[in]     Event          If Event is NULL, blocking I/O is performed. If Event is
                                not NULL, then nonblocking I/O is performed, and Event
                                will be signaled when the Packet completes.

  @retval EFI_SUCCESS           The SD Command Packet was sent by the host.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SD
                                command Packet.
  @retval EFI_INVALID_PARAMETER Packet, Slot, or the contents of the Packet is invalid.
  @retval EFI_INVALID_PARAMETER Packet defines a data command but both InDataBuffer and
                                OutDataBuffer are NULL.
  @retval EFI_NO_MEDIA          SD Device not present in the Slot.
  @retval EFI_UNSUPPORTED       The command described by the SD Command Packet is not
                                supported by the host controller.
  @retval EFI_BAD_BUFFER_SIZE   The InTransferLength or OutTransferLength exceeds the
                                limit supported by SD card ( i.e. if the number of bytes
                                exceed the Last LBA).

**/
EFI_STATUS
EFIAPI
SdMmcPassThruPassThru (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL         *This,
  IN     UINT8                                 Slot,
  IN OUT EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   *Packet,
  IN EFI_SD_MMC_CALLBACK                       EventCb OPTIONAL,
  IN VOID                                      *EventCbCtx OPTIONAL
  )
{
  EFI_STATUS                      Status;
  SD_MMC_HC_PRIVATE_DATA          *Private;
  SD_MMC_HC_TRB                   *Trb;

  if ((This == NULL) || (Packet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->SdMmcCmdBlk == NULL) || (Packet->SdMmcStatusBlk == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->OutDataBuffer == NULL) && (Packet->OutTransferLength != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->InDataBuffer == NULL) && (Packet->InTransferLength != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (This);

  if (!Private->Slot[Slot].Enable) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Private->Slot[Slot].MediaPresent) {
    return EFI_NO_MEDIA;
  }

  if (!Private->Slot[Slot].Initialized) {
    return EFI_DEVICE_ERROR;
  }

  Trb = SdMmcCreateTrb (Private, Slot, Packet, EventCb, EventCbCtx);
  if (Trb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Immediately return for async I/O.
  //
  if (EventCb != NULL) {
    return EFI_SUCCESS;
  }

  Status = SdMmcPassThruExecSyncTrb (Private, Trb);

  SdMmcFreeTrb (Trb);

  return Status;
}

/**
  Used to retrieve next slot numbers supported by the SD controller. The function
  returns information about all available slots (populated or not-populated).

  The GetNextSlot() function retrieves the next slot number on an SD controller.
  If on input Slot is 0xFF, then the slot number of the first slot on the SD controller
  is returned.

  If Slot is a slot number that was returned on a previous call to GetNextSlot(), then
  the slot number of the next slot on the SD controller is returned.

  If Slot is not 0xFF and Slot was not returned on a previous call to GetNextSlot(),
  EFI_INVALID_PARAMETER is returned.

  If Slot is the slot number of the last slot on the SD controller, then EFI_NOT_FOUND
  is returned.

  @param[in]     This           A pointer to the EFI_SD_MMMC_PASS_THRU_PROTOCOL instance.
  @param[in,out] Slot           On input, a pointer to a slot number on the SD controller.
                                On output, a pointer to the next slot number on the SD controller.
                                An input value of 0xFF retrieves the first slot number on the SD
                                controller.

  @retval EFI_SUCCESS           The next slot number on the SD controller was returned in Slot.
  @retval EFI_NOT_FOUND         There are no more slots on this SD controller.
  @retval EFI_INVALID_PARAMETER Slot is not 0xFF and Slot was not returned on a previous call
                                to GetNextSlot().

**/
EFI_STATUS
EFIAPI
SdMmcPassThruGetNextSlot (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL        *This,
  IN OUT UINT8                                *Slot
  )
{
  SD_MMC_HC_PRIVATE_DATA          *Private;
  UINT8                           Index;

  if ((This == NULL) || (Slot == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (This);

  if (*Slot == 0xFF) {
    for (Index = 0; Index < SD_MMC_HC_MAX_SLOT; Index++) {
      if (Private->Slot[Index].Enable) {
        *Slot = Index;
        Private->PreviousSlot = Index;
        return EFI_SUCCESS;
      }
    }
    return EFI_NOT_FOUND;
  } else if (*Slot == Private->PreviousSlot) {
    for (Index = *Slot + 1; Index < SD_MMC_HC_MAX_SLOT; Index++) {
      if (Private->Slot[Index].Enable) {
        *Slot = Index;
        Private->PreviousSlot = Index;
        return EFI_SUCCESS;
      }
    }
    return EFI_NOT_FOUND;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

/**
  Resets an SD card that is connected to the SD controller.

  The ResetDevice() function resets the SD card specified by Slot.

  If this SD controller does not support a device reset operation, EFI_UNSUPPORTED is
  returned.

  If Slot is not in a valid slot number for this SD controller, EFI_INVALID_PARAMETER
  is returned.

  If the device reset operation is completed, EFI_SUCCESS is returned.

  @param[in]  This              A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot              Specifies the slot number of the SD card to be reset.

  @retval EFI_SUCCESS           The SD card specified by Slot was reset.
  @retval EFI_UNSUPPORTED       The SD controller does not support a device reset operation.
  @retval EFI_INVALID_PARAMETER Slot number is invalid.
  @retval EFI_NO_MEDIA          SD Device not present in the Slot.
  @retval EFI_DEVICE_ERROR      The reset command failed due to a device error

**/
EFI_STATUS
EFIAPI
SdMmcPassThruResetDevice (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL           *This,
  IN UINT8                                   Slot
  )
{
  SD_MMC_HC_PRIVATE_DATA          *Private;
  LIST_ENTRY                      *Link;
  LIST_ENTRY                      *NextLink;
  SD_MMC_HC_TRB                   *Trb;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (This);

  if (!Private->Slot[Slot].Enable) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Private->Slot[Slot].MediaPresent) {
    return EFI_NO_MEDIA;
  }

  if (!Private->Slot[Slot].Initialized) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Free all async I/O requests in the queue
  //

  for (Link = GetFirstNode (&Private->Queue);
       !IsNull (&Private->Queue, Link);
       Link = NextLink) {
    NextLink = GetNextNode (&Private->Queue, Link);
    RemoveEntryList (Link);
    Trb = SD_MMC_HC_TRB_FROM_THIS (Link);
    Trb->Packet->TransactionStatus = EFI_ABORTED;
    if (Trb->EventCallback) Trb->EventCallback(Trb->EventCallbackContext);
    SdMmcFreeTrb (Trb);
  }

  return EFI_SUCCESS;
}
