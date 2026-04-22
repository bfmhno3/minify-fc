# Minifly 固件现代化重构软件系统架构与实施计划

## 1. 文档目的

本文档用于指导 `minifly-fc` 仓库从“原始 Minifly 固件 + STM32CubeMX 初步配置”的混合状态，演进为一套：

- 可由 **STM32CubeMX** 持续维护底层外设配置
- 可由 **CMake / CLion / clangd** 构建和开发
- 结构清晰、分层明确、便于长期维护
- 适合形成**独立、完整、可展示的软件系统架构**
- 满足你当前“**尽量重写，逻辑可以相似，但代码不要直接搬运**”的目标

这份文档不是“把旧工程文件换个位置”的搬迁说明，而是：

1. 以旧工程为**行为与资源参考**。
2. 重新设计适合现代工具链的系统边界。
3. 在尽量保留 CubeMX 目录习惯的前提下，给出新的软件架构、模块职责、文件规划、接口规划与实施顺序。

---

## 2. 当前工程的真实架构判断

当前仓库实际上存在两套并行体系。

### 2.1 legacy 固件体系是真正的业务实现

旧工程中，真实飞控逻辑主要分布在这些目录：

- `USER/`
- `HARDWARE/`
- `FLIGHT/`
- `COMMUNICATE/`
- `CONFIG/`
- `EXP_MODULES/`
- `SYSTEM/`
- `COMMON/`

从源码行为看：

- `USER/main.c` 是 legacy 侧真实入口。
- `HARDWARE/src/system.c` 中的 `systemInit()` 承担了真实初始化编排。
- `FLIGHT/src/stabilizer.c` 是飞控核心主循环。
- `FLIGHT/src/sensors.c` 负责传感器初始化、采样与 data-ready 中断路径。
- `CONFIG/src/config_param.c` 负责参数版本、校验与 Flash 持久化。
- `EXP_MODULES/src/module_mgt.c` 负责扩展模块识别与管理。

也就是说，旧工程具备完整业务功能，但目录边界和依赖关系偏“历史堆叠型”，不适合直接作为现代工程长期维护。

### 2.2 CubeMX 体系目前只完成了底层骨架

当前 `Core/Inc` 与 `Core/Src` 下主要是 CubeMX 自动生成内容，包括：

- `Core/Src/main.c`
- `Core/Src/freertos.c`
- `Core/Src/adc.c`
- `Core/Src/i2c.c`
- `Core/Src/spi.c`
- `Core/Src/tim.c`
- `Core/Src/usart.c`
- `Core/Src/usb_otg.c`
- `Core/Src/stm32f4xx_it.c`
- `Core/Src/stm32f4xx_hal_msp.c`
- `Core/Src/stm32f4xx_hal_timebase_tim.c`

从 `CMakeLists.txt` 和 `cmake/stm32cubemx/CMakeLists.txt` 看，当前现代构建链主要编译的是 CubeMX 生成文件，而没有真正承接 legacy 飞控业务主体。

### 2.3 当前最本质的问题

当前仓库不是“一个工程”，而是：

- 一套**旧架构的完整业务实现**
- 一套**新工具链下的外设初始化骨架**

两者还没有在架构层完成融合。

因此最合理的方向不是继续在旧目录上打补丁，也不是把旧代码整块搬进 `Core/`，而是：

> 用旧代码确认行为、资源、调度、接口和工程约束；
> 在 CubeMX 保持可再生的前提下，重新建立新的分层软件系统。

---

## 3. 架构目标与设计原则

## 3.1 总体目标

最终形成如下结构分工：

- **CubeMX**：负责 MCU、HAL、时钟、GPIO、DMA、I2C、SPI、TIM、USART、USB、FreeRTOS 基础骨架
- **Drivers/BSP**：负责板级硬件抽象与板级资源表达
- **Core**：负责应用、平台、服务、控制、通信、扩展模块等手写业务逻辑
- **Legacy 目录**：只作为参考源，不再作为长期主构建主体

## 3.2 你的约束在架构中的落地方式

你给出的关键约束是：

1. 尽量保留 STM32CubeMX 默认目录结构
2. 不新增 `Application/` 顶层目录
3. 允许新增 `Drivers/BSP/Inc` 和 `Drivers/BSP/Src`
4. 应用层尽量放在 `Core` 下
5. 为软件著作权申请考虑，**尽量重写，不直接搬旧代码**

基于这些约束，推荐方案是：

- 不破坏 `Core/Src/*.c`、`Core/Inc/*.h` 这一 CubeMX 默认习惯
- 在 `Core` 下增加分层子目录，而不是另起新的顶层业务目录
- 在 `Drivers` 下增加 `BSP`
- 所有新的手写飞控代码，以“**重新设计接口 + 重新实现逻辑**”的方式落地
- 旧工程源码只作为“行为参考资料”和“硬件资源说明资料”

## 3.3 重写原则

为了兼顾工程连续性和，建议执行以下原则：

1. **重写功能，不重抄实现**
   - 保留控制目标、任务关系、硬件占用、数据流方向
   - 不直接照搬 legacy 的函数拆分、变量组织、调用顺序写法和实现细节

2. **先抽象接口，再写实现**
   - 先定义模块职责、输入输出、状态边界
   - 再写新的 `.c/.h` 文件

3. **旧工程作为行为对照，不作为代码模板**
   - 可验证“旧工程做了什么”
   - 但新工程应重新表达“为什么这样做、接口如何更清晰、边界如何更可维护”

4. **保持 CubeMX 可再生**
   - 用户逻辑尽量不塞进 CubeMX 自动生成代码主体
   - 用新增子目录和桥接层承接手写逻辑

---

## 4. 推荐目标目录结构

```text
Core/
  Inc/
    app/
    platform/
    services/
    control/
    comm/
    modules/
  Src/
    main.c
    freertos.c
    adc.c
    dma.c
    gpio.c
    i2c.c
    spi.c
    tim.c
    usart.c
    usb_otg.c
    stm32f4xx_it.c
    stm32f4xx_hal_msp.c
    stm32f4xx_hal_timebase_tim.c
    system_stm32f4xx.c
    app/
    platform/
    services/
    control/
    comm/
    modules/

Drivers/
  BSP/
    Inc/
    Src/

Docs/
cmake/
Drivers/CMSIS/
Drivers/STM32F4xx_HAL_Driver/
Middlewares/
```

### 4.1 这样设计的理由

- `Core/Src/*.c` 仍然保留 CubeMX 默认观感，阅读者不会觉得工程被“推翻重建”。
- `Core/Src/app`、`platform`、`services`、`control`、`comm`、`modules` 把手写逻辑放回 `Core` 内部，符合你的偏好。
- `Drivers/BSP` 明确区分“芯片厂 HAL”与“本板级驱动”，可读性远高于把板级驱动继续散落在多个历史目录。
- 后续如果 CubeMX 再生成底层代码，结构冲击也最小。

---

## 5. 分层软件系统架构

### 5.1 Boot / App 层

职责：

- 作为系统启动入口和任务装配层
- 承接 `main()` 后的应用级初始化
- 创建实际业务任务
- 提供 FreeRTOS hook 的应用侧落点

#### 模块 1：应用引导模块

**建议文件**

- `Core/Inc/app/app_boot.h`
- `Core/Src/app/app_boot.c`

**职责**

- 在 `main()` 完成 HAL/CubeMX 初始化后进入应用初始化逻辑
- 调用平台初始化与任务创建
- 保持 `Core/Src/main.c` 尽量干净

**建议接口**

- `void app_boot_init(void);`

**建议行为**

- 调用 `platform_init()`
- 调用 `app_tasks_create()`

#### 模块 2：任务装配模块

**建议文件**

- `Core/Inc/app/app_tasks.h`
- `Core/Src/app/app_tasks.c`

**职责**

- 创建全系统业务任务
- 统一管理优先级、栈大小、任务句柄
- 作为 legacy 任务拓扑在新架构下的重写表达

**建议接口**

- `void app_tasks_create(void);`

**建议创建的任务**

- `radiolink_task`
- `usblink_rx_task`
- `usblink_tx_task`
- `atkp_tx_task`
- `atkp_rx_task`
- `config_service_task`
- `pm_service_task`
- `sensors_task`
- `stabilizer_task`
- `module_manager_task`

#### 模块 3：系统 Hook 模块

**建议文件**

- `Core/Inc/app/app_hooks.h`
- `Core/Src/app/app_hooks.c`

**职责**

- 承接 FreeRTOS idle hook 等系统钩子
- 将 `__WFI()`、看门狗喂狗等行为从入口逻辑中解耦

**建议接口**

- `void app_idle_hook(void);`

---

### 5.2 Platform 层

职责：

- 提供与具体业务无关的系统级能力
- 负责初始化编排、中断桥接、故障处理、时间基准、基础工具
- 让 CubeMX 自动生成文件和手写业务代码之间形成稳定边界

#### 模块 1：平台初始化模块

**建议文件**

- `Core/Inc/platform/platform_init.h`
- `Core/Src/platform/platform_init.c`

**职责**

- 统一编排板级驱动、服务层、控制层初始化
- 重写 legacy `systemInit()` 的工程作用，但不复刻其代码结构

**建议接口**

- `void platform_init(void);`
- `bool platform_self_test(void);`

**建议初始化顺序**

1. `platform_irq_init()`
2. `platform_timebase_init()`
3. `bsp_led_init()`
4. `ledseq_init()`
5. `comm_stack_init()`
6. `console_service_init()`
7. `config_service_init()`
8. `pm_service_init()`
9. `stabilizer_init()`
10. `module_manager_init()`
11. `bsp_watchdog_init()`

#### 模块 2：中断桥接模块

**建议文件**

- `Core/Inc/platform/platform_irq.h`
- `Core/Src/platform/platform_irq.c`

**职责**

- 将 CubeMX 生成的 `stm32f4xx_it.c` 与手写业务中断逻辑解耦
- 管理 SysTick、EXTI、关键外设中断的用户侧入口

**建议接口**

- `void platform_irq_init(void);`
- `uint32_t platform_get_tick_ms(void);`
- `void platform_systick_pre_scheduler(void);`

#### 模块 3：故障处理模块

**建议文件**

- `Core/Inc/platform/platform_fault.h`
- `Core/Src/platform/platform_fault.c`

**职责**

- 统一硬故障后的停机保护策略
- 提供异常停机、电机关闭、错误指示等基础能力

**建议接口**

- `void platform_fault_shutdown(void);`
- `void platform_fault_panic(uint32_t code);`

#### 模块 4：时间基准模块

**建议文件**

- `Core/Inc/platform/timebase.h`
- `Core/Src/platform/timebase.c`

**职责**

- 提供毫秒级系统时间获取
- 为控制与调度模块提供一致时基

**建议接口**

- `void platform_timebase_init(void);`
- `uint32_t timebase_get_ms(void);`

#### 模块 5：基础算法工具模块

**建议文件**

- `Core/Inc/platform/filter.h`
- `Core/Src/platform/filter.c`
- `Core/Inc/platform/maths.h`
- `Core/Src/platform/maths.c`
- `Core/Inc/platform/axis.h`

**职责**

- 承载低通滤波、向量运算、限幅、插值等基础能力
- 供 sensors、estimator、controller 公共使用

**建议接口**

- `void lpf2p_init(...);`
- `float lpf2p_apply(...);`
- `float maths_constrainf(float value, float min, float max);`

---

### 5.3 BSP 层

职责：

- 负责“这块板子怎么接”的问题
- 负责具体 GPIO、TIM、ADC、Flash、EXTI、供电与板级资源映射
- 对上暴露稳定板级接口，不把 HAL 细节泄漏到业务层

#### 模块 1：板级定义模块

**建议文件**

- `Drivers/BSP/Inc/board.h`
- `Drivers/BSP/Inc/board_config.h`
- `Drivers/BSP/Inc/board_memory.h`

**职责**

- 统一定义系统频率、Flash 布局、电机数量、模块开关、资源映射
- 消除零散常量

**建议核心定义**

- `BOARD_SYSCLK_HZ`
- `BOARD_FLASH_BOOT_SIZE`
- `BOARD_FLASH_CONFIG_SIZE`
- `BOARD_FLASH_APP_START`
- `BOARD_MOTOR_COUNT`
- `BOARD_HAS_USB_LINK`
- `BOARD_HAS_OPTICAL_FLOW`

#### 模块 2：电机驱动模块

**建议文件**

- `Drivers/BSP/Inc/bsp_motors.h`
- `Drivers/BSP/Src/bsp_motors.c`

**职责**

- 封装四路 PWM 输出
- 屏蔽 TIM2/TIM4 与 channel 细节
- 保证四路输出统一接口

**建议接口**

- `void bsp_motors_init(void);`
- `void bsp_motors_start(void);`
- `void bsp_motors_stop_all(void);`
- `void bsp_motors_set_ratio(uint8_t id, uint16_t ratio);`

**必须覆盖的映射**

- `PB7 / TIM4_CH2`
- `PB6 / TIM4_CH1`
- `PB10 / TIM2_CH3`
- `PA5 / TIM2_CH1`

#### 模块 3：LED 驱动模块

**建议文件**

- `Drivers/BSP/Inc/bsp_led.h`
- `Drivers/BSP/Src/bsp_led.c`

**职责**

- 封装板载状态灯控制
- 屏蔽负逻辑/正逻辑差异

**建议接口**

- `void bsp_led_init(void);`
- `void bsp_led_set(uint8_t id, bool on);`
- `void bsp_led_toggle(uint8_t id);`

#### 模块 4：看门狗模块

**建议文件**

- `Drivers/BSP/Inc/bsp_watchdog.h`
- `Drivers/BSP/Src/bsp_watchdog.c`

**职责**

- 提供 IWDG 初始化与喂狗接口
- 供 idle hook 或系统保活逻辑调用

**建议接口**

- `void bsp_watchdog_init(uint32_t timeout_ms);`
- `void bsp_watchdog_kick(void);`

#### 模块 5：扩展模块板级模块

**建议文件**

- `Drivers/BSP/Inc/bsp_module.h`
- `Drivers/BSP/Src/bsp_module.c`

**职责**

- 负责模块识别 ADC 读取、供电控制、GPIO 切换
- 不承载状态机，只承载板级动作

**建议接口**

- `void bsp_module_init(void);`
- `uint16_t bsp_module_detect_read_raw(void);`
- `void bsp_module_power_set(bool on);`

#### 模块 6：传感器板级胶水模块

**建议文件**

- `Drivers/BSP/Inc/bsp_sensors.h`
- `Drivers/BSP/Src/bsp_sensors.c`

**职责**

- 管理 IMU data-ready 中断引脚
- 管理与底层总线/EXTI 的板级交互
- 向上层提供“等待数据就绪”的统一方式

**建议接口**

- `void bsp_sensors_init(void);`
- `void bsp_sensors_irq_init(void);`
- `bool bsp_sensors_wait_data_ready(uint32_t timeout_ms);`
- `void bsp_sensors_data_ready_isr(void);`

#### 模块 7：Flash 参数区模块

**建议文件**

- `Drivers/BSP/Inc/bsp_flash.h`
- `Drivers/BSP/Inc/bsp_flash_layout.h`
- `Drivers/BSP/Src/bsp_flash.c`

**职责**

- 封装参数区读写
- 对外提供明确的 config 分区访问接口

**建议接口**

- `bool bsp_flash_read_config(void *buf, size_t len);`
- `bool bsp_flash_write_config(const void *buf, size_t len);`
- `uint32_t bsp_flash_get_config_address(void);`

---

### 5.4 Services 层

职责：

- 封装系统公共服务
- 为控制层、通信层、模块层提供稳定能力
- 负责可持久化状态、系统状态与通用服务

#### 模块 1：配置服务模块

**建议文件**

- `Core/Inc/services/config_types.h`
- `Core/Inc/services/config_service.h`
- `Core/Src/services/config_service.c`

**职责**

- 负责配置对象版本、校验、默认值、持久化与 dirty 管理
- 用新的服务接口表达参数系统，不延续 legacy 全局裸变量结构

**建议接口**

- `void config_service_init(void);`
- `bool config_service_load(void);`
- `void config_service_task(void *arg);`
- `const config_param_t *config_service_get(void);`
- `config_param_t *config_service_mut(void);`
- `void config_service_mark_dirty(void);`
- `void config_service_reset_pid(void);`

#### 模块 2：电源管理服务模块

**建议文件**

- `Core/Inc/services/pm_service.h`
- `Core/Src/services/pm_service.c`

**职责**

- 管理电池电压、低压状态、电源告警

**建议接口**

- `void pm_service_init(void);`
- `void pm_service_task(void *arg);`
- `float pm_service_get_battery_voltage(void);`
- `bool pm_service_is_low_power(void);`

#### 模块 3：LED 序列服务模块

**建议文件**

- `Core/Inc/services/ledseq.h`
- `Core/Src/services/ledseq.c`

**职责**

- 管理系统状态灯逻辑
- 基于 `bsp_led` 运行状态机或序列模式

**建议接口**

- `void ledseq_init(void);`
- `void ledseq_run(uint8_t target, uint8_t pattern);`
- `void ledseq_stop(uint8_t target);`

#### 模块 4：控制台服务模块

**建议文件**

- `Core/Inc/services/console_service.h`
- `Core/Src/services/console_service.c`

**职责**

- 提供调试串口输出
- 可承接简单命令调试入口

**建议接口**

- `void console_service_init(void);`
- `void console_service_write(const char *buf, size_t len);`

#### 模块 5：传感器服务模块

**建议文件**

- `Core/Inc/services/sensors.h`
- `Core/Src/services/sensors.c`

**职责**

- 负责传感器初始化、采样、滤波、校准、缓存与对外读取
- 这是重写后的“感知服务层”，而不是旧 `sensors.c` 的直接翻版

**建议接口**

- `void sensors_init(void);`
- `bool sensors_test(void);`
- `bool sensors_are_calibrated(void);`
- `void sensors_task(void *arg);`
- `void sensors_acquire(sensor_data_t *out, uint32_t tick);`
- `bool sensors_read_gyro(axis3f_t *gyro);`
- `bool sensors_read_acc(axis3f_t *acc);`
- `bool sensors_read_baro(baro_t *baro);`

---

### 5.5 Comm 层

职责：

- 封装无线链路、USB 链路、协议栈与飞控命令入口
- 提供清晰的数据流边界：输入 -> 协议 -> commander -> setpoint

#### 模块 1：通信栈编排模块

**建议文件**

- `Core/Inc/comm/comm_stack.h`
- `Core/Src/comm/comm_stack.c`

**职责**

- 统一初始化通信子系统

**建议接口**

- `void comm_stack_init(void);`

#### 模块 2：RadioLink 模块

**建议文件**

- `Core/Inc/comm/radiolink.h`
- `Core/Src/comm/radiolink.c`

**职责**

- 处理无线链路接收与解析
- 向 commander 提供遥控输入

**建议接口**

- `void radiolink_init(void);`
- `void radiolink_task(void *arg);`
- `bool radiolink_get_frame(radio_frame_t *frame);`

#### 模块 3：USBLink 模块

**建议文件**

- `Core/Inc/comm/usblink.h`
- `Core/Src/comm/usblink.c`

**职责**

- 负责 USB CDC 收发

**建议接口**

- `void usblink_init(void);`
- `void usblink_rx_task(void *arg);`
- `void usblink_tx_task(void *arg);`

#### 模块 4：ATKP 协议模块

**建议文件**

- `Core/Inc/comm/atkp.h`
- `Core/Src/comm/atkp.c`

**职责**

- 管理协议编码、解码、收发分发

**建议接口**

- `void atkp_init(void);`
- `void atkp_tx_task(void *arg);`
- `void atkp_rx_task(void *arg);`
- `void atkp_dispatch_packet(const uint8_t *buf, size_t len);`

#### 模块 5：Commander 模块

**建议文件**

- `Core/Inc/comm/commander.h`
- `Core/Src/comm/commander.c`

**职责**

- 汇聚 radio / USB / 上位机控制输入
- 输出控制层使用的标准化 setpoint

**建议接口**

- `void commander_init(void);`
- `void commander_get_setpoint(setpoint_t *sp, const state_t *state);`
- `uint8_t commander_get_ctrl_mode(void);`

---

### 5.6 Control 层

职责：

- 负责姿态估计、位置估计、控制器、主稳定器与动力输出
- 这是飞控最核心的软件层
- 应该重写为边界更清晰、职责更明确的控制子系统

#### 模块 1：姿态估计模块

**建议文件**

- `Core/Inc/control/attitude_estimator.h`
- `Core/Src/control/attitude_estimator.c`

**职责**

- 负责姿态解算

**建议接口**

- `void attitude_estimator_init(void);`
- `void attitude_estimator_update(...);`
- `void attitude_estimator_get(attitude_t *attitude);`

#### 模块 2：位置估计模块

**建议文件**

- `Core/Inc/control/position_estimator.h`
- `Core/Src/control/position_estimator.c`

**职责**

- 融合加速度、气压计、光流等数据进行位置估计

**建议接口**

- `void position_estimator_init(void);`
- `void position_estimator_update(...);`
- `void estimator_reset_height(void);`
- `void estimator_reset_all(void);`

#### 模块 3：姿态 PID 模块

**建议文件**

- `Core/Inc/control/attitude_pid.h`
- `Core/Src/control/attitude_pid.c`

**职责**

- 管理姿态内外环 PID

**建议接口**

- `void attitude_pid_init(void);`
- `void attitude_pid_reset(void);`
- `void attitude_pid_run(...);`

#### 模块 4：位置 PID 模块

**建议文件**

- `Core/Inc/control/position_pid.h`
- `Core/Src/control/position_pid.c`

**职责**

- 管理位置控制与高度控制

**建议接口**

- `void position_pid_init(void);`
- `void position_pid_reset(void);`
- `void position_pid_run(...);`

#### 模块 5：状态控制模块

**建议文件**

- `Core/Inc/control/state_control.h`
- `Core/Src/control/state_control.c`

**职责**

- 汇总姿态、角速度、位置控制结果
- 输出统一 control 量

**建议接口**

- `void state_control_init(void);`
- `bool state_control_test(void);`
- `void state_control_run(control_t *out, const sensor_data_t *sensor, const state_t *state, const setpoint_t *sp, uint32_t tick);`

#### 模块 6：动力输出模块

**建议文件**

- `Core/Inc/control/power_control.h`
- `Core/Src/control/power_control.c`

**职责**

- 将控制量混控成四路电机输出
- 通过 `bsp_motors` 落到板级驱动

**建议接口**

- `void power_control_init(void);`
- `bool power_control_test(void);`
- `void power_control_run(const control_t *ctl);`

#### 模块 7：异常检测模块

**建议文件**

- `Core/Inc/control/anomaly_detect.h`
- `Core/Src/control/anomaly_detect.c`

**职责**

- 检测翻转、异常姿态、传感器异常等保护条件

**建议接口**

- `void anomaly_detect_init(void);`
- `bool anomaly_detect_check(...);`

#### 模块 8：翻转动作模块

**建议文件**

- `Core/Inc/control/flip.h`
- `Core/Src/control/flip.c`

**职责**

- 独立封装翻转飞行动作逻辑

**建议接口**

- `void flyer_flip_check(...);`

#### 模块 9：主稳定器模块

**建议文件**

- `Core/Inc/control/stabilizer.h`
- `Core/Src/control/stabilizer.c`

**职责**

- 作为 1kHz 主控制任务
- 编排感知、估计、命令、保护与输出

**建议接口**

- `void stabilizer_init(void);`
- `bool stabilizer_test(void);`
- `void stabilizer_task(void *arg);`
- `void get_attitude_data(attitude_t *out);`
- `void get_sensor_data(sensor_data_t *out);`
- `void get_state_data(axis3f_t *acc, axis3f_t *vel, axis3f_t *pos);`

---

### 5.7 Modules 层

职责：

- 负责扩展模块识别后的上层功能管理
- 对每种扩展模块建立独立的软件模块

#### 模块 1：模块管理器

**建议文件**

- `Core/Inc/modules/module_manager.h`
- `Core/Src/modules/module_manager.c`

**职责**

- 根据 `bsp_module` 的检测结果管理当前扩展模块生命周期

**建议接口**

- `void module_manager_init(void);`
- `void module_manager_task(void *arg);`
- `uint8_t module_manager_get_active(void);`
- `void module_manager_power_enable(bool on);`

#### 模块 2：光流模块

**建议文件**

- `Core/Inc/modules/optical_flow_module.h`
- `Core/Src/modules/optical_flow_module.c`

**职责**

- 提供光流模块初始化与数据更新

**建议接口**

- `void optical_flow_module_init(void);`
- `void optical_flow_module_update(state_t *state, float dt);`

#### 模块 3：WiFi 模块

**建议文件**

- `Core/Inc/modules/wifi_module.h`
- `Core/Src/modules/wifi_module.c`

**职责**

- 负责 WiFi 扩展模块业务控制

**建议接口**

- `void wifi_module_init(void);`
- `void wifi_module_process(void);`

#### 模块 4：LED Ring 模块

**建议文件**

- `Core/Inc/modules/ledring_module.h`
- `Core/Src/modules/ledring_module.c`

**职责**

- 负责 LED Ring 扩展模块控制

**建议接口**

- `void ledring_module_init(void);`
- `void ledring_module_update(void);`

---

## 6. legacy 目录与新架构的对应关系

这里的重点不是“把文件搬过去”，而是明确“参考来源”和“新架构归宿”。

| legacy 目录 | 新架构归宿 | 处理策略 |
|---|---|---|
| `USER/` | `Core/Src/app` | 仅参考任务拓扑与入口行为，重新实现 |
| `HARDWARE/` | `Drivers/BSP` + `Core/Src/platform` | 板级细节下沉 BSP，系统级逻辑重写到 platform |
| `SYSTEM/` | `Core/Src/platform` / `Drivers/BSP` | 按职责拆分，避免保留历史混杂结构 |
| `FLIGHT/` | `Core/Src/services` + `Core/Src/control` | 感知与控制拆层重写 |
| `COMMUNICATE/` | `Core/Src/comm` + `Core/Src/services` | 通信协议与控制输入重新组织 |
| `CONFIG/` | `Core/Src/services` + `Drivers/BSP` | 服务逻辑与 Flash 访问分离 |
| `EXP_MODULES/` | `Core/Src/modules` + `Drivers/BSP` | 上层状态机与板级操作分离 |
| `COMMON/` | `Core/Src/platform` / `Core/Src/services` | 只保留必要通用能力，重新整理 |

---

## 7. 构建系统规划

## 7.1 构建原则

- CubeMX 生成代码继续通过 `cmake/stm32cubemx/CMakeLists.txt` 接入
- 新手写代码通过新增源文件列表与 include path 接入
- 不把手写业务逻辑直接混进 CubeMX 自动生成文件主体

## 7.2 顶层 CMake 需要逐步接入的目录

**新增源目录**

- `Core/Src/app/*`
- `Core/Src/platform/*`
- `Core/Src/services/*`
- `Core/Src/control/*`
- `Core/Src/comm/*`
- `Core/Src/modules/*`
- `Drivers/BSP/Src/*`

**新增头文件目录**

- `Core/Inc`
- `Core/Inc/app`
- `Core/Inc/platform`
- `Core/Inc/services`
- `Core/Inc/control`
- `Core/Inc/comm`
- `Core/Inc/modules`
- `Drivers/BSP/Inc`

## 7.3 构建边界建议

- `Core/Src/main.c` 保持为唯一入口
- `Core/Src/stm32f4xx_it.c` 保持中断分发入口
- `Drivers/BSP` 不直接依赖高层业务模块
- `Core/control` 不直接依赖 HAL，而是经由 `services` / `BSP` 访问底层

---

## 8. 已确认的关键工程风险

### 风险 1：Flash 布局冲突

旧工程定义：

- Bootloader 16KB
- Config 参数区 16KB
- 应用起始地址 `FLASH_BASE + 0x8000`

但当前 linker 仍从 `0x08000000` 起。

**后果**

- 向量表、代码段、下载地址与 legacy 运行假设冲突
- 后续 Bootloader / 参数区布局将不一致

**优先级：最高**

### 风险 2：四路电机输出表达不完整

旧工程明确需要：

- `PB7/TIM4_CH2`
- `PB6/TIM4_CH1`
- `PB10/TIM2_CH3`
- `PA5/TIM2_CH1`

当前 CubeMX 配置对 MOTOR4 的表达还不够完整。

**后果**

- 运行时四电机输出不闭环
- 新 BSP 电机模块无法稳定落地

**优先级：最高**

### 风险 3：SysTick / FreeRTOS tick 行为差异

旧工程在调度器启动前后对 tick 的处理有明确语义。

**后果**

- 主控制周期不稳定
- `vTaskDelayUntil()` 节拍漂移

### 风险 4：IMU data-ready 中断路径丢失

旧工程依赖 `PA4/EXTI4` 驱动 IMU 数据就绪。

**后果**

- 传感器任务唤醒机制失真
- 采样节奏与控制节奏脱钩

### 风险 5：扩展模块复用引脚冲突

`Docs/hardware.md` 已记录：

- `PB4`
- `PA8`
- `PB0`

等资源存在冲突。

**后果**

- 扩展模块组合能力不明确
- CubeMX 配置可能与真实模块策略冲突

---

## 9. 重构实施阶段

### Phase 0：冻结架构边界与命名规则

**目标**

- 先把未来软件结构边界确定下来
- 不着急搬功能

**动作**

1. 在 `Core/Inc`、`Core/Src` 下建立子域目录规划
2. 建立 `Drivers/BSP/Inc` 与 `Drivers/BSP/Src`
3. 确认 `Core/Src/main.c` 为唯一长期入口
4. 在文档中明确 legacy 只作参考，不作直接代码迁移目标

**交付物**

- 本文档
- 初始目录骨架

### Phase 1：修复平台级硬冲突

**目标**

- 解决最影响启动和硬件运行的冲突

**动作**

1. 修正 `STM32F411XX_FLASH.ld`
2. 建立 `board_memory.h`
3. 补齐四路电机 PWM 的 BSP 表达
4. 建立中断桥接边界

**交付物**

- `board_memory.h`
- `bsp_motors.*`
- `platform_irq.*`
- linker 更新

### Phase 2：先落地 BSP 层

**目标**

- 先把最底层硬件适配稳定下来

**动作**

1. 实现 `bsp_motors`
2. 实现 `bsp_led`
3. 实现 `bsp_watchdog`
4. 实现 `bsp_module`
5. 实现 `bsp_flash`
6. 实现 `bsp_sensors`

**交付物**

- `Drivers/BSP/Inc/*`
- `Drivers/BSP/Src/*`

### Phase 3：建立 App / Platform / Services 骨架

**目标**

- 建立可运行的软件骨架

**动作**

1. 实现 `app_boot`
2. 实现 `app_tasks`
3. 实现 `app_hooks`
4. 实现 `platform_init`
5. 实现 `config_service`
6. 实现 `pm_service`
7. 实现 `console_service`
8. 实现 `ledseq`

### Phase 4：建立 Comm 层

**目标**

- 让通信路径先跑通

**动作**

1. 实现 `comm_stack`
2. 实现 `radiolink`
3. 实现 `usblink`
4. 实现 `atkp`
5. 实现 `commander`

### Phase 5：建立 Sensors / Control 主链路

**目标**

- 重写飞控主控制链路

**动作**

1. 实现 `sensors`
2. 实现 `attitude_estimator`
3. 实现 `position_estimator`
4. 实现 `attitude_pid`
5. 实现 `position_pid`
6. 实现 `state_control`
7. 实现 `power_control`
8. 实现 `anomaly_detect`
9. 实现 `flip`
10. 实现 `stabilizer`

### Phase 6：建立扩展模块层

**目标**

- 恢复扩展模块能力

**动作**

1. 实现 `module_manager`
2. 实现 `optical_flow_module`
3. 实现 `wifi_module`
4. 实现 `ledring_module`

### Phase 7：移除 legacy 主构建依赖

**目标**

- 让新架构成为唯一主构建体系

**动作**

1. 从主构建中去除 legacy 目录依赖
2. 保留 legacy 目录作为参考资料
3. 后续再决定是否归档或删除

---

## 10. 每一层的实现要求

为了既利于维护，也利于材料整理，建议每个模块都遵循统一模板：

### 10.1 头文件要求

每个模块头文件至少明确：

- 模块职责
- 对外类型定义
- 对外初始化函数
- 对外运行函数
- 对外状态访问函数

### 10.2 源文件要求

每个模块源文件尽量只负责：

- 该模块私有状态
- 该模块内部辅助函数
- 该模块对外接口实现

### 10.3 依赖要求

- `control` 不直接碰 HAL
- `services` 尽量不感知具体定时器/引脚
- `BSP` 不进入高层业务决策
- `app` 只负责编排，不写具体控制逻辑

---

## 11. 验证计划

### 11.1 静态验证

1. `CMake` 能生成工程
2. 所有新增 include path 正确
3. 不存在双入口
4. 不存在重复 ISR 归属
5. 板级地址定义与 linker 一致

### 11.2 编译验证

1. Debug 构建通过
2. Release 构建通过
3. 能输出 `.elf`、`.map`、`.bin`

### 11.3 运行时验证

1. 系统能进入调度器
2. Idle hook 能喂狗并 `__WFI()`
3. 传感器中断路径正常
4. 1kHz 稳定器主循环节拍正常
5. 四路电机 PWM 全部正常
6. radio / USB / ATKP 链路可用
7. 参数写入 Flash 后重启仍有效
8. 扩展模块识别与上电逻辑正常

### 11.4 文档一致性验证

以下内容必须长期保持一致：

- `.ioc`
- `Docs/hardware.md`
- `board_memory.h`
- linker 脚本
- 架构文档

---

## 12. 推荐实施顺序

推荐你后续真正落地时按下面顺序推进：

1. 修 linker 与内存布局表达
2. 建立 `Drivers/BSP` 和 `Core/*` 子域骨架
3. 先做 `bsp_motors`、`bsp_led`、`bsp_watchdog`、`bsp_flash`、`bsp_sensors`
4. 再做 `app_boot`、`app_tasks`、`platform_init`
5. 再做 `config_service`、`pm_service`、`console_service`、`ledseq`
6. 再做 `comm_stack`、`radiolink`、`usblink`、`atkp`、`commander`
7. 再做 `sensors`、`estimator`、`PID`、`state_control`、`power_control`、`stabilizer`
8. 最后做扩展模块层
9. 最终去掉 legacy 主构建依赖

---

## 13. 最终结论

最适合这个仓库的长期软件系统架构是：

- **CubeMX 继续管理 `Core/Src` 与 `Core/Inc` 中的自动生成基础层**
- **新增 `Drivers/BSP` 作为板级抽象层**
- **将新的应用、平台、服务、通信、控制、模块逻辑全部收敛到 `Core` 子域目录**
- **旧工程只作为行为与资源参考，不作为代码直接迁移目标**

这条路线同时满足：

- 目录改动尽量小
- 阅读心理负担低
- 工具链现代化
- 软件结构清晰
- 有利于后续持续重构
- 有利于形成一套独立的软件系统说明材料
