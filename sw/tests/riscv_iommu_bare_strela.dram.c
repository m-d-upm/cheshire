// Copyright 2025 CEI - UPM.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Milos Dordevic <milos.dordevic@upm.es>

#include "regs/cheshire.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "util.h"
#include "dif/strela.h"
#include "dif/rv_iommu.h"

#define FLUSH_D_CACHE() do {\
                            __asm__ volatile ("csrwi 0x7C1 , 0x00");\
                            __asm__ volatile ("csrwi 0x7C1 , 0x01");\
                        } while(0)

#define IMAGE_SIDE              (32)
#define DATA_SIZE               (IMAGE_SIDE * IMAGE_SIDE) // 1024
#define RESULT_SIZE             (IMAGE_SIDE * IMAGE_SIDE)

#define CONV2D_1_KRNL_NPE       (8)
#define CONV2D_1_KRNL_SIZE      (CONV2D_1_KRNL_NPE * (5))
#define CONV2D_1_KRNL_BYTES     (CONV2D_1_KRNL_SIZE * (4))

#define CONV2D_2_KRNL_NPE       (10)
#define CONV2D_2_KRNL_SIZE      (CONV2D_2_KRNL_NPE * (5))
#define CONV2D_2_KRNL_BYTES     (CONV2D_2_KRNL_SIZE * (4))

/*
// Input 32x32
// Output: 32x32 but external 1-pixel boundary is to be ignored
// To do it in one pass, start at (x, y): (1,1) end at (n-2, n-2)
// Number of elements in the final result: (n*n) - n - n - 2
// Offset of result: n+1
*/

#define WRITE_RESULT_SIZE   (IMAGE_SIDE * IMAGE_SIDE - IMAGE_SIDE - IMAGE_SIDE - 2)
#define WRITE_RESULT_OFFSET (IMAGE_SIDE + 1)

static int32_t image[DATA_SIZE] =
{
    12, 88, 59, 42, 14, 96, 65, 59, 82, 87, 71, 87, 99, 89, 79, 20, 45,  4,  8, 38, 30, 97, 26, 28, 78, 24, 43,  4, 65, 52, 16, 94,
    87, 95, 11, 98, 36, 44, 52, 89, 95, 87, 55, 64, 28, 47, 11, 67, 87, 67, 49, 22, 66, 95, 65, 25, 64, 77, 34, 75, 13, 86, 33, 27,
    14, 87, 70, 96, 60,  0,  2, 72,  1, 25, 87, 94,  6, 50, 12, 51, 54, 58, 71, 29, 37, 80, 45, 99, 64, 82, 18, 85, 68, 51, 62, 65,
    55, 85, 29, 19, 66, 24, 13,  1, 13, 93, 37, 29, 11, 43, 95, 90, 35, 87, 65, 39, 16, 89, 52, 62, 16, 84, 17, 23, 79, 44, 62, 92,
    57, 23, 68, 75, 56, 41, 84, 92, 70, 95, 88, 15, 16, 37,  9, 28, 31, 97, 69, 80, 61, 93, 73, 69, 19, 37, 15, 48, 72, 80, 74, 91,
    74, 48, 80, 98, 72, 30, 47, 15,  7, 12,  4, 34, 24,  7, 22,  3, 69, 65, 51, 34, 60, 10, 90,  0, 37, 92, 48, 97, 40, 60, 93, 73,
    58, 70, 42, 41, 48,  8, 65, 21, 40, 85, 73, 82, 46,  6, 99, 13, 27,  6, 85, 17, 26, 22, 77,  3, 30, 86, 33, 22, 63, 21, 58, 49,
    42, 82, 97, 39, 52, 85, 62, 35,  1, 71,  1, 76, 85, 12,  8, 91, 67, 56, 66, 30, 54, 55, 38, 15, 53, 55, 78, 37, 52, 87, 29, 55,
    43, 65, 92, 51,  7, 91, 67, 76, 95, 67, 39, 30, 71, 64, 92, 63, 45, 15, 70, 33, 20, 14, 52, 97, 97, 69, 89, 97, 73, 91, 47, 51,
    28, 18, 25, 35, 49,  7,  7, 20, 81,  4, 81,  4, 46, 49, 98, 25, 94, 16, 98, 36, 87, 83, 41, 40, 66, 23, 50, 88, 78, 92, 23, 23,
    24, 42,  9, 82, 79, 67, 72, 70,  4,  0, 68, 43, 28, 86, 77, 26, 39, 88, 47, 99, 23, 16, 27, 94, 57, 89, 30, 56, 90, 62, 30, 12,
    97, 72, 41, 29, 95, 86, 55, 14, 47, 18, 42, 89, 18, 65, 61, 82,  0, 67,  0, 80, 92, 72, 59, 92, 43, 40,  5, 74, 90, 18, 62, 69,
    63, 68, 21, 12,  6, 17, 57, 68, 13, 39, 78, 17, 43, 31, 59, 95, 44, 96, 19, 16, 84, 92, 10, 83, 57,  8, 58, 41, 10, 84, 71, 77,
    64, 92, 43, 74, 64,  2, 65, 10, 84,  0, 74, 13, 73, 30, 25, 85, 70, 16, 39, 30, 49, 17, 55, 57, 67, 29, 23, 34, 45, 48, 80, 26,
    56, 71, 79, 65, 94,  2, 35, 75, 73, 95, 63, 17, 64, 41, 31,  2, 67,  1, 89, 93, 19, 99,  8, 39, 65, 99, 81, 38, 50, 55, 92, 24,
    75,  3, 96, 45, 45, 11,  8, 62, 65, 98, 25, 55, 26, 60, 45, 39, 67, 65, 55, 24, 50, 25, 39, 52, 98, 37, 56, 31, 53, 25, 84, 20,
    27, 93, 31,  1, 68,  0, 51, 48, 34, 60, 77, 69, 84, 67, 83, 46, 52, 60, 18, 20, 23, 94, 27, 36, 11, 30, 24, 52, 46, 59, 43, 66,
    76,  2, 78, 84, 81, 92, 53, 85, 84, 17, 16, 82, 46, 74, 57, 16, 15, 97, 34, 14, 58, 51, 98, 67, 32, 69, 12, 65, 60, 29, 18,  3,
    68, 80, 35, 15, 10, 20, 46, 84, 45,  2,  2, 18, 50, 24,  9, 51, 70, 68, 93, 43, 46,  7, 88, 40, 40,  2, 22, 61, 52, 53, 76, 10,
    59,  2, 61, 48, 90, 95, 21, 65, 28, 45, 31, 88, 17, 16, 59, 19, 64, 32, 97, 81, 49, 36, 81, 98, 25, 29, 98, 43, 62, 98, 49, 47,
    58, 44, 32, 44, 88, 66, 56, 34, 11, 32, 72, 79, 77, 14, 73, 77, 27, 13, 28, 36, 53,  3,  4, 35,  4, 34, 98, 30, 88, 75, 86, 62,
    28, 76, 51, 76, 78, 17, 60, 68, 91, 77, 56, 39,  9, 38, 85, 42, 76, 29, 50, 69, 23, 83,  6, 31, 94, 64, 15, 86, 87, 12, 22,  8,
    30, 39, 97, 97, 49, 38, 52, 87, 31, 36, 26, 32, 87,  6, 36, 47, 15, 77, 86, 99, 91, 45, 33, 14, 22, 46, 18, 17, 47, 12, 67, 43,
    71, 10, 59, 88,  3, 42, 25, 14, 92, 67, 82, 44, 89, 81, 94, 92, 74, 22, 78,  0, 71, 42, 78, 39, 98, 73, 76,  9, 53, 64,  9, 27,
    69, 54, 54,  2, 99,  3, 14, 96, 61, 31, 16, 97, 34, 66, 46, 19, 70, 26, 70, 86,  8, 68,  6,  5, 38, 13, 31, 29, 63, 12, 98, 45,
    34, 19, 70, 20, 91, 68, 36, 56, 19, 29, 20, 84, 73, 62, 80, 63, 39,  3,  1, 12, 96,  4, 77,  6, 51, 55,  3,  0, 38, 69, 71, 97,
    99, 24, 44, 11, 19, 60, 98, 19, 30, 74, 27, 76, 64, 61, 62, 55,  0, 32, 44, 82, 64, 84, 40, 88, 58, 43,  8, 59, 30, 56,  0, 75,
    13, 22, 35, 79, 88, 36, 78, 27, 60, 87,  6, 29, 34,  9, 81, 50, 48,  4, 83, 29, 17, 84, 75, 16, 98, 77, 61, 53, 17, 93, 97, 29,
    38,  7, 36, 34,  9, 72,  5, 48, 82, 25, 46, 77, 35,  8, 87, 48, 13, 27, 52, 29, 65, 19, 54, 27, 39, 70, 53, 40, 27,  6,  5, 82,
    49, 17, 83, 18, 35, 19, 49, 92, 46, 28, 93, 94, 92, 40, 44, 10, 83, 50, 57, 48, 14, 46, 12, 55, 72, 56, 92, 84, 98, 10, 31, 23,
    40, 92, 72,  5, 13, 19, 11, 71, 94, 91, 75, 70, 64, 30, 30, 84, 11, 32, 65, 86, 36, 55, 78,  5, 83,  3, 62, 83, 61, 97, 87, 97,
    44, 87, 95, 64, 55,  6, 14, 49,  5, 88, 29,  1, 10, 60, 89, 83, 94, 19, 47, 58, 16, 33,  3, 26, 89, 72, 63, 21, 75, 85, 36, 94,
};

uint32_t conv2d_1_kernel[CONV2D_1_KRNL_SIZE] = {
    0xC0000020, 0x00202200, 0x00000000, 0x00000000, 0xC5000000, // 0
    0xC0000020, 0x00202200, 0x00000000, 0xFFFFFFFF, 0xC5010000, // 1
    0xC0000020, 0x00202200, 0x00000000, 0x00000000, 0xC5020000, // 2
    0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x05040000, // 4
    0x18800010, 0x00400030, 0x00000000, 0x00000000, 0xE5050000, // 5
    0xC0800010, 0x00200030, 0x00000000, 0x00000000, 0xE5060000, // 6
    0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x050A0000, // 10
    0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x050E0000  // 14
};

uint32_t conv2d_2_kernel[CONV2D_2_KRNL_SIZE] = {
    0xC0000020, 0x00202200, 0x00000000, 0x00000000, 0xC5000000, // 0
    0xC0000020, 0x00202200, 0x00000000, 0xFFFFFFFF, 0xC5010000, // 1
    0xC0000020, 0x00202200, 0x00000000, 0x00000000, 0xC5020000, // 2
    0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x05030000, // 3
    0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x05040000, // 4
    0x18800010, 0x00400030, 0x00000000, 0x00000000, 0xE5050000, // 5
    0x18800010, 0x00400030, 0x00000000, 0x00000000, 0xE5060000, // 6
    0xC0800010, 0x00200030, 0x00000000, 0x00000000, 0xE5070000, // 7
    0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x050B0000, // 11
    0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x050F0000  // 15
};

static uint32_t result[RESULT_SIZE];
static uint32_t result_sw[RESULT_SIZE];

void test_conv2d(void);
void print_uart(char* str);
void uart_print_unsigned_dec(uint32_t num);
void examine_mem_hex(uint32_t ram_addr1, uint32_t ram_addr2);

int main(void) {
    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
    uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);

    print_uart("STRELA test application with IOMMU in bare mode.\n\r");

    //init_iommu();
    //fence_i();
    set_iommu_bare();

    test_conv2d();

    return 0;
 }

void test_conv2d()
{
    *reg32(&__base_cgra, CGRA_OUT_ARB_HOLD_OFFSET) = 1;

    print_uart("CONV2D config. 32x32 image.\n\r");
    print_uart("Baremetal \n\r");
    print_uart("-------------------------\n\r");

    // Clear results buffer in RAM
    for (int i = 0; i < RESULT_SIZE; i++)
    {
        result[i] = 0;
    }

    uint32_t *cgra_kernel = conv2d_1_kernel;
    uint32_t cgra_kernel_size = CONV2D_1_KRNL_SIZE;

    // Setup data transfer ---------------------------
   
    //---------------------------------------//
    *reg32(&__base_cgra, CGRA_CONF_OFFSET) = (uint64_t)cgra_kernel;
    *reg32(&__base_cgra, CGRA_CONF_SIZE_OFFSET) = cgra_kernel_size * 4;

    *reg32(&__base_cgra, CGRA_IN0_OFFSET) = (uint64_t)image;
    *reg32(&__base_cgra, CGRA_IN0_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN1_OFFSET) = (uint64_t)(image + 1);
    *reg32(&__base_cgra, CGRA_IN1_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN2_OFFSET) = (uint64_t)(image + 2);
    *reg32(&__base_cgra, CGRA_IN2_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN3_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_IN3_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT0_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT0_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT1_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT1_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT2_OFFSET) = (uint64_t)(result + WRITE_RESULT_OFFSET);
    *reg32(&__base_cgra, CGRA_OUT2_SIZE_OFFSET) = CGRA_OUT_BITS_STRIDE4_COUNT(WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_OUT3_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT3_SIZE_OFFSET) = 0;
    //---------------------------------------//

    print_uart("Setup transfer 1:\t");
    print_uart("\n\r");

    // Load CONFIG 1 -----------------------

    //---------------------------------------//
    // Reset CGRA state and DMA module
	*reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_CLEAR_STATE;
    *reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_CLEAR_CONFIG;
    *reg32(&__base_cgra, CGRA_RESET_DMA_OFFSET) = 1;

    *reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_LOAD_CONFIG; // Start config read

    // Wait for config done:
    while(!(*reg32(&__base_cgra, CGRA_CTRL_OFFSET) & CGRA_CTRL_BIT_DONE_CONFIG)){};
    //---------------------------------------//


    print_uart("Config 1: \t");
    print_uart("\n\r");

    // Execute --------------------------

    //---------------------------------------//
    *reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_START_EXEC;

    // Wait for config done:
    while(!(*reg32(&__base_cgra, CGRA_CTRL_OFFSET) & CGRA_CTRL_BIT_DONE_EXEC)){};

    FLUSH_D_CACHE();
    //---------------------------------------//
    print_uart("Execute 1: \t");
    print_uart("\n\r");

    cgra_kernel = conv2d_2_kernel;
    cgra_kernel_size = CONV2D_2_KRNL_SIZE;

    // Setup data transfer ---------------------------

    //---------------------------------------//
    *reg32(&__base_cgra, CGRA_CONF_OFFSET) = (uint64_t)cgra_kernel;
    *reg32(&__base_cgra, CGRA_CONF_SIZE_OFFSET) = cgra_kernel_size * 4;

	*reg32(&__base_cgra, CGRA_IN0_SIZE_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_IN1_SIZE_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_IN2_SIZE_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_IN3_SIZE_OFFSET) = 0;
	*reg32(&__base_cgra, CGRA_OUT0_SIZE_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT1_SIZE_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT2_SIZE_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT3_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_IN0_OFFSET) = (uint64_t)(image + IMAGE_SIDE);
    *reg32(&__base_cgra, CGRA_IN0_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN1_OFFSET) = (uint64_t)(image + IMAGE_SIDE + 1);
    *reg32(&__base_cgra, CGRA_IN1_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN2_OFFSET) = (uint64_t)(image + IMAGE_SIDE + 2);
    *reg32(&__base_cgra, CGRA_IN2_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN3_OFFSET) = (uint64_t)(result + WRITE_RESULT_OFFSET);
    *reg32(&__base_cgra, CGRA_IN3_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);;

    *reg32(&__base_cgra, CGRA_OUT0_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT0_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT1_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT1_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT2_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT2_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT3_OFFSET) = (uint64_t)(result + WRITE_RESULT_OFFSET);
    *reg32(&__base_cgra, CGRA_OUT3_SIZE_OFFSET) = CGRA_OUT_BITS_STRIDE4_COUNT(WRITE_RESULT_SIZE);
    //---------------------------------------//


    print_uart("Setup transfer 2:\t");
    print_uart("\n\r");

    // Load CONFIG 2 -----------------------
    //---------------------------------------//

    // Reset CGRA state and DMA module
	*reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_CLEAR_STATE;
    *reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_CLEAR_CONFIG;
    *reg32(&__base_cgra, CGRA_RESET_DMA_OFFSET) = 1;

    *reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_LOAD_CONFIG; // Start config read

    // Wait for config done:
    while(!(*reg32(&__base_cgra, CGRA_CTRL_OFFSET) & CGRA_CTRL_BIT_DONE_CONFIG)){};
    //---------------------------------------//


    print_uart("Config 2: \t");
    print_uart("\n\r");

    // Execute --------------------------

    //---------------------------------------//
    *reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_START_EXEC;

    // Wait for config done:
    while(!(*reg32(&__base_cgra, CGRA_CTRL_OFFSET) & CGRA_CTRL_BIT_DONE_EXEC)){};

    FLUSH_D_CACHE();
    //---------------------------------------//


    print_uart("Execute 2: \t");
    print_uart("\n\r");

    // Config data transfer ---------------------------

    //---------------------------------------//
    *reg32(&__base_cgra, CGRA_IN0_OFFSET) = (uint64_t)(image + 2 * IMAGE_SIDE);
    *reg32(&__base_cgra, CGRA_IN0_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN1_OFFSET) = (uint64_t)(image + 2 * IMAGE_SIDE + 1);
    *reg32(&__base_cgra, CGRA_IN1_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN2_OFFSET) = (uint64_t)(image + 2 * IMAGE_SIDE + 2);
    *reg32(&__base_cgra, CGRA_IN2_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);

    *reg32(&__base_cgra, CGRA_IN3_OFFSET) = (uint64_t)(result + WRITE_RESULT_OFFSET);
    *reg32(&__base_cgra, CGRA_IN3_SIZE_OFFSET) = CGRA_IN_BITS_STRIDE_COUNT(0x04, WRITE_RESULT_SIZE);;

    *reg32(&__base_cgra, CGRA_OUT0_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT0_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT1_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT1_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT2_OFFSET) = 0;
    *reg32(&__base_cgra, CGRA_OUT2_SIZE_OFFSET) = 0;

    *reg32(&__base_cgra, CGRA_OUT3_OFFSET) = (uint64_t)(result + WRITE_RESULT_OFFSET);
    *reg32(&__base_cgra, CGRA_OUT3_SIZE_OFFSET) = CGRA_OUT_BITS_STRIDE4_COUNT(WRITE_RESULT_SIZE);
    //---------------------------------------//


    print_uart("Setup transfer 3:\t");
    print_uart("\n\r");

    // Execute --------------------------

    //---------------------------------------//
    *reg32(&__base_cgra, CGRA_CTRL_OFFSET) = CGRA_CTRL_BIT_START_EXEC;

    // Wait for config done:
    while(!(*reg32(&__base_cgra, CGRA_CTRL_OFFSET) & CGRA_CTRL_BIT_DONE_EXEC)){};

    FLUSH_D_CACHE();
    //---------------------------------------//


    print_uart("Execute 3: \t");
    print_uart("\n\r");

    print_uart("TOTAL CGRA: \t");
    print_uart("\n\r");

    int32_t filter[] = { 0, -1, 0, \
                         0, -1, 0, \
                         0, -1, 0, };

    // SW implementation

    //---------------------------------------//
    for(int i = 1; i < IMAGE_SIDE - 1; i ++) {
        for(int j = 1; j < IMAGE_SIDE - 1; j ++) {
            result_sw[i * IMAGE_SIDE + j] = \
            filter[0] * image[(i - 1) * IMAGE_SIDE + j - 1] + filter[1] * image[(i - 1) * IMAGE_SIDE + j + 0] + filter[2] * image[(i - 1) * IMAGE_SIDE + j + 1] +
            filter[3] * image[(i + 0) * IMAGE_SIDE + j - 1] + filter[4] * image[(i + 0) * IMAGE_SIDE + j + 0] + filter[5] * image[(i + 0) * IMAGE_SIDE + j + 1] +
            filter[6] * image[(i + 1) * IMAGE_SIDE + j - 1] + filter[7] * image[(i + 1) * IMAGE_SIDE + j + 0] + filter[8] * image[(i + 1) * IMAGE_SIDE + j + 1];
        }
    }
    //---------------------------------------//

    //print_uart("CPU Execute: \t");
    //print_uart("\n\r");

    print_uart("------\n\r");
    examine_mem_hex((uint64_t)(result + IMAGE_SIDE + 1), (uint64_t)(result + IMAGE_SIDE + 1 + 10));

    print_uart("------\n\r");
    examine_mem_hex((uint64_t)(result_sw + IMAGE_SIDE + 1), (uint64_t)(result_sw + IMAGE_SIDE + 1 + 10));
}

void print_uart(char* str)
{
    uint32_t num = 0;
    
    while (str[num] != '\0') 
    {
        ++num;
    }

    uart_write_str(&__base_uart, str, num);
    uart_write_flush(&__base_uart);
}

void uart_print_unsigned_dec(uint32_t num)
{
    static uint8_t bin_to_hex_table[16] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    char buffer[20];
    int buffer_idx = 0;

    do
    {
        buffer[buffer_idx++] = bin_to_hex_table[num % 10];
        num /= 10;
    } while (num != 0);

    while(buffer_idx != 0)
    {
        uart_write(&__base_uart, buffer[--buffer_idx]);
    }
}

void examine_mem_hex(uint32_t ram_addr1, uint32_t ram_addr2)
{
	ram_addr1 &= 0xfffffffC;

    for(uint64_t addr = ram_addr1; addr < ram_addr2; addr += 8)
    {
		uart_print_unsigned_dec(addr);
		print_uart(": ");
		uart_print_unsigned_dec(*((volatile uint32_t*)addr));
		print_uart(" ");
		uart_print_unsigned_dec(*((volatile uint32_t*)(addr + 4)));
		print_uart("\n\r");

        if((addr / 8 + 1) % 4 == 0)
            print_uart("\n");
    }
}
