/** @file
  The helper functions for BlockIo protocol.

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  FreeLoader ISD mod (C) 2021  Xen (https://gitlab.com/XenRE)

**/

#include "EmmcIsd.h"

/**
  Send command SELECT to the device to select/deselect the device.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSelect (
  IN     EMMC_DEVICE                  *Device,
  IN     UINT16                       Rca
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SELECT_DESELECT_CARD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL, NULL);

  return Status;
}

/**
  Send command SEND_STATUS to the device to get device status.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] DevStatus         The buffer to store the device status.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSendStatus (
  IN     EMMC_DEVICE                  *Device,
  IN     UINT16                       Rca,
     OUT UINT32                       *DevStatus
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_STATUS;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL, NULL);
  if (!EFI_ERROR (Status)) {
    CopyMem (DevStatus, &SdMmcStatusBlk.Resp0, sizeof (UINT32));
  }

  return Status;
}

/**
  Send command SEND_CSD to the device to get the CSD register data.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] Csd               The buffer to store the EMMC_CSD register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetCsd (
  IN     EMMC_DEVICE                  *Device,
  IN     UINT16                       Rca,
     OUT EMMC_CSD                     *Csd
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (Csd, sizeof (EMMC_CSD));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_CSD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8*)Csd) + 1, &SdMmcStatusBlk.Resp0, sizeof (EMMC_CSD) - 1);
  }

  return Status;
}

/**
  Send command SEND_CID to the device to get the CID register data.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] Cid               The buffer to store the EMMC_CID register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetCid (
  IN     EMMC_DEVICE            *Device,
  IN     UINT16                 Rca,
     OUT EMMC_CID               *Cid
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (Cid, sizeof (EMMC_CID));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_CID;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8*)Cid) + 1, &SdMmcStatusBlk.Resp0, sizeof (EMMC_CID) - 1);
  }

  return Status;
}

/**
  Send command SEND_EXT_CSD to the device to get the EXT_CSD register data.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[out] ExtCsd            The buffer to store the EXT_CSD register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetExtCsd (
  IN     EMMC_DEVICE                  *Device,
     OUT EMMC_EXT_CSD                 *ExtCsd
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (ExtCsd, sizeof (EMMC_EXT_CSD));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_EXT_CSD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = 0x00000000;
  Packet.InDataBuffer     = ExtCsd;
  Packet.InTransferLength = sizeof (EMMC_EXT_CSD);

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL, NULL);

  return Status;
}

/**
  Set the specified EXT_CSD register field through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  Offset            The offset of the specified field in EXT_CSD register.
  @param[in]  Value             The byte value written to the field specified by Offset.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSetExtCsd (
  IN  EMMC_PARTITION            *Partition,
  IN  UINT8                     Offset,
  IN  UINT8                     Value
  )
{
  EFI_STATUS                    Status;
  EMMC_DEVICE                   *Device;
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;
  UINT32                        CommandArgument;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SWITCH;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1b;
  //
  // Write the Value to the field specified by Offset.
  //
  CommandArgument = (Value << 8) | (Offset << 16) | BIT24 | BIT25;
  SdMmcCmdBlk.CommandArgument = CommandArgument;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL, NULL);

  return Status;
}

/**
  Set the number of blocks for a block read/write cmd through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  BlockNum          The number of blocks for transfer.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSetBlkCount (
  IN  EMMC_PARTITION            *Partition,
  IN  UINT16                    BlockNum
  )
{
  EFI_STATUS                    Status;
  EMMC_DEVICE                   *Device;
  EFI_SD_MMC_COMMAND_BLOCK      SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK       SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SET_BLOCK_COUNT;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = BlockNum;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL, NULL);

  return Status;
}

/**
  Read/write multiple blocks through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  Lba               The starting logical block address to be read/written.
                                The caller is responsible for reading/writing to only
                                legitimate locations.
  @param[in]  Buffer            A pointer to the destination/source buffer for the data.
  @param[in]  BufferSize        Size of Buffer, must be a multiple of device block size.
  @param[in]  IsRead            Indicates it is a read or write operation.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcRwMultiBlocks (
  IN  EMMC_PARTITION            *Partition,
  IN  EFI_LBA                   Lba,
  IN  VOID                      *Buffer,
  IN  UINTN                     BufferSize,
  IN  BOOLEAN                   IsRead
  )
{
  EFI_STATUS                    Status;
  EMMC_DEVICE                   *Device;
  EFI_SD_MMC_COMMAND_BLOCK      SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK       SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  //
  // Calculate timeout value through the below formula.
  // Timeout = (transfer size) / (2MB/s).
  // Taking 2MB/s as divisor is because it's nearest to the eMMC lowest
  // transfer speed (2.4MB/s).
  // Refer to eMMC 5.0 spec section 6.9.1 for details.
  //
  Packet.Timeout = (BufferSize / (2 * 1024 * 1024) + 1) * 1000 * 1000;

  if (IsRead) {
    Packet.InDataBuffer     = Buffer;
    Packet.InTransferLength = (UINT32)BufferSize;

    SdMmcCmdBlk.CommandIndex = EMMC_READ_MULTIPLE_BLOCK;
    SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  } else {
    Packet.OutDataBuffer     = Buffer;
    Packet.OutTransferLength = (UINT32)BufferSize;

    SdMmcCmdBlk.CommandIndex = EMMC_WRITE_MULTIPLE_BLOCK;
    SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  }

  if (Partition->Device->SectorAddressing) {
    SdMmcCmdBlk.CommandArgument = (UINT32)Lba;
  } else {
    SdMmcCmdBlk.CommandArgument = (UINT32)MultU64x32 (Lba, Partition->BlockMedia.BlockSize);
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL, NULL);

  return Status;
}

/**
  This function transfers data from/to EMMC device.

  @param[in]       Partition    A pointer to the EMMC_PARTITION instance.
  @param[in]       MediaId      The media ID that the read/write request is for.
  @param[in]       Lba          The starting logical block address to be read/written.
                                The caller is responsible for reading/writing to only
                                legitimate locations.
  @param[in, out]  Buffer       A pointer to the destination/source buffer for the data.
  @param[in]       BufferSize   Size of Buffer, must be a multiple of device block size.
  @param[in]       IsRead       Indicates it is a read or write operation.
  @param[in, out]  Token        A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           The data was read/written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be read/written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read/write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read/write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EmmcReadWrite (
  IN     EMMC_PARTITION                 *Partition,
  IN     UINT32                         MediaId,
  IN     EFI_LBA                        Lba,
  IN OUT VOID                           *Buffer,
  IN     UINTN                          BufferSize,
  IN     BOOLEAN                        IsRead
  )
{
  EFI_STATUS                            Status;
  EMMC_DEVICE                           *Device;
  EFI_BLOCK_IO_MEDIA                    *Media;
  UINTN                                 BlockSize;
  UINTN                                 BlockNum;
  UINTN                                 IoAlign;
  UINT8                                 PartitionConfig;
  UINTN                                 Remaining;
  UINT32                                MaxBlock;

  Status = EFI_SUCCESS;
  Device = Partition->Device;
  Media  = &Partition->BlockMedia;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (!IsRead && Media->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  //
  // Check parameters.
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BlockSize = Media->BlockSize;
  if ((BufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  BlockNum  = BufferSize / BlockSize;
  if ((Lba + BlockNum - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  IoAlign = Media->IoAlign;
  if (IoAlign > 0 && (((UINTN) Buffer & (IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if needs to switch partition access.
  //
  PartitionConfig = Device->ExtCsd.PartitionConfig;
  if ((PartitionConfig & 0x7) != Partition->PartitionType) {
    PartitionConfig &= (UINT8)~0x7;
    PartitionConfig |= Partition->PartitionType;
    Status = EmmcSetExtCsd (Partition, OFFSET_OF (EMMC_EXT_CSD, PartitionConfig), PartitionConfig);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Device->ExtCsd.PartitionConfig = PartitionConfig;
  }
  //
  // Start to execute data transfer. The max block number in single cmd is 65535 blocks.
  //
  Remaining = BlockNum;
  MaxBlock  = 0xFFFF;

  while (Remaining > 0) {
    if (Remaining <= MaxBlock) {
      BlockNum = Remaining;
    } else {
      BlockNum = MaxBlock;
    }
    Status = EmmcSetBlkCount (Partition, (UINT16)BlockNum);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BufferSize = BlockNum * BlockSize;
    Status = EmmcRwMultiBlocks (Partition, Lba, Buffer, BufferSize, IsRead);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    DEBUG ((DEBUG_BLKIO,
      "Emmc%a(): Part %d Lba 0x%x BlkNo 0x%x with %x\n",
      IsRead ? "Read " : "Write", Partition->PartitionType, Lba, BlockNum,
      Status));

    Lba   += BlockNum;
    Buffer = (UINT8*)Buffer + BufferSize;
    Remaining -= BlockNum;
  }

  return Status;
}

/**
  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
EmmcReset (
  IN  EFI_BLOCK_IO_PROTOCOL     *This,
  IN  BOOLEAN                   ExtendedVerification
  )
{
  EFI_STATUS                    Status;
  EMMC_PARTITION                *Partition;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO (This);

  PassThru = Partition->Device->Private->PassThru;
  Status   = PassThru->ResetDevice (PassThru, Partition->Device->Slot);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
EmmcReadBlocks (
  IN     EFI_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN     UINTN                  BufferSize,
     OUT VOID                   *Buffer
  )
{
  EFI_STATUS             Status;
  EMMC_PARTITION         *Partition;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO (This);

  Status = EmmcReadWrite (Partition, MediaId, Lba, Buffer, BufferSize, TRUE);
  return Status;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
EmmcWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  EFI_STATUS             Status;
  EMMC_PARTITION         *Partition;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO (This);

  Status = EmmcReadWrite (Partition, MediaId, Lba, Buffer, BufferSize, FALSE);
  return Status;
}

/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
EmmcFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
{
  //
  // return directly
  //
  return EFI_SUCCESS;
}
