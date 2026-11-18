/****************************************************************************
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
 * 3. Neither the name PX4 nor the names of its contributors may be
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
 ****************************************************************************/

/**
 * @file board_config.h
 *
 * HSRCF405_TF internal definitions
 */

#pragma once

/****************************************************************************************************
 * Included Files
 ****************************************************************************************************/

#include <px4_platform_common/px4_config.h>
#include <nuttx/compiler.h>
#include <stdint.h>

/****************************************************************************************************
 * Definitions
 ****************************************************************************************************/

/* LEDs
 *
 * LED0 - PC15
 * LED1 - PC14
 *
 * Both are active high (Betaflight target does not define LEDx_INVERTED).
 */
#define GPIO_LED0       (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTC|GPIO_PIN15)
#define GPIO_LED1       (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTC|GPIO_PIN14)

#define GPIO_LED_BLUE   GPIO_LED0

#define BOARD_OVERLOAD_LED     LED_BLUE

#define FLASH_BASED_PARAMS

/* Beeper (PC13) is driven through an inverting transistor: low = on */
#define GPIO_BEEPER     (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTC|GPIO_PIN13)

/* PINIO (PC2 / PC5), active low push-pull as on the original target */
#define GPIO_PINIO1     (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTC|GPIO_PIN2)
#define GPIO_PINIO2     (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTC|GPIO_PIN5)

/*
 * ADC channels
 *
 * The board uses ADC3:
 *   PC0 -> ADC3_IN10 (RSSI)
 *   PC1 -> ADC3_IN11 (battery voltage)
 *   PC3 -> ADC3_IN13 (battery current)
 */
#define ADC3_CH(n)                  (n)

#define SYSTEM_ADC_BASE             STM32_ADC3_BASE

#define PX4_ADC_GPIO  \
	/* PC0 */  GPIO_ADC3_IN10,   \
	/* PC1 */  GPIO_ADC3_IN11,   \
	/* PC3 */  GPIO_ADC3_IN13

#define ADC_BATTERY_VOLTAGE_CHANNEL  ADC3_CH(11)
#define ADC_BATTERY_CURRENT_CHANNEL  ADC3_CH(13)
#define ADC_RC_RSSI_CHANNEL          ADC3_CH(10)

#define ADC_CHANNELS \
	((1 << ADC_BATTERY_VOLTAGE_CHANNEL) | \
	 (1 << ADC_BATTERY_CURRENT_CHANNEL) | \
	 (1 << ADC_RC_RSSI_CHANNEL))

/* Placeholder values derived from the Betaflight target. These MUST be
 * calibrated against the real resistor divider / current sensor.
 */
#define BOARD_BATTERY1_V_DIV         (11.0f)
#define BOARD_BATTERY1_A_PER_V       (100.0f)

/* PWM / motor outputs (8 channels)
 *
 * 0: PC8  TIM8_CH3   M1
 * 1: PC9  TIM8_CH4   M2
 * 2: PA8  TIM1_CH1   M3
 * 3: PA9  TIM1_CH2   M4
 * 4: PA10 TIM1_CH3   M5
 * 5: PB4  TIM3_CH1   M6
 * 6: PB0  TIM3_CH3   M7
 * 7: PB1  TIM3_CH4   M8
 *
 * PB3 (TIM2_CH2) is the onboard WS2812 LED strip data pin; PX4 does not
 * implement a WS2812 driver, so it is left unconfigured.
 */
#define BOARD_NUM_IO_TIMERS         3
#define DIRECT_PWM_OUTPUT_CHANNELS  8
#define BOARD_HAS_PWM               DIRECT_PWM_OUTPUT_CHANNELS

/* High-resolution timer. TIM4 is not used for PWM and no 32.768 kHz LSE
 * crystal is fitted (PC14/PC15 are LEDs).
 */
#define HRT_TIMER                   4
#define HRT_TIMER_CHANNEL           1

#define RC_SERIAL_PORT              "/dev/ttyS1"

/* This board provides a DMA pool and APIs */
#define BOARD_DMA_ALLOC_POOL_SIZE   5120

#define BOARD_HAS_ON_RESET          1

#define BOARD_ENABLE_CONSOLE_BUFFER
#define BOARD_CONSOLE_BUFFER_SIZE   1024

__BEGIN_DECLS

/****************************************************************************************************
 * Public Types
 ****************************************************************************************************/

/****************************************************************************************************
 * Public data
 ****************************************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************************************
 * Public Functions
 ****************************************************************************************************/

extern void stm32_spiinitialize(void);
extern void stm32_usbinitialize(void);
extern void board_peripheral_reset(int ms);

#include <px4_platform_common/board_common.h>

#endif /* __ASSEMBLY__ */

__END_DECLS
