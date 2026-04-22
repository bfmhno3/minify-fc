#ifndef BSP_MOTORS_H
#define BSP_MOTORS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    BSP_MOTOR_1 = 0,
    BSP_MOTOR_2 = 1,
    BSP_MOTOR_3 = 2,
    BSP_MOTOR_4 = 3,
    BSP_MOTOR_COUNT = 4
};

void bsp_motors_init(void);
void bsp_motors_start(void);
void bsp_motors_stop_all(void);
void bsp_motors_set_ratio(uint8_t id, uint16_t ratio);

#ifdef __cplusplus
}
#endif

#endif
