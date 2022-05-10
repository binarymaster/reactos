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

#ifndef __SFI_TABLE_H__
#define __SFI_TABLE_H__

#include "sfi.h"
#include "Acpi.h"

struct sfi_table_simple *SfiFindSyst(VOID);
struct sfi_table_simple * SfiFindTable(struct sfi_table_simple * syst, UINT32 sig);
UINT32 SfiTableEntryCount(struct sfi_table_simple * table, UINT32 entry_size);
EFI_ACPI_DESCRIPTION_HEADER * AcpiFindTableInXSDT (XSDT_TABLE *Xsdt, UINT32 Signature);

#endif // __SFI_TABLE_H__
