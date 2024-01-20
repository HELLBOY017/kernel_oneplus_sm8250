/** @file
  Oplus Storage protocol as defined in the UEFI 2.0 specification.

  The Oplus StorageO protocol is used to abstract oplus storage drivers

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

 **/
#ifndef __OPLUS_STORAGE_H__
#define __OPLUS_STORAGE_H__
#include "BlockIo.h"
typedef struct _EFI_BLOCK_IO_PROTOCOL  EFI_BLOCK_IO_PROTOCOL;
typedef struct __EFI_OPLUS_STORAGE_PROTOCOL EFI_OPLUS_STORAGE_PROTOCOL;
#define EFI_OPLUS_STORAGE_PROTOCOL_GUID \
{ \
	0x62b1b2da, 0xd0df, 0x4531, {0x80, 0xa4, 0xab, 0xe7, 0xa3, 0xff, 0xdf, 0x78 } \
}

typedef
EFI_STATUS
(EFIAPI * EFI_OPLUS_FW_OPERATION)(
    IN EFI_OPLUS_STORAGE_PROTOCOL *This,
    IN EFI_BLOCK_IO_PROTOCOL *blockio,
    VOID *in,
    VOID *out
);

struct __EFI_OPLUS_STORAGE_PROTOCOL {
	EFI_OPLUS_FW_OPERATION  SdccFwOps;
};

extern EFI_GUID gEfiOplusStorageProtocolGuid;
#endif //__OPLUS_STORAGE_H__
