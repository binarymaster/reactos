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
#include <multiboot.h>

#include <arch/drivers/IsdDrivers.h>

#include <hwdefs/sfitable.h>
#include <hwdefs/EfiMemType.h>
#include <hwdefs/Rtc.h>

#include <debug.h>

DBG_DEFAULT_CHANNEL(HWDETECT);

/* vRTC YEAR reg contains the offset to 1970 */
#define SfiMrtcYearMin 1970

#define DefaultVgaBiosLoadAddr 0xC0000
const char * MultibootModuleVgaBiosCmd = "VgaBios";

static struct sfi_table_simple * SfiSystTable = NULL;

static UINT32 SfiRtcBaseAddr = 0;
struct sfi_timer_table_entry * SfiTimer = NULL;

//this functions will be set later by detected keypad driver
BOOLEAN (*SfiKbHitFunc)(VOID) = NULL;
int (*SfiConsGetChFunc)(VOID) = NULL;

static BOOLEAN SfiHwInited = FALSE;

static MCFG_STRUCTURE SfiMcfgData;
static VOID * SfiMcfgPciContext;

extern multiboot_info_t * MultibootInfoPtr;
 
extern FREELDR_MEMORY_DESCRIPTOR PcMemoryMap[MAX_BIOS_DESCRIPTORS + 1];
extern ULONG PcMapCount;

extern BOOLEAN SfiCalibrateStallExecution(VOID);

extern VOID
SetMemory(
	PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
	ULONG_PTR BaseAddress,
	SIZE_T Size,
	TYPE_OF_MEMORY MemoryType);

extern ULONG
PcMemFinalizeMemoryMap(
	PFREELDR_MEMORY_DESCRIPTOR MemoryMap);

VOID WaitForAnyKey()
{
	printf("Press any key to continue...\n");
	while (!MachConsKbHit());
	MachConsGetCh();
}

MCFG_STRUCTURE * SfiGetMcfg (VOID)
{
	XSDT_TABLE * XsdtTable;
	EFI_ACPI_DESCRIPTION_HEADER * McfgTable;

	XsdtTable = (XSDT_TABLE *)SfiFindTable(SfiSystTable, SFI_SIG_XSDT);

	if (XsdtTable == NULL) return NULL;
	if (XsdtTable->Header.Length < sizeof (XSDT_TABLE)) return NULL;
	
	McfgTable = AcpiFindTableInXSDT(XsdtTable, SFI_SIG_XSDT_MCFG);
	
	if (McfgTable == NULL) return NULL;
	if (McfgTable->Length < (sizeof(EFI_ACPI_DESCRIPTION_HEADER) + sizeof(MCFG_STRUCTURE))) return NULL;	
	
	return (MCFG_STRUCTURE *)((UINT8 *)McfgTable + sizeof(EFI_ACPI_DESCRIPTION_HEADER) + sizeof(UINT64));
}

BOOLEAN SfiInitHardware(VOID)
{
	MCFG_STRUCTURE * Mcfg;

	if (SfiHwInited) return FALSE;

	if (SfiMcfgPciDriverInterface.RegisterInterface(&SdMmcHciDriverInterface)) return FALSE;
	if (SfiMcfgPciDriverInterface.RegisterInterface(&MrstKeypadDriverInterface)) return FALSE;
	if (SdMmcHciDriverInterface.RegisterInterface(&SdDriverInterface)) return FALSE;
	if (SdMmcHciDriverInterface.RegisterInterface(&EmmcDriverInterface)) return FALSE;
	if (SdDriverInterface.RegisterInterface(&BlockIoDriverInterface)) return FALSE;
	if (EmmcDriverInterface.RegisterInterface(&BlockIoDriverInterface)) return FALSE;

	Mcfg = SfiGetMcfg();
	
	if (Mcfg == NULL) {
		printf("Unable to locate MCFG table\n");
		return FALSE;
	}

	SfiMcfgData = *Mcfg;
	
	if (SfiMcfgPciDriverInterface.Start(&SfiMcfgData, &SfiMcfgPciContext)) return FALSE;

	SfiHwInited = TRUE;
	return TRUE;
}

BOOLEAN SfiFreeHardware(VOID)
{
	if (!SfiHwInited) return FALSE;

	if (SfiMcfgPciDriverInterface.Stop(&SfiMcfgData, SfiMcfgPciContext)) return FALSE;

	if (EmmcDriverInterface.UnregisterInterfaces()) return FALSE;
	if (SdDriverInterface.UnregisterInterfaces()) return FALSE;
	if (SdMmcHciDriverInterface.UnregisterInterfaces()) return FALSE;
	if (SfiMcfgPciDriverInterface.UnregisterInterfaces()) return FALSE;
	SfiHwInited = FALSE;
	return TRUE;
}

void RelocateAndInitVgaBios(void * Rom, void * RelocAddr, size_t Size)
{
	REGS  Regs;

	/*
	VGA BIOS start with 55 AA magic and must be called to offset 3
	During initialization it will relocate itself to the address, passed via BX register
	(C000 is default for VGA BIOS)
	*/

	//check size and magic
	if ((Size < 256) || (*(UINT16 *)Rom != 0xAA55)) return;

	//relocate module to the provided address
	memcpy(RelocAddr, Rom, Size);

	Regs.w.bx = (UINT16)((ULONG)RelocAddr >> 4);
	CallFar16((ULONG)RelocAddr >> 4, 3, &Regs, &Regs);
}

VOID
SfiGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
	/* Unused for SFI systems */
	*ExtendedBIOSDataArea = 0;
	*ExtendedBIOSDataSize = 0;
}

static
UCHAR
SfiGetFloppyCount(VOID)
{
	/* No floppy drives on SFI systems */

	return 0;
}

PCONFIGURATION_COMPONENT_DATA
SfiHwDetect(VOID)
{
	PCONFIGURATION_COMPONENT_DATA SystemKey;

	TRACE("DetectHardware()\n");

	/* Create the 'System' key */
	FldrCreateSystemKey(&SystemKey);

	//nothing to do here for SFI systems

	TRACE("DetectHardware() Done\n");
	return SystemKey;
}

//Keyboard input functions will be set during hardware detections by its drivers
BOOLEAN
SfiConsKbHit(VOID)
{
	if (SfiKbHitFunc == NULL) return FALSE;
	return SfiKbHitFunc();
}

int
SfiConsGetCh(VOID)
{
	if (SfiConsGetChFunc == NULL) return 0;
	return SfiConsGetChFunc();
}

VOID
SfiHwIdle(VOID)
{
	/* UNIMPLEMENTED */
}

VOID
FrLdrCheckCpuCompatibility(VOID)
{
	/* SFI system cannot have incompatible CPU */
}

TIMEINFO*
SfiGetTime(VOID)
{
	static TIMEINFO TimeInfo;

	if (SfiRtcBaseAddr == 0) return NULL;

	TimeInfo.Second = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_SECONDS * 4);
	TimeInfo.Minute = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_MINUTES * 4);
	TimeInfo.Hour = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_HOURS * 4);
	TimeInfo.Day = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_DAY_OF_THE_MONTH * 4);
	TimeInfo.Month = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_MONTH * 4);
	TimeInfo.Year = SfiMrtcYearMin + READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_YEAR * 4);

	return &TimeInfo;
}

PFREELDR_MEMORY_DESCRIPTOR
SfiMemGetMemoryMap(ULONG *MemoryMapSize)
{
	struct sfi_table_simple * MmapTable;
	struct sfi_mem_entry * SfiMemEntry;
	UINT32 EntryCnt, i;
	TYPE_OF_MEMORY MemType;

	PcMemoryMap[0].PageCount = 0;
	MmapTable = SfiFindTable(SfiSystTable, SFI_SIG_MMAP);
	if (MmapTable != NULL)
	{
		SfiMemEntry = (struct sfi_mem_entry *)MmapTable->pentry;
		EntryCnt = SfiTableEntryCount(MmapTable, sizeof(struct sfi_mem_entry));

		for (i = 0; i < EntryCnt; i++)
		{
			switch (SfiMemEntry[i].type)
			{
			case EfiConventionalMemory:
				MemType = LoaderFree;
				break;
			case EfiMemoryMappedIO:
				MemType = LoaderMaximum; //skip this memory descriptor
				break;
			default:
				MemType = LoaderFirmwarePermanent;
			}
			if ((MemType != LoaderMaximum) && (PcMapCount < MAX_BIOS_DESCRIPTORS))
			{
				SetMemory(PcMemoryMap,
					(ULONG_PTR)SfiMemEntry[i].phy_start.QuadPart,
					(SIZE_T)(SfiMemEntry[i].pages * PAGE_SIZE),
					MemType);
			}
		}
	}
	/* Setup some protected ranges */
	SetMemory(PcMemoryMap, 0x000000, 0x01000, LoaderFirmwarePermanent); // Realmode IVT / BDA
	SetMemory(PcMemoryMap, 0x0A0000, 0x50000, LoaderFirmwarePermanent); // Video memory
	SetMemory(PcMemoryMap, 0x0F0000, 0x10000, LoaderSpecialMemory); // ROM

	*MemoryMapSize = PcMemFinalizeMemoryMap(PcMemoryMap);
	return PcMemoryMap; 
}

// Initialize RTC and timer

BOOLEAN
SfiInitTime(VOID)
{
	struct sfi_table_simple * SfiTable;
	struct sfi_rtc_table_entry * RtcTable;

	SfiTable = SfiFindTable(SfiSystTable, SFI_SIG_MRTC);

	if (SfiTable == NULL) return FALSE;

	if (SfiTableEntryCount(SfiTable, sizeof(struct sfi_rtc_table_entry)) < 1) return FALSE;

	RtcTable = (struct sfi_rtc_table_entry *)SfiTable->pentry;
	if (RtcTable->phy_addr.HighPart != 0) return FALSE;
	SfiRtcBaseAddr = RtcTable->phy_addr.LowPart;

	SfiTable = SfiFindTable(SfiSystTable, SFI_SIG_MTMR);

	//SFI timer is not critical device
	if (SfiTable != NULL)
	{
		if (SfiTableEntryCount(SfiTable, sizeof(struct sfi_timer_table_entry)) > 0)
		{
			SfiTimer = (struct sfi_timer_table_entry *)SfiTable->pentry;
		}
	}

	return TRUE;
}

BOOLEAN
SfiInitializeBootDevices(VOID)
{
	if (SfiInitHardware())
	{
		if (FrLdrBootPath[0] == 0) {
			printf("No bootpath defined\n");
			WaitForAnyKey();
		}
	} else {
		printf("SfiInitHardware error\n");
		WaitForAnyKey();
	}
	return TRUE;
}

BOOLEAN
SfiDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
	/* Unused for SFI systems */
	return FALSE;
}

BOOLEAN
SfiDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
	/* Unused for SFI systems */
	return FALSE;
}

ULONG
SfiDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
	/* Unused for SFI systems */
	return 0;
}

VOID SfiBeep(VOID)
{
	/* UNIMPLEMENTED */
}

//TODO: move this function to common file
VOID __cdecl ChainLoadBiosBootSectorCode(
	IN UCHAR BootDrive OPTIONAL,
	IN ULONG BootPartition OPTIONAL)
{
	REGS Regs;

	RtlZeroMemory(&Regs, sizeof(Regs));

	/* Set the boot drive and the boot partition */
	Regs.b.dl = (UCHAR)(BootDrive ? BootDrive : FrldrBootDrive);
	Regs.b.dh = (UCHAR)(BootPartition ? BootPartition : FrldrBootPartition);

	Relocator16Boot(&Regs,
		/* Stack segment:pointer */
		0x0000, 0x7C00,
		/* Code segment:pointer */
		0x0000, 0x7C00);
}

VOID
SfiVideoInit(VOID)
{
	multiboot_module_t * MbModule;
	CHAR * ModCmdLine;
	ULONG i;
	ULONG ModRelocAddr, CmdArgOffs;

	/* SFI device may not have VGA-compatible BIOS, but this BIOS may be available
	 * on another devices with the same graphics controller.
	 * User can pass VGA BIOS image as multiboot module, tagged with "VgaBios" or "VgaBios=xxx"
	 * module command line string, where xxx is base address (hex).
	 */
	if (MultibootInfoPtr != NULL)
	{
		MbModule = (multiboot_module_t *)MultibootInfoPtr->mods_addr;

		if ((MultibootInfoPtr->flags & MULTIBOOT_INFO_MODS) && (MbModule != 0))
		{
			for (i = 0; i < MultibootInfoPtr->mods_count; i++)
			{
				ModCmdLine = (CHAR *)MbModule[i].cmdline;
				if (ModCmdLine != NULL)
				{
					CmdArgOffs = strlen(MultibootModuleVgaBiosCmd);
					if (_strnicmp(ModCmdLine, MultibootModuleVgaBiosCmd, CmdArgOffs) == 0)
					{
						if (ModCmdLine[CmdArgOffs] == '=')
						{
							CmdArgOffs++;
							ModRelocAddr = strtol(ModCmdLine + CmdArgOffs, NULL, 16);
						}
						else 
						{
							ModRelocAddr = DefaultVgaBiosLoadAddr;
						}
						if (ModRelocAddr > 0)
						{
							RelocateAndInitVgaBios((void *)MbModule[i].mod_start, (void *)ModRelocAddr, MbModule[i].mod_end - MbModule[i].mod_start);
						}
					}
				}
			}
		}
	}
}

/******************************************************************************/

BOOLEAN
SfiMachDetect(const char *CmdLine)
{
	SfiSystTable = SfiFindSyst();
	return (SfiSystTable != NULL);
}

VOID
MachInit(const char *CmdLine)
{
	
	/* SfiMachDetect must be called to obtain SYST table */
	if (!SfiMachDetect(CmdLine)) return;

	/* Load VGA BIOS */
	SfiVideoInit();
	
	/* Setup vtbl */
	MachVtbl.ConsPutChar = PcConsPutChar;
	MachVtbl.ConsKbHit = SfiConsKbHit;
	MachVtbl.ConsGetCh = SfiConsGetCh;
	MachVtbl.VideoClearScreen = PcVideoClearScreen;
	MachVtbl.VideoSetDisplayMode = PcVideoSetDisplayMode;
	MachVtbl.VideoGetDisplaySize = PcVideoGetDisplaySize;
	MachVtbl.VideoGetBufferSize = PcVideoGetBufferSize;
	MachVtbl.VideoGetFontsFromFirmware = PcVideoGetFontsFromFirmware;
	MachVtbl.VideoSetTextCursorPosition = PcVideoSetTextCursorPosition;
	MachVtbl.VideoHideShowTextCursor = PcVideoHideShowTextCursor;
	MachVtbl.VideoPutChar = PcVideoPutChar;
	MachVtbl.VideoCopyOffScreenBufferToVRAM = PcVideoCopyOffScreenBufferToVRAM;
	MachVtbl.VideoIsPaletteFixed = PcVideoIsPaletteFixed;
	MachVtbl.VideoSetPaletteColor = PcVideoSetPaletteColor;
	MachVtbl.VideoGetPaletteColor = PcVideoGetPaletteColor;
	MachVtbl.VideoSync = PcVideoSync;
	MachVtbl.Beep = SfiBeep;
	MachVtbl.PrepareForReactOS = SfiPrepareForReactOS;
	MachVtbl.GetMemoryMap = SfiMemGetMemoryMap;
	MachVtbl.GetExtendedBIOSData = SfiGetExtendedBIOSData;
	MachVtbl.GetFloppyCount = SfiGetFloppyCount;
	MachVtbl.DiskReadLogicalSectors = SfiDiskReadLogicalSectors;
	MachVtbl.DiskGetDriveGeometry = SfiDiskGetDriveGeometry;
	MachVtbl.DiskGetCacheableBlockCount = SfiDiskGetCacheableBlockCount;
	MachVtbl.GetTime = SfiGetTime;
	MachVtbl.InitializeBootDevices = SfiInitializeBootDevices;
	MachVtbl.HwDetect = SfiHwDetect;
	MachVtbl.HwIdle = SfiHwIdle;

	if (!SfiInitTime())
	{
		printf("Critical error: SfiInitTime failed\n");
		WaitForAnyKey();
		return;
	}

	if (!SfiCalibrateStallExecution())
	{
		printf("CalibrateStallExecution failed - leaving default DelayCount value\n");
	}
}

VOID
SfiPrepareForReactOS(VOID)
{
	/* On SFI, prepare video */
	if (!SfiFreeHardware())
	{
		UiMessageBoxCritical("Error: SfiFreeHardware failed\n");
	}

	PcVideoPrepareForReactOS();
}

/* EOF */
