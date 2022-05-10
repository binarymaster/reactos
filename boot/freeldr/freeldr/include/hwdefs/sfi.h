/* sfi.h Simple Firmware Interface */

/*
 * Copyright (C) 2009, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef _LINUX_SFI_H
#define _LINUX_SFI_H

#include <ntdef.h>

/* Table signatures reserved by the SFI specification */
#define SFI_SIG_SYST		'TSYS'	//"SYST"
#define SFI_SIG_FREQ		'QERF'	//"FREQ"
#define SFI_SIG_IDLE		'ELDI'	//"IDLE"
#define SFI_SIG_CPUS		'SUPC'	//"CPUS"
#define SFI_SIG_MTMR		'RMTM'	//"MTMR"
#define SFI_SIG_MRTC		'CTRM'	//"MRTC"
#define SFI_SIG_MMAP		'PAMM'	//"MMAP"
#define SFI_SIG_APIC		'CIPA'	//"APIC"
#define SFI_SIG_XSDT		'TDSX'	//"XSDT"
#define SFI_SIG_WAKE		'EKAW'	//"WAKE"
#define SFI_SIG_SPIB		'BIPS'	//"SPIB"
#define SFI_SIG_I2CB		'BC2I'	//"I2CB"
#define SFI_SIG_GPEM		'MEPG'	//"GPEM"

#define SFI_SIG_XSDT_MCFG	'GFCM'	//"MCFG"

#define SFI_ACPI_TABLE		(1 << 0)
#define SFI_NORMAL_TABLE	(1 << 1)

#define SFI_SIGNATURE_SIZE	4
#define SFI_OEM_ID_SIZE		6
#define SFI_OEM_TABLE_ID_SIZE	8

#define SFI_SYST_SEARCH_BEGIN		0x000E0000
#define SFI_SYST_SEARCH_END		0x000FFFFF

#define SFI_GET_NUM_ENTRIES(ptable, entry_type) \
	((ptable->header.length - sizeof(struct sfi_table_header)) / \
	(sizeof(entry_type)))

#pragma pack(1)

/*
 * Table structures must be byte-packed to match the SFI specification,
 * as they are provided by the BIOS.
 */
struct sfi_table_header {
	TCHAR	signature[SFI_SIGNATURE_SIZE];
	UINT32	length;
	UINT8	revision;
	UINT8	checksum;
	TCHAR	oem_id[SFI_OEM_ID_SIZE];
	TCHAR	oem_table_id[SFI_OEM_TABLE_ID_SIZE];
};

struct sfi_table_simple {
	struct sfi_table_header		header;
	PHYSICAL_ADDRESS		pentry[1];
};

/* comply with UEFI spec 2.1 */
struct sfi_mem_entry {
	UINT32	type;
	PHYSICAL_ADDRESS	phy_start;
	UINT64	vir_start;
	UINT64	pages;
	UINT64	attrib;
};

struct sfi_cpu_table_entry {
	UINT32	apicid;
};

struct sfi_cstate_table_entry {
	UINT32	hint;		/* MWAIT hint */
	UINT32	latency;	/* latency in ms */
};

struct sfi_apic_table_entry {
	PHYSICAL_ADDRESS	phy_addr;	/* phy base addr for APIC reg */
};

struct sfi_freq_table_entry {
	UINT32	freq;
	UINT32	latency;	/* transition latency in ms */
	UINT32	ctrl_val;	/* value to write to PERF_CTL */
};

struct sfi_wake_table_entry {
	PHYSICAL_ADDRESS	phy_addr;	/* pointer to where the wake vector locates */
};

struct sfi_timer_table_entry {
	PHYSICAL_ADDRESS	phy_addr;	/* phy base addr for the timer */
	UINT32	freq;		/* in HZ */
	UINT32	irq;
};

struct sfi_rtc_table_entry {
	PHYSICAL_ADDRESS	phy_addr;	/* phy base addr for the RTC */
	UINT32	irq;
};

struct sfi_spi_table_entry {
	UINT16	host_num;	/* attached to host 0, 1...*/
	UINT16	cs;		/* chip select */
	UINT16	irq_info;
	TCHAR	name[16];
	UINT8	dev_info[10];
};

struct sfi_i2c_table_entry {
	UINT16	host_num;
	UINT16	addr;		/* slave addr */
	UINT16	irq_info;
	TCHAR	name[16];
	UINT8	dev_info[10];
};

struct sfi_gpe_table_entry {
	UINT16	logical_id;	/* logical id */
	UINT16	phy_id;		/* physical GPE id */
};

#pragma pack()

typedef int (*sfi_table_handler) (struct sfi_table_header *table);

#endif	/*_LINUX_SFI_H*/
