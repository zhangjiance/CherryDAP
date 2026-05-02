/*!
 * @file       derivative.h
 *
 * @brief      Compatibility header for OSBDM JM60 code on APM32E103
 *
 * @note       This file provides compatibility definitions for code
 *             originally written for Freescale JM60 MCU
 */

#ifndef __DERIVATIVE_H
#define __DERIVATIVE_H

/* Include APM32 headers instead of JM60 headers */
#include "apm32e10x.h"
#include "board_osbdm.h"

/* JM60 to APM32 compatibility types - do not redefine if typedef.h is included */

/* GPIO register compatibility - map to our macros */
/* These are defined as dummy variables to satisfy old code that directly accesses them */
/* The actual GPIO control is done through our macros in board_osbdm.h */

/* Note: The JM60 code uses direct register access like:
 *   PTBD |= 0x04;    // Set bit
 *   PTBD &= ~0x04;   // Clear bit
 * We replace these with our macros like TCK_SET(), TCK_RESET(), etc.
 */

#endif /* __DERIVATIVE_H */
