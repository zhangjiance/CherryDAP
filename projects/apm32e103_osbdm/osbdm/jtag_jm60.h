/* OSBDM-JM60 Target Interface Software Package
 * Copyright (C) 2009  Freescale
 *
 * This software package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



#ifndef _JTAG_JM60_H_
#define _JTAG_JM60_H_

/* Use board-specific GPIO definitions from board.h */
#include "board.h"

/*
 * Legacy JM60 drivers directly read/write port registers (PTED/PTBD/...).
 * On APM32 those registers do not exist, so provide a dummy lvalue-backed
 * register alias to keep legacy code buildable while porting.
 */
extern volatile uint8_t osbdm_dummy_gpio_reg;

#define PTED           osbdm_dummy_gpio_reg
#define PTEDD          osbdm_dummy_gpio_reg
#define PTBD           osbdm_dummy_gpio_reg
#define PTBDD          osbdm_dummy_gpio_reg

#define PTBPE_PTBPE2   osbdm_dummy_gpio_reg
#define PTBPE_PTBPE4   osbdm_dummy_gpio_reg
#define PTBD_PTBD2     osbdm_dummy_gpio_reg
#define PTBD_PTBD4     osbdm_dummy_gpio_reg

#define PTED_PTED2     osbdm_dummy_gpio_reg
#define PTED_PTED3     osbdm_dummy_gpio_reg
#define PTED_PTED4     osbdm_dummy_gpio_reg
#define PTED_PTED5     osbdm_dummy_gpio_reg
#define PTED_PTED7     osbdm_dummy_gpio_reg

#define TDSCLK_EN      osbdm_dummy_gpio_reg

#define PTED_PTED2_MASK 0
#define PTED_PTED3_MASK 0
#define PTED_PTED4_MASK 0
#define PTED_PTED5_MASK 0
#define PTED_PTED7_MASK 0

#define PTEDD_PTEDD2   osbdm_dummy_gpio_reg
#define PTEDD_PTEDD3   osbdm_dummy_gpio_reg
#define PTEDD_PTEDD4   osbdm_dummy_gpio_reg
#define PTEDD_PTEDD5   osbdm_dummy_gpio_reg
#define PTEDD_PTEDD7   osbdm_dummy_gpio_reg

#define sci_virtual_serial_port_is_enabled 0

#define	JTAG_OK			              0
#define JTAG_ERROR	                  1

#define DATA_REGISTER	              0
#define INSTRUCTION_REGISTER	      1

/*==============================================================================
 * GPIO Signal Mappings - Map JM60 signals to APM32E103 GPIO macros
 *============================================================================*/

/* JTAG Control Signals - use board.h macros directly, no redefinition needed */

/* Other JM60 signals - not used in our implementation, define as dummy */
#define VSW_EN			        0
#define VSW_EN_MASK		        0
#define VSW_EN_DIR		        0

#define VTRG_EN			        0
#define VTRG_EN_MASK	        0
#define VTRG_EN_DIR		        0

#define TCLK_CTL		        0
#define TCLK_CTL_MASK	        0
#define TCLK_CTL_DIR	        0

#define TA_OUT			        0
#define TA_OUT_MASK		        0
#define TA_OUT_DIR		        0

#define VTRG_IN			        0
#define VTRG_IN_MASK	        0
#define VTRG_IN_DIR		        0

#define P4_IN			        0
#define P4_IN_MASK		        0
#define P4_IN_DIR		        0

#define P5_IN			        0
#define P5_IN_MASK		        0
#define P5_IN_DIR		        0

#define P6_IN			        0
#define P6_IN_MASK		        0
#define P6_IN_DIR		        0

#define P7_DE_IN		        0
#define P7_DE_IN_MASK	        0
#define P7_DE_IN_DIR	        0

#define tRSTO			        osbdm_dummy_gpio_reg
#define tRSTO_MASK		        0
#define tRSTO_DIR		        osbdm_dummy_gpio_reg

#define VPP_ON			        0
#define VPP_ON_MASK		        0
#define VPP_ON_DIR		        0

#define tPWR_LED		        0
#define tPWR_LED_MASK	        0
#define tPWR_LED_DIR	        0

#define STATUS_LED		        0
#define STATUS_LED_MASK	        0
#define STATUS_LED_DIR	        0

#define tRSTI			        0
#define tRSTI_MASK		        0
#define tRSTI_DIR		        0

#define SCLK_OUT		        0
#define SCLK_OUT_MASK	        0
#define SCLK_OUT_DIR	        0

#define OUT_EN			        osbdm_dummy_gpio_reg
#define OUT_EN_MASK		        0
#define OUT_EN_DIR		        osbdm_dummy_gpio_reg

#define VPP_EN			        0
#define VPP_EN_MASK		        0
#define VP_EN_DIR		        0

#define VSW_FAULT		        0
#define VSW_FAULT_MASK	        0
#define VSW_FAULT_DIR	        0

#define VTRG_FAULT		        0
#define VTRG_FAULT_MASK	        0
#define VTRG_FAULT_DIR	        0

#define RTS				        0
#define RTS_MASK		        0
#define RTS_DIR			        0

#define CTS				        0
#define CTS_MASK		        0
#define CTS_DIR			        0

/* TMS level values - for compatibility with old code */
#define	TMS_HIGH_VAL	1
#define	TMS_LOW_VAL		0

/*==============================================================================
 * JTAG Signal Macros - All macros are provided by board.h
 * No redefinition needed here. The following macros are available:
 * 
 * - TMS control: TMS_SET(), TMS_RESET(), TMS_HIGH(), TMS_LOW()
 * - TCK control: TCK_SET(), TCK_RESET(), TCK_HIGH(), TCK_LOW()
 * - TRST control: TRST_SET(), TRST_RESET(), TRST_ASSERT(), TRST_DEASSERT()
 * - TDI control: TDI_SET(), TDI_RESET(), TDI_HIGH(), TDI_LOW()
 * - TDO read: TDO_READ(), TDO_GET()
 *============================================================================*/

/* Compatibility aliases for JTAG drivers */
#define TCLK_SET()        TCK_SET()
#define TCLK_RESET()      TCK_RESET()
#define TDI_OUT_SET()     TDI_SET()
#define TDI_OUT_RESET()   TDI_RESET()
#define TDO_IN_SET        (TDO_READ() == 1)


typedef enum {
    TEST_LOGIC_RESET,
    RUN_TEST_IDLE,
    PAUSE_DR,
    PAUSE_IR,
    SHIFT_DR,
    SHIFT_IR,
    UPDATE_DR,
    UPDATE_IR,

    MAX_JTAG_STATE,

} JTAG_STATE_TYPE;


// JTAG functions for Freescale JM60 OSBDM

void jdly_loop(int i);
char TCLK_transition(char tms, char tdi);
void Jtag_ScanIO (char jtag_register, char bitcount, unsigned char *out_data,
                  unsigned char *in_data, JTAG_STATE_TYPE state);
void Jtag_ScanIn(char jtag_register, char bitcount,
                  unsigned char *in_data, JTAG_STATE_TYPE state);
void Jtag_ScanOut(char jtag_register, char bitcount,
                  unsigned char *out_data, JTAG_STATE_TYPE state);
void Jtag_UlongToScanData (unsigned long ulong_data, unsigned char * scandata);
unsigned long Jtag_ScanDataToUlong (unsigned char * scandata);

#endif
