/** @file
  The EFI_SD_MMC_PASS_THRU_PROTOCOL provides the ability to send SD/MMC Commands
  to any SD/MMC device attached to the SD compatible pci host controller.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SD_MMC_PASS_THRU_H__
#define __SD_MMC_PASS_THRU_H__

#include <efidef.h>
#include <SimpleDrvInterface.h>

#define EDKII_SD_MMC_BUS_WIDTH_IGNORE MAX_UINT8
#define EDKII_SD_MMC_CLOCK_FREQ_IGNORE MAX_UINT32
#define EDKII_SD_MMC_DRIVER_STRENGTH_IGNORE  MAX_UINT8

typedef struct _EFI_SD_MMC_PASS_THRU_PROTOCOL EFI_SD_MMC_PASS_THRU_PROTOCOL;

typedef enum {
  UnknownCardType,
  SdCardType,
  SdioCardType,
  MmcCardType,
  EmmcCardType
} SD_MMC_CARD_TYPE;

typedef enum {
  RemovableSlot,
  EmbeddedSlot,
  SharedBusSlot,
  UnknownSlot
} EFI_SD_MMC_SLOT_TYPE;

typedef enum {
  SdMmcCommandTypeBc,  // Broadcast commands, no response
  SdMmcCommandTypeBcr, // Broadcast commands with response
  SdMmcCommandTypeAc,  // Addressed(point-to-point) commands
  SdMmcCommandTypeAdtc // Addressed(point-to-point) data transfer commands
} EFI_SD_MMC_COMMAND_TYPE;

typedef enum {
  SdMmcResponseTypeR1,
  SdMmcResponseTypeR1b,
  SdMmcResponseTypeR2,
  SdMmcResponseTypeR3,
  SdMmcResponseTypeR4,
  SdMmcResponseTypeR5,
  SdMmcResponseTypeR5b,
  SdMmcResponseTypeR6,
  SdMmcResponseTypeR7
} EFI_SD_MMC_RESPONSE_TYPE;

typedef struct _EFI_SD_MMC_COMMAND_BLOCK {
  UINT16                            CommandIndex;
  UINT32                            CommandArgument;
  UINT32                            CommandType;      // One of the EFI_SD_MMC_COMMAND_TYPE values
  UINT32                            ResponseType;     // One of the EFI_SD_MMC_RESPONSE_TYPE values
} EFI_SD_MMC_COMMAND_BLOCK;

typedef struct _EFI_SD_MMC_STATUS_BLOCK {
  UINT32                            Resp0;
  UINT32                            Resp1;
  UINT32                            Resp2;
  UINT32                            Resp3;
} EFI_SD_MMC_STATUS_BLOCK;

typedef struct _EFI_SD_MMC_PASS_THRU_COMMAND_PACKET {
  UINT64                            Timeout;
  EFI_SD_MMC_COMMAND_BLOCK          *SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK           *SdMmcStatusBlk;
  VOID                              *InDataBuffer;
  VOID                              *OutDataBuffer;
  UINT32                            InTransferLength;
  UINT32                            OutTransferLength;
  EFI_STATUS                        TransactionStatus;
} EFI_SD_MMC_PASS_THRU_COMMAND_PACKET;

typedef enum {
  SdDriverStrengthTypeB        = 0,
  SdDriverStrengthTypeA,
  SdDriverStrengthTypeC,
  SdDriverStrengthTypeD,
  SdDriverStrengthIgnore = EDKII_SD_MMC_DRIVER_STRENGTH_IGNORE
} SD_DRIVER_STRENGTH_TYPE;

typedef enum {
  EmmcDriverStrengthType0      = 0,
  EmmcDriverStrengthType1,
  EmmcDriverStrengthType2,
  EmmcDriverStrengthType3,
  EmmcDriverStrengthType4,
  EmmcDriverStrengthIgnore = EDKII_SD_MMC_DRIVER_STRENGTH_IGNORE
} EMMC_DRIVER_STRENGTH_TYPE;

typedef union {
  SD_DRIVER_STRENGTH_TYPE      Sd;
  EMMC_DRIVER_STRENGTH_TYPE    Emmc;
} EDKII_SD_MMC_DRIVER_STRENGTH;

typedef struct {
  //
  // The target width of the bus. If user tells driver to ignore it
  // or specifies unsupported width driver will choose highest supported
  // bus width for a given mode.
  //
  UINT8                         BusWidth;
  //
  // The target clock frequency of the bus in MHz. If user tells driver to ignore
  // it or specifies unsupported frequency driver will choose highest supported
  // clock frequency for a given mode.
  //
  UINT32                        ClockFreq;
  //
  // The target driver strength of the bus. If user tells driver to
  // ignore it or specifies unsupported driver strength, driver will
  // default to Type0 for eMMC cards and TypeB for SD cards. Driver strength
  // setting is only considered if chosen bus timing supports them.
  //
  EDKII_SD_MMC_DRIVER_STRENGTH  DriverStrength;
} EDKII_SD_MMC_OPERATING_PARAMETERS;

typedef enum {
  SdMmcSdDs,
  SdMmcSdHs,
  SdMmcUhsSdr12,
  SdMmcUhsSdr25,
  SdMmcUhsSdr50,
  SdMmcUhsDdr50,
  SdMmcUhsSdr104,
  SdMmcMmcLegacy,
  SdMmcMmcHsSdr,
  SdMmcMmcHsDdr,
  SdMmcMmcHs200,
  SdMmcMmcHs400,
} SD_MMC_BUS_MODE;

typedef
EFI_STATUS
(EFIAPI *EFI_SD_MMC_CALLBACK) (
  IN VOID * Context
);

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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_MMC_PASS_THRU_PASSTHRU) (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL         *This,
  IN     UINT8                                 Slot,
  IN OUT EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   *Packet,
  IN EFI_SD_MMC_CALLBACK                       EventCb OPTIONAL,
  IN VOID                                      *EventCbCtx OPTIONAL
);

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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_MMC_PASS_THRU_GET_NEXT_SLOT) (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL        *This,
  IN OUT UINT8                                *Slot
);

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
typedef
EFI_STATUS
(EFIAPI *EFI_SD_MMC_PASS_THRU_RESET_DEVICE) (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL           *This,
  IN UINT8                                   Slot
);

struct _EFI_SD_MMC_PASS_THRU_PROTOCOL {
  UINT32                                     IoAlign;
  EFI_SD_MMC_PASS_THRU_PASSTHRU              PassThru;
  EFI_SD_MMC_PASS_THRU_GET_NEXT_SLOT         GetNextSlot;
  EFI_SD_MMC_PASS_THRU_RESET_DEVICE          ResetDevice;
};

typedef struct {
  BOOLEAN                            Enable;
  EFI_SD_MMC_SLOT_TYPE               SlotType;
  UINT8                              SlotNumber;
  BOOLEAN                            MediaPresent;
  BOOLEAN                            Initialized;
  SD_MMC_CARD_TYPE                   CardType;
  UINT64                             CurrentFreq;
  EDKII_SD_MMC_OPERATING_PARAMETERS  OperatingParameters;
} SD_MMC_HC_SLOT;

typedef struct {
  SD_MMC_HC_SLOT *Slot;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;
  PISD_DEVICE_PATH DevicePath;
} EFI_SD_MMC_DEVICE;

#endif
