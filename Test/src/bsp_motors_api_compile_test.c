#include "bsp_motors.h"

int main(void)
{
    bsp_motors_init();
    bsp_motors_start();
    bsp_motors_set_ratio(0u, 0u);
    bsp_motors_stop_all();
    return 0;
}
