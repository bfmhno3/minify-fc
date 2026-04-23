#include "bsp_led.h"

int main(void)
{
    bsp_led_init();
    bsp_led_set(BSP_LED_BLUE_L, true);
    bsp_led_toggle(BSP_LED_RED_R);
    return 0;
}
