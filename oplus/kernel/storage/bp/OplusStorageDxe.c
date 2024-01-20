/**
* @file    OplusStorageDxe.c
* @brief   Universal Flash Storage (UFS) Dxe Driver
*
*  Copyright (c) 2013 - 2022 Qualcomm Technologies, Inc. All rights reserved.
*/


/*=============================================================================
                               EDIT HISTORY


 when            who   what, where, why
 ----------      ---   -----------------------------------------------------------
 2023-5-26	 zhoumaowei Add for decoupling of ufs drivers
 =============================================================================*/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Library/ArmLib.h>
#include <Library/SerialPortShLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/StorSecApp.h>
#include <Library/UefiCfgLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/BlockIoCrypto.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EFICardInfo.h>
#include <Protocol/EFIClock.h>
#include <Protocol/EFIHWIO.h>
#include <Protocol/EFIStorageDeviceMgmt.h>
#include <Protocol/OplusStorage.h>

#include <Library/GPTListener.h>
#include <Library/RpmbListener.h>
#include <Library/RpmbLib.h>
#include <Protocol/EFIRpmb.h>
#include <Protocol/EFIEraseBlock.h>
#include <Protocol/EFIStorageWriteProtect.h>
#include <Protocol/EFIHALIOMMUProtocol.h>

#include "OplusStorageDxe.h"
#include "Library/KernelLib.h"
#include <api/storage/ufs_api.h>
#include <BootConfig.h>
#include <api/storage/ufs_upgrade.h>

EFI_OPLUS_STORAGE_PROTOCOL gOplusStorage = {
	UFSFirmwareOps
};

EFI_STATUS EFIAPI UFSFirmwareOps(
    IN EFI_OPLUS_STORAGE_PROTOCOL *This,
    IN EFI_BLOCK_IO_PROTOCOL *blockio,
    VOID *in1,
    VOID *in2
)
{
	int ret = 0;
	EFI_STATUS Status = EFI_NOT_STARTED;
	EFI_PHYSICAL_ADDRESS fw_buff_addr = 0;
	unsigned long long fw_offset = 0;
	struct ufs_handle *hUFS = NULL;
	OPLUS_PARTITION_PRIVATE_DATA *PartitionData = NULL;
        OPLUS_UFS_DEV *UfsDevice = NULL;
        uint64_t logo_partition_start_lba = 0;
        
	if (NULL == This || NULL == blockio)
	{
		return EFI_INVALID_PARAMETER;
	}

	DEBUG ((EFI_D_ERROR, "BOOT: You are calling oplus UFSFirmwareOps function now.0x%x\n", blockio));
    PartitionData = BASE_CR(blockio, OPLUS_PARTITION_PRIVATE_DATA, BlockIo);
	if (NULL == PartitionData || NULL == PartitionData->ParentBlockIo)
	{
		return EFI_INVALID_PARAMETER;
	}

    logo_partition_start_lba = PartitionData->Start/PartitionData->BlockSize;
	UfsDevice = BASE_CR(PartitionData->ParentBlockIo, OPLUS_UFS_DEV, BlkIo);
	if (!UfsDevice || !(hUFS = (struct ufs_handle *)UfsDevice->DeviceHandle))
	{
		DEBUG((EFI_D_ERROR, "UEFI UFS lib is not initilized.\n"));
		return EFI_NOT_STARTED;
	}
	Status = gBS->AllocatePages(AllocateAnyPages, EfiBootServicesCode, 
							EFI_SIZE_TO_PAGES(FIRMWARE_LENGTH_MAX), &fw_buff_addr);
	if (EFI_ERROR(Status))
	{
		DEBUG((EFI_D_ERROR, "UEFI AllocatePages fail.\n"));
		return EFI_NOT_STARTED;
	}

	Status = EFI_NOT_STARTED;
	in2 = (VOID*)&logo_partition_start_lba;
	fw_offset = UNIFIED_RESERVE_UFS_FW_OFFSET;
	if (need_upgrade_ufs_fimware(hUFS, (in1 ? (*(unsigned long long*)in1) : INVALID_LBA),
						(in2 ? (*(unsigned long long*)in2) : 0),
						fw_offset, fw_buff_addr) ||
						need_upgrade_ufs_fimware(hUFS, (in1 ? (*(unsigned long long*)in1) : INVALID_LBA),
						(in2 ? (*(unsigned long long*)in2) : 0),
						UNIFIED_RESERVE_UFS_FW_OFFSET2,
						fw_buff_addr))
	{
		ret = ufs_firmware_update(hUFS, fw_buff_addr);
		if (ret == 0)
		{
			DEBUG((EFI_D_ERROR, "ufs FW upgrade success.\n"));
			Status = EFI_SUCCESS;
		}
	}
	gBS->FreePages(fw_buff_addr, EFI_SIZE_TO_PAGES(FIRMWARE_LENGTH_MAX));
	return Status;
}

EFI_STATUS
EFIAPI
OplusStorageEntry (
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
)
{
	EFI_HANDLE handle = NULL;
	EFI_STATUS status;
        DEBUG((EFI_D_ERROR, "start init oplus storage entry.\n"));
	status = gBS->InstallMultipleProtocolInterfaces(&handle,
						&gEfiOplusStorageProtocolGuid,
						(void **)&gOplusStorage,
						NULL, 
						NULL,
						NULL);

	return status;
}


