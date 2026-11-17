/****************************************************************************
 *
 *   Copyright (c) 2012-2018 PX4 Development Team. All rights reserved.
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
 * @file init.c
 *
 * HSRCF405_TF-specific early startup code.
 */

#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/tasks.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <errno.h>
#include <syslog.h>

#include <nuttx/board.h>
#include <nuttx/spi/spi.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/mmcsd.h>
#include <nuttx/analog/adc.h>
#include <nuttx/mm/gran.h>

#include <stm32.h>
#include "board_config.h"
#include <stm32_uart.h>

#include <arch/board/board.h>

#include <drivers/drv_hrt.h>
#include <drivers/drv_board_led.h>

#include <systemlib/px4_macros.h>

#include <px4_arch/io_timer.h>
#include <px4_platform_common/init.h>
#include <px4_platform/board_dma_alloc.h>

#if defined(FLASH_BASED_PARAMS)
#  include <parameters/flashparams/flashfs.h>
#endif

__BEGIN_DECLS
extern void led_init(void);
extern void led_on(int led);
extern void led_off(int led);
__END_DECLS

__EXPORT void board_peripheral_reset(int ms)
{
	UNUSED(ms);
}

__EXPORT void board_on_reset(int status)
{
	/* configure the PWM pins to outputs and keep them low */
	for (int i = 0; i < DIRECT_PWM_OUTPUT_CHANNELS; ++i) {
		px4_arch_configgpio(io_timer_channel_get_gpio_output(i));
	}

	if (status >= 0) {
		up_mdelay(400);
	}
}

__EXPORT void stm32_boardinitialize(void)
{
	/* Reset all PWM to low outputs */
	board_on_reset(-1);

	/* configure LEDs */
	board_autoled_initialize();

	/* configure ADC pins (ADC3) */
	stm32_configgpio(GPIO_ADC3_IN10);	/* RSSI */
	stm32_configgpio(GPIO_ADC3_IN11);	/* BATT_VOLTAGE */
	stm32_configgpio(GPIO_ADC3_IN13);	/* BATT_CURRENT */

	/* Beeper is active low: drive high (off) */
	stm32_configgpio(GPIO_BEEPER);

	/* PINIO outputs are active low: drive high (off) */
	stm32_configgpio(GPIO_PINIO1);
	stm32_configgpio(GPIO_PINIO2);

	/* configure SPI buses / chip selects */
	stm32_spiinitialize();
}

static struct spi_dev_s *spi_imu;
static struct spi_dev_s *spi_sd;

__EXPORT int board_app_initialize(uintptr_t arg)
{
	px4_platform_init();

	/* configure the DMA allocator */
	if (board_dma_alloc_init() < 0) {
		syslog(LOG_ERR, "DMA alloc FAILED\n");
	}

#if defined(SERIAL_HAVE_RXDMA)
	static struct hrt_call serial_dma_call;
	hrt_call_every(&serial_dma_call, 1000, 1000, (hrt_callout)stm32_serial_dma_poll, NULL);
#endif

	/* initial LED state */
	drv_led_start();
	led_off(LED_BLUE);

	if (board_hardfault_init(2, true) != 0) {
		led_on(LED_BLUE);
	}

	/* SPI1: ICM42605 */
	spi_imu = stm32_spibus_initialize(1);

	if (!spi_imu) {
		syslog(LOG_ERR, "[boot] FAILED to initialize SPI port 1\n");
		led_on(LED_BLUE);

	} else {
		SPI_SETFREQUENCY(spi_imu, 10 * 1000 * 1000);
		SPI_SETBITS(spi_imu, 8);
		SPI_SETMODE(spi_imu, SPIDEV_MODE3);
		up_udelay(20);
	}

	/* SPI3: microSD card */
	spi_sd = stm32_spibus_initialize(CONFIG_NSH_MMCSDSPIPORTNO);

	if (!spi_sd) {
		syslog(LOG_ERR, "[boot] FAILED to initialize SPI port %d\n", CONFIG_NSH_MMCSDSPIPORTNO);
		led_on(LED_BLUE);

	} else {
		SPI_SETFREQUENCY(spi_sd, 10 * 1000 * 1000);
		SPI_SETBITS(spi_sd, 8);
		SPI_SETMODE(spi_sd, SPIDEV_MODE3);
		up_udelay(20);

		int result = mmcsd_spislotinitialize(CONFIG_NSH_MMCSDMINOR, CONFIG_NSH_MMCSDSLOTNO, spi_sd);

		if (result != OK) {
			led_on(LED_BLUE);
			syslog(LOG_ERR, "[boot] FAILED to bind SPI port %d to the MMCSD driver\n",
			       CONFIG_NSH_MMCSDSPIPORTNO);
		}
	}

#if defined(FLASH_BASED_PARAMS)
	static sector_descriptor_t params_sector_map[] = {
		{1, 16 * 1024, 0x08004000},
		{0, 0, 0},
	};

	int result = parameter_flashfs_init(params_sector_map, NULL, 0);

	if (result != OK) {
		syslog(LOG_ERR, "[boot] FAILED to init params in FLASH %d\n", result);
		led_on(LED_AMBER);
	}

#endif

	/* Configure the HW based on the manifest */
	px4_platform_configure();

	return OK;
}
