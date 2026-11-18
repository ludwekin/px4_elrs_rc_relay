# HSRCF405_TF 引脚/资源对照表

来源交叉核对：

- 原厂 Betaflight 2025.12.1 全片 dump / `diff all`（`config rev: 5987fd5`，board `HS / HSRCF405_TF`）
- `docs/HSRCF405_TF_RESOURCES.md`（引脚/外设重建文档）
- 本仓库 PX4 板级支持（`boards/hsrc/f405tf`）

> 注意：原厂全片 dump 和 `diff all` 含唯一 MCU ID，未放入本仓库；下表是从这些文件提取的引脚关系。

## 1. 主对照表

| MCU 引脚 | 原厂 BF 资源 | BF 定时器/AF | PX4 功能 | PX4 定时器/AF | 备注 |
|---|---|---|---|---|---|
| PA0  | `SERIAL_TX 4` | AF8 | UART4 TX（ttyS3） | AF8 | 原厂 UART4 = MSP + VTX MSP（OSD） |
| PA1  | `SERIAL_RX 4` | AF8 | UART4 RX（ttyS3） | AF8 | 同上 |
| PA2  | `SERIAL_TX 2` | AF7 | UART2 TX（ttyS1，备用） | AF7 | |
| PA3  | `SERIAL_RX 2` | AF7 | UART2 RX（ttyS1，备用） | AF7 | |
| PA4  | `GYRO_CS 1` | GPIO | SPI1 CS（ICM42605） | GPIO | |
| PA5  | `SPI_SCK 1` | AF5 | SPI1 SCK | AF5 | |
| PA6  | `SPI_SDI 1`（MISO） | AF5 | SPI1 MISO | AF5 | |
| PA7  | `SPI_SDO 1`（MOSI） | AF5 | SPI1 MOSI | AF5 | |
| PA8  | `MOTOR 3` | TIM1 CH1 / AF1 | 电机 M3 | TIM1 CH1 / AF1 | BF DMA2 S6 C0 |
| PA9  | `MOTOR 4` | TIM1 CH2 / AF1 | 电机 M4 | TIM1 CH2 / AF1 | BF DMA2 S6 C0 |
| PA10 | `LED_STRIP 1` | TIM1 CH3 / AF1 | LED 灯带（PX4 未实现） | TIM1 CH3 / AF1 | 原厂同时 `MOTOR 5 NONE`；见第 4 节 |
| PA11 | USB DM | — | USB FS DM | — | |
| PA12 | USB DP | — | USB FS DP | — | |
| PA13 | SWDIO | — | SWD | — | JTAG 关闭，保留 SWD |
| PA14 | SWCLK | — | SWD | — | |
| PA15 | `SDCARD_CS 1` | GPIO | SPI3 CS（SD 卡） | GPIO | JTDI；JTAG 关闭 |
| PB0  | `MOTOR 7` | TIM3 CH3 / AF2 | 电机 M7 | TIM3 CH3 / AF2 | BF DMA1 S7 C5 |
| PB1  | `MOTOR 8` | TIM3 CH4 / AF2 | 电机 M8 | TIM3 CH4 / AF2 | BF DMA1 S2 C5 |
| PB3  | 无资源（原厂有 `timer B03 AF1`） | TIM2 CH2 / AF1 | 预留 | — | JTDO；参考设计曾用作 LED_STRIP，原厂改到 A10 |
| PB4  | `MOTOR 6` | TIM3 CH1 / AF2 | 电机 M6 | TIM3 CH1 / AF2 | NJTRST；JTAG 关闭；BF DMA1 S4 C5 |
| PB5  | `SPI_SDO 3`（MOSI） | AF6 | SPI3 MOSI（SD 卡） | AF6 | **本板专用**；参考设计是 PC12 |
| PB6  | `SERIAL_TX 1` | AF7 | UART1 TX（ttyS0） | AF7 | 最终 PX4：RC / ELRS |
| PB7  | `SERIAL_RX 1` | AF7 | UART1 RX（ttyS0） | AF7 | 最终 PX4：RC / ELRS |
| PB8  | `I2C_SCL 1` | AF4（timer B08 AF2 = TIM4 CH3） | I2C1 SCL | AF4 | DPS310 / 外接磁罗盘 |
| PB9  | `I2C_SDA 1` | AF4（timer B09 AF3 = TIM11 CH1） | I2C1 SDA | AF4 | 同上 |
| PB10 | `SERIAL_TX 3` | AF7 | UART3 TX（ttyS2） | AF7 | GPS |
| PB11 | `SERIAL_RX 3` | AF7 | UART3 RX（ttyS2） | AF7 | GPS |
| PB12 | `OSD_CS 1` | GPIO | SPI2 CS（MAX7456，未使用） | GPIO | |
| PB13 | `SPI_SCK 2` | AF5 | SPI2 SCK（未使用） | AF5 | |
| PB14 | `SPI_SDI 2`（MISO） | AF5 | SPI2 MISO | AF5 | |
| PB15 | `SPI_SDO 2`（MOSI） | AF5 | SPI2 MOSI | AF5 | |
| PC0  | `ADC_RSSI 1` | ADC3 IN10 | ADC3 RSSI | ADC3_IN10 | |
| PC1  | `ADC_BATT 1` | ADC3 IN11 | ADC3 电池电压 | ADC3_IN11 | 原厂 `vbat_scale=112` |
| PC2  | `PINIO 1` | GPIO | PINIO1 | GPIO | 原厂 box 40；用途待确认 |
| PC3  | `ADC_CURR 1` | ADC3 IN13 | ADC3 电流 | ADC3_IN13 | 原厂 `ibata_scale=100` |
| PC4  | `GYRO_EXTI 1` | EXTI | ICM42605 DRDY | EXTI | |
| PC5  | `PINIO 2` | GPIO | PINIO2 | GPIO | 原厂 box 41；用途待确认 |
| PC6  | `SERIAL_TX 6` | AF8 | UART6 TX（ttyS5） | AF8 | 最终 PX4：MAVLink |
| PC7  | `SERIAL_RX 6` | AF8 | UART6 RX（ttyS5） | AF8 | 最终 PX4：MAVLink |
| PC8  | `MOTOR 1` | TIM8 CH3 / AF3 | 电机 M1 | TIM8 CH3 / AF3 | BF DMA2 S2 C0 |
| PC9  | `MOTOR 2` | TIM8 CH4 / AF3 | 电机 M2 | TIM8 CH4 / AF3 | BF DMA2 S7 C7 |
| PC10 | `SPI_SCK 3` | AF6 | SPI3 SCK（SD 卡） | AF6 | |
| PC11 | `SPI_SDI 3`（MISO） | AF6 | SPI3 MISO（SD 卡） | AF6 | |
| PC12 | `SERIAL_TX 5` | AF8 | UART5 TX（ttyS4，备用） | AF8 | 参考设计 SPI3_MOSI，本板改为 PB5 |
| PC13 | `BEEPER 1` | GPIO | 蜂鸣器 | GPIO | 原厂 `BEEPER_INVERTED`，PX4 低电平响 |
| PC14 | `LED 2` | GPIO | LED1（状态灯） | GPIO | |
| PC15 | `LED 1` | GPIO | LED0（状态灯） | GPIO | |
| PD2  | `SERIAL_RX 5` | AF8 | UART5 RX（ttyS4，备用） | AF8 | |

## 2. 电机输出

| 输出 | 引脚 | 原厂 BF | PX4 输出 | 定时器 | BF DMA |
|---|---|---|---|---|---|
| M1 | PC8 | MOTOR 1 | Motor1 | TIM8 CH3 | DMA2 S2 C0 |
| M2 | PC9 | MOTOR 2 | Motor2 | TIM8 CH4 | DMA2 S7 C7 |
| M3 | PA8 | MOTOR 3 | Motor3 | TIM1 CH1 | DMA2 S6 C0 |
| M4 | PA9 | MOTOR 4 | Motor4 | TIM1 CH2 | DMA2 S6 C0 |
| —  | PA10 | **MOTOR 5 NONE** | （LED_STRIP） | TIM1 CH3 | — |
| M5 | — | 无 | 无 | — | 本板无 M5 |
| M6 | PB4 | MOTOR 6 | Motor6 | TIM3 CH1 | DMA1 S4 C5 |
| M7 | PB0 | MOTOR 7 | Motor7 | TIM3 CH3 | DMA1 S7 C5 |
| M8 | PB1 | MOTOR 8 | Motor8 | TIM3 CH4 | DMA1 S2 C5 |

## 3. 串口映射

| 接口 | 引脚 | 原厂 BF | 最终 PX4 |
|---|---|---|---|
| USB VCP | PA11/PA12 | MSP / VCP | MAVLink（USB） |
| UART1 | PB6/PB7 | 未分配 | **RC / ELRS（CRSF）**，ttyS0 |
| UART2 | PA2/PA3 | 未分配 | 备用，ttyS1 |
| UART3 | PB10/PB11 | 未分配 | **GPS**，ttyS2 |
| UART4 | PA0/PA1 | **MSP + VTX MSP（OSD）**，131073 | OSD（PX4 因 RAM 未启用），ttyS3 |
| UART5 | PC12/PD2 | 未分配 | 备用，ttyS4 |
| UART6 | PC6/PC7 | 未分配 | **MAVLink**，ttyS5 |

## 4. LED_STRIP / MOTOR5 的关键差异

| 来源 | LED_STRIP | MOTOR 5 |
|---|---|---|
| 原厂 full dump / `diff all` | **A10** | **NONE** |
| 本仓库 PX4 / 重建 BF target | A10 | 无 M5 |
| 参考设计 HAKRCF405V2 / CADDXF4 | PB3 | PA10 |
| 曾经刷错的 BF 固件（参考 target） | PB3 | PA10 |

原因：本板把 WS2812 数据脚布在 **PA10（TIM1_CH3）**；参考设计把 PA10 当第 5 路电机，并用 PB3 做 LED_STRIP。原厂因此需要：

```text
resource MOTOR 5 NONE
resource LED_STRIP 1 A10
```

否则电机混控/DSHOT 会占用 PA10，LED 灯带无法工作。

## 5. 定时器与 DMA

原厂 BF `timer` 分配：

```text
C08 AF3   TIM8 CH3   (M1)
C09 AF3   TIM8 CH4   (M2)
A08 AF1   TIM1 CH1   (M3)
A09 AF1   TIM1 CH2   (M4)
A10 AF1   TIM1 CH3   (LED_STRIP)
B04 AF2   TIM3 CH1   (M6)
B00 AF2   TIM3 CH3   (M7)
B01 AF2   TIM3 CH4   (M8)
B03 AF1   TIM2 CH2   (原厂无资源占用)
B08 AF2   TIM4 CH3   (I2C1_SCL 复用)
B09 AF3   TIM11 CH1  (I2C1_SDA 复用)
```

原厂 BF DMA（实测）：

```text
ADC3 : DMA2 Stream 0 Channel 2
PC8  : DMA2 Stream 2 Channel 0
PC9  : DMA2 Stream 7 Channel 7
PA8  : DMA2 Stream 6 Channel 0
PA9  : DMA2 Stream 6 Channel 0
PA10 : DMA2 Stream 6 Channel 0
PB4  : DMA1 Stream 4 Channel 5
PB0  : DMA1 Stream 7 Channel 5
PB1  : DMA1 Stream 2 Channel 5
PB3  : DMA1 Stream 6 Channel 3
PB8  : DMA1 Stream 7 Channel 2
PB9  : NONE
```

PX4 DShot 使用“定时器更新 DMA”，与 BF 的“通道 DMA”不同：

```text
TIM8_UP : DMA2 Stream 1 Channel 7
TIM1_UP : DMA2 Stream 5 Channel 6
TIM3_UP : DMA1 Stream 2 Channel 5
```

PX4 其他 DMA：

```text
SPI1 RX : DMA2 Stream 0 Channel 3
SPI1 TX : DMA2 Stream 3 Channel 3
SPI3 RX : DMA1 Stream 0 Channel 0
SPI3 TX : DMA1 Stream 7 Channel 0
USART2 RX : DMA1 Stream 5 Channel 4
USART3 RX : DMA1 Stream 1 Channel 4
```

## 6. 其他资源

| 功能 | 引脚 | 原厂 BF | PX4 |
|---|---|---|---|
| 板载状态灯 1 | PC15 | `LED 1` | LED0 |
| 板载状态灯 2 | PC14 | `LED 2` | LED1 |
| WS2812 数据 | PA10 | `LED_STRIP 1` | 未实现（预留） |
| 蜂鸣器 | PC13 | `BEEPER 1`（反相） | 反相蜂鸣器 |
| PINIO1 | PC2 | `PINIO 1`（box 40） | PINIO1（预留） |
| PINIO2 | PC5 | `PINIO 2`（box 41） | PINIO2（预留） |
| SD 卡 CS | PA15 | `SDCARD_CS 1` | SPI3 CS |
| 陀螺仪 CS | PA4 | `GYRO_CS 1` | SPI1 CS |
| 陀螺仪 DRDY | PC4 | `GYRO_EXTI 1` | ICM42605 DRDY |
| OSD CS | PB12 | `OSD_CS 1` | SPI2 CS（未使用） |
| 调试 | PA13/PA14 | SWD | SWD |

原厂 BF 特性/串口：

```text
feature TELEMETRY
feature LED_STRIP
feature OSD
serial UART4 131073 115200 57600 0 115200   # MSP + VTX MSP
ledstrip_brightness = 16
vbat_scale = 112
```

原厂 LED 配置为 32 颗 WS2812（4×8 网格，索引 0–31），GRB，亮度 16；PX4 端因 RAM 限制未启用 WS2812 驱动。
