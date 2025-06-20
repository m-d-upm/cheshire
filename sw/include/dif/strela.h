// Copyright 2025 CEI - UPM.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Juan Granja <juan.granja@upm.es>
// Milos Dordevic <milos.dordevic@upm.es>

#pragma once

#include <stdint.h>

#define CGRA_CTRL_BIT_START_EXEC    0x1
#define CGRA_CTRL_BIT_CLEAR_STATE   0x2
#define CGRA_CTRL_BIT_LOAD_CONFIG   0x4
#define CGRA_CTRL_BIT_CLEAR_CONFIG  0x8

#define CGRA_CTRL_BIT_DONE_CONFIG   0x2
#define CGRA_CTRL_BIT_DONE_EXEC     0x1

#define CGRA_IN_BITS_STRIDE_COUNT(stride, count) ( (stride << 16) | stride * count )
#define CGRA_OUT_BITS_STRIDE4_COUNT(count) ( 4 * count )

#define CGRA_CTRL_OFFSET        (0x00)

#define CGRA_CONF_OFFSET	    (0x04)
#define CGRA_CONF_SIZE_OFFSET	(0x08)

#define CGRA_IN0_OFFSET	        (0x10)
#define CGRA_IN0_SIZE_OFFSET	(0x14)
#define CGRA_IN1_OFFSET	        (0x18)
#define CGRA_IN1_SIZE_OFFSET	(0x1C)
#define CGRA_IN2_OFFSET	        (0x20)
#define CGRA_IN2_SIZE_OFFSET	(0x24)
#define CGRA_IN3_OFFSET	        (0x28)
#define CGRA_IN3_SIZE_OFFSET	(0x2C)

#define CGRA_OUT0_OFFSET	    (0x50)
#define CGRA_OUT0_SIZE_OFFSET	(0x54)
#define CGRA_OUT1_OFFSET	    (0x58)
#define CGRA_OUT1_SIZE_OFFSET	(0x5C)
#define CGRA_OUT2_OFFSET	    (0x60)
#define CGRA_OUT2_SIZE_OFFSET	(0x64)
#define CGRA_OUT3_OFFSET	    (0x68)
#define CGRA_OUT3_SIZE_OFFSET	(0x6C)

#define CGRA_CNTR_CONF_OFFSET   (0x90)
#define CGRA_CNTR_EXEC_OFFSET   (0x94)
#define CGRA_CNTR_STALL_OFFSET  (0x98)

#define CGRA_OUT_ARB_HOLD_OFFSET (0xA0)

#define CGRA_RESET_DMA_OFFSET   (0xf8)

#define AM_OPA 		            (0xF0)
#define AM_OPB 		            (0xF4)
#define AM_OPR 		            (0xF8)
