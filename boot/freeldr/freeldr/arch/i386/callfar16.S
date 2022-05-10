/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

.code32

EXTERN SwitchToReal:PROC
EXTERN ContinueAddress:DWORD

CallFar16_regsin:
    .long 0
CallFar16_regsout:
    .long 0

/*
 * int CallFar16(int segm, int offs, REGS* in, REGS* out);
 */
PUBLIC _CallFar16
_CallFar16:

    /* Get the function parameters */
    mov eax, dword ptr [esp + 4]
    mov dword ptr ds:[BSS_CallFarSegment], eax
    mov eax, dword ptr [esp + 8]
    mov dword ptr ds:[BSS_CallFarOffset], eax
    mov eax, dword ptr [esp + 12]
    mov dword ptr [CallFar16_regsin], eax
    mov eax, dword ptr [esp + 16]
    mov dword ptr [CallFar16_regsout], eax

    /* Save all registers + segment registers */
    push ds
    push es
    push fs
    push gs
    pusha

    /* Copy input registers */
    mov esi, dword ptr [CallFar16_regsin]
    mov edi, BSS_RegisterSet
    mov ecx, REGS_SIZE / 4
    rep movsd

    /* Set the function ID */
    mov bx, FNID_CallFar16

    /* Set continue address and switch to real mode */
    mov dword ptr [ContinueAddress], offset CallFar16_return
    jmp SwitchToReal

CallFar16_return:

    /* Copy output registers */
    mov esi, BSS_RegisterSet
    mov edi, dword ptr [CallFar16_regsout]
    mov ecx, REGS_SIZE / 4
    rep movsd

    popa
    pop gs
    pop fs
    pop es
    pop ds
    ret

END
