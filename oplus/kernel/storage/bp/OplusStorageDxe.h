/**
@file    OplusStorageDxe.h
@brief   Universal Flash Storage (UFS) DXE Header File

This file contains the definitions of the constants, data structures,
and interfaces for the OplusStorage driver in UEFI.

 Copyright (c) 2013 - 2018, 2021,2022 Qualcomm Technologies, Inc. All rights reserved.
**/

 /*=============================================================================
                               EDIT HISTORY


 when            who   what, where, why
 ----------      ---   -----------------------------------------------------------
2023-5-26   zhoumaowei Add for decoupling of ufs drivers
 =============================================================================*/

#ifndef _OPLUSSTORAGEDXE_H_
#define _OPLUSSTORAGEDXE_H_
#include <Protocol/ComponentName.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DiskIo.h>
#include <Protocol/PartitionInfo.h>
#include <Protocol/DiskIo2.h>
typedef struct _EFI_BLOCK_IO_PROTOCOL  EFI_BLOCK_IO_PROTOCOL;
#define OPLUS_UFS_DEV_SIGNATURE SIGNATURE_32 ('u', 'f', 's', ' ')   /**< -- UFS  */

typedef struct {
  UINT64                       Signature;

  EFI_HANDLE                   Handle;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
  EFI_BLOCK_IO_PROTOCOL        BlockIo;
  EFI_BLOCK_IO2_PROTOCOL       BlockIo2;
  EFI_BLOCK_IO_MEDIA           Media;
  EFI_BLOCK_IO_MEDIA           Media2;//For BlockIO2
  EFI_PARTITION_INFO_PROTOCOL  PartitionInfo;

  EFI_DISK_IO_PROTOCOL         *DiskIo;
  EFI_DISK_IO2_PROTOCOL        *DiskIo2;
  EFI_BLOCK_IO_PROTOCOL        *ParentBlockIo;
  EFI_BLOCK_IO2_PROTOCOL       *ParentBlockIo2;
  UINT64                       Start;
  UINT64                       End;
  UINT32                       BlockSize;
  BOOLEAN                      InStop;

  EFI_GUID                     TypeGuid;
#if defined (QCOM_EDK2_PATCH)
  EFI_PARTITION_ENTRY          PartEntry;

  EFI_ERASE_BLOCK_PROTOCOL    *ParentEraseBlock;
  EFI_ERASE_BLOCK_PROTOCOL    BlockErase;
#endif
} OPLUS_PARTITION_PRIVATE_DATA;

typedef struct {
  UINT32                        Signature;
  EFI_HANDLE                    ClientHandle;  /**< -- Client Handle */
  EFI_HANDLE                    DeviceHandle;  /**< -- Device Handle */
  EFI_BLOCK_IO_PROTOCOL         BlkIo;         /**< -- Block I/O */
  EFI_BLOCK_IO_CRYPTO_PROTOCOL  BlkIoCrypto;   /**< -- Block I/O Crypto */
  EFI_BLOCK_IO2_PROTOCOL        BlkIo2;        /**< -- Block I/O 2 */
  EFI_SDCC_RPMB_PROTOCOL        RPMB;          /**< -- RPMB protocol */
  EFI_MEM_CARDINFO_PROTOCOL     CardInfo;      /**< -- Memory card into */
  EFI_ERASE_BLOCK_PROTOCOL      EraseBlk;      /**< -- Erase block */
  EFI_STORAGE_WP_PROTOCOL       WriteProtect;  /**< -- Write Protect */
  EFI_STORAGE_DEV_MGMT_PROTOCOL DeviceMgmt;    /**< -- Device Management */
} OPLUS_UFS_DEV;

EFI_STATUS EFIAPI UFSFirmwareOps(
  IN EFI_OPLUS_STORAGE_PROTOCOL *This,
  IN EFI_BLOCK_IO_PROTOCOL *blockio,
  VOID *in1,
  VOID *in2
);

#define OPLUS_UFS_DEV_FROM_BLOCKIO(a)   CR (a, OPLUS_UFS_DEV, BlkIo, OPLUS_UFS_DEV_SIGNATURE)

#endif /* _UFS_H_ */

