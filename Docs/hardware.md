# Minifly 硬件配置基线（面向 STM32CubeMX / CMake / CLion 重建）

## 1. 文档说明

本文档不是源码阅读笔记，而是为了后续将原始 Minifly 固件迁移/重建到 **STM32CubeMX + CMake + CLion** 时，作为硬件配置输入清单使用。

### 1.1 证据等级

- **明确**：源码中直接出现了 `GPIO_Init`、`GPIO_PinAFConfig`、`RCC_*ClockCmd`、`TIM/I2C/SPI/USART/ADC/USB` 初始化、`DMA`/`NVIC`/`EXTI` 绑定、或 FreeRTOS 宏配置。
- **推断**：通过调用链、模块用途、工程文件、启动文件等间接推断。
- **待确认**：同一资源存在多套方案、注释与实现不一致、或仅能证明“支持/可选”，不能证明“板上固定连接”。

### 1.2 主要证据来源

- `USER/Firmware_F411.uvprojx`
- `CORE/startup_stm32f411xe.s`
- `USER/system_stm32f4xx.c`
- `CONFIG/interface/config.h`
- `CONFIG/interface/FreeRTOSConfig.h`
- `HARDWARE/src/system.c`
- `HARDWARE/src/nvic.c`
- `HARDWARE/src/exti.c`
- `HARDWARE/src/led.c`
- `HARDWARE/src/motors.c`
- `HARDWARE/src/i2c_drv.c`
- `HARDWARE/src/spi.c`
- `HARDWARE/src/uart_syslink.c`
- `HARDWARE/src/uart1.c`
- `SYSTEM/usart/usart.c`
- `HARDWARE/src/ws2812.c`
- `HARDWARE/src/module_detect.c`
- `FLIGHT/src/sensors.c`
- `USB/USB_APP/src/usb_bsp.c`
- `EXP_MODULES/src/module_mgt.c`
- `EXP_MODULES/src/optical_flow.c`
- `EXP_MODULES/src/wifi_ctrl.c`
- `USER/main.c`

---

## 2. MCU 与系统基线

### 2.1 MCU 型号

- **MCU**：`STM32F411CEUx`
- **芯片族宏**：`STM32F411xE`
- **证据等级**：明确

证据：

- `USER/Firmware_F411.uvprojx:15` 指定 `<Device>STM32F411CEUx</Device>`
- `USER/system_stm32f4xx.c:291` 开始进入 `STM32F411xE` 条件分支
- 启动文件为 `CORE/startup_stm32f411xe.s`

### 2.2 存储器规模

- **Flash**：`0x08000000 ~ 0x0807FFFF`（512 KB）
- **SRAM**：`0x20000000 ~ 0x2001FFFF`（128 KB）
- **证据等级**：明确

证据：

- `USER/Firmware_F411.uvprojx:19`：`IRAM(0x20000000,0x20000) IROM(0x08000000,0x80000)`

### 2.3 启动与向量表

- 上电后由 `startup_stm32f411xe.s` 调用 `SystemInit()`，随后进入 `main()`。
- 固件运行时会在 `nvicInit()` 中重新设置向量表到应用区起始地址，而不是直接固定使用 `FLASH_BASE`。
- **证据等级**：明确

证据：

- `CORE/startup_stm32f411xe.s`：`Reset_Handler -> SystemInit -> __main`
- `HARDWARE/src/nvic.c:30-34`
- `CONFIG/interface/config.h:19-23`

### 2.4 Flash 布局

- Bootloader 预留：`16 KB`
- Config 参数区预留：`16 KB`
- 应用固件起始：`FLASH_BASE + 32 KB`
- **证据等级**：明确

证据：

- `CONFIG/interface/config.h:19-23`

即：

- `BOOTLOADER_SIZE = 16*1024`
- `CONFIG_PARAM_SIZE = 16*1024`
- `FIRMWARE_START_ADDR = FLASH_BASE + 0x8000`

### 2.5 系统初始化顺序

硬件初始化入口位于 `systemInit()`：

1. `nvicInit()`
2. `extiInit()`
3. `delay_init(96)`
4. `ledInit()`
5. `ledseqInit()`
6. `commInit()`
7. `atkpInit()`
8. `consoleInit()`
9. `configParamInit()`
10. `pmInit()`
11. `stabilizerInit()`
12. `expModuleDriverInit()`
13. `watchdogInit()`

证据：`HARDWARE/src/system.c:21-61`

---

## 3. 时钟树配置

### 3.1 时钟结论

对于当前 F411 固件，源码的有效目标配置是：

| 项目 | 值 | 证据等级 |
|---|---:|---|
| SYSCLK | 96 MHz | 明确 |
| HCLK | 96 MHz | 明确 |
| APB1(PCLK1) | 48 MHz | 明确 |
| APB2(PCLK2) | 96 MHz | 明确 |
| PLL source | HSE 8 MHz（源码意图） | 待确认 |
| PLL_M | 8 | 明确 |
| PLL_N | 192 | 明确 |
| PLL_P | 2 | 明确 |
| PLL_Q | 4 | 明确 |
| USB FS clock | 48 MHz | 明确 |
| Flash Latency | 2 WS | 明确 |

### 3.2 关键源码证据

- `USER/system_stm32f4xx.c:299`：定义 `USE_HSE_BYPASS`
- `USER/system_stm32f4xx.c:318-322`：F411 在 `USE_HSE_BYPASS` 下 `PLL_M = 8`
- `USER/system_stm32f4xx.c:326`：`PLL_Q = 4`
- `USER/system_stm32f4xx.c:347-349`：`PLL_N = 192`, `PLL_P = 2`
- `USER/system_stm32f4xx.c:382-383`：`SystemCoreClock = 96000000`
- `USER/system_stm32f4xx.c:699-706`：`HPRE=1`, `PPRE2=1`, `PPRE1=2`
- `USER/system_stm32f4xx.c:721`：`FLASH_ACR_LATENCY_2WS`
- `CONFIG/interface/FreeRTOSConfig.h:103`：`configCPU_CLOCK_HZ = 96000000`
- `HARDWARE/interface/motors.h:23`：`TIM_CLOCK_HZ = 96000000`
- `HARDWARE/src/system.c:27`：`delay_init(96)`

### 3.3 时钟树闭环说明

如果以 `HSE = 8 MHz` 计算：

- PLL 输入：`8 / 8 = 1 MHz`
- VCO：`1 * 192 = 192 MHz`
- SYSCLK：`192 / 2 = 96 MHz`
- USB：`192 / 4 = 48 MHz`

这与：

- `SystemCoreClock = 96 MHz`
- `configCPU_CLOCK_HZ = 96 MHz`
- `delay_init(96)`
- USB FS 需要 48 MHz

是完全一致的。

### 3.4 `USE_HSE_BYPASS` 的真实含义

这里需要谨慎：

- `USER/system_stm32f4xx.c:292-299` 的注释来自 ST Nucleo 模板，描述的是“通过 ST-LINK MCO 输入 8 MHz HSE bypass”。
- 但当前工程虽然**定义了** `USE_HSE_BYPASS`，在 F411 分支实际启振代码却只执行了 `RCC_CR_HSEON`，而没有同时置 `HSEBYP`（`RCC_CR_HSEBYP` 被注释掉，见 `USER/system_stm32f4xx.c:672-675`）。

因此目前可以下结论：

- **代码明确按 8 MHz 外部时钟源参数配置 PLL**。
- **但板级究竟是外部有源时钟、晶振、还是模板迁移残留，需要后续结合原理图/PCB 再确认。**
- 在 CubeMX 中，**可以先按 HSE=8 MHz、SYSCLK=96 MHz 重建**；是否勾选 **Bypass Clock Source** 需要原理图二次确认。

**证据等级**：待确认

### 3.5 CubeMX 时钟配置建议

在 CubeMX 中建议先这样配置：

1. MCU 选 `STM32F411CEUx`
2. RCC：
   - 先尝试 `HSE = Crystal/Ceramic Resonator` 或 `Bypass Clock Source` 二选一
   - 根据原理图最终决定
3. PLL：
   - Source = HSE 8 MHz
   - `PLLM = 8`
   - `PLLN = 192`
   - `PLLP = 2`
   - `PLLQ = 4`
4. AHB Prescaler = `/1`
5. APB1 Prescaler = `/2`
6. APB2 Prescaler = `/1`
7. USB OTG FS 观察是否得到 48 MHz
8. Flash Latency = 2 WS

---

## 4. FreeRTOS / 中断系统基线

### 4.1 FreeRTOS 基本配置

| 配置项 | 值 | 证据 |
|---|---:|---|
| FreeRTOS 版本 | V9.0.0 | `CONFIG/interface/FreeRTOSConfig.h:1-2` |
| CPU Clock | 96 MHz | `FreeRTOSConfig.h:103` |
| Tick Rate | 1000 Hz | `FreeRTOSConfig.h:104` |
| Max Priorities | 10 | `FreeRTOSConfig.h:105` |
| Minimal Stack | 150 words | `FreeRTOSConfig.h:106` |
| Heap model | 动态分配 | `FreeRTOSConfig.h:127-128` |
| Heap size | 30 KB | `FreeRTOSConfig.h:128` |
| Idle Hook | 启用 | `FreeRTOSConfig.h:133` |
| Tick Hook | 关闭 | `FreeRTOSConfig.h:134` |
| Software Timers | 启用 | `FreeRTOSConfig.h:152-155` |

### 4.2 异常处理归属

- `PendSV_Handler` 由 FreeRTOS port 接管
- `SVC_Handler` 由 FreeRTOS port 接管
- `SysTick_Handler` 使用项目自定义实现，在调度器启动前累加 `sysTickCnt`，启动后转发到 `xPortSysTickHandler()`

证据：

- `CONFIG/interface/FreeRTOSConfig.h:187-188`
- `HARDWARE/src/nvic.c:42-51`

### 4.3 中断优先级边界

- `configPRIO_BITS = 4`
- `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15`
- `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`
- `NVIC_PriorityGroup = 4`

这意味着：

- 数字越小优先级越高
- 调用 FreeRTOS ISR API 的中断优先级应不高于 5

证据：

- `CONFIG/interface/FreeRTOSConfig.h:173-182`
- `HARDWARE/src/nvic.c:30-34`

### 4.4 任务入口

- `main()` 在 `systemInit()` 后创建 `startTask`
- `startTask` 再创建主要业务任务：radio、USB、ATKP、config、pm、sensors、stabilizer、expModule

证据：

- `USER/main.c:20-57`

### 4.5 Idle Hook 行为

Idle Hook 中：

- 周期性喂狗
- 执行 `__WFI()` 进入低功耗等待中断

证据：`USER/main.c:60-72`

---

## 5. Pinout 总表

> 说明：以下只列出已在源码中出现明确用途或高度相关的引脚。未出现的引脚不应在 CubeMX 中擅自启用。

| 引脚 | GPIO模式 | AF/外设 | 方向 | 上下拉 | 速度 | 默认电平/极性 | 代码证据 | 证据等级 | CubeMX备注 |
|---|---|---|---|---|---|---|---|---|---|
| PA0 | GPIO Input + EXTI0 | Syslink TXEN flowctrl | 输入 | UP | - | 高低沿触发 | `uart_syslink.c:141-157`, `uart_syslink.h:39-42` | 明确 | GPIO_Input + EXTI0 |
| PA2 | AF | USART2_TX | 输出 | 未显式单独写，沿用结构体 | 25MHz | - | `uart_syslink.h:32-37`, `uart_syslink.c:108-117` | 明确 | USART2 TX |
| PA3 | AF | USART2_RX | 输入 | UP | 默认 | - | `uart_syslink.h:32-37`, `uart_syslink.c:102-117` | 明确 | USART2 RX |
| PA4 | GPIO Input + EXTI4 | IMU data ready | 输入 | DOWN | - | Rising | `sensors.c:120-140` | 明确 | GPIO_Input + EXTI4 |
| PA5 | AF | TIM2_CH1 / MOTOR4 | 输出 | UP | 100MHz | 高有效 PWM | `motors.c:44-45,54-55,73-74` | 明确 | TIM2 CH1 PWM Generation |
| PA6 | GPIO Output | LED_GREEN_L | 输出 | 未显式 | 25MHz | 负逻辑 | `led.c:35,53-58` | 明确 | GPIO_Output |
| PA7 | GPIO Output | LED_RED_L | 输出 | 未显式 | 25MHz | 负逻辑 | `led.c:36,53-58` | 明确 | GPIO_Output |
| PA8 | AF / GPIO | I2C3_SCL；也被光流模块作为 NCS 使用 | 双重用途 | I2C 开漏；模块代码直接宏控 | 50MHz/未明 | 冲突 | `i2c_drv.c:138-146`, `optical_flow.c:33` | 待确认 | 与扩展模块复用，需按实际模块策略处理 |
| PA9 | AF | USART1_TX | 输出 | UP | 50MHz | - | `SYSTEM/usart/usart.c:84-103` | 明确 | 传统 USART1 TX |
| PA10 | AF | USART1_RX | 输入 | UP | 50MHz | - | `SYSTEM/usart/usart.c:84-103` | 明确 | 传统 USART1 RX |
| PA11 | AF | USB_OTG_FS_DM | 双向 | NOPULL | 100MHz | - | `usb_bsp.c:44-57` | 明确 | USB_OTG_FS DM |
| PA12 | AF | USB_OTG_FS_DP | 双向 | NOPULL | 100MHz | - | `usb_bsp.c:44-57` | 明确 | USB_OTG_FS DP |
| PB0 | GPIO Output | 头灯 / 光流电源 / WiFi相关复用 | 输出 | NOPULL | 默认 | 模块相关 | `ws2812.c:50-56`, `optical_flow.c:34`, `wifi_ctrl.c` | 待确认 | 扩展模块复用脚 |
| PB1 | Analog | ADC1_IN9 模块识别 | 输入 | NOPULL | - | - | `module_detect.c:49-53,89` | 明确 | ADC1 IN9 + DMA2 Stream0 |
| PB3 | AF | USART1_RX（替代方案） | 输入 | UP | 默认 | - | `uart1.c:46-61` | 待确认 | 与 PA9/PA10 方案冲突 |
| PB4 | AF | I2C3_SDA / TIM3_CH1(WS2812) | 双重用途 | I2C OD / NOPULL | 25MHz | 冲突 | `i2c_drv.c:142-146`, `ws2812.c:58-65` | 待确认 | 核心冲突脚，不能同时启用 |
| PB5 | GPIO Output | WS2812/LED Ring 电源控制 | 输出 | NOPULL | 默认 | 上电置高 | `ws2812.c:42-49` | 明确 | GPIO_Output |
| PB6 | AF | TIM4_CH1 / MOTOR2 | 输出 | UP | 100MHz | 高有效 PWM | `motors.c:43,47-52,72` | 明确 | TIM4 CH1 PWM |
| PB7 | AF | TIM4_CH2 / MOTOR1 | 输出 | UP | 100MHz | 高有效 PWM | `motors.c:42,47-52,71` | 明确 | TIM4 CH2 PWM |
| PB8 | AF | I2C1_SCL | 双向 | 开漏 | 50MHz | - | `i2c_drv.c:106-114,267-275` | 明确 | I2C1 SCL |
| PB9 | AF | I2C1_SDA | 双向 | 开漏 | 50MHz | - | `i2c_drv.c:110-114,267-275` | 明确 | I2C1 SDA |
| PB10 | AF | TIM2_CH3 / MOTOR3 | 输出 | UP | 100MHz | 高有效 PWM | `motors.c:44,47-52,73` | 明确 | TIM2 CH3 PWM |
| PB12 | GPIO Output | LED_BLUE_L | 输出 | 未显式 | 25MHz | 正逻辑 | `led.c:34,60-62` | 明确 | GPIO_Output |
| PB13 | AF | SPI2_SCK | 输出 | DOWN | 50MHz | - | `spi.c:51-56,103-114` | 明确 | SPI2 SCK |
| PB14 | AF | SPI2_MISO | 输入 | DOWN | 50MHz | - | `spi.c:57-62,120-122` | 明确 | SPI2 MISO |
| PB15 | AF | SPI2_MOSI | 输出 | DOWN | 50MHz | - | `spi.c:63-67,116-118` | 明确 | SPI2 MOSI |
| PC13 | GPIO Output | LED_GREEN_R | 输出 | 未显式 | 25MHz | 负逻辑 | `led.c:37,64-66` | 明确 | GPIO_Output |
| PC14 | GPIO Output | LED_RED_R | 输出 | 未显式 | 25MHz | 负逻辑 | `led.c:38,64-66` | 明确 | GPIO_Output |
| PA15 | AF（注释中） | USART1_TX 替代方案 | 输出 | 未启用 | - | - | `uart1.c:53-61` | 待确认 | 代码注释掉，当前未启用 |

---

## 6. 按外设展开

### 6.1 LEDs

#### 板载状态灯

| 逻辑名 | 引脚 | 极性 |
|---|---|---|
| LED_BLUE_L | PB12 | 正逻辑 |
| LED_GREEN_L | PA6 | 负逻辑 |
| LED_RED_L | PA7 | 负逻辑 |
| LED_GREEN_R | PC13 | 负逻辑 |
| LED_RED_R | PC14 | 负逻辑 |

证据：`HARDWARE/src/led.c:32-39`

CubeMX 建议：

- 配置为普通 GPIO Output
- 不需要 AF / DMA / EXTI
- 逻辑极性需要在软件层处理

### 6.2 Motors / PWM

- 电机数量：4
- 定时器：`TIM4 + TIM2`
- PWM 位宽：8-bit
- PWM 基准时钟：96 MHz
- Prescaler：0
- Period：255
- 推导 PWM 频率：`96MHz / 256 = 375kHz`
- 证据等级：明确

证据：

- `HARDWARE/interface/motors.h:22-26`
- `HARDWARE/src/motors.c:30-87`

映射：

| 电机 | 引脚 | 定时器通道 |
|---|---|---|
| M1 | PB7 | TIM4_CH2 |
| M2 | PB6 | TIM4_CH1 |
| M3 | PB10 | TIM2_CH3 |
| M4 | PA5 | TIM2_CH1 |

CubeMX 建议：

- TIM2：PWM CH1、CH3
- TIM4：PWM CH1、CH2
- 所有通道极性 `High`
- Prescaler=0，Counter Period=255

### 6.3 I2C1 传感器总线

- 外设：`I2C1`
- SCL：`PB8`
- SDA：`PB9`
- 时钟：`400 kHz`
- DMA：`DMA1_Stream0` RX, `DMA_Channel_1`
- IRQ：`I2C1_EV_IRQn`, `I2C1_ER_IRQn`
- DMA IRQ 优先级：6
- I2C IRQ 优先级：7
- 证据等级：明确

证据：

- `HARDWARE/src/i2c_drv.c:99-121`
- `HARDWARE/src/i2c_drv.c:214-237`
- `HARDWARE/src/i2c_drv.c:242-301`

传感器使用：

- MPU6500/9250
- AK8963（条件编译）
- BMP280 或 SPL06

证据：`FLIGHT/src/sensors.c:143-214`

CubeMX 建议：

- I2C1 Fast Mode 400k
- PB8/PB9 AF4 Open Drain
- 开启 I2C1 event/error interrupt
- 如要尽量接近原工程，补上 RX DMA

### 6.4 I2C3 扩展总线

- 外设：`I2C3`
- SCL：`PA8`
- SDA：`PB4`
- 时钟：`400 kHz`
- DMA：`DMA1_Stream2` RX, `DMA_Channel_3`
- IRQ：`I2C3_EV_IRQn`, `I2C3_ER_IRQn`
- 证据等级：明确（但引脚与 WS2812 冲突）

证据：`HARDWARE/src/i2c_drv.c:131-153`

CubeMX 备注：

- 该总线是扩展模块/Deck 总线定义
- 但 `PB4` 同时被 LED Ring 使用，实际板型/模块形态上很可能不是同时启用

### 6.5 SPI2

- 外设：`SPI2`
- SCK/MISO/MOSI：`PB13/PB14/PB15`
- 模式：Master, Full Duplex, 8-bit, CPOL Low, CPHA 1Edge, NSS Soft
- Prescaler 宏：`SPI_BAUDRATE_2MHZ = Prescaler_32`
- 结合 48 MHz APB1 timer/peripheral 背景注释，实际约 `1.5 MHz`
- DMA：
  - TX：`DMA1_Stream4`, Channel 0
  - RX：`DMA1_Stream3`, Channel 0
- DMA IRQ 优先级：9
- 证据等级：明确

证据：

- `HARDWARE/src/spi.c:28-67`
- `HARDWARE/src/spi.c:127-139`
- `HARDWARE/src/spi.c:145-189`
- `HARDWARE/interface/spi.h:19-25`

CubeMX 建议：

- SPI2 Master Full Duplex
- NSS Software
- Baudrate Prescaler 32
- 视需要启用 DMA RX/TX

### 6.6 USART2 / Syslink

- 外设：`USART2`
- TX/RX：`PA2/PA3`
- 波特率：`1000000`
- 数据格式：8N1
- TX DMA：`DMA1_Stream6`, Channel 4
- USART IRQ 优先级：5
- DMA IRQ 优先级：7
- 另有 `PA0 -> EXTI0` 作为 TXEN flow control 输入
- 证据等级：明确

证据：

- `HARDWARE/interface/uart_syslink.h:19-42`
- `HARDWARE/src/uart_syslink.c:51-81`
- `HARDWARE/src/uart_syslink.c:84-160`

CubeMX 建议：

- USART2 Async
- PA2 TX / PA3 RX
- 1,000,000 baud
- PA0 配 GPIO_Input + EXTI0
- 若需贴近原工程，启用 TX DMA

### 6.7 USART1（两套方案）

#### 方案 A：传统 USART1 调试串口

- 引脚：`PA9/PA10`
- 模式：全双工 RX/TX
- 证据：`SYSTEM/usart/usart.c:84-103`
- 证据等级：明确

#### 方案 B：WiFi 模块串口

- 引脚：`PB3` 作为 RX
- `PA15` 作为 TX 的代码被注释掉
- `USART_Mode = USART_Mode_Rx`
- 波特率由 `uart1Init(19200)` 传入
- 证据：`HARDWARE/src/uart1.c:34-85`, `EXP_MODULES/src/wifi_ctrl.c:65-69`
- 证据等级：待确认

结论：

- 该代码库中 **USART1 存在两套互斥映射/用途**。
- `SYSTEM/usart/usart.c` 更像通用/传统串口初始化；`HARDWARE/src/uart1.c` 是扩展 WiFi 模块专用接收方案。
- 后续 CubeMX 重建应先确认：
  - 是否真的保留 WiFi 模块功能
  - 如果保留，是否仍用 `PB3(+PA15)` 方案
  - 如果只做基础飞控，可能只需保留 `PA9/PA10`

### 6.8 USB OTG FS

- 引脚：`PA11/PA12`
- AF：`GPIO_AF_OTG1_FS`
- OTG FS 时钟：启用
- IRQ：`OTG_FS_IRQn`
- IRQ 优先级：10
- 证据等级：明确

证据：

- `USB/USB_APP/src/usb_bsp.c:38-58`
- `USB/USB_APP/src/usb_bsp.c:65-78`

CubeMX 建议：

- USB_OTG_FS Device_Only 或按后续需求设置
- 确保 48 MHz 时钟可用

### 6.9 ADC1 / Module Detect

- 输入引脚：`PB1`
- ADC 通道：`ADC_Channel_9` = `ADC1_IN9`
- 连续转换，12-bit
- Sample time：480 cycles
- DMA：`DMA2_Stream0`, Channel 0, Circular
- 证据等级：明确

证据：`HARDWARE/src/module_detect.c:38-93`

用途：

- 检测扩展模块插入类型（LED_RING / WIFI_CAMERA / OPTICAL_FLOW / MODULE1）

阈值：

| 模块 | ADC 参考值 |
|---|---:|
| LED_RING | 2048 |
| WIFI_CAMERA | 4095 |
| OPTICAL_FLOW | 2815 |
| MODULE1 | 1280 |

证据：`HARDWARE/src/module_detect.c:20-27`

### 6.10 EXTI / 传感器中断

全局 EXTI NVIC 初始化：

| IRQ | 优先级 |
|---|---:|
| EXTI0_IRQn | 5 |
| EXTI1_IRQn | 12 |
| EXTI2_IRQn | 12 |
| EXTI3_IRQn | 12 |
| EXTI4_IRQn | 12 |
| EXTI9_5_IRQn | 12 |
| EXTI15_10_IRQn | 10 |

证据：`HARDWARE/src/exti.c:21-63`

#### IMU 中断

- 引脚：`PA4`
- Line：`EXTI_Line4`
- Trigger：Rising
- 证据：`FLIGHT/src/sensors.c:120-141`

#### Syslink TXEN 中断

- 引脚：`PA0`
- Line：`EXTI_Line0`
- Trigger：Rising/Falling
- 证据：`HARDWARE/src/uart_syslink.c:149-157`

### 6.11 WS2812 / Headlights / Power Control

该部分属于 LED Ring 扩展模块：

| 功能 | 引脚 | 说明 |
|---|---|---|
| RGB data | PB4 | TIM3_CH1 + DMA |
| Headlights | PB0 | 普通 GPIO 输出 |
| Power enable | PB5 | 普通 GPIO 输出 |

关键参数：

- Timer：`TIM3_CH1`
- PWM Period：119（800 kHz）
- DMA：`DMA1_Stream4`, Channel 5
- DMA IRQ 优先级：9

证据：`HARDWARE/src/ws2812.c:30-123`

注意：

- `PB4` 与 I2C3 SDA 冲突
- `DMA1_Stream4` 与 SPI2 TX 共享中断入口，由 `nvic.c` 中根据 `getModuleID()` 分流

证据：`HARDWARE/src/nvic.c:232-259`

### 6.12 扩展模块管理

扩展模块由 `expModuleMgtTask()` 周期检测：

- 检测周期：500 tick 软件定时器
- 通过 `getModuleDriverID()` 读取 ADC 检测结果
- 根据模块类型初始化：
  - `ledring12Init()`
  - `wifiModuleInit()`
  - `opticalFlowInit()`

证据：`EXP_MODULES/src/module_mgt.c:62-104`

这说明：

- 一些外设复用不是“同一板上固定共存”，而是“扩展模块插入后动态启用”
- 因而 CubeMX 迁移时，最好区分：
  - 主板固定资源
  - 扩展模块条件资源

### 6.13 光流模块

光流模块关键点：

- 使用 `SPI2`
- `PA8` 作为 NCS 宏控制
- `PB0` 作为电源控制宏

证据：`EXP_MODULES/src/optical_flow.c:33-35`

这进一步说明：

- `PA8` 并非始终可作为 I2C3_SCL 使用
- 在扩展模块场景下可能被当作普通 GPIO 片选使用

**证据等级**：待确认（因为这是模块态复用，不一定与 deck 总线同时存在）

---

## 7. DMA / NVIC 资源占用矩阵

### 7.1 DMA

| 外设/用途 | DMA 控制器/流/通道 | 方向 | 证据等级 |
|---|---|---|---|
| I2C1 RX | DMA1_Stream0 / Ch1 | Peripheral -> Memory | 明确 |
| I2C3 RX | DMA1_Stream2 / Ch3 | Peripheral -> Memory | 明确 |
| SPI2 RX | DMA1_Stream3 / Ch0 | Peripheral -> Memory | 明确 |
| SPI2 TX | DMA1_Stream4 / Ch0 | Memory -> Peripheral | 明确 |
| WS2812 | DMA1_Stream4 / Ch5 | Memory -> Peripheral | 明确 |
| USART2 TX | DMA1_Stream6 / Ch4 | Memory -> Peripheral | 明确 |
| ADC1 | DMA2_Stream0 / Ch0 | Peripheral -> Memory | 明确 |

### 7.2 DMA 冲突

`DMA1_Stream4` 被两处使用：

- SPI2 TX
- WS2812

源码通过 `HARDWARE/src/nvic.c:232-259` 在同一个 `DMA1_Stream4_IRQHandler()` 里，根据当前扩展模块类型动态分流。

结论：

- 这不是 CubeMX 可以直接自动表达的“静态无冲突配置”
- 需要在重构时决定：
  1. 保持模块互斥复用
  2. 重新换脚/换 DMA 方案

### 7.3 NVIC 重点资源

| IRQ | 用途 | 优先级 |
|---|---|---:|
| SysTick | RTOS tick / 启动前时基 | RTOS 接管 |
| EXTI0 | Syslink TXEN | 5 |
| EXTI4 | IMU DRDY | 12 |
| USART2_IRQn | Syslink | 5 |
| DMA1_Stream6_IRQn | USART2 TX DMA | 7 |
| I2C1_EV/ER | 传感器总线 | 7 |
| I2C3_EV/ER | Deck 总线 | 7 |
| DMA1_Stream0_IRQn | I2C1 RX DMA | 6 |
| DMA1_Stream2_IRQn | I2C3 RX DMA | 6 |
| DMA1_Stream3_IRQn | SPI2 RX DMA | 9 |
| DMA1_Stream4_IRQn | SPI2 TX / WS2812 | 9 |
| OTG_FS_IRQn | USB FS | 10 |
| USART1_IRQn | WiFi UART1 路径 | 7 或 15（取决于哪套代码） |

备注：`USART1` 在两套实现中优先级不同，因此也属于待确认项。

---

## 8. 片上资源占用汇总

| 资源 | 当前用途 |
|---|---|
| TIM2 | MOTOR3 / MOTOR4 PWM |
| TIM3 | WS2812 RGB 数据 |
| TIM4 | MOTOR1 / MOTOR2 PWM |
| SPI2 | 光流模块 |
| I2C1 | IMU / 磁力计 / 气压计 |
| I2C3 | 扩展总线 / Deck |
| USART1 | 调试串口或 WiFi 接收 |
| USART2 | Syslink |
| ADC1 | 扩展模块识别 |
| USB_OTG_FS | USB 通信 |
| DMA1 | I2C/SPI/USART2/WS2812 |
| DMA2 | ADC1 |
| EXTI0 | Syslink TXEN |
| EXTI4 | MPU 中断 |
| CAN | 当前未发现实际使用证据 |

关于 CAN：

- 本轮源码中未发现明确 `CAN_Init` / 引脚映射 / 实际业务使用。
- 因此当前应视为 **未使用**，不要在 CubeMX 中预先启用。

---

## 9. 冲突项与待确认项

### 9.1 PB4：`I2C3_SDA` vs `TIM3_CH1 (WS2812)`

- `HARDWARE/src/i2c_drv.c:142-146`：PB4 = I2C3_SDA
- `HARDWARE/src/ws2812.c:58-65`：PB4 = TIM3_CH1

当前最可能解释：

- 二者对应不同扩展形态，不能同时在线工作
- 原始工程通过扩展模块检测/互斥使用来规避冲突

后续确认方法：

- 查原理图 / 扩展模块接口定义
- 确认 LED Ring 与 Deck I2C3 是否真共用同一接口排针

### 9.2 PA8：`I2C3_SCL` vs 光流模块 NCS

- `i2c_drv.c` 将 PA8 定义为 I2C3_SCL
- `optical_flow.c` 将 PA8 当作片选 GPIO 使用

当前最可能解释：

- 光流模块和 deck I2C3 同属扩展接口资源，运行时按模块类型复用

### 9.3 PB0：头灯 / 光流供电 / WiFi相关扩展控制

- `ws2812.c` 中 PB0 是头灯控制
- `optical_flow.c` 中 PB0 是光流供电控制宏
- 说明这不是“主板固定功能引脚”，而是扩展模块场景复用脚

### 9.4 USART1：`PA9/PA10` vs `PB3/(PA15)`

- `SYSTEM/usart/usart.c`：PA9/PA10 全双工 USART1
- `HARDWARE/src/uart1.c`：PB3 RX，PA15 TX 被注释，当前仅 RX
- `wifi_ctrl.c`：WiFi 模块依赖 `uart1Init(19200)`

当前最可能解释：

- `SYSTEM/usart/usart.c` 是通用串口模板/传统调试口
- `uart1.c` 是针对 WiFi 摄像头扩展模块的专用串口方案

后续确认方法：

- 查最终实际编译路径与调用关系
- 确认重构目标是否还保留 WiFi 模块功能

### 9.5 `USE_HSE_BYPASS`

- 宏明确开启
- 但 HSEBYP 位的设置代码被注释掉
- 说明代码从 ST 模板改过，板级时钟来源需要额外确认

后续确认方法：

- 查原理图：外部晶振还是外部有源时钟
- 查实机示波或 BOM

### 9.6 扩展模块是“板载固定”还是“运行时探测”

当前证据显示：

- 通过 ADC1/PB1 电阻分压检测扩展模块类型
- 模块初始化由 `expModuleMgtTask` 动态触发

结论：

- LED_RING / WIFI_CAMERA / OPTICAL_FLOW 更像“扩展模块”而非主板固定焊接器件

---

## 10. CubeMX Pinout Checklist

### 10.1 主板固定基础项（优先配置）

- [ ] MCU 选择 `STM32F411CEUx`
- [ ] RCC 配到 96 MHz 系统时钟
- [ ] PB12 -> GPIO Output（LED 蓝）
- [ ] PA6 -> GPIO Output（LED 绿左，负逻辑）
- [ ] PA7 -> GPIO Output（LED 红左，负逻辑）
- [ ] PC13 -> GPIO Output（LED 绿右，负逻辑）
- [ ] PC14 -> GPIO Output（LED 红右，负逻辑）
- [ ] PB7 -> TIM4_CH2 PWM（M1）
- [ ] PB6 -> TIM4_CH1 PWM（M2）
- [ ] PB10 -> TIM2_CH3 PWM（M3）
- [ ] PA5 -> TIM2_CH1 PWM（M4）
- [ ] PB8/PB9 -> I2C1 SCL/SDA（传感器）
- [ ] PA4 -> GPIO_Input + EXTI4（IMU 中断）
- [ ] PA2/PA3 -> USART2 TX/RX（Syslink）
- [ ] PA0 -> GPIO_Input + EXTI0（Syslink TXEN）
- [ ] PB1 -> ADC1_IN9（扩展模块识别）
- [ ] PA11/PA12 -> USB_OTG_FS DM/DP

### 10.2 条件资源（按扩展模块二选一/按需配置）

- [ ] PA8/PB4 -> I2C3 SCL/SDA（Deck 总线）
- [ ] PB4 -> TIM3_CH1（WS2812 RGB）
- [ ] PB5 -> GPIO Output（LED Ring 电源）
- [ ] PB0 -> GPIO Output（头灯/扩展电源）
- [ ] PB13/PB14/PB15 -> SPI2 SCK/MISO/MOSI（光流）
- [ ] PA8 -> GPIO Output（光流 NCS，如果采用模块实现而非 deck I2C3）
- [ ] USART1 方案 A：PA9/PA10
- [ ] USART1 方案 B：PB3/(PA15)

---

## 11. CubeMX Peripheral Checklist

### 系统

- [ ] SYS Debug 方式按调试需求配置（SWD）
- [ ] RCC -> HSE 8MHz 方案建立
- [ ] Clock tree: SYSCLK 96MHz / APB1 48MHz / APB2 96MHz
- [ ] Flash latency 2WS

### 定时器

- [ ] TIM2 PWM CH1 + CH3
- [ ] TIM4 PWM CH1 + CH2
- [ ] TIM3 CH1（仅在 LED Ring 方案启用）

### I2C

- [ ] I2C1 Fast Mode 400k
- [ ] I2C3 Fast Mode 400k（仅在 deck 总线方案启用）

### SPI

- [ ] SPI2 Master Full Duplex（仅在光流模块方案启用）

### USART / UART

- [ ] USART2 Async 1,000,000 baud
- [ ] USART1 根据最终方案二选一

### ADC

- [ ] ADC1 IN9 continuous + DMA circular

### USB

- [ ] USB_OTG_FS Device/CDC 按重构目标选择
- [ ] 确认 48MHz USB clock 正常

### DMA

- [ ] ADC1 -> DMA2_Stream0
- [ ] I2C1 RX -> DMA1_Stream0
- [ ] I2C3 RX -> DMA1_Stream2
- [ ] SPI2 RX -> DMA1_Stream3
- [ ] SPI2 TX -> DMA1_Stream4
- [ ] USART2 TX -> DMA1_Stream6
- [ ] 若保留 WS2812，则处理 DMA1_Stream4 与 SPI2 TX 复用策略

### NVIC

- [ ] EXTI0
- [ ] EXTI4
- [ ] USART2
- [ ] DMA1 Stream6
- [ ] I2C1 EV/ER
- [ ] I2C3 EV/ER（如启用）
- [ ] SPI2 DMA IRQ（如启用）
- [ ] OTG_FS_IRQn

### FreeRTOS

- [ ] CMSIS-RTOS v1/v2 或裸 FreeRTOS 二选一
- [ ] Tick 1000Hz
- [ ] Heap 至少 30KB 起步
- [ ] 保留 Idle Hook（喂狗 + WFI）逻辑
- [ ] 保留 `PendSV/SVC/SysTick` 与 RTOS 的归属关系

---

## 12. 重建建议

如果目标是先把工程在 CubeMX / CMake 下跑起来，建议分两阶段：

### 阶段 1：只还原“主板固定最小可运行集”

先只配置：

- 96 MHz 时钟
- LEDs
- Motors PWM
- I2C1 + IMU EXTI
- USART2 Syslink
- USB FS
- ADC1 模块识别
- FreeRTOS 基础

暂时不要启用：

- I2C3
- WS2812
- 光流 SPI2
- WiFi USART1 替代方案

### 阶段 2：再加入扩展模块复用逻辑

按模块拆分：

- LED Ring profile
- Optical Flow profile
- WiFi Camera profile

避免在 CubeMX 单一静态配置中同时启用互斥资源。
