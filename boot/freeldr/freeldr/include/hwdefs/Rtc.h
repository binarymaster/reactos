/** @file
  Header file for real time clock driver.

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _RTC_H_
#define _RTC_H_

#define PCAT_RTC_ADDRESS_REGISTER 0x70
#define PCAT_RTC_DATA_REGISTER    0x71

//
// Dallas DS12C887 Real Time Clock
//
#define RTC_ADDRESS_SECONDS           0   // R/W  Range 0..59
#define RTC_ADDRESS_SECONDS_ALARM     1   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES           2   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES_ALARM     3   // R/W  Range 0..59
#define RTC_ADDRESS_HOURS             4   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_HOURS_ALARM       5   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_DAY_OF_THE_WEEK   6   // R/W  Range 1..7
#define RTC_ADDRESS_DAY_OF_THE_MONTH  7   // R/W  Range 1..31
#define RTC_ADDRESS_MONTH             8   // R/W  Range 1..12
#define RTC_ADDRESS_YEAR              9   // R/W  Range 0..99
#define RTC_ADDRESS_REGISTER_A        10  // R/W[0..6]  R0[7]
#define RTC_ADDRESS_REGISTER_B        11  // R/W
#define RTC_ADDRESS_REGISTER_C        12  // RO
#define RTC_ADDRESS_REGISTER_D        13  // RO
//
// Date and time initial values.
// They are used if the RTC values are invalid during driver initialization
//
#define RTC_INIT_SECOND 0
#define RTC_INIT_MINUTE 0
#define RTC_INIT_HOUR   0
#define RTC_INIT_DAY    1
#define RTC_INIT_MONTH  1

//
// Register initial values
//
#define RTC_INIT_REGISTER_A 0x26
#define RTC_INIT_REGISTER_B 0x02
#define RTC_INIT_REGISTER_D 0x0

#pragma pack(1)
//
// Register A
//
typedef struct {
  UINT8 Rs : 4;   // Rate Selection Bits
  UINT8 Dv : 3;   // Divisor
  UINT8 Uip : 1;  // Update in progress
} RTC_REGISTER_A_BITS;

typedef union {
  RTC_REGISTER_A_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_A;

//
// Register B
//
typedef struct {
  UINT8 Dse : 1;  // 0 - Daylight saving disabled  1 - Daylight savings enabled
  UINT8 Mil : 1;  // 0 - 12 hour mode              1 - 24 hour mode
  UINT8 Dm : 1;   // 0 - BCD Format                1 - Binary Format
  UINT8 Sqwe : 1; // 0 - Disable SQWE output       1 - Enable SQWE output
  UINT8 Uie : 1;  // 0 - Update INT disabled       1 - Update INT enabled
  UINT8 Aie : 1;  // 0 - Alarm INT disabled        1 - Alarm INT Enabled
  UINT8 Pie : 1;  // 0 - Periodic INT disabled     1 - Periodic INT Enabled
  UINT8 Set : 1;  // 0 - Normal operation.         1 - Updates inhibited
} RTC_REGISTER_B_BITS;

typedef union {
  RTC_REGISTER_B_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_B;

//
// Register C
//
typedef struct {
  UINT8 Reserved : 4; // Read as zero.  Can not be written.
  UINT8 Uf : 1;       // Update End Interrupt Flag
  UINT8 Af : 1;       // Alarm Interrupt Flag
  UINT8 Pf : 1;       // Periodic Interrupt Flag
  UINT8 Irqf : 1;     // Iterrupt Request Flag = PF & PIE | AF & AIE | UF & UIE
} RTC_REGISTER_C_BITS;

typedef union {
  RTC_REGISTER_C_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_C;

//
// Register D
//
typedef struct {
  UINT8 Reserved : 7; // Read as zero.  Can not be written.
  UINT8 Vrt : 1;      // Valid RAM and Time
} RTC_REGISTER_D_BITS;

typedef union {
  RTC_REGISTER_D_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_D;

#pragma pack()

#endif
