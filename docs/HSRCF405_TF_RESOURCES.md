# HSRCF405_TF 飞控资源文档

**用途：** PX4 移植 / 板级配置  
**来源：** 运行中的 Betaflight 2025.12.1 CLI dump（原厂固件 `config rev: 5987fd5`），并结合重建 target 交叉校验。  
**参考设计：** HAKRCF405V2 / CADDXF4（引脚 38/39 吻合）。  
**注意：** 原理图 / Gerber 已丢失；以下为 MCU 引脚与外设级资源，不含电源树、连接器、无源器件、PCB 叠层。

## 1. 基本信息

| 项目 | 值 |
|---|---|
| 板名 | `HSRCF405_TF` |
| 厂商 ID | `HS` |
| MCU | STM32F405（推断 STM32F405RGT6，LQFP64） |
| Flash | 1 MB |
| RAM | 128 KB SRAM + 64 KB CCM + 4 KB backup SRAM |
| 主频 | 168 MHz |
| 时钟源 | HSI，无 HSE（`system_hse_mhz=0`，status 显示 `PLLP-HSI`） |
| USB | USB FS VCP，`0483:5740` |
| UID | `<redacted>` |
| 原厂固件 | Betaflight 2025.12.1，`config rev: 5987fd5` |
| 重建固件 | Betaflight 2025.12.1，`config rev: 5987fd5`，board name 一致 |

**PX4 重点：** 没有使用外部晶振（HSE）。PX4 板级时钟必须按 HSI 配置，否则 USB / 串口波特率可能异常。

## 2. 传感器

### 2.1 IMU：ICM42605（陀螺仪 + 加速度计）

| 信号 | 引脚 | 说明 |
|---|---|---|
| SPI | SPI1 | SCK PA5，MISO PA6，MOSI PA7 |
| CS | PA4 | 片选 |
| DRDY / EXTI | PC4 | 数据就绪中断 |
| WHO_AM_I | 0x42 | ICM42605 |

- 3.3 V 供电（按参考设计，需实测）。
- PX4：确认是否有原生 ICM42605 驱动；若无，ICM42688P 驱动可能兼容，但需核对 WHO_AM_I / 寄存器。

### 2.2 气压计：DPS310

| 信号 | 引脚 | 说明 |
|---|---|---|
| I2C1 SCL | PB8 | |
| I2C1 SDA | PB9 | |
| 7-bit 地址 | 0x76 | Betaflight 驱动固定 0x76；若 SDO 拉高可能为 0x77，需实测 |

- PX4 有 DPS310 驱动。

### 2.3 磁罗盘

- 板载无磁罗盘；I2C1（PB8/PB9）可外接。

## 3. 存储

| 项目 | 资源 |
|---|---|
| 内部 Flash | 1 MB |
| 外部 SPI Flash | 未发现（PA15 用于 SD 卡 CS） |
| SD 卡 | SPI3：SCK PC10，MISO PC11，MOSI PB5，CS PA15；无卡检测脚 |
| Betaflight SD 配置 | `sdcard_mode=SPI`，`sdcard_spi_bus=3`，`blackbox_device=SDCARD` |
| 实测 | 61 GB SD 卡识别正常 |

Betaflight 原始 Flash 布局（仅参考）：

- `0x08000000–0x08003FFF`：16 KB（ISR vector / FLASH）
- `0x08004000–0x08007FFF`：16 KB（FLASH_CONFIG）
- `0x08008000–0x080FFFFF`：992 KB（FLASH1 / 主固件）

PX4 会使用自己的 bootloader / 分区；刷 PX4 前保留 1 MB 原厂备份。

## 4. 通信接口

### 4.1 UART / USART

| 接口 | TX | RX | AF | 当前用途 / 备注 |
|---|---|---|---|---|
| UART1 | PB6 | PB7 | AF7 | 可用（GPS / Telemetry / RC） |
| UART2 | PA2 | PA3 | AF7 | 可用；PC0 是 RSSI ADC，不是反相器 |
| UART3 | PB10 | PB11 | AF7 | 可用（GPS / Telemetry / RC） |
| UART4 | PA0 | PA1 | AF8 | 当前 `MSP + VTX MSP`（131073），用于 MSP DisplayPort OSD |
| UART5 | PC12 | PD2 | AF8 | 可用 |
| UART6 | PC6 | PC7 | AF8 | 可用 |
| USB VCP | PA11 DM / PA12 DP | — | AF10 | `0483:5740` |

- 原配置 `feature RX_SERIAL` + `serialrx_provider=CRSF`，但没有给任何 UART 分配 `RX_SERIAL`，所以当前 `RXLOSS`。
- PX4 建议：RC 选 UART2/3/5/6；GPS / Telemetry 选 UART1/3/6；UART4 留给 MSP OSD。

### 4.2 SPI

| Bus | SCK | MISO | MOSI | CS | 设备 |
|---|---|---|---|---|---|
| SPI1 | PA5 | PA6 | PA7 | PA4 | ICM42605 |
| SPI2 | PB13 | PB14 | PB15 | PB12 | MAX7456（资源已定义；当前 OSD 用 MSP，芯片是否焊接需确认） |
| SPI3 | PC10 | PC11 | PB5 | PA15 | SD 卡 |

- 注意：HAKRC 参考设计 SPI3_MOSI 是 PC12；本板改为 PB5，PC12 用作 UART5_TX。

### 4.3 I2C

| Bus | SCL | SDA | 设备 |
|---|---|---|---|
| I2C1 | PB8 | PB9 | DPS310；可外接磁罗盘 |

### 4.4 USB / 调试

- USB FS：PA11 (DM)，PA12 (DP)，AF10。
- SWD：PA13 (SWDIO)，PA14 (SWCLK)，NRST。
- JTAG：PA15 (JTDI)、PB3 (JTDO)、PB4 (NJTRST) 被复用，JTAG 不可用，保留 SWD。
- BOOT0：标准启动脚（不在资源表内）。

## 5. 输出

### 5.1 电机

| Motor | 引脚 | 定时器 / 通道 | Betaflight DMA | 备注 |
|---|---|---|---|---|
| M1 | PC8 | TIM8_CH3 | DMA2 Stream 2 Ch 0 | |
| M2 | PC9 | TIM8_CH4 | DMA2 Stream 7 Ch 7 | |
| M3 | PA8 | TIM1_CH1 | DMA2 Stream 6 Ch 0 | |
| M4 | PA9 | TIM1_CH2 | DMA2 Stream 6 Ch 0 | |
| M5 | — | — | — | 本板无 M5 |
| M6 | PB4 | TIM3_CH1 | DMA1 Stream 4 Ch 5 | PB4 = NJTRST，需关 JTAG |
| M7 | PB0 | TIM3_CH3 | DMA1 Stream 7 Ch 5 | |
| M8 | PB1 | TIM3_CH4 | DMA1 Stream 2 Ch 5 | |

- 原配置 `motor_pwm_protocol=DSHOT600`。
- 7 路电机输出；PX4 `pwm_out` 按 7 通道配置。

### 5.2 LED

| 功能 | 引脚 | 说明 |
|---|---|---|
| LED0 | PC15 | 状态 LED |
| LED1 | PC14 | 状态 LED |
| LED_STRIP | PA10 | TIM1_CH3；原配置 32 颗 WS2812（4x8），GRB，亮度 16 |

- PC14 / PC15 复用为 LED，因此没有 32.768 kHz LSE 晶振；PX4 RTC 只能用 LSI 或外部 RTC。

### 5.3 蜂鸣器

| 功能 | 引脚 | 说明 |
|---|---|---|
| Beeper | PC13 | 反相驱动（`BEEPER_INVERTED`，`beeper_inversion=ON`，`beeper_od=OFF`） |

### 5.4 PINIO

| 功能 | 引脚 | 配置 | 备注 |
|---|---|---|---|
| PINIO1 | PC2 | 129（0x81，反相推挽） | box 40；参考设计标为 CAM 控制，未确认 |
| PINIO2 | PC5 | 129（0x81，反相推挽） | box 41；参考设计标为 VTX OFF，未确认 |

- 用途需按实物 / 参考设计确认后再在 PX4 中使用。

### 5.5 舵机 / PPM

- 当前未配置；PB3（TIM2_CH2）等定时器脚可作为预留。

## 6. ADC

| 信号 | 引脚 | ADC 通道 | Betaflight 设置 |
|---|---|---|---|
| VBAT | PC1 | ADC3_IN11 | `vbat_scale=112`，`vbat_divider=10`，`vbat_multiplier=1` |
| Current | PC3 | ADC3_IN13 | `ibata_scale=100`，`ibata_offset=0` |
| RSSI | PC0 | ADC3_IN10 | 原配置 `ADC_RSSI`；注意不是 UART2 反相器 |

- ADC 实例：ADC3；`ADC3_DMA_OPT=0`，`dma ADC 3 0`。
- `vbat_scale` / `ibata_scale` 是 Betaflight 标定值，不代表实际分压电阻 / 分流器；PX4 需实测后设置 `BAT1_V_DIV` / `BAT1_A_PER_V`。

## 7. 定时器与 DMA 映射

Betaflight `TIMER_PIN_MAPPING`（第 3 参数是同一引脚的定时器 occurrence，不是通道号）：

```c
TIMER_PIN_MAP( 0, PC8 , 2,  0)   // TIM8_CH3
TIMER_PIN_MAP( 1, PC9 , 2,  0)   // TIM8_CH4
TIMER_PIN_MAP( 2, PA8 , 1,  0)   // TIM1_CH1
TIMER_PIN_MAP( 3, PA9 , 1,  0)   // TIM1_CH2
TIMER_PIN_MAP( 4, PA10, 1,  0)   // TIM1_CH3
TIMER_PIN_MAP( 5, PB4 , 1,  0)   // TIM3_CH1
TIMER_PIN_MAP( 6, PB0 , 2,  0)   // TIM3_CH3
TIMER_PIN_MAP( 7, PB1 , 2,  0)   // TIM3_CH4
TIMER_PIN_MAP( 8, PB3 , 1,  0)   // TIM2_CH2（当前未用）
TIMER_PIN_MAP( 9, PB8 , 1,  0)   // TIM4_CH3（I2C1_SCL 复用）
TIMER_PIN_MAP(10, PB9 , 2, -1)   // TIM11_CH1（I2C1_SDA 复用，DMA 禁用）
```

DMA（Betaflight 实测）：

```text
ADC3: DMA2 Stream 0 Channel 2
PC8 : DMA2 Stream 2 Channel 0
PC9 : DMA2 Stream 7 Channel 7
PA8 : DMA2 Stream 6 Channel 0
PA9 : DMA2 Stream 6 Channel 0
PA10: DMA2 Stream 6 Channel 0
PB4 : DMA1 Stream 4 Channel 5
PB0 : DMA1 Stream 7 Channel 5
PB1 : DMA1 Stream 2 Channel 5
PB3 : DMA1 Stream 6 Channel 3
PB8 : DMA1 Stream 7 Channel 2
PB9 : NONE
```

- PX4 的 DMA 分配器 / `board_config.h` 可能与本表不同；把此表作为冲突参考。

## 8. 原始配置与备份

原厂固件全片备份、Betaflight 重建固件、完整 CLI dump 等文件包含本机唯一 ID 和原厂配置，**未包含在本仓库中**。

建议自行保留以下私有备份：

| 文件 | 用途 |
|---|---|
| `original_firmware_backup.bin` | 原始 1 MB 全片备份（Betaflight + 原配置） |
| `betaflight_2025.12.1_STM32F405_HSRCF405_TF.hex` | Configurator 用 |
| `original_full_dump.txt` / `original_diff_all.txt` | 原机 CLI dump |
| `restore_cli.txt` | 恢复原配置的 CLI 脚本 |

恢复 Betaflight（DFU，需自行准备备份文件）：

```bash
dfu-util -a 0 -s 0x08000000:leave -D original_firmware_backup.bin
```

## 9. PX4 移植注意事项

1. **时钟：** 无 HSE，`system_hse_mhz=0`，HSI 168 MHz；PX4 必须按 HSI 配置，否则 USB / 串口可能异常。
2. **Flash / RAM：** 1 MB / 128 KB SRAM + 64 KB CCM；注意 PX4 链接脚本和 bootloader 分区。
3. **外部 Flash：** 没有；日志用 SD 卡（SPI3）。PX4 需启用 SPI SD 卡支持（`sdspi` / board 配置）。
4. **IMU：** ICM42605（WHO_AM_I 0x42）；确认 PX4 驱动支持，必要时添加。
5. **Baro：** DPS310（I2C1，0x76），PX4 有驱动。
6. **OSD：** UART4 `MSP + VTX MSP`（当前 MSP DisplayPort）；PX4 可用 `msp_osd`。MAX7456 资源存在但当前未使用，是否焊接需确认。
7. **电机：** 7 路（M1–M4、M6–M8），M5 无；TIM8 / TIM1 / TIM3；PA10 是 LED strip，不是 M5。
8. **串口：** UART4 给 OSD；RC / Telemetry / GPS 用 UART1/2/3/5/6；UART2 的 PC0 是 ADC RSSI，不要当反相器。
9. **ADC：** VBAT PC1 / Current PC3 / RSSI PC0，ADC3；Betaflight 标定值仅作起点，需实测分压 / 分流。
10. **DMA：** 参考第 7 节；PA8 / PA9 / PA10 共用 DMA2 Stream 6，PX4 配置时注意冲突。
11. **JTAG / SWD：** PB3 / PB4 / PA15 被复用，JTAG 关闭；SWD PA13 / PA14 可用。
12. **PINIO：** PC2 / PC5，129 反相推挽；功能未确认，先在 PX4 中预留或实测。
13. **蜂鸣器：** PC13 反相；PX4 `tone_alarm` 需注意极性。
14. **LED：** PC15 / PC14 状态灯；PA10 WS2812（TIM1_CH3）；PC14 / PC15 占用 LSE 引脚，无 32.768 kHz 晶振。
15. **电源树 / 连接器 / 无源器件：** 固件无法恢复，需要实物测绘；原厂原理图 / Gerber 已丢失。

## 10. SWD 调试（JTAG 不可用）

### 10.1 含义

- STM32F405 的调试口有两套：JTAG（TDI/TDO/TMS/TCK/nTRST）和 SWD（SWDIO/SWCLK）。
- 本板 PB3(TDO)、PB4(nTRST)、PA15(TDI) 被复用为其他功能（PB4=M6，PA15=SD CS，PB3 预留），所以完整 JTAG 不可用。
- SWD 只用 PA13(SWDIO)、PA14(SWCLK)，这两个脚在本板没有被外设占用，因此 SWD 仍然可用。
- SWD 与 USB DFU 是两回事：DFU 通过 USB 刷写；SWD 通过 ST-Link/J-Link 等硬件探针调试/刷写。

### 10.2 接线

| 探针 | 飞控 |
|---|---|
| 3.3V / VTref | 3.3V（仅参考电平；飞控用 USB 供电时不要用探针供电） |
| GND | GND |
| SWDIO | PA13 |
| SWCLK | PA14 |
| NRST（可选） | NRST（建议接，便于 connect under reset） |

- 不要接 5V。
- `VTref/VREF` 是高阻输入，只采样目标板 3.3V 作为电平基准；`3.3V`/`5V` 丝印很多是探针的输出。飞控已供电时不要并接，避免两个稳压器倒灌。
- 板上通常有 4-pin SWD 焊盘/排针，丝印可能是 `3V3/GND/SWDIO/SWCLK` 或 `GND/3V3/DIO/CLK`；无丝印时用万用表从 MCU PA13/PA14 找通断。
- LQFP64 封装中 PA13/PA14 大约在第 46/49 脚（按实际封装核对）。

### 10.3 工具与命令

本机已安装：`openocd`、`st-flash`、`st-info`、`gdb-multiarch`。

ST-Link 快速检查/刷写：

```bash
st-info --probe
st-flash read backup.bin 0x08000000 0x100000
st-flash write firmware.bin 0x08000000
st-flash --connect-under-reset write firmware.bin 0x08000000
```

OpenOCD + GDB：

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
# 另一个终端
gdb-multiarch firmware.elf
(gdb) target extended-remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
```

OpenOCD 连不上时（低功耗/复位状态）：

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "reset_config srst_only srst_nogate connect_assert_srst"
```

并确认 NRST 已接。

### 10.4 注意事项

- 共地；SWD 是 3.3V 电平，禁止 5V。
- 飞控可用 USB 或电池供电；用探针 3.3V 只作参考，不要反灌。
- Betaflight/PX4 正常运行时不占用 PA13/PA14，SWD 可用；若固件进入低功耗，使用 connect-under-reset。
- 全 JTAG 需要 PB3/PB4/PA15，会与 M6/SD 卡冲突，不建议。

## 11. 参考

- Betaflight target 基线：HAKRCF405V2 / CADDXF4。
- 原机固件：Betaflight 2025.12.1 / `config rev: 5987fd5`。
- 本文件由 CLI dump 与重建 target 交叉校验生成。
