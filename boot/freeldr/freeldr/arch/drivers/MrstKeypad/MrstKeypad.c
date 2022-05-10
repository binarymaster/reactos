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

#include <arch/drivers/IsdDrivers.h>
#include <arch/drivers/interface/PciIo.h>
#include <hwdefs/Pci.h>
#include <hwdefs/MrstKeypad.h>

#define MRST_KEYPAD_DIRECT_KEY_COUNT 4
#define MRST_KEYPAD_DIRECT_KEY_MASK ((1 << MRST_KEYPAD_DIRECT_KEY_COUNT) - 1)
#define MRST_KEYPAD_DIRECT_KEY_INVERSE MRST_KEYPAD_DIRECT_KEY_MASK

#define MRST_KEYPAD_BAR 0

int MkpKeyCodes[MRST_KEYPAD_DIRECT_KEY_COUNT] = {
	KEY_UP,	//Volume up
	KEY_DOWN,	//Volume down
	KEY_ESC,	//Home
	KEY_ENTER	//Mute
};

EFI_PCI_IO_PROTOCOL * MrstKeypadPciIo = NULL;

UINT32 MrstKeypadCurrKeys = 0;
UINT32 MrstKeypadLastKeys = 0;

extern BOOLEAN (*SfiKbHitFunc)(VOID);
extern int (*SfiConsGetChFunc)(VOID);

static int MrstKeypadGetDirectKeyPressCount()
{
	UINT32 Keys = MrstKeypadCurrKeys;
	int Res = 0;

	while (Keys != 0)
	{
		if (Keys & 1) Res ++;
		Keys >>= 1;
	}

	return Res;
}

static int MrstKeypadGetDirectKeyPressChCount()
{
	UINT32 Keys = MrstKeypadCurrKeys;
	UINT32 KeysCh = Keys ^ MrstKeypadLastKeys;
	int Res = 0;

	while (Keys != 0)
	{
		if ((Keys & 1) && (KeysCh & 1))  Res ++;
		Keys >>= 1;
		KeysCh >>= 1;
	}

	return Res;
}

static int MrstKeypadGetDirectKeyPressChCode()
{
	UINT32 Keys = MrstKeypadCurrKeys;
	UINT32 KeysCh = Keys ^ MrstKeypadLastKeys;
	int KeyIndex = 0;

	while (Keys != 0)
	{
		if ((Keys & 1) && (KeysCh & 1)) return MkpKeyCodes[KeyIndex];
		Keys >>= 1;
		KeysCh >>= 1;
		KeyIndex ++;
	}
	return 0;
}

static
EFI_STATUS
MrstKeypadReadRegister(
	EFI_PCI_IO_PROTOCOL *PciIo,
	UINT32 Reg,
	UINT32 * Value
)
{
	return PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, MRST_KEYPAD_BAR, Reg, 1, Value);
}

static
EFI_STATUS
MrstKeypadWriteRegister(
	EFI_PCI_IO_PROTOCOL *PciIo,
	UINT32 Reg,
	UINT32 Value
)
{
    return PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, MRST_KEYPAD_BAR, Reg, 1, &Value);
}

BOOLEAN
MrstKeypadKbHit(VOID)
{
	EFI_STATUS Status;

	if (MrstKeypadPciIo == NULL) return FALSE;

	MrstKeypadLastKeys = MrstKeypadCurrKeys;
	Status = MrstKeypadReadRegister(MrstKeypadPciIo, KPDK, &MrstKeypadCurrKeys);
	if (EFI_ERROR (Status))
	{
		return FALSE;
	}
	MrstKeypadCurrKeys = (KPDK_DK(MrstKeypadCurrKeys) & MRST_KEYPAD_DIRECT_KEY_MASK) ^ MRST_KEYPAD_DIRECT_KEY_INVERSE;

	return (MrstKeypadGetDirectKeyPressChCount() > 0);
}

int
MrstKeypadGetCh(VOID)
{
	int res;

	if (MrstKeypadPciIo == NULL) return 0;
	res = MrstKeypadGetDirectKeyPressChCode();
	MrstKeypadLastKeys = MrstKeypadCurrKeys;
	return res;
}

EFI_STATUS
EFIAPI
MrstKeypadProbe (
	IN EFI_PCI_IO_PROTOCOL *PciIo
	)
{
	EFI_STATUS Status;
	PCI_TYPE00 PciData;

	Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0, sizeof(PciData), &PciData);
	if (EFI_ERROR (Status))
	{
		return Status;
	}

	if ((PciData.Hdr.VendorId != MrstKeypadVendorId) || (PciData.Hdr.DeviceId != MrstKeypadDeviceId))
	{
		return EFI_UNSUPPORTED;
	}

	printf("MrstKeypad detected\n");

	return EFI_SUCCESS;
}

EFI_STATUS
MrstKeypadStart (
	IN EFI_PCI_IO_PROTOCOL  *PciIo,
	OUT VOID ** Context
)
{
	EFI_STATUS Status;

	//double device / double start is not allowed
	if (MrstKeypadPciIo != NULL) return EFI_UNSUPPORTED;

	Status = MrstKeypadWriteRegister(PciIo, KPC, KPC_DE | KPC_DKN(MRST_KEYPAD_DIRECT_KEY_COUNT));
	if (EFI_ERROR (Status))
	{
		return Status;
	}
	Status = MrstKeypadWriteRegister(PciIo, KPREC, DEFAULT_KPREC);
	if (EFI_ERROR (Status))
	{
		return Status;
	}
	Status = MrstKeypadWriteRegister(PciIo, KPKDI, DEBOUNCE_INTERVAL);
	if (EFI_ERROR (Status))
	{
		return Status;
	}
	
	Status = MrstKeypadReadRegister(PciIo, KPDK, &MrstKeypadCurrKeys);
	if (EFI_ERROR (Status))
	{
		return Status;
	}
	MrstKeypadCurrKeys = (KPDK_DK(MrstKeypadCurrKeys) & MRST_KEYPAD_DIRECT_KEY_MASK) ^ MRST_KEYPAD_DIRECT_KEY_INVERSE;

	//disable device if multiple keypress detected
	if (MrstKeypadGetDirectKeyPressCount() > 1)
	{
		printf("Multiple keypress detected - MrstKeypad disabled\n");
		return EFI_DEVICE_ERROR;
	}
	MrstKeypadLastKeys = MrstKeypadCurrKeys;

	MrstKeypadPciIo = PciIo;

	//register keyboard functions
	SfiKbHitFunc = MrstKeypadKbHit;
	SfiConsGetChFunc = MrstKeypadGetCh;

	return EFI_SUCCESS;
}

EFI_STATUS
MrstKeypadStop (
	IN EFI_PCI_IO_PROTOCOL  *PciIo,
	IN VOID *Context
)
{
	if (PciIo != MrstKeypadPciIo) return EFI_INVALID_PARAMETER;

	//unregister keyboard functions
	SfiKbHitFunc = NULL;
	SfiConsGetChFunc = NULL;

	MrstKeypadPciIo = NULL;

	return EFI_SUCCESS;
}

ISD_DRIVER_INTERFACE MrstKeypadDriverInterface = {
	{
		NULL,
		NULL,
	},
	NULL,
	NULL,
	(ISD_DRIVER_PROBE)MrstKeypadProbe,
	(ISD_DRIVER_START)MrstKeypadStart,
	(ISD_DRIVER_STOP)MrstKeypadStop
};
