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

#ifndef __SFI_TIMER_H__
#define __SFI_TIMER_H__

#define SFI_TIMER_LOAD_COUNT_ADDR	0
#define SFI_TIMER_CURRENT_VALUE_ADDR	4
#define SFI_TIMER_CONTROL_ADDR	8
#define SFI_TIMER_CLEAR_INTERRUPT_ADDR	12
#define SFI_TIMER_INTERRUPT_STATUS_ADDR	16

#define SFI_TIMER_CTRL_ENABLE 1
#define SFI_TIMER_CTRL_WRAP_LOAD_VAL 2
#define SFI_TIMER_CTRL_INT_DISABLE 4

#endif // __SFI_TIMER_H__
