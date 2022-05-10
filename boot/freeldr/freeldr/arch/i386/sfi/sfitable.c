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

#include <hwdefs/sfitable.h>

BOOLEAN SfiCheckTableSum(struct sfi_table_simple * table)
{
	UINT8 * tblbuf;
	UINT32 i;
	UINT8 sum;

	tblbuf = (UINT8 *)table;
	sum = 0;
	for (i = 0; i < table->header.length; i++) {
		sum += tblbuf[i];
	}
	return (sum == 0);
}

struct sfi_table_simple *SfiFindSyst(VOID)
{
    UINT32 physAddr;
    struct sfi_table_simple *SfiTable;

	/* SFI spec defines the SYST starts at a 16-byte boundary */
	for (physAddr = SFI_SYST_SEARCH_BEGIN; physAddr < SFI_SYST_SEARCH_END; physAddr += 16) {
		SfiTable = (struct sfi_table_simple *)physAddr;
		if (*((UINT32 *)SfiTable) == SFI_SIG_SYST) {
			if (!SfiCheckTableSum(SfiTable)) {
				return NULL;
			}
			return SfiTable;
		}
	}
	return NULL;
}

struct sfi_table_simple * SfiFindTable(struct sfi_table_simple * syst, UINT32 sig)
{
	INT32 i, tbl_cnt;
	PHYSICAL_ADDRESS *pentry;
	struct sfi_table_simple *SfiTable;

	tbl_cnt = (syst->header.length - sizeof(struct sfi_table_header)) / sizeof(UINT64);
	pentry = syst->pentry;
	
	/* walk through the syst to search the table */
	for (i = 0; i < tbl_cnt; i++) {
		SfiTable = (struct sfi_table_simple *)pentry->LowPart;
		if (SfiTable == NULL) {
			return NULL;
		}
		if (*((UINT32 *)SfiTable) == sig) {
			if (!SfiCheckTableSum(SfiTable)) {
				return NULL;
			}
			return SfiTable;
		}
		pentry++;
	}
	return NULL;
}

UINT32 SfiTableEntryCount(struct sfi_table_simple * table, UINT32 entry_size)
{
	if (table->header.length < sizeof(struct sfi_table_header)) return 0;
	return (table->header.length - sizeof(struct sfi_table_header)) / entry_size;
}

EFI_ACPI_DESCRIPTION_HEADER * AcpiFindTableInXSDT (XSDT_TABLE *Xsdt, UINT32 Signature)
{
	UINT32 Index;
	UINT32 EntryCount;
	UINT64 * XsdtEntrys;
	EFI_ACPI_DESCRIPTION_HEADER *Table;
  
	EntryCount = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);
  
	XsdtEntrys = (UINT64 *)(&(Xsdt->Entry));
	for (Index = 0; Index < EntryCount; Index ++) {
		Table = (EFI_ACPI_DESCRIPTION_HEADER*)(unsigned int)(XsdtEntrys[Index]);
		if (Table->Signature == Signature) {
			return Table;
			break;
		}
	}

	return NULL;
}
