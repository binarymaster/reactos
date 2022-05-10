/*
ReactOS EFI base types defines
*/

#ifndef _EFI_DEF_H
#define _EFI_DEF_H

#include <freeldr.h>

#define EFI_STATUS NTSTATUS
#define EFIAPI
typedef VOID *EFI_EVENT;
typedef VOID *EFI_HANDLE;
typedef unsigned int UINTN;
typedef int INTN;
#define EFI_PHYSICAL_ADDRESS UINT64
#define CHAR8 UINT8
#define CHAR16 UINT16
#define EFI_LBA UINT64

///
/// Maximum values for common UEFI Data Types
///
#define MAX_INT8    ((INT8)0x7F)
#define MAX_UINT8   ((UINT8)0xFF)
#define MAX_INT16   ((INT16)0x7FFF)
#define MAX_UINT16  ((UINT16)0xFFFF)
#define MAX_INT32   ((INT32)0x7FFFFFFF)
#define MAX_UINT32  ((UINT32)0xFFFFFFFF)
#define MAX_INT64   ((INT64)0x7FFFFFFFFFFFFFFFULL)
#define MAX_UINT64  ((UINT64)0xFFFFFFFFFFFFFFFFULL)

#define  BIT0     0x00000001
#define  BIT1     0x00000002
#define  BIT2     0x00000004
#define  BIT3     0x00000008
#define  BIT4     0x00000010
#define  BIT5     0x00000020
#define  BIT6     0x00000040
#define  BIT7     0x00000080
#define  BIT8     0x00000100
#define  BIT9     0x00000200
#define  BIT10    0x00000400
#define  BIT11    0x00000800
#define  BIT12    0x00001000
#define  BIT13    0x00002000
#define  BIT14    0x00004000
#define  BIT15    0x00008000
#define  BIT16    0x00010000
#define  BIT17    0x00020000
#define  BIT18    0x00040000
#define  BIT19    0x00080000
#define  BIT20    0x00100000
#define  BIT21    0x00200000
#define  BIT22    0x00400000
#define  BIT23    0x00800000
#define  BIT24    0x01000000
#define  BIT25    0x02000000
#define  BIT26    0x04000000
#define  BIT27    0x08000000
#define  BIT28    0x10000000
#define  BIT29    0x20000000
#define  BIT30    0x40000000
#define  BIT31    0x80000000
#define  BIT32    0x0000000100000000ULL
#define  BIT33    0x0000000200000000ULL
#define  BIT34    0x0000000400000000ULL
#define  BIT35    0x0000000800000000ULL
#define  BIT36    0x0000001000000000ULL
#define  BIT37    0x0000002000000000ULL
#define  BIT38    0x0000004000000000ULL
#define  BIT39    0x0000008000000000ULL
#define  BIT40    0x0000010000000000ULL
#define  BIT41    0x0000020000000000ULL
#define  BIT42    0x0000040000000000ULL
#define  BIT43    0x0000080000000000ULL
#define  BIT44    0x0000100000000000ULL
#define  BIT45    0x0000200000000000ULL
#define  BIT46    0x0000400000000000ULL
#define  BIT47    0x0000800000000000ULL
#define  BIT48    0x0001000000000000ULL
#define  BIT49    0x0002000000000000ULL
#define  BIT50    0x0004000000000000ULL
#define  BIT51    0x0008000000000000ULL
#define  BIT52    0x0010000000000000ULL
#define  BIT53    0x0020000000000000ULL
#define  BIT54    0x0040000000000000ULL
#define  BIT55    0x0080000000000000ULL
#define  BIT56    0x0100000000000000ULL
#define  BIT57    0x0200000000000000ULL
#define  BIT58    0x0400000000000000ULL
#define  BIT59    0x0800000000000000ULL
#define  BIT60    0x1000000000000000ULL
#define  BIT61    0x2000000000000000ULL
#define  BIT62    0x4000000000000000ULL
#define  BIT63    0x8000000000000000ULL

#define  SIZE_1KB    0x00000400
#define  SIZE_2KB    0x00000800
#define  SIZE_4KB    0x00001000
#define  SIZE_8KB    0x00002000
#define  SIZE_16KB   0x00004000
#define  SIZE_32KB   0x00008000
#define  SIZE_64KB   0x00010000
#define  SIZE_128KB  0x00020000
#define  SIZE_256KB  0x00040000
#define  SIZE_512KB  0x00080000
#define  SIZE_1MB    0x00100000
#define  SIZE_2MB    0x00200000
#define  SIZE_4MB    0x00400000
#define  SIZE_8MB    0x00800000
#define  SIZE_16MB   0x01000000
#define  SIZE_32MB   0x02000000
#define  SIZE_64MB   0x04000000
#define  SIZE_128MB  0x08000000
#define  SIZE_256MB  0x10000000
#define  SIZE_512MB  0x20000000
#define  SIZE_1GB    0x40000000
#define  SIZE_2GB    0x80000000
#define  SIZE_4GB    0x0000000100000000ULL
#define  SIZE_8GB    0x0000000200000000ULL
#define  SIZE_16GB   0x0000000400000000ULL
#define  SIZE_32GB   0x0000000800000000ULL
#define  SIZE_64GB   0x0000001000000000ULL
#define  SIZE_128GB  0x0000002000000000ULL
#define  SIZE_256GB  0x0000004000000000ULL
#define  SIZE_512GB  0x0000008000000000ULL
#define  SIZE_1TB    0x0000010000000000ULL
#define  SIZE_2TB    0x0000020000000000ULL
#define  SIZE_4TB    0x0000040000000000ULL
#define  SIZE_8TB    0x0000080000000000ULL
#define  SIZE_16TB   0x0000100000000000ULL
#define  SIZE_32TB   0x0000200000000000ULL
#define  SIZE_64TB   0x0000400000000000ULL
#define  SIZE_128TB  0x0000800000000000ULL
#define  SIZE_256TB  0x0001000000000000ULL
#define  SIZE_512TB  0x0002000000000000ULL
#define  SIZE_1PB    0x0004000000000000ULL
#define  SIZE_2PB    0x0008000000000000ULL
#define  SIZE_4PB    0x0010000000000000ULL
#define  SIZE_8PB    0x0020000000000000ULL
#define  SIZE_16PB   0x0040000000000000ULL
#define  SIZE_32PB   0x0080000000000000ULL
#define  SIZE_64PB   0x0100000000000000ULL
#define  SIZE_128PB  0x0200000000000000ULL
#define  SIZE_256PB  0x0400000000000000ULL
#define  SIZE_512PB  0x0800000000000000ULL
#define  SIZE_1EB    0x1000000000000000ULL
#define  SIZE_2EB    0x2000000000000000ULL
#define  SIZE_4EB    0x4000000000000000ULL
#define  SIZE_8EB    0x8000000000000000ULL

#define  BASE_1KB    0x00000400
#define  BASE_2KB    0x00000800
#define  BASE_4KB    0x00001000
#define  BASE_8KB    0x00002000
#define  BASE_16KB   0x00004000
#define  BASE_32KB   0x00008000
#define  BASE_64KB   0x00010000
#define  BASE_128KB  0x00020000
#define  BASE_256KB  0x00040000
#define  BASE_512KB  0x00080000
#define  BASE_1MB    0x00100000
#define  BASE_2MB    0x00200000
#define  BASE_4MB    0x00400000
#define  BASE_8MB    0x00800000
#define  BASE_16MB   0x01000000
#define  BASE_32MB   0x02000000
#define  BASE_64MB   0x04000000
#define  BASE_128MB  0x08000000
#define  BASE_256MB  0x10000000
#define  BASE_512MB  0x20000000
#define  BASE_1GB    0x40000000
#define  BASE_2GB    0x80000000
#define  BASE_4GB    0x0000000100000000ULL
#define  BASE_8GB    0x0000000200000000ULL
#define  BASE_16GB   0x0000000400000000ULL
#define  BASE_32GB   0x0000000800000000ULL
#define  BASE_64GB   0x0000001000000000ULL
#define  BASE_128GB  0x0000002000000000ULL
#define  BASE_256GB  0x0000004000000000ULL
#define  BASE_512GB  0x0000008000000000ULL
#define  BASE_1TB    0x0000010000000000ULL
#define  BASE_2TB    0x0000020000000000ULL
#define  BASE_4TB    0x0000040000000000ULL
#define  BASE_8TB    0x0000080000000000ULL
#define  BASE_16TB   0x0000100000000000ULL
#define  BASE_32TB   0x0000200000000000ULL
#define  BASE_64TB   0x0000400000000000ULL
#define  BASE_128TB  0x0000800000000000ULL
#define  BASE_256TB  0x0001000000000000ULL
#define  BASE_512TB  0x0002000000000000ULL
#define  BASE_1PB    0x0004000000000000ULL
#define  BASE_2PB    0x0008000000000000ULL
#define  BASE_4PB    0x0010000000000000ULL
#define  BASE_8PB    0x0020000000000000ULL
#define  BASE_16PB   0x0040000000000000ULL
#define  BASE_32PB   0x0080000000000000ULL
#define  BASE_64PB   0x0100000000000000ULL
#define  BASE_128PB  0x0200000000000000ULL
#define  BASE_256PB  0x0400000000000000ULL
#define  BASE_512PB  0x0800000000000000ULL
#define  BASE_1EB    0x1000000000000000ULL
#define  BASE_2EB    0x2000000000000000ULL
#define  BASE_4EB    0x4000000000000000ULL
#define  BASE_8EB    0x8000000000000000ULL

//
// Declare bits for PcdDebugPrintErrorLevel and the ErrorLevel parameter of DebugPrint()
//
#define DEBUG_INIT      0x00000001  // Initialization
#define DEBUG_WARN      0x00000002  // Warnings
#define DEBUG_LOAD      0x00000004  // Load events
#define DEBUG_FS        0x00000008  // EFI File system
#define DEBUG_POOL      0x00000010  // Alloc & Free's
#define DEBUG_PAGE      0x00000020  // Alloc & Free's
#define DEBUG_INFO      0x00000040  // Informational debug messages
#define DEBUG_DISPATCH  0x00000080  // PEI/DXE/SMM Dispatchers
#define DEBUG_VARIABLE  0x00000100  // Variable
#define DEBUG_BM        0x00000400  // Boot Manager
#define DEBUG_BLKIO     0x00001000  // BlkIo Driver
#define DEBUG_NET       0x00004000  // SNI Driver
#define DEBUG_UNDI      0x00010000  // UNDI Driver
#define DEBUG_LOADFILE  0x00020000  // UNDI Driver
#define DEBUG_EVENT     0x00080000  // Event messages
#define DEBUG_GCD       0x00100000  // Global Coherency Database changes
#define DEBUG_CACHE     0x00200000  // Memory range cachability changes
#define DEBUG_VERBOSE   0x00400000  // Detailed debug messages that may significantly impact boot performance
#define DEBUG_ERROR     0x80000000  // Error

#define _DEBUG_PRINT(PrintLevel, Msg, ...) printf (Msg, ##__VA_ARGS__)
#define DEBUG(x)
//#define DEBUG(x) (_DEBUG_PRINT x)

// Stall for x microsecond.
#define Stall(x) StallExecutionProcessor(x)

#define EFI_ERROR(x) (!NT_SUCCESS(x))
#define EFI_SUCCESS STATUS_SUCCESS
#define EFI_INVALID_PARAMETER STATUS_INVALID_PARAMETER
#define EFI_NOT_READY STATUS_DEVICE_NOT_READY
#define EFI_TIMEOUT STATUS_IO_TIMEOUT
#define EFI_NOT_FOUND STATUS_NOT_FOUND
#define EFI_UNSUPPORTED STATUS_NOT_SUPPORTED
#define EFI_MEDIA_CHANGED STATUS_MEDIA_CHANGED
#define EFI_DEVICE_ERROR STATUS_IO_DEVICE_ERROR
#define EFI_CRC_ERROR STATUS_CRC_ERROR
#define EFI_OUT_OF_RESOURCES STATUS_INSUFFICIENT_RESOURCES
#define EFI_BAD_BUFFER_SIZE STATUS_INVALID_BUFFER_SIZE
#define EFI_NO_MEDIA STATUS_NO_MEDIA_IN_DEVICE
#define EFI_WRITE_PROTECTED STATUS_MEDIA_WRITE_PROTECTED
#define EFI_ABORTED STATUS_REQUEST_ABORTED

#define CopyMem RtlCopyMemory

#define EFI_SIZE_TO_PAGES(Size)  (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))
#define LShiftU64(n, s) ((UINT64)(n) << (s))
#define RShiftU64(n, s) ((UINT64)(n) >> (s))

#define MultU64x32(a, b) ((UINT64)(a) * (b))
#define DivU64x32(a, b) ((UINT64)(a) / (b))

#define ZeroMem(Buffer,Size) (memset((Buffer), 0, (Size)))

#ifdef __GNUC__
#if __GNUC__ >= 4
#define OFFSET_OF(TYPE, Field) ((UINTN) __builtin_offsetof(TYPE, Field))
#endif
#endif

#ifndef OFFSET_OF
#define OFFSET_OF(TYPE, Field) ((UINTN) &(((TYPE *)0)->Field))
#endif

#define BASE_CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))

__inline
VOID *
AllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
	VOID * Pool;

	Pool = ExAllocatePoolWithTag(PagedPool, AllocationSize, 'NTAG');
	if (Pool) RtlZeroMemory(Pool, AllocationSize); 
	return Pool;
}

__inline
VOID *
AllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
	VOID * Pool;

	Pool = ExAllocatePoolWithTag(PagedPool, AllocationSize, 'NTAG');
	if (Pool) RtlCopyMemory(Pool, Buffer, AllocationSize);
	return Pool;
}

#ifndef FreePool
#define FreePool ExFreePool
#endif

__inline
INTN
HighBitSet32 (
  IN      UINT32                    Operand
  )
{
  INTN                              BitIndex;

  if (Operand == 0) {
    return - 1;
  }
  for (BitIndex = 31; (INT32)Operand > 0; BitIndex--, Operand <<= 1);
  return BitIndex;
}

#define INITIALIZE_LIST_HEAD_VARIABLE(ListHead)  {&(ListHead), &(ListHead)}

__inline
LIST_ENTRY *
EFIAPI
GetFirstNode (
  IN      CONST LIST_ENTRY          *List
  )
{
  return List->Flink;
}

__inline
LIST_ENTRY *
EFIAPI
GetNextNode (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  )
{
  return Node->Flink;
}

__inline
LIST_ENTRY *
EFIAPI
GetPreviousNode (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  )
{
  return Node->Blink;
}

__inline
BOOLEAN
EFIAPI
IsNull (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  )
{
  return (BOOLEAN)(Node == List);
}

#endif
