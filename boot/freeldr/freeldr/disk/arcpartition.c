/*
 *  FreeLoader partitions support
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

#define TAG_ARC_PARTITION 'PcrA'

typedef struct
{
	LIST_ENTRY Link;
	CHAR Path[MAX_PATH];
	struct _DRIVE_LAYOUT_INFORMATION *Partitions;
	ULONG SectorSize;
	ULONG RefCount;
	ULONG FileId;
	OPENMODE OpenMode;
} ArcPartDevice;

typedef struct
{
	ArcPartDevice * Device;

	ULONGLONG PartStartOffset;
	ULONGLONG PartSize;

	BOOLEAN IsOpen;
	OPENMODE OpenMode;
	ULONGLONG Position;
} ArcPartContext;

LIST_ENTRY ArcPartDevices = {&ArcPartDevices, &ArcPartDevices};

#define PartitonString "partition("
#define PartitonStringSize 10
#define PartitonStringEnd ")"
#define PartitonStringEndSize 1

static ARC_STATUS GetArcNamePartition(CHAR* FullPath, CHAR* DevicePath, PULONG Partition)
{
	ULONG DevPathOffs = strlen(DevicePath);
	ULONG DevPathEndOffs;

	if (strncmp(FullPath, DevicePath, DevPathOffs) != 0) return ENOENT;

	if (strncmp(FullPath + DevPathOffs, PartitonString, PartitonStringSize) != 0) return ENOENT;
	
	DevPathOffs += PartitonStringSize;
	DevPathEndOffs = DevPathOffs;
	
	while ((FullPath[DevPathEndOffs] >='0') && (FullPath[DevPathEndOffs] <='9')) DevPathEndOffs++;
	
	if (DevPathEndOffs == DevPathOffs) return ENOENT;
	
	if (strncmp(FullPath + DevPathEndOffs, PartitonStringEnd, PartitonStringEndSize) != 0) return ENOENT;

	*Partition = atoi(FullPath + DevPathOffs);

	return ESUCCESS;
}

static ARC_STATUS GetArcPartitionDevice(CHAR* Path, ArcPartDevice ** Device, PULONG Partition)
{
	PLIST_ENTRY Link = ArcPartDevices.Flink;
	ArcPartDevice * Dev = NULL;
	ULONG Part;
	ARC_STATUS Status = ENOENT;

	while (Link != &ArcPartDevices)
	{
		Dev = CONTAINING_RECORD(Link, ArcPartDevice, Link);
		Status = GetArcNamePartition(Path, Dev->Path, &Part);
		if (Status == ESUCCESS) break;
		Link = Link->Flink; 
	}
	if (Status == ESUCCESS)
	{
		*Device = Dev;
		*Partition = Part;
	}
	return Status;
}

static ARC_STATUS ArcPartOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
	ArcPartDevice * Device = NULL;
	ArcPartContext * Context;
	struct _DRIVE_LAYOUT_INFORMATION * Partitions;
	ULONG Partition;
	ULONG i;
	FILEINFORMATION FileInfo;
	ARC_STATUS Status;

	if ((OpenMode != OpenReadOnly) && (OpenMode != OpenReadWrite)) return EINVAL;
	
	Status = GetArcPartitionDevice(Path, &Device, &Partition);
	if (Status != ESUCCESS) return Status;

	//check if lower device is readonly
	if ((OpenMode == OpenReadWrite) && (Device->OpenMode != OpenReadWrite)) return EINVAL;
	
	i = 0;
	Partitions = Device->Partitions;
	if (Partition > 0)
	{
		while ((i < Partitions->PartitionCount) && (Partition != Partitions->PartitionEntry[i].PartitionNumber)) i++;
		if (i < Partitions->PartitionCount)
		{
			if (Partitions->PartitionEntry[i].PartitionType == PARTITION_ENTRY_UNUSED) return ENOENT;
			if (Partitions->PartitionEntry[i].PartitionLength.QuadPart == 0) return EINVAL;
		}
		else return ENOENT;
	}

	if (Partition == 0)
	{
		Status = ArcGetFileInformation(Device->FileId, &FileInfo);
		if (Status != ESUCCESS) return Status;
	}

	Context = FrLdrTempAlloc(sizeof(ArcPartContext), TAG_ARC_PARTITION);
	if (!Context) return ENOMEM;
	
	Context->Device = Device;
	Context->Position = 0;
	if (Partition == 0)
	{
		//partition 0 represent whole device
		Context->PartStartOffset = 0;
		Context->PartSize = FileInfo.EndingAddress.QuadPart - FileInfo.StartingAddress.QuadPart;
	} else {
		Context->PartStartOffset = Partitions->PartitionEntry[i].StartingOffset.QuadPart;
		Context->PartSize = Partitions->PartitionEntry[i].PartitionLength.QuadPart;
	}

	Context->OpenMode = OpenMode;

	//success - register context
	Device->RefCount ++;
	FsSetDeviceSpecific(*FileId, Context);

	return Status;
}

static ARC_STATUS ArcPartClose(ULONG FileId)
{
	ArcPartContext* Context = FsGetDeviceSpecific(FileId);

	if (Context->Device->RefCount <= 0) return ENOENT; //double close call?

	Context->Device->RefCount --;

	FrLdrTempFree(Context, TAG_ARC_PARTITION);
	return ESUCCESS;
}

static ARC_STATUS ArcPartGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
	ArcPartContext* Context = FsGetDeviceSpecific(FileId);

	RtlZeroMemory(Information, sizeof(*Information));

	/*
	 * The ARC specification mentions that for partitions, StartingAddress and
	 * EndingAddress are the start and end positions of the partition in terms
	 * of byte offsets from the start of the disk.
	 * CurrentAddress is the current offset into (i.e. relative to) the partition.
	 */
	Information->StartingAddress.QuadPart = Context->PartStartOffset;
	Information->EndingAddress.QuadPart   = Context->PartStartOffset + Context->PartSize;
	Information->CurrentAddress.QuadPart  = Context->Position;

	return ESUCCESS;
}

static ARC_STATUS ArcPartRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
	ArcPartContext* Context = FsGetDeviceSpecific(FileId);
	ARC_STATUS Status;
	LARGE_INTEGER AbsPos;

	if (Context->Position + N > Context->PartSize) return EINVAL;
	
	AbsPos.QuadPart = Context->PartStartOffset + Context->Position;
	
	Status = ArcSeek(Context->Device->FileId, &AbsPos, SeekAbsolute);
	if (Status != ESUCCESS) return Status;
	Status = ArcRead(Context->Device->FileId, Buffer, N, Count);

	return Status;
}

static ARC_STATUS ArcPartSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
	ArcPartContext* Context = FsGetDeviceSpecific(FileId);
	LARGE_INTEGER NewPosition = *Position;

	switch (SeekMode)
	{
		case SeekAbsolute:
			break;
		case SeekRelative:			
			NewPosition.QuadPart += Context->Position;
			break;
		default:
			ASSERT(FALSE);
			return EINVAL;
	}
	if (NewPosition.QuadPart >= Context->PartSize)
		return EINVAL;

	Context->Position = NewPosition.QuadPart;

	return ESUCCESS;
}

static const DEVVTBL ArcPartVtbl = {
	ArcPartClose,
	ArcPartGetFileInformation,
	ArcPartOpen,
	ArcPartRead,
	ArcPartSeek
};

ARC_STATUS GetDiskSignatureAndChecksum(ULONG FileId, ULONG SectorSize, PULONG Signature, PULONG Checksum)
{
	PULONG BootSect = NULL;
	LARGE_INTEGER Pos;
	ARC_STATUS Status;
	ULONG cs, i;

	if (SectorSize < 512) return EINVAL;

	BootSect = FrLdrTempAlloc(SectorSize, TAG_ARC_PARTITION);
	if (BootSect == NULL) return ENOMEM;

	Pos.QuadPart = 0;
	Status = ArcSeek(FileId, &Pos, SeekAbsolute);
	if (Status == ESUCCESS)
	{
		Status = ArcRead(FileId, BootSect, SectorSize, &i);
		if (Status == ESUCCESS)
		{
			*Signature = BootSect[110]; //MBR signature
			cs = 0;
			for (i = 0; i < (SectorSize / sizeof(ULONG)); i++) cs += BootSect[i];
			*Checksum = 0 - cs;
		}
	}

	FrLdrTempFree(BootSect, TAG_ARC_PARTITION);
	return Status;
}

ARC_STATUS ArcRegisterDevicePartitions(CHAR* ArcName, ULONG SectorSize)
{
	ULONG FileId, i;
	ARC_STATUS Status;
	NTSTATUS NtStatus;
	struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer;
	CHAR PartitionName[MAX_PATH];
	ArcPartDevice * Device;
	ULONG Signature, Checksum;

	Status = ArcOpen(ArcName, OpenReadOnly, &FileId);
	if (Status != ESUCCESS) return Status;
	
	/* Read device partition table */
	NtStatus = HALDISPATCH->HalIoReadPartitionTable((PDEVICE_OBJECT)FileId, SectorSize, FALSE, &PartitionBuffer);
	if (!NT_SUCCESS(NtStatus))
	{
		PartitionBuffer = NULL;
		Status = EINVAL;
	}

	if (Status == ESUCCESS)
	{
		Status = GetDiskSignatureAndChecksum(FileId, SectorSize, &Signature, &Checksum);
	}

	ArcClose(FileId);

	if (Status == ESUCCESS)
	{
		/* Register device with partition(0) suffix */
		NtStatus = RtlStringCbPrintfA(PartitionName, sizeof(PartitionName), "%spartition(0)", ArcName);
		if (NT_SUCCESS(NtStatus))
		{
			AddReactOSArcDiskInfo(ArcName, Signature, Checksum, TRUE);
			printf("register Arc part %s\n", PartitionName);
			FsRegisterDevice(PartitionName, &ArcPartVtbl);
		}
		else Status = ENAMETOOLONG;
	}

	if (Status == ESUCCESS)
	{
		for (i = 0; i < PartitionBuffer->PartitionCount; i++)
		{
			if (PartitionBuffer->PartitionEntry[i].PartitionType != PARTITION_ENTRY_UNUSED)
			{
				NtStatus = RtlStringCbPrintfA(PartitionName,
					sizeof(PartitionName),
					"%spartition(%lu)",
					ArcName,
					PartitionBuffer->PartitionEntry[i].PartitionNumber);

				if (!NT_SUCCESS(NtStatus))
				{
					Status = ENAMETOOLONG;
					break;
				}
				printf("register Arc part %s\n", PartitionName);
				FsRegisterDevice(PartitionName, &ArcPartVtbl);
			}
		}
	}

	if (Status == ESUCCESS)
	{
		Device = FrLdrTempAlloc(sizeof(ArcPartDevice), TAG_ARC_PARTITION);
		if (Device != NULL)
		{
			NtStatus = RtlStringCbCopyA(Device->Path, sizeof(Device->Path), ArcName);
			if (!NT_SUCCESS(NtStatus))
			{
				Status = ENAMETOOLONG;
			}
			if (Status == ESUCCESS)
			{
				//ArcOpen is not support recursive calls, so open device now, try RW mode first
				Device->OpenMode = OpenReadWrite;
				Status = ArcOpen(ArcName, Device->OpenMode, &Device->FileId);
				if (Status != ESUCCESS)
				{
					//Unable to open device in RW mode - try RO
					Device->OpenMode = OpenReadOnly;
					Status = ArcOpen(ArcName, Device->OpenMode, &Device->FileId);
				}
				if (Status == ESUCCESS)
				{
					Device->Partitions = PartitionBuffer;
					PartitionBuffer = NULL; //dont free on return
					Device->SectorSize = SectorSize;
					Device->RefCount = 0;
					InsertTailList(&ArcPartDevices, &Device->Link);
					Device = NULL; //dont free on return
				}
			}
		} else {
			Status = ENOMEM;
		}
	}

	if (PartitionBuffer) ExFreePool(PartitionBuffer);
	if (Device) FrLdrTempFree(Device, TAG_ARC_PARTITION);

	return Status;
}

ARC_STATUS ArcUnregisterDevicePartitions(CHAR* ArcName)
{
	ArcPartDevice * Device = NULL;
	PLIST_ENTRY Link = ArcPartDevices.Flink;
	ARC_STATUS Status = ENOENT;

	while (Link != &ArcPartDevices)
	{
		Device = CONTAINING_RECORD(Link, ArcPartDevice, Link);
		if (strcmp(ArcName, Device->Path) == 0)
		{
			Status = ESUCCESS;
			break;
		}
		Link = Link->Flink; 
	}

	if (Status != ESUCCESS) return Status;

	//TODO: enable this check when all forgot-to-close bugs in freeldr will be fixed
	//if (Device->RefCount) return EBUSY;
	
	RemoveEntryList(&Device->Link);
	
	ArcClose(Device->FileId);
	ExFreePool(Device->Partitions);
	
	FrLdrTempFree(Device, TAG_ARC_PARTITION);

	return ESUCCESS;
}
