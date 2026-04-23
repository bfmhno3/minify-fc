cmake_minimum_required(VERSION 3.22)
project(minifly_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_executable(bsp_motors_api_compile_test
    src/bsp_motors_api_compile_test.c
)

add_executable(bsp_led_api_compile_test
    src/bsp_led_api_compile_test.c
)

add_executable(bsp_watchdog_api_compile_test
    src/bsp_watchdog_api_compile_test.c
)

target_include_directories(bsp_motors_api_compile_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(bsp_led_api_compile_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(bsp_watchdog_api_compile_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
