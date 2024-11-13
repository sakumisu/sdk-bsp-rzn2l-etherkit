/*
* This source file is part of the EtherCAT Slave Stack Code licensed by Beckhoff Automation GmbH & Co KG, 33415 Verl, Germany.
* The corresponding license agreement applies. This hint shall not be removed.
* https://www.beckhoff.com/media/downloads/slave-stack-code/ethercat_ssc_license.pdf
*/

/**
\addtogroup ESM EtherCAT State Machine
@{
*/

/**
\file bootmode.c
\author EthercatSSC@beckhoff.com
\brief Implementation

\version 5.12

<br>Changes to version V4.20:<br>
V5.12 BOOT2: call BL_Start() from Init to Boot<br>
<br>Changes to version - :<br>
V4.20: File created
*/

/*--------------------------------------------------------------------------------------
------
------    Includes
------
--------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "ecatfoe.h"
#include "ecat_def.h"
#include "ecatslv.h"
#include "mailbox.h"
#include "ecatappl.h"
#include "foeappl.h"
#include "sampleappl.h"
#include "renesashw.h"

#define    _BOOTMODE_ 1
#include "bootmode.h"
#undef _BOOTMODE_

#ifdef __ICCARM__
#include "intrinsics.h"												// intrinsic functions header
#endif // __iccarm__

/* Access to Firmware updata API */
#include "hal_data.h"
#include "r_fw_up_rz_if.h"

bool BL_IsSectorErased(uint16_t offset_sector_number);
void BL_EraseSector(uint16_t offset_sector_number);
extern void handle_error(fsp_err_t err);

/*--------------------------------------------------------------------------------------
------
------    local Types and Defines
------
--------------------------------------------------------------------------------------*/
#define BL_WRITE_BUFFER_SIZE		FW_UP_PAGE_SIZE		// Byte
#define BL_DATA_STATUS_IDLE			(0)					// Idle
#define BL_DATA_STATUS_ERASE_START	(1)					// Flash Erase
#define BL_DATA_STATUS_ERASE		(2)					// Flash Erase
#define BL_DATA_STATUS_WRITE		(3)					// Data write to Flash
#define BL_DATA_STATUS_ERASE_LDRPRM	(4)					// LdrPrm Erase
#define BL_DATA_STATUS_WRITE_LDRPRM	(5)					// LdrPrm Update
#define BL_DATA_STATUS_SII_UPDATE	(6)					// SII Update

/*-----------------------------------------------------------------------------------------
------
------    local variables and constants
------
-----------------------------------------------------------------------------------------*/
static UINT8  FlashWriteBuffer[BL_WRITE_BUFFER_SIZE];
static UINT8  EraseSectorNumber;
static UINT16 EraseSectorFlag;
static UINT8  DataStatus = BL_DATA_STATUS_IDLE;
static BOOL   bReBoot;

/*-----------------------------------------------------------------------------------------
------
------    Module internal function declarations
------
-----------------------------------------------------------------------------------------*/
static void BL_SectorIsErased(uint16_t offset_sector_number);
/*-----------------------------------------------------------------------------------------
------
------    Module internal variable definitions
------
-----------------------------------------------------------------------------------------*/
BSP_DONT_REMOVE const uint32_t g_identify[4] BSP_PLACE_IN_SECTION(".identify") = {(VENDOR_ID), (PRODUCT_CODE), (REVISION_NUMBER), (SERIAL_NUMBER)};

/******************************************************************************
* Function Name: BL_Start
* Description  : Boot Loader start function
* Arguments    : State  -- Current State
* Return Value : None
******************************************************************************/
void BL_Start( UINT8 State)
{
	FSP_PARAMETER_NOT_USED(State);

} /* BL_Start() */

/******************************************************************************
* Function Name: BL_Stop
* Description  : Called in the state transition from BOOT to Init
* Arguments    : None
* Return Value : None
******************************************************************************/
void BL_Stop(void)
{
}

/******************************************************************************
* Function Name: BL_StartDownload
* Description  : File download start function
* Arguments    : password -- download password
* Return Value : None
******************************************************************************/
void BL_StartDownload(UINT32 password)
{
	fw_up_return_t status;

	status = fw_up_open();					// Initialize firmware update function
	handle_error((fsp_err_t)status);

	EraseSectorNumber = 0;
	EraseSectorFlag = 0;

#if (BANK == 0)
	/* Copy the loader param of BANK0 to FlashWriteBuffer. */
	FWUPMEMCPY(&FlashWriteBuffer[0], (UINT8 *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET), FW_UP_PAGE_SIZE);
#elif (BANK == 1)
    /* Copy the loader param of BANK1 to FlashWriteBuffer. */
    FWUPMEMCPY(&FlashWriteBuffer[0], (UINT8 *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET), FW_UP_PAGE_SIZE);
#endif

	DataStatus = BL_DATA_STATUS_ERASE_LDRPRM;

	FSP_PARAMETER_NOT_USED(password);
} /* BL_StartDownload() */

/******************************************************************************
* Function Name: BL_Data
* Description  : File data receive function
* Arguments    : pData -- Data pointer
*              : Size  -- Data Length
* Return Value : FoE error code
******************************************************************************/
EEPBUFFER     Buffer;

UINT16 BL_Data(UINT16 *pData,UINT16 Size)
{
	UINT16 ErrorCode = 0;
	UINT8  LastData;
	UINT32 i;
	UINT32 ldr_addr;

	fw_up_return_t status;
    spi_flash_status_t status_erase;

#if (BANK == 0)
    volatile UINT32 *pIdentify = (UINT32 *)(FW_UP_BANK1_ADDR + FW_UP_APPLI_ID_OFFSET - FW_UP_MIRROR_OFFSET);    // Identify section address
#elif (BANK == 1)
    volatile UINT32 *pIdentify = (UINT32 *)(FW_UP_BANK0_ADDR + FW_UP_APPLI_ID_OFFSET - FW_UP_MIRROR_OFFSET);    // Identify section address
#endif

	switch(DataStatus)
	{
	case BL_DATA_STATUS_ERASE_START:
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)														// In write progress ?
		{
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-1S-1S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif

#if (BANK == 0)
			/* Erase 1sector(64KB) of BANK1 */
			R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK1_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),
							  (uint32_t)FW_UP_SECTOR_SIZE);
#elif (BANK == 1)
            /* Erase 1sector(64KB) of BANK0 */
            R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK0_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),
                              (uint32_t)FW_UP_SECTOR_SIZE);
#endif

			DataStatus = BL_DATA_STATUS_ERASE;
		}
		ErrorCode = FOE_MAXBUSY_ZERO;				// return "BUSY"
		break;

	case BL_DATA_STATUS_ERASE:
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)														// Is flash erasing end ?
		{
			BL_SectorIsErased(EraseSectorNumber);   // Update EraseSectorFlag.
			status = analyze_and_write_data((const uint8_t *)pData, (uint32_t)Size);					// yes. data copy to write buffer
			handle_error((fsp_err_t)status);
			DataStatus = BL_DATA_STATUS_WRITE;
		}
		else
		{
			ErrorCode = FOE_MAXBUSY_ZERO;									// no. return "BUSY"
		}
		break;

	case BL_DATA_STATUS_WRITE:
		LastData = (Size != (u16ReceiveMbxSize - MBX_HEADER_SIZE - FOE_HEADER_SIZE));
		status = analyze_and_write_data((const uint8_t *)pData, (uint32_t)Size);	// data copy to write buffer and write to flash
		handle_error((fsp_err_t)status);
		if(LastData == TRUE)											// last receive data ?
		{
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-4S-4S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
#endif
#if (BANK == 0)
			/* Copy the loader param of BANK1 to FlashWriteBuffer. */
			FWUPMEMCPY(&FlashWriteBuffer[0], (UINT8 *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET), FW_UP_PAGE_SIZE);
#elif (BANK == 1)
            /* Copy the loader param of BANK0 to FlashWriteBuffer. */
            FWUPMEMCPY(&FlashWriteBuffer[0], (UINT8 *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET), FW_UP_PAGE_SIZE);
#endif

			DataStatus = BL_DATA_STATUS_ERASE_LDRPRM;					// yes. next -> erase ldrprm.
			ErrorCode = FOE_MAXBUSY_ZERO;								// return "BUSY"
		}
		break;

	case BL_DATA_STATUS_ERASE_LDRPRM:
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)							// Is flash erasing end ?
		{
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-1S-1S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif
			/* Erase flash memory from 0x6000_0000 to 0x6000_1000 */
			R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)FW_UP_LDRPRM_ADDR, FW_UP_SECTOR_SIZE_4K);
			DataStatus = BL_DATA_STATUS_WRITE_LDRPRM;							// next -> update ldrprm.
		}
		ErrorCode = FOE_MAXBUSY_ZERO;										// return "BUSY"
		break;

	case BL_DATA_STATUS_WRITE_LDRPRM:
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)							// Is flash erasing end ?
		{
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-1S-1S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif
			/* Write FlashWriteBuffer[0 - 255] to loder param area (from 0x6000_0000 to 0x6000_1000)*/
			for ( i = 0; i < FW_UP_PAGE_SIZE / FW_UP_WRITE_ATONCE_SIZE ; i++)
			{
				R_XSPI_QSPI_Write(&g_qspi0_ctrl, &FlashWriteBuffer[0 + (i * FW_UP_WRITE_ATONCE_SIZE)], 
								  (uint8_t *)(FW_UP_LDRPRM_ADDR + (i * FW_UP_WRITE_ATONCE_SIZE) - FW_UP_MIRROR_OFFSET),
								  (uint32_t)FW_UP_WRITE_ATONCE_SIZE);
				do
				{
					(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
				} while (true == status_erase.write_in_progress);
			}
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-4S-4S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
#endif
			ldr_addr = *(UINT32 *)(&FlashWriteBuffer[FW_UP_LDRPRM_LDR_ADDR - FW_UP_LDRPRM_ADDR]);

#if (BANK == 0)
			if((ldr_addr & FW_UP_BANK0_ADDR) == FW_UP_BANK0_ADDR)			// recover BANK0 LDRPRM ?
#elif (BANK == 1)
            if((ldr_addr & FW_UP_BANK1_ADDR) == FW_UP_BANK1_ADDR)           // recover BANK1 LDRPRM ?
#endif
			{
				DataStatus = BL_DATA_STATUS_ERASE_START;					// yes. next -> erase BANK
			}
			else
			{
				DataStatus = BL_DATA_STATUS_SII_UPDATE;						// No. write BANK LDRPRM next -> update SII.
			}
			ErrorCode = FOE_MAXBUSY_ZERO;									// return "BUSY"
		}
		else
		{
			ErrorCode = FOE_MAXBUSY_ZERO;									// no. return "BUSY"
		}
		break;

	case BL_DATA_STATUS_SII_UPDATE:
#if (FW_UP_FLASH_TYPE == 1)
		/* For following sequnces, set protocol 1S-4S-4S */
		R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
#endif
		//--------------------------------------------------
		// SII update, update firmware Revision Number
		//--------------------------------------------------
		for ( i = 0; i < 4 ; i++)											// get new firmware identify 
		{
			Buffer.dword[i] = *pIdentify++;
		}
		ESC_EepromAccess(SII_EEP_IDENTIFY_OFFSET + SII_EEP_REVESIONNO, 2, &Buffer.word[SII_EEP_REVESIONNO], ESC_WR);
		fw_up_close();
		BL_SetRebootFlag(TRUE);												// yes. reboot is available.
		break;
	case BL_DATA_STATUS_IDLE:
	default:
		break;
	}

	return(ErrorCode);
} /* BL_Data() */

/******************************************************************************
* Function Name: BL_SetRebootFlag
* Description  : Reboot flag set function
* Arguments    : Flag -- TRUE/FALSE
* Return Value : None
******************************************************************************/
void BL_SetRebootFlag(BOOL Flag)
{
	bReBoot = Flag;
}

/******************************************************************************
* Function Name: BL_CheckRebootFlag
* Description  : Check reboot flag function
* Arguments    : None
* Return Value : Flag
******************************************************************************/
BOOL BL_CheckRebootFlag(void)
{
	return(bReBoot);
}


/******************************************************************************
* Function Name: BL_Reboot
* Description  : Reboot boot loader function
* Arguments    : None
* Return Value : None
******************************************************************************/
void BL_Reboot(void)
{
#if (FW_UP_DUAL_BANK_MODE)	// Daul mode
	uint32_t reset_vector_value;
    fw_up_return_t flash_status;
    bool reset_vector_ok = false;

	/* get reset vector of the BANK1 */
	reset_vector_value = *(uint32_t*)(Bank[FW_UP_BANK1].high_addr - (sizeof(uint32_t*) - 1u));
	/* The reset vector is checked to see if the address is within the range of BANK0 */
	if(reset_vector_value != FW_UP_BLANK_VALUE)
	{
		reset_vector_ok = fw_up_check_addr_value(reset_vector_value, FW_UP_BANK0);
	}

    if (reset_vector_ok)
    {
        flash_status = bank_toggle();
        if (FW_UP_SUCCESS == flash_status)
        {
            /* soft reset */
            fw_up_soft_reset();
        }
        else
        {
            // Do Nothing!
        }
    }
    else
    {
        // Do Nothing!
    }

#else  // if (FW_UP_DUAL_BANK_MODE)

	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
	R_BSP_SystemReset();	// System Software Reset
	while(1)
	{
		/* Do nothing */
	};
#endif  // if (FW_UP_DUAL_BANK_MODE)

} /* BL_Reboot() */

/******************************************************************************
* Function Name: BL_IsSectorErased
* Description  : Check if sector is erased 
* Arguments    : Sector number
* Return Value : None
******************************************************************************/
bool BL_IsSectorErased(uint16_t offset_sector_number)
{
	if((1U << offset_sector_number) & EraseSectorFlag)
	{
		return(true);
	}
	else
	{
		return(false);
	}
} /* BL_IsSectorErased */

/******************************************************************************
* Function Name: BL_EraseSector
* Description  : Erase the sector
* Arguments    : Sector number
* Return Value : None
******************************************************************************/
void BL_EraseSector(uint16_t offset_sector_number)
{
	volatile uint16_t dummy16;
    spi_flash_status_t status_erase;

	EraseSectorNumber = (uint8_t)offset_sector_number;
#if (FW_UP_FLASH_TYPE == 1)
    /* For following sequnces, set protocol 1S-1S-1S */
    R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif
#if (BANK == 0)
    /* Erase 1sector(64KB) of BANK1 */
    R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK1_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),
					  (uint32_t)FW_UP_SECTOR_SIZE);
#elif (BANK == 1)
    /* Erase 1sector(64KB) of BANK0 */
    R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK0_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),
                      (uint32_t)FW_UP_SECTOR_SIZE);
#endif

	do
	{
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		HW_EscReadWord(dummy16, ESC_EEPROM_CONFIG_OFFSET);			// Countermeasure against PD watchdog timeout
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);

	} while (true == status_erase.write_in_progress);

	BL_SectorIsErased(EraseSectorNumber);

	FSP_PARAMETER_NOT_USED(dummy16);
} /* BL_EraseSector */

/******************************************************************************
* Function Name: BL_SectorIsErased
* Description  : Set erased sector flag
* Arguments    : Sector number
* Return Value : None
******************************************************************************/
static void BL_SectorIsErased(uint16_t offset_sector_number)
{
	EraseSectorFlag |= (uint16_t)(1U << offset_sector_number);
} /* BL_SectorIsErased */


