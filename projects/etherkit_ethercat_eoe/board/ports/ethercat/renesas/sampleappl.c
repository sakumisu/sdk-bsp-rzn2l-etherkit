/*
* This source file is part of the EtherCAT Slave Stack Code licensed by Beckhoff Automation GmbH & Co KG, 33415 Verl, Germany.
* The corresponding license agreement applies. This hint shall not be removed.
*/

/**
\addtogroup SampleAppl Sample Application
@{
*/

/**
\file Sampleappl.c
\author EthercatSSC@beckhoff.com
\brief Implementation

\version 5.12

<br>Changes to version V5.11:<br>
V5.12 ECAT1: update SM Parameter measurement (based on the system time), enhancement for input only devices and no mailbox support, use only 16Bit pointer in process data length caluclation<br>
V5.12 ECAT2: big endian changes<br>
V5.12 EOE1: move icmp sample to the sampleappl,add EoE application interface functions<br>
V5.12 EOE2: prevent static ethernet buffer to be freed<br>
V5.12 EOE3: fix memory leaks in sample ICMP application<br>
V5.12 FOE1: update new interface,move the FoE sample to sampleappl,add FoE application callback functions<br>
<br>Changes to version V5.10:<br>
V5.11 ECAT11: create application interface function pointer, add eeprom emulation interface functions<br>
V5.11 ECAT4: enhance SM/Sync monitoring for input/output only slaves<br>
<br>Changes to version V5.01:<br>
V5.10 ECAT10: Add missing include 'objdef.h'<br>
              Add process data size calculation to sampleappl<br>
V5.10 ECAT6: Add "USE_DEFAULT_MAIN" to enable or disable the main function<br>
V5.10 FC1100: Stop stack if hardware init failed<br>
<br>Changes to version V5.0:<br>
V5.01 APPL2: Update Sample Application Output mapping<br>
V5.0: file created
*/


/*-----------------------------------------------------------------------------------------
------
------    Includes
------
-----------------------------------------------------------------------------------------*/
#include "ecat_def.h"

#include "applInterface.h"


#define _SAMPLE_APPLICATION_
#include "sampleappl.h"
#undef _SAMPLE_APPLICATION_

#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZN2L_RSK) | defined(BOARD_RZT2L_RSK)
UINT16 APPL_GetDipSw(void);
void APPL_SetLed(UINT16 value);
#endif

#if (USE_SHARED_MEMORY == TRUE)
#include "sample_shared_memory.h"
#else
#include "sampleios.h"
#endif

/*--------------------------------------------------------------------------------------
------
------    local types and defines
------
--------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------
------
------    local variables and constants
------
-----------------------------------------------------------------------------------------*/

/*ECATCHANGE_START(V5.12) EOE1*/
/*Create broadcast Ethernet address*/
const    UINT8    BroadcastEthernetAddress[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
/*ECATCHANGE_END(V5.12) EOE1*/



/*-----------------------------------------------------------------------------------------
------
------    application specific functions
------
-----------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------
------
------    generic functions
------
-----------------------------------------------------------------------------------------*/

/////////////////////////////////////////////////////////////////////////////////////////
/**

\brief    This function calculates a checksum (only for an even number of bytes).
\brief Note that if you are going to checksum a checksummed packet that includes the checksum,
\brief you have to compliment the output.

*////////////////////////////////////////////////////////////////////////////////////////

UINT16 EOEAPPL_CalcCheckSum(UINT16 MBXMEM *pWord, UINT16 nLen)
{
    UINT32 crc;
    UINT32 CrcLo;
    UINT32 CrcHi;
    UINT16 RetCrc;

    crc = 0;
    while (nLen > 1)
    {
        crc += SWAPWORD(*pWord);
        pWord++;
        nLen -= 2;
    }
    if (nLen == 1)                          // if nLen odd
    {
        crc += *((UINT8*)pWord);
    }

    CrcLo = LOWORD(crc);
    CrcHi = HIWORD(crc);
    crc = CrcLo + CrcHi;

    CrcHi = HIWORD(crc);
    crc += CrcHi;
    if (crc == 0xFFFF)                     // remove the -0 ambiguity
    {
        crc = 0;
    }

    RetCrc = (UINT16)crc;
    RetCrc = ~RetCrc;
    return(RetCrc);
}


void EoeReceive(UINT16 *pFrame, UINT16 frameSize)
{
    /*ECATCHANGE_START(V5.12) ECAT2*/
    switch (SWAPWORD(((ETHERNET_FRAME *)pFrame)->FrameType))
        /*ECATCHANGE_END(V5.12) ECAT2*/
    {
    case ETHERNET_FRAME_TYPE_ARP1_SW:
    {
        ETHERNET_FRAME MBXMEM * pSendFrame = (ETHERNET_FRAME MBXMEM *) ALLOCMEM(frameSize);
        ARP_IP_HEADER MBXMEM    * pArpIp = (ARP_IP_HEADER MBXMEM    *) &pSendFrame[1];

        /*Copy Receive Frame to create ARP Reply*/
        MBXMEMCPY(pSendFrame, pFrame, frameSize);
        if ((MBXMEMCMP(BroadcastEthernetAddress, pSendFrame->Destination.b, 4) == 0)
            && (pArpIp->lengthHwAddr == ETHERNET_ADDRESS_LEN)
            && (pArpIp->lengthProtAddr == SIZEOF(UINT32))
            && (pArpIp->hwAddrSpace == SWAPWORD(ARP_HW_ADDR_SPACE_ETHERNET_SW))
            && (pArpIp->protAddrSpace == SWAPWORD(ETHERNET_FRAME_TYPE_IP_SW))
            && (pArpIp->opcode == SWAPWORD(ARP_OPCODE_REQUEST_SW))
            )
        {
            MBXMEMCPY(pSendFrame->Destination.b, pSendFrame->Source.b, 6);
            MBXMEMCPY(pSendFrame->Source.b, &aMacAdd[0], 6);

            MBXMEMCPY(pArpIp->macDest.b, pArpIp->macSource.b, 6);
            MBXMEMCPY(pArpIp->macSource.b, &aMacAdd[0], 6);
            MBXMEMCPY(pArpIp->ipDest, pArpIp->ipSource, 4);
            MBXMEMCPY(pArpIp->ipSource, aIpAdd, 4);

            pArpIp->opcode = SWAPWORD(ARP_OPCODE_REPLY_SW);

            EOE_SendFrameRequest((UINT16 MBXMEM *) pSendFrame, ARP_IP_HEADER_LEN + ETHERNET_FRAME_LEN);
        }
        /*ECATCHANGE_START(V5.12) EOE3*/
        else
        {
            if (pSendFrame != NULL)
            {
                FREEMEM(pSendFrame);
                pSendFrame = NULL;
            }
        }
        /*ECATCHANGE_END(V5.12) EOE3*/
    }
    break;
    case ETHERNET_FRAME_TYPE_IP_SW:
    {
        ETHERNET_IP_ICMP_MAX_FRAME MBXMEM * pIPHeader = (ETHERNET_IP_ICMP_MAX_FRAME MBXMEM *) ALLOCMEM(frameSize);

        /*Copy Receive Frame to create ICMP Reply*/
        MBXMEMCPY(pIPHeader, pFrame, frameSize);


        if ((pIPHeader->Ip.protocol == IP_PROTOCOL_ICMP)
            && (pIPHeader->IpData.Icmp.type == ICMP_TYPE_ECHO)
            && (MBXMEMCMP(pIPHeader->Ip.dest, aIpAdd, 4) == 0)
            )
        {
            // ping requested
            UINT16 length;
            UINT16 lo = 0;
            UINT16 hi = 0;
            UINT32 tmp;

            // length is in BigEndian format -> swap bytes
            lo = ((pIPHeader->Ip.length) & 0xff) << 8;
            hi = pIPHeader->Ip.length >> 8;
            length = hi + lo;
            // swap src and dest ip address
            MEMCPY(&tmp, pIPHeader->Ip.src, 4);
            MEMCPY(pIPHeader->Ip.src, pIPHeader->Ip.dest, 4);
            MEMCPY(pIPHeader->Ip.dest, &tmp, 4);

            // set ping reply command
            pIPHeader->IpData.Icmp.type = ICMP_TYPE_ECHO_REPLY;

            // swap src and dest mac address
            MBXMEMCPY(pIPHeader->Ether.Destination.b, pIPHeader->Ether.Source.b, 6);
            MBXMEMCPY(pIPHeader->Ether.Source.b, aMacAdd, 6);

            // calculate ip checksum
            pIPHeader->Ip.cksum = 0;
            pIPHeader->Ip.cksum = SWAPWORD(EOEAPPL_CalcCheckSum((UINT16 MBXMEM *) &pIPHeader->Ip, IP_HEADER_MINIMUM_LEN));
            // calculate icmp checksum
            pIPHeader->IpData.Icmp.checksum = 0;
            /* type cast because of warning was added */
            pIPHeader->IpData.Icmp.checksum = SWAPWORD(EOEAPPL_CalcCheckSum((UINT16 MBXMEM *) &pIPHeader->IpData.Icmp, (UINT16)(length - 20)));
            /* type cast because of warning was added */
            EOE_SendFrameRequest((UINT16 MBXMEM *) pIPHeader, (UINT16)(ETHERNET_FRAME_LEN + length));
        }
        else
        {
            //protocol not supported => free allocated buffer
            if (pIPHeader != NULL)
            {
                FREEMEM(pIPHeader);
                pIPHeader = NULL;
            }
        }
    }
    break;
    }
}
/*ECATCHANGE_END(V5.12) EOE1*/


/////////////////////////////////////////////////////////////////////////////////////////
/**
 \brief    The function is called when an error state was acknowledged by the master

*////////////////////////////////////////////////////////////////////////////////////////

void    APPL_AckErrorInd(UINT16 stateTrans)
{
    FSP_PARAMETER_NOT_USED(stateTrans);
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return    AL Status Code (see ecatslv.h ALSTATUSCODE_....)

 \brief    The function is called in the state transition from INIT to PREOP when
             all general settings were checked to start the mailbox handler. This function
             informs the application about the state transition, the application can refuse
             the state transition when returning an AL Status error code.
            The return code NOERROR_INWORK can be used, if the application cannot confirm
            the state transition immediately, in that case this function will be called cyclically
            until a value unequal NOERROR_INWORK is returned

*////////////////////////////////////////////////////////////////////////////////////////

UINT16 APPL_StartMailboxHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return     0, NOERROR_INWORK

 \brief    The function is called in the state transition from PREEOP to INIT
             to stop the mailbox handler. This functions informs the application
             about the state transition, the application cannot refuse
             the state transition.

*////////////////////////////////////////////////////////////////////////////////////////

UINT16 APPL_StopMailboxHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param    pIntMask    pointer to the AL Event Mask which will be written to the AL event Mask
                        register (0x204) when this function is succeeded. The event mask can be adapted
                        in this function
 \return    AL Status Code (see ecatslv.h ALSTATUSCODE_....)

 \brief    The function is called in the state transition from PREOP to SAFEOP when
           all general settings were checked to start the input handler. This function
           informs the application about the state transition, the application can refuse
           the state transition when returning an AL Status error code.
           The return code NOERROR_INWORK can be used, if the application cannot confirm
           the state transition immediately, in that case the application need to be complete 
           the transition by calling ECAT_StateChange.
*////////////////////////////////////////////////////////////////////////////////////////

UINT16 APPL_StartInputHandler(UINT16 *pIntMask)
{
    FSP_PARAMETER_NOT_USED(pIntMask);
    return ALSTATUSCODE_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return     0, NOERROR_INWORK

 \brief    The function is called in the state transition from SAFEOP to PREEOP
             to stop the input handler. This functions informs the application
             about the state transition, the application cannot refuse
             the state transition.

*////////////////////////////////////////////////////////////////////////////////////////

UINT16 APPL_StopInputHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return    AL Status Code (see ecatslv.h ALSTATUSCODE_....)

 \brief    The function is called in the state transition from SAFEOP to OP when
             all general settings were checked to start the output handler. This function
             informs the application about the state transition, the application can refuse
             the state transition when returning an AL Status error code.
           The return code NOERROR_INWORK can be used, if the application cannot confirm
           the state transition immediately, in that case the application need to be complete 
           the transition by calling ECAT_StateChange.
*////////////////////////////////////////////////////////////////////////////////////////

UINT16 APPL_StartOutputHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return     0, NOERROR_INWORK

 \brief    The function is called in the state transition from OP to SAFEOP
             to stop the output handler. This functions informs the application
             about the state transition, the application cannot refuse
             the state transition.

*////////////////////////////////////////////////////////////////////////////////////////

UINT16 APPL_StopOutputHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
\return     0(ALSTATUSCODE_NOERROR), NOERROR_INWORK
\param      pInputSize  pointer to save the input process data length
\param      pOutputSize  pointer to save the output process data length

\brief    This function calculates the process data sizes from the actual SM-PDO-Assign
            and PDO mapping
*////////////////////////////////////////////////////////////////////////////////////////
UINT16 APPL_GenerateMapping(UINT16 *pInputSize,UINT16 *pOutputSize)
{
    UINT16 result = ALSTATUSCODE_NOERROR;
    UINT16 PDOAssignEntryCnt = 0;
    OBJCONST TOBJECT OBJMEM * pPDO = NULL;
    UINT16 PDOSubindex0 = 0;
    UINT32 *pPDOEntry = NULL;
    UINT16 PDOEntryCnt = 0;
    UINT16 InputSize = 0;
    UINT16 OutputSize = 0;

    /*Scan object 0x1C12 RXPDO assign*/
    for(PDOAssignEntryCnt = 0; PDOAssignEntryCnt < sRxPDOassign.u16SubIndex0; PDOAssignEntryCnt++)
    {
        pPDO = OBJ_GetObjectHandle(sRxPDOassign.aEntries[PDOAssignEntryCnt]);
        if(pPDO != NULL)
        {
            PDOSubindex0 = *((UINT16 *)pPDO->pVarPtr);
            for(PDOEntryCnt = 0; PDOEntryCnt < PDOSubindex0; PDOEntryCnt++)
            {
/*ECATCHANGE_START(V5.12) ECAT1*/
                pPDOEntry = (UINT32 *)((UINT16 *)pPDO->pVarPtr + (OBJ_GetEntryOffset((PDOEntryCnt+1),pPDO)>>3)/2);    //goto PDO entry
/*ECATCHANGE_END(V5.12) ECAT1*/
                // we increment the expected output size depending on the mapped Entry
                OutputSize += (UINT16) ((*pPDOEntry) & 0xFF);
            }
        }
        else
        {
            /*assigned PDO was not found in object dictionary. return invalid mapping*/
            OutputSize = 0;
            result = ALSTATUSCODE_INVALIDOUTPUTMAPPING;
            break;
        }
    }

    OutputSize = (OutputSize + 7) >> 3;

    if(result == 0)
    {
        /*Scan Object 0x1C13 TXPDO assign*/
        for(PDOAssignEntryCnt = 0; PDOAssignEntryCnt < sTxPDOassign.u16SubIndex0; PDOAssignEntryCnt++)
        {
            pPDO = OBJ_GetObjectHandle(sTxPDOassign.aEntries[PDOAssignEntryCnt]);
            if(pPDO != NULL)
            {
                PDOSubindex0 = *((UINT16 *)pPDO->pVarPtr);
                for(PDOEntryCnt = 0; PDOEntryCnt < PDOSubindex0; PDOEntryCnt++)
                {
/*ECATCHANGE_START(V5.12) ECAT1*/
                    pPDOEntry = (UINT32 *)((UINT16 *)pPDO->pVarPtr + (OBJ_GetEntryOffset((PDOEntryCnt+1),pPDO)>>3)/2);    //goto PDO entry
/*ECATCHANGE_END(V5.12) ECAT1*/
                    // we increment the expected output size depending on the mapped Entry
                    InputSize += (UINT16) ((*pPDOEntry) & 0xFF);
                }
            }
            else
            {
                /*assigned PDO was not found in object dictionary. return invalid mapping*/
                InputSize = 0;
                result = ALSTATUSCODE_INVALIDINPUTMAPPING;
                break;
            }
        }
    }
    InputSize = (InputSize + 7) >> 3;

    *pInputSize = InputSize;
    *pOutputSize = OutputSize;
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
\param      pData  pointer to input process data

\brief      This function will copies the inputs from the local memory to the ESC memory
            to the hardware
*////////////////////////////////////////////////////////////////////////////////////////
void APPL_InputMapping(UINT16* pData)
{
    MEMCPY(pData,&InputCounter,SIZEOF(InputCounter));
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
\param      pData  pointer to output process data

\brief    This function will copies the outputs from the ESC memory to the local memory
            to the hardware
*////////////////////////////////////////////////////////////////////////////////////////
void APPL_OutputMapping(UINT16* pData)
{
    MEMCPY(&OutputCounter,pData,SIZEOF(OutputCounter));
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
\brief    This function will called from the synchronisation ISR 
            or from the mainloop if no synchronisation is supported
*////////////////////////////////////////////////////////////////////////////////////////
void APPL_Application(void)
{
#if (USE_SHARED_MEMORY)
    read_shared_memory();
#endif
#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZN2L_RSK) | defined(BOARD_RZT2L_RSK)
    /*Hardware independent sample application*/
	InputCounter = APPL_GetDipSw() & 0x000f;		// SW3.4-1
	APPL_SetLed((UINT16)OutputCounter);
#else
    if(OutputCounter > 0)
    {
        InputCounter = OutputCounter+1;
    }
    else
    {
        InputCounter++;
    }
#endif
#if (USE_SHARED_MEMORY)
    write_shared_memory();
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \return    The Explicit Device ID of the EtherCAT slave

 \brief     Calculate the Explicit Device ID
*////////////////////////////////////////////////////////////////////////////////////////
UINT16 APPL_GetDeviceID()
{
#if defined(BOARD_RZT2M_RSK)
    return (APPL_GetDipSw() & 0x00FF);  // SW3.1-8
#elif defined(BOARD_RZN2L_RSK) | defined(BOARD_RZT2L_RSK)
	return (APPL_GetDipSw());							// SW3.4-1
#else
    return 0x5;
#endif
}

#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZN2L_RSK) | defined(BOARD_RZT2L_RSK)
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \param    UINT16 LED output value. The value one means ON.

 \brief    SET LED
*////////////////////////////////////////////////////////////////////////////////////////
void APPL_SetLed(UINT16 value)
{
#if (USE_SHARED_MEMORY)
    g_shared_memory_local_data.from_CPU1_to_CPU0.output_LED = (UINT16)(value & 0x00FF);
#else
    /* LED type structure */
    sample_leds_t leds = g_sample_leds;

    /* Holds level to set for pins */
    bsp_io_level_t pin_level[4];

    /* This code uses BSP IO functions to show how it is used.*/
    R_BSP_PinAccessEnable();

    pin_level[SAMPLE_LED_RLED0] = ((value & 1) ?  BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
    pin_level[SAMPLE_LED_RLED1] = ((value & 2) ?  BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
    pin_level[SAMPLE_LED_RLED2] = ((value & 4) ?  BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
    // pin_level[SAMPLE_LED_RLED3] = ((value & 8) ?  BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);

    R_IOPORT_PinWrite(&g_ioport_ctrl, (bsp_io_port_pin_t)leds.p_leds[SAMPLE_LED_RLED0], pin_level[SAMPLE_LED_RLED0]);
    R_IOPORT_PinWrite(&g_ioport_ctrl, (bsp_io_port_pin_t)leds.p_leds[SAMPLE_LED_RLED1], pin_level[SAMPLE_LED_RLED1]);

#if defined(BOARD_RZT2M_RSK) | defined(BOARD_RZN2L_RSK)
    R_IOPORT_PinWrite(&g_ioport_ctrl, (bsp_io_port_pin_t)leds.p_leds[SAMPLE_LED_RLED2], pin_level[SAMPLE_LED_RLED2]);
    // R_IOPORT_PinWrite(&g_ioport_ctrl, (bsp_io_port_pin_t)leds.p_leds[SAMPLE_LED_RLED3], pin_level[SAMPLE_LED_RLED3]);
#endif

    /* Protect PFS registers */
    R_BSP_PinAccessDisable();
#endif

}
/////////////////////////////////////////////////////////////////////////////////////////
/**
 \retuen   UINT16 DIP SW value. Low input level means ON.

 \brief    Get DIP SW
*////////////////////////////////////////////////////////////////////////////////////////
UINT16 APPL_GetDipSw(void)
{
    uint16_t u16DipSw;
#if (USE_SHARED_MEMORY)
    u16DipSw = (UINT16)(g_shared_memory_local_data.from_CPU0_to_CPU1.input_DIPSW & 0x00FF);
#else
    u16DipSw = 0;

    /* DIP SW type structure */
    sample_dip_sws_t dipsws = g_sample_dip_sws;

    /* This code uses BSP IO functions to show how it is used.*/
    R_BSP_PinAccessEnable();

    if (R_BSP_FastPinRead(R_BSP_IoRegionGet((bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_0]), (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_0]) == BSP_IO_LEVEL_LOW) u16DipSw |= 0x01;
    if (R_BSP_FastPinRead(R_BSP_IoRegionGet((bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_1]), (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_1]) == BSP_IO_LEVEL_LOW) u16DipSw |= 0x02;
    // if (R_BSP_FastPinRead(R_BSP_IoRegionGet((bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_2]), (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_2]) == BSP_IO_LEVEL_LOW) u16DipSw |= 0x04;
    // if (R_BSP_FastPinRead(R_BSP_IoRegionGet((bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_3]), (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_3]) == BSP_IO_LEVEL_LOW) u16DipSw |= 0x08;

#if defined(BOARD_RZT2M_RSK)
    if (R_BSP_FastPinRead(R_BSP_IoRegionGet((bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_4]), (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_4]) == BSP_IO_LEVEL_LOW) u16DipSw |= 0x10;
    if (R_BSP_FastPinRead(R_BSP_IoRegionGet((bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_5]), (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_5]) == BSP_IO_LEVEL_LOW) u16DipSw |= 0x20;
    if (R_BSP_FastPinRead(R_BSP_IoRegionGet((bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_6]), (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_6]) == BSP_IO_LEVEL_LOW) u16DipSw |= 0x40;
    if (R_BSP_FastPinRead(R_BSP_IoRegionGet((bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_7]), (bsp_io_port_pin_t)dipsws.p_sws[SAMPLE_DIPSW_7]) == BSP_IO_LEVEL_LOW) u16DipSw |= 0x80;
#endif

    /* Protect PFS registers */
    R_BSP_PinAccessDisable();
#endif
    return u16DipSw;
}
#endif
/** @} */

