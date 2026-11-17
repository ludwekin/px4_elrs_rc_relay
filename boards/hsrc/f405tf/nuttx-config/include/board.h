/************************************************************************************
 * nuttx-configs/hsrc_f405tf/include/board.h
 * include/arch/board/board.h
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 *   Copyright (c) 2026 HSRC Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************************/

#ifndef __CONFIG_HSRC_F405TF_INCLUDE_BOARD_H
#define __CONFIG_HSRC_F405TF_INCLUDE_BOARD_H

/************************************************************************************
 * Included Files
 ************************************************************************************/
#include "board_dma_map.h"

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
# include <stdint.h>
#endif

#include "stm32_rcc.h"
#include "stm32_sdio.h"
#include "stm32.h"

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* Clocking *************************************************************************/
/* The HSRCF405_TF has NO external high speed crystal (HSE). The original
 * Betaflight target runs from the internal 16 MHz HSI (system_hse_mhz=0).
 *
 * This configuration uses HSI as the PLL source:
 *   SYSCLK(Hz)                    : 168000000
 *   HCLK(Hz)                      : 168000000
 *   APB1 Prescaler                : 4            (PCLK1 = 42 MHz)
 *   APB2 Prescaler                : 2            (PCLK2 = 84 MHz)
 *   HSI Frequency(Hz)             : 16000000
 *   PLLM                          : 16           (VCO input = 1 MHz)
 *   PLLN                          : 336
 *   PLLP                          : 2
 *   PLLQ                          : 7            (USB 48 MHz)
 *   Flash Latency(WS)             : 5
 */

#define STM32_BOARD_USEHSI      1

#define STM32_HSI_FREQUENCY     16000000ul
#define STM32_LSI_FREQUENCY     32000
#define STM32_HSE_FREQUENCY     8000000ul   /* not fitted, kept for RCC compile */
#define STM32_LSE_FREQUENCY     32768       /* not fitted */

/* PLL source is HSI */
#define STM32_PLLCFG_PLLM       RCC_PLLCFG_PLLM(16)
#define STM32_PLLCFG_PLLN       RCC_PLLCFG_PLLN(336)
#define STM32_PLLCFG_PLLP       RCC_PLLCFG_PLLP_2
#define STM32_PLLCFG_PLLQ       RCC_PLLCFG_PLLQ(7)

#define STM32_SYSCLK_FREQUENCY  168000000ul

/* AHB clock (HCLK) is SYSCLK (168MHz) */
#define STM32_RCC_CFGR_HPRE     RCC_CFGR_HPRE_SYSCLK
#define STM32_HCLK_FREQUENCY    STM32_SYSCLK_FREQUENCY
#define STM32_BOARD_HCLK        STM32_HCLK_FREQUENCY

/* APB1 clock (PCLK1) is HCLK/4 (42MHz) */
#define STM32_RCC_CFGR_PPRE1    RCC_CFGR_PPRE1_HCLKd4
#define STM32_PCLK1_FREQUENCY   (STM32_HCLK_FREQUENCY/4)

/* Timers driven from APB1 will be twice PCLK1 */
#define STM32_APB1_TIM2_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM3_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM4_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM5_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM6_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM7_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM12_CLKIN  (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM13_CLKIN  (2*STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM14_CLKIN  (2*STM32_PCLK1_FREQUENCY)

/* APB2 clock (PCLK2) is HCLK/2 (84MHz) */
#define STM32_RCC_CFGR_PPRE2    RCC_CFGR_PPRE2_HCLKd2
#define STM32_PCLK2_FREQUENCY   (STM32_HCLK_FREQUENCY/2)

/* Timers driven from APB2 will be twice PCLK2 */
#define STM32_APB2_TIM1_CLKIN   (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM8_CLKIN   (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM9_CLKIN   (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM10_CLKIN  (2*STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM11_CLKIN  (2*STM32_PCLK2_FREQUENCY)

#define BOARD_TIM1_FREQUENCY    STM32_APB2_TIM1_CLKIN
#define BOARD_TIM2_FREQUENCY    STM32_APB1_TIM2_CLKIN
#define BOARD_TIM3_FREQUENCY    STM32_APB1_TIM3_CLKIN
#define BOARD_TIM4_FREQUENCY    STM32_APB1_TIM4_CLKIN
#define BOARD_TIM5_FREQUENCY    STM32_APB1_TIM5_CLKIN
#define BOARD_TIM6_FREQUENCY    STM32_APB1_TIM6_CLKIN
#define BOARD_TIM7_FREQUENCY    STM32_APB1_TIM7_CLKIN
#define BOARD_TIM8_FREQUENCY    STM32_APB2_TIM8_CLKIN
#define BOARD_TIM9_FREQUENCY    STM32_APB2_TIM9_CLKIN
#define BOARD_TIM10_FREQUENCY   STM32_APB2_TIM10_CLKIN
#define BOARD_TIM11_FREQUENCY   STM32_APB2_TIM11_CLKIN
#define BOARD_TIM12_FREQUENCY   STM32_APB1_TIM12_CLKIN
#define BOARD_TIM13_FREQUENCY   STM32_APB1_TIM13_CLKIN
#define BOARD_TIM14_FREQUENCY   STM32_APB1_TIM14_CLKIN

/* SDIO dividers (SDIO is not used on this board, kept for completeness) */
#define SDIO_INIT_CLKDIV        (118 << SDIO_CLKCR_CLKDIV_SHIFT)

#ifdef CONFIG_STM32_SDIO_DMA
#  define SDIO_MMCXFR_CLKDIV    (1 << SDIO_CLKCR_CLKDIV_SHIFT)
#else
#  define SDIO_MMCXFR_CLKDIV    (2 << SDIO_CLKCR_CLKDIV_SHIFT)
#endif

#ifdef CONFIG_STM32_SDIO_DMA
#  define SDIO_SDXFR_CLKDIV     (1 << SDIO_CLKCR_CLKDIV_SHIFT)
#else
#  define SDIO_SDXFR_CLKDIV     (2 << SDIO_CLKCR_CLKDIV_SHIFT)
#endif

/* LED definitions ******************************************************************/
/* If CONFIG_ARCH_LEDS is not defined, then the user can control the LEDs in any
 * way. The following definitions are used to access individual LEDs.
 */

#define BOARD_LED1        0
#define BOARD_LED2        1
#define BOARD_NLEDS       2

#define BOARD_LED_BLUE    BOARD_LED1
#define BOARD_LED_RED     BOARD_LED2

#define BOARD_LED1_BIT    (1 << BOARD_LED1)
#define BOARD_LED2_BIT    (1 << BOARD_LED2)

#define LED_STARTED       0  /* LED1 */
#define LED_HEAPALLOCATE  1  /* LED2 */
#define LED_IRQSENABLED   2  /* LED1 */
#define LED_STACKCREATED  3  /* LED1 + LED2 */
#define LED_INIRQ         4  /* LED1 */
#define LED_SIGNAL        5  /* LED2 */
#define LED_ASSERTION     6  /* LED1 + LED2 */
#define LED_PANIC         7  /* LED1 + LED2 */

/* Alternate function pin selections ************************************************/

/* USART1: TX PB6, RX PB7 */
#define GPIO_USART1_RX GPIO_USART1_RX_2
#define GPIO_USART1_TX GPIO_USART1_TX_2

/* USART2: TX PA2, RX PA3 */
#define GPIO_USART2_RX GPIO_USART2_RX_1
#define GPIO_USART2_TX GPIO_USART2_TX_1

/* USART3: TX PB10, RX PB11 */
#define GPIO_USART3_RX GPIO_USART3_RX_1
#define GPIO_USART3_TX GPIO_USART3_TX_1

/* UART4: TX PA0, RX PA1 */
#define GPIO_UART4_RX  GPIO_UART4_RX_1
#define GPIO_UART4_TX  GPIO_UART4_TX_1

/* UART5: TX PC12, RX PD2
 *
 * The legacy F4 pinmap already provides the only valid UART5 mapping, so
 * GPIO_UART5_TX / GPIO_UART5_RX are left to NuttX.
 */

/* USART6: TX PC6, RX PC7 */
#define GPIO_USART6_RX GPIO_USART6_RX_1
#define GPIO_USART6_TX GPIO_USART6_TX_1

/* SPI1: ICM42605 on PA4..PA7 */
#define GPIO_SPI1_SCK  GPIO_SPI1_SCK_1
#define GPIO_SPI1_MISO GPIO_SPI1_MISO_1
#define GPIO_SPI1_MOSI GPIO_SPI1_MOSI_1

/* SPI2: MAX7456 (not populated / not used) PB13..PB15, CS PB12 */
#define GPIO_SPI2_SCK  GPIO_SPI2_SCK_3
#define GPIO_SPI2_MISO GPIO_SPI2_MISO_3
#define GPIO_SPI2_MOSI GPIO_SPI2_MOSI_3

/* SPI3: SD card - SCK PC10, MISO PC11, MOSI PB5 (board specific) */
#define GPIO_SPI3_SCK  GPIO_SPI3_SCK_2
#define GPIO_SPI3_MISO GPIO_SPI3_MISO_2
#define GPIO_SPI3_MOSI GPIO_SPI3_MOSI_1

/* I2C1: SCL PB8, SDA PB9 */
#define GPIO_I2C1_SCL  GPIO_I2C1_SCL_2
#define GPIO_I2C1_SDA  GPIO_I2C1_SDA_2

#endif  /* __CONFIG_HSRC_F405TF_INCLUDE_BOARD_H */
