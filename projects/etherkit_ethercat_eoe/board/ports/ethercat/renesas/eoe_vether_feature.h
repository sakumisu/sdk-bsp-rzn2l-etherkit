/***********************************************************************************************************************
 * Copyright [2020-2021] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics Corporation and/or its affiliates and may only
 * be used with products of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.
 * Renesas products are sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for
 * the selection and use of Renesas products and Renesas assumes no liability.  No license, express or implied, to any
 * intellectual property right is granted by Renesas.  This software is protected under all applicable laws, including
 * copyright laws. Renesas reserves the right to change or discontinue this software and/or this documentation.
 * THE SOFTWARE AND DOCUMENTATION IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND
 * TO THE FULLEST EXTENT PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY,
 * INCLUDING WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE
 * SOFTWARE OR DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR
 * DOCUMENTATION (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER,
 * INCLUDING, WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY
 * LOST PROFITS, OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

#ifndef EOE_VETHER_FEATURE_H_
#define EOE_VETHER_FEATURE_H_

#include "hal_data.h"

/** Exclusively defines the TCP/IP stack to use */
#define EOE_VETHER_FEATURE_TCPIP_STACK_PLUS_TCP       (0)
#define EOE_VETHER_FEATURE_TCPIP_STACK_LWIP           (1)

fsp_err_t EOE_VETHER_Open (ether_ctrl_t * const p_ctrl, ether_cfg_t const * const p_cfg);
fsp_err_t EOE_VETHER_Close (ether_ctrl_t * const p_ctrl);
fsp_err_t EOE_VETHER_Read(ether_ctrl_t * const p_ctrl, void * const p_buffer, uint32_t * const length_bytes);
fsp_err_t EOE_VETHER_Write(ether_ctrl_t * const p_ctrl, void * const p_buffer, uint32_t const frame_length);
fsp_err_t EOE_VETHER_LinkProcess (ether_ctrl_t * const p_ctrl);
fsp_err_t EOE_VETHER_GetLinkStatus (ether_ctrl_t * const p_ctrl, uint8_t port, gmac_link_status_t * p_status);
void eoe_vether_frame_received_callback (uint16_t *pData, uint16_t length);

#endif /** EOE_VETHER_FEATURE_H_ */
