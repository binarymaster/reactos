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
#include <efidef.h>

#include <arch/drivers/interface/BlockIo.h>

#define TAG_BLOCKIODISK 'OIlB'

typedef struct {
	LIST_ENTRY Link;
	EFI_BLOCK_IO_PROTOCOL * Protocol;
	CHAR Path[MAX_PATH];
	BOOLEAN IsOpen;
	ULONG FileId;
} BlockIoDevice;

typedef struct {
	BlockIoDevice * Device;
	UINT32 MediaId;
	OPENMODE OpenMode;
	ULONG SectorSize;
	ULONGLONG Size;
	LARGE_INTEGER Position;
} BlockIoDiskContext;

ARC_STATUS ArcRegisterDevicePartitions(CHAR* Path, ULONG SectorSize);
ARC_STATUS ArcUnregisterDevicePartitions(CHAR* ArcName);

LIST_ENTRY BlockIoDevices = {&BlockIoDevices, &BlockIoDevices};

static ARC_STATUS ArcPathFromIsdPath(PISD_DEVICE_PATH IsdPath, CHAR * ArcPath, ULONG MaxArcPath)
{
	ULONG TailOffs = MaxArcPath - 1;
	PISD_DEVICE_PATH CurrPath = IsdPath;
	int ElementSize;
	NTSTATUS Status;

	if ((IsdPath == NULL) || (ArcPath == NULL)) return EINVAL;
	
	//IsdPath starts with upper level device
	//Print path element to start of the buffer and then copy it to the tail
	if (MaxArcPath < 5) return ENAMETOOLONG;
	ArcPath[TailOffs] = 0; //string terminating 0
	while (CurrPath != NULL) {
		Status = RtlStringCbPrintfA(ArcPath, TailOffs, "%s(%lu)", CurrPath->Name, CurrPath->Index);
		if (!NT_SUCCESS(Status)) return ENAMETOOLONG;
		ElementSize = strlen(ArcPath);

		if (ElementSize >= TailOffs) return ENAMETOOLONG;
		memcpy(ArcPath + TailOffs - ElementSize, ArcPath, ElementSize);
		TailOffs -= ElementSize;
		CurrPath = CurrPath->Parent;
	}
	//move path to start of buffer
	memcpy(ArcPath, ArcPath + TailOffs, MaxArcPath - TailOffs);
	
	return ESUCCESS;
}

static ARC_STATUS BlockIoDiskClose(ULONG FileId)
{
	BlockIoDiskContext* Context = FsGetDeviceSpecific(FileId);
	Context->Device->IsOpen = FALSE;
	FrLdrTempFree(Context, TAG_BLOCKIODISK);
	return ESUCCESS;
}

static ARC_STATUS BlockIoDiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
	BlockIoDiskContext* Context = FsGetDeviceSpecific(FileId);

	RtlZeroMemory(Information, sizeof(*Information));
	Information->EndingAddress.QuadPart = Context->Size;
	Information->CurrentAddress = Context->Position;

	return ESUCCESS;
}

static ARC_STATUS BlockIoDiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
	BlockIoDevice * Device = NULL;
	BlockIoDiskContext * Context;
	LIST_ENTRY * Link;

	Link = GetFirstNode (&BlockIoDevices);
	while ((Device == NULL) && (!IsNull (&BlockIoDevices, Link))) {
		Device = BASE_CR(Link, BlockIoDevice, Link);
		if (strcmp(Device->Path, Path) != 0) {
			Link = GetNextNode (&BlockIoDevices, Link);
			Device = NULL;
		}
	}

	if (Device == NULL) return ENOENT;

	if (Device->IsOpen) return EBUSY;
	if (Device->Protocol->Media->MediaPresent == FALSE) return ENODEV;
	switch (OpenMode){
		case OpenReadOnly:
			break;
		case OpenReadWrite:
			if (Device->Protocol->Media->ReadOnly) return EROFS;
			break;
		default:
			return EINVAL;
	}

	Context = FrLdrTempAlloc(sizeof(BlockIoDiskContext), TAG_BLOCKIODISK);
	if (!Context) return ENOMEM;

	Device->IsOpen = TRUE;
	
	Context->Device = Device;
	Context->OpenMode = OpenMode;
	Context->MediaId = Device->Protocol->Media->MediaId;
	Context->SectorSize = Device->Protocol->Media->BlockSize;
	Context->Size = Device->Protocol->Media->LastBlock + 1;
	Context->Size *= Context->SectorSize;
	Context->Position.QuadPart = 0;

	FsSetDeviceSpecific(*FileId, Context);

	return ESUCCESS;
}

static ARC_STATUS BlockIoDiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
	BlockIoDiskContext* Context = FsGetDeviceSpecific(FileId);
	EFI_LBA Lba;
	EFI_STATUS Status;
	
	//
	// Check size align
	//
	if ((N % Context->SectorSize) != 0)
		return EINVAL;

	//
	// Don't allow reads past our image
	//
	if ((Context->Position.QuadPart + N) > Context->Size)
	{
		*Count = 0;
		return EIO;
	}

	Lba = Context->Position.QuadPart;
	Lba /= Context->SectorSize;
	
	Status = Context->Device->Protocol->ReadBlocks(Context->Device->Protocol, Context->MediaId, Lba, N, Buffer);

	if (EFI_ERROR(Status)) return EIO;
	
	*Count = N;

	return ESUCCESS;
}

static ARC_STATUS BlockIoDiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
	BlockIoDiskContext* Context = FsGetDeviceSpecific(FileId); 
	LARGE_INTEGER NewPosition = *Position;

	switch (SeekMode)
	{
		case SeekAbsolute:
			break;
		case SeekRelative:			
			NewPosition.QuadPart += Context->Position.QuadPart;
			break;
		default:
			ASSERT(FALSE);
			return EINVAL;
	}
	if (NewPosition.QuadPart >= Context->Size)
		return EINVAL;

	Context->Position = NewPosition;

	return ESUCCESS;
}

static const DEVVTBL BlockIoDiskVtbl = {
	BlockIoDiskClose,
	BlockIoDiskGetFileInformation,
	BlockIoDiskOpen,
	BlockIoDiskRead,
	BlockIoDiskSeek,
};

static EFI_STATUS BlockIoStart (
	IN EFI_BLOCK_IO_PROTOCOL * Protocol,
	OUT VOID ** DriverContext
)
{
	BlockIoDevice * Device;
	ARC_STATUS Status;
	EFI_STATUS Res;
	
	Device = FrLdrTempAlloc(sizeof(BlockIoDevice), TAG_BLOCKIODISK);
	if (!Device) return EFI_OUT_OF_RESOURCES;
	
	Device->Protocol = Protocol;
	Device->IsOpen = FALSE;

	Status = ArcPathFromIsdPath(Protocol->Media->DevicePath, Device->Path, sizeof(Device->Path));
	
	if (Status == ESUCCESS) {
		printf("register Arc device %s\n", Device->Path);
	
		FsRegisterDevice(Device->Path, &BlockIoDiskVtbl);
	
		InsertTailList(&BlockIoDevices, &Device->Link);
 
		*DriverContext = Device;
	
		Status = ArcRegisterDevicePartitions(Device->Path, Device->Protocol->Media->BlockSize);
		if (Status == ESUCCESS) return EFI_SUCCESS;

		Res = EFI_DEVICE_ERROR;
	} else {
		Res = EFI_INVALID_PARAMETER;
	}
	if (Res != EFI_SUCCESS) {
		FrLdrTempFree(Device, TAG_BLOCKIODISK);
	}
	return Res;
}

static EFI_STATUS BlockIoStop (
	IN EFI_BLOCK_IO_PROTOCOL * Protocol,
	IN BlockIoDevice * Device
)
{
	if (ArcUnregisterDevicePartitions(Device->Path) != ESUCCESS) return EFI_DEVICE_ERROR;

	if (Device->IsOpen) {
		ArcClose(Device->FileId); 
	}

	RemoveEntryList(&Device->Link);
	
	FrLdrTempFree(Device, TAG_BLOCKIODISK);

	return EFI_SUCCESS;
}

ISD_DRIVER_INTERFACE BlockIoDriverInterface = {
	{
		NULL,
		NULL,
	},
	NULL,
	NULL,
	IsdAlwaysSuccessfulProbe,
	(ISD_DRIVER_START)BlockIoStart,
	(ISD_DRIVER_STOP)BlockIoStop
};
